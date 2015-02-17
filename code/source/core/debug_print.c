#include "debug_print.h"

#include <stdarg.h>
#include <stdio.h>

void debug_print(const char *format, ...)
{
	va_list args;

	va_start(args, format);
    vprintf(format, args);
    va_end(args);

	printf("\n");
}
