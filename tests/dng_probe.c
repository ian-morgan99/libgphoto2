/* Locate + fetch a specific DNG object on a Pentax camera via raw PTP.
 *
 * Usage: ./dng_probe [port] <filename> [outdir]
 *   - enumerates every storage (GetStorageIDs/GetStorageInfo) and lists all
 *     objects per storage via GetObjectHandles(root), so folder structure is
 *     irrelevant — the camera's own object table is authoritative
 *   - prints handle/storage/size for every DNG-like object found
 *   - if <filename> matches, fetches it with GetObject into outdir (default
 *     /tmp/k1ii-dng) and reports byte count + PTP RC
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "gphoto2/gphoto2.h"
#include "ptp.h"

static Camera *camera = NULL;
static GPContext *context = NULL;

static void die(const char *msg, int ret) {
	fprintf(stderr, "FATAL: %s (%d)\n", msg, ret);
	exit(1);
}

int main(int argc, **argv) {
	const char *port_arg = NULL;
	const char *want = NULL;
	const char *outdir = "/tmp/k1ii-dng";
	for (int i = 1; i < argc; i++) {
		if (!port_arg && !want && argv[i][0] != '-') port_arg = argv[i];
		else if (!want) want = argv[i];
		else outdir = argv[i];
	}
	if (!want) die("usage: dng_probe [port] <filename> [outdir]", 1);

	printf("probe pid=%d\n", getpid());
	context = gp_context_new();

	{
		CameraList *list;
		const char *model = NULL, *port = NULL;
		gp_list_new(&list);
		if (gp_camera_autodetect(list, context) < GP_OK || gp_list_count(list) < 1)
			die("no camera autodetected", 0);
		int idx = 0;
		for (int i = 0; i < gp_list_count(list); i++) {
			const char *v = NULL;
			gp_list_get_value(list, i, &v);
			if (port_arg && strcmp(v, port_arg) == 0) idx = i;
		}
		gp_list_get_name(list, idx, &model);
		gp_list_get_value(list, idx, &port);
		printf("detected: %s @ %s\n", model, port);

		CameraAbilitiesList *al;
		gp_abilities_list_new(&al);
		gp_abilities_list_load(al, context);
		int n = gp_abilities_list_lookup_model(al, model);
		if (n < 0) die("model lookup failed", n);
		CameraAbilities a;
		gp_abilities_list_get_abilities(al, n, &a);
		gp_abilities_list_free(al);
		gp_camera_new(&camera);
		gp_camera_set_abilities(camera, a);

		GPPortInfoList *pil;
		gp_port_info_list_new(&pil);
		gp_port_info_list_load(pil);
		int i = gp_port_info_list_lookup_path(pil, port);
		if (i < 0) die("port lookup failed", i);
		GPPortInfo info = NULL;
		gp_port_info_list_get_info(pil, i, &info);
		int ret = gp_camera_set_port_info(camera, info);
		gp_port_info_list_free(pil);
		if (ret < GP_OK) die("set port info failed", ret);
		gp_list_free(list);
	}

	int ret = gp_camera_init(camera, context);
	if (ret < GP_OK) die("camera init failed", ret);
	PTPParams *params = &camera->pl->params;

	/* 1. storages */
	PTPStorageIDs sids;
	uint16_t rc = ptp_getstorageids(params, &sids);
	printf("getstorageids rc=0x%04x count=%d\n", rc, (int)sids.len);
	for (uint32_t si = 0; si < sids.len; si++) {
		uint32_t sid = sids.data[si];
		PTPStorageInfo info;
		memset(&info, 0, sizeof(info));
		rc = ptp_getstorageinfo(params, sid, &info);
		printf("storage 0x%08x rc=0x%04x type=%u access=%u max=%llu free_bytes=%llu label='%s'\n",
		       sid, rc, info.StorageType, info.AccessCapability,
		       (unsigned long long)info.MaxCapability,
		       (unsigned long long)info.FreeSpaceInBytes,
		       info.VolumeLabel ? info.VolumeLabel : "(null)");

		/* 2. all objects on this storage */
		PTPObjectHandles handles = {0};
		rc = ptp_list_folder(params, sid, PTP_HANDLER_ROOT, &handles);
		printf("  list_folder(root) rc=0x%04x objects=%d\n", rc, (int)handles.len);
		for (uint32_t hi = 0; hi < handles.len; hi++) {
			uint32_t h = handles.data[hi];
			PTPObject *ob = NULL;
			if (ptp_object_want(params, h, PTPOBJECT_OBJECTINFO_LOADED, &ob) != PTP_RC_OK || !ob)
				continue;
			const char *fn = ob->oi.Filename ? ob->oi.Filename : "(null)";
			int is_dng = strstr(fn, ".DNG") || strstr(fn, ".dng");
			if (is_dng)
				printf("  DNG handle=0x%08x storage=0x%08x size=%llu parent=0x%08x '%s'\n",
				       h, ob->oi.StorageID,
				       (unsigned long long)ob->oi.ObjectSize,
				       ob->oi.ParentObject, fn);
			if (!want || strcmp(fn, want) == 0) {
				printf("MATCH handle=0x%08x storage=0x%08x size=%llu '%s'\n",
				       h, ob->oi.StorageID,
				       (unsigned long long)ob->oi.ObjectSize, fn);
			}
		}
	}

	/* 3. fetch the wanted file if we saw it */
	int found = 0;
	for (uint32_t si = 0; !found && si < sids.len; si++) {
		uint32_t sid = sids.data[si];
		PTPObjectHandles handles = {0};
		if (ptp_list_folder(params, sid, PTP_HANDLER_ROOT, &handles) != PTP_RC_OK)
			continue;
		for (uint32_t hi = 0; !found && hi < handles.len; hi++) {
			uint32_t h = handles.data[hi];
			PTPObject *ob = NULL;
			if (ptp_object_want(params, h, PTPOBJECT_OBJECTINFO_LOADED, &ob) != PTP_RC_OK || !ob)
				continue;
			const char *fn = ob->oi.Filename ? ob->oi.Filename : "";
			if (strcmp(fn, want) != 0) continue;

			mkdir(outdir, 0755);
			char path[4096];
			snprintf(path, sizeof(path), "%s/%s", outdir, fn);
			int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd < 0) die("open outpath failed", fd);
			rc = ptp_getobject_tofd(params, h, fd);
			close(fd);
			struct stat st;
			long got = stat(path, &st) ? -1 : (long)st.st_size;
			printf("FETCH '%s' handle=0x%08x rc=0x%04x bytes=%lld expected=%llu -> %s\n",
			       fn, h, rc, got,
			       (unsigned long long)ob->oi.ObjectSize, path);
			found = 1;
		}
	}
	if (!found)
		printf("NOT FOUND: '%s' not in any storage's PTP object table\n", want);

	gp_camera_exit(camera, context);
	return found ? 0 : 2;
}
