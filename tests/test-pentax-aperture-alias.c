/* test-pentax-aperture-alias.c
 *
 * Regression test for the Pentax generic `aperture` compatibility alias
 * (issue #53, umbrella #52). Proves that ptp2 registers an "aperture" widget
 * for PTP_VENDOR_PENTAX backed by the same safe handlers as
 * `pentaxdirectaperture` (PTP_DPC_FNumber path), i.e. a real compatibility
 * wrapper rather than a name-only stub.
 *
 * The test dlopens the built ptp2 camlib found via the CAMLIBS environment
 * variable (set by meson to the build-tree camlib directories) and calls the
 * exported `ptp2_pentax_aperture_alias_present` introspection hook.
 */

#include "config.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*alias_present_func)(void);

static void
die (const char *msg)
{
	fprintf (stderr, "FAIL: %s\n", msg);
	exit (1);
}

int
main (void)
{
	const char *camlibs = getenv ("CAMLIBS");
	char *ptp2_path;
	void *handle;
	alias_present_func present;
	int result;

	if (!camlibs || !*camlibs) {
		fprintf (stderr, "SKIP: CAMLIBS not set; cannot locate ptp2 camlib\n");
		return 77; /* automake-style skip */
	}

	/* CAMLIBS is a colon-separated list of camlib directories. Try each
	 * entry for a ptp2.so and dlopen the first one that loads. */
	handle = NULL;
	ptp2_path = malloc (strlen (camlibs) + strlen ("/ptp2.so") + 1);
	if (!ptp2_path)
		die ("out of memory");

	for (char *entry = strtok (strdup (camlibs), ":"); entry;
	     entry = strtok (NULL, ":")) {
		char *candidate = malloc (strlen (entry) + strlen ("/ptp2.so") + 1);
		if (!candidate)
			die ("out of memory");
		strcpy (candidate, entry);
		strcat (candidate, "/ptp2.so");
		handle = dlopen (candidate, RTLD_NOW | RTLD_LOCAL);
		free (candidate);
		if (handle) {
			strcpy (ptp2_path, entry);
			strcat (ptp2_path, "/ptp2.so");
			break;
		}
	}
	if (!handle) {
		fprintf (stderr, "SKIP: no ptp2.so found under CAMLIBS=%s (%s)\n",
			 camlibs, dlerror());
		return 77;
	}

	{
		union { void *p; alias_present_func f; } u;

		u.p = dlsym (handle, "ptp2_pentax_aperture_alias_present");
		present = u.f;
	}
	if (!present) {
		fprintf (stderr, "FAIL: symbol ptp2_pentax_aperture_alias_present not exported by %s (%s)\n",
			 ptp2_path, dlerror());
		return 1;
	}

	result = present ();
	if (result != 1) {
		fprintf (stderr, "FAIL: Pentax `aperture` alias not wired to the "
			 "_get/_put_Pentax_DirectAperture handlers in capture_settings_menu\n");
		return 1;
	}

	printf ("OK: ptp2 registers a PENTAX `aperture` widget reusing the safe "
	       "direct-aperture (FNumber) handlers\n");
	return 0;
}
