#include <stdarg.h>
#include <stdio.h>
void gp_log_with_source_location(int level, const char *file, int line, const char *func, const char *format, ...) {
    va_list ap; va_start(ap, format);
    fprintf(stderr, "[%d] %s:%d %s: ", level, file, line, func);
    vfprintf(stderr, format, ap); fprintf(stderr, "\n");
    va_end(ap);
}
void gp_log(int level, const char *domain, const char *format, ...) {
    va_list ap; va_start(ap, format);
    fprintf(stderr, "[%d] %s: ", level, domain);
    vfprintf(stderr, format, ap); fprintf(stderr, "\n");
    va_end(ap);
}
