#include "core/ensure.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void fail(const char *format, ...)
{
	printf("\033[0;31mfail: ");
	va_list a;
	va_start(a, format);
	vprintf(format, a);
	va_end(a);
	printf("\033[0m\n");

	abort();
}
