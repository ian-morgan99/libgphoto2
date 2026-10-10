#include <stdarg.h>
#include <stdio.h>

void
gp_log_with_source_location (int level, const char *file, int line,
	const char *function, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	fprintf (stderr, "[%d] %s:%d %s: ", level, file, line, function);
	vfprintf (stderr, format, args);
	fputc ('\n', stderr);
	va_end (args);
}

void
gp_log (int level, const char *domain, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	fprintf (stderr, "[%d] %s: ", level, domain);
	vfprintf (stderr, format, args);
	fputc ('\n', stderr);
	va_end (args);
}
