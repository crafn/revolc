#include "core/ensure.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void fail(const char *format, ...)
{
	printf("fail: ");
	va_list a;
	va_start(a, format);
	vprintf(format, a);
	va_end(a);
	printf("\n");

	abort();
}
