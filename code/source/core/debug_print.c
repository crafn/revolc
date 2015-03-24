#include "debug_print.h"
#include "platform/stdlib.h"

void debug_print(const char *fmt, ...)
{
	va_list a;
	va_start(a, fmt);
    vprintf(fmt, a);
    va_end(a);

	printf("\n");
}

void critical_print(const char *fmt, ...)
{
	printf("\033[0;31m"); // Red
	va_list a;
	va_start(a, fmt);
    vprintf(fmt, a);
    va_end(a);

	printf("\033[0m\n");
}
