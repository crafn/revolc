#include "core/ensure.h"
#include "platform/io.h"
#include "platform/stdlib.h"

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
