#include <gphoto2/gphoto2.h>
#include <glib.h>

int create_directory(const char *path) {
    if (g_mkdir(path, 0755) != 0)
        return GP_ERROR_FAILED;
    
    return GP_OK;
}

int delete_file(const char *path) {
    if (g_remove(path) != 0)
        return GP_ERROR_FAILED;
    
    return GP_OK;
}

int copy_file(const char *src, const char *dest) {
    GFileInputStream *in = NULL;
    GFileOutputStream *out = NULL;
    GError *err = NULL;

    in = g_file_open(src, "r", NULL, &err);
    if (!in)
        return GP_ERROR_FAILED;

    out = g_file_open(dest, "w", NULL, &err);
    if (!out) {
        g_object_unref(in);
        return GP_ERROR_FAILED;
    }

    // Copy file contents
    char buffer[1024];
    ssize_t size;
    
    while ((size = g_input_stream_read(G_INPUT_STREAM(in), buffer, sizeof(buffer))) > 0)
        g_output_stream_write(G_OUTPUT_STREAM(out), buffer, size, NULL);

    g_object_unref(in);
    g_object_unref(out);
    
    return GP_OK;
}

int move_file(const char *src, const char *dest) {
    if (g_rename(src, dest) != 0)
        return GP_ERROR_FAILED;
    
    return GP_OK;
}
