#include "basic.h"
#include "debug.h"

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
	plat_set_term_color(TermColor_red);

	va_list a;
	va_start(a, fmt);
    vprintf(fmt, a);
    va_end(a);
	printf("\n");

	plat_set_term_color(TermColor_default);
}

void fail(const char *format, ...)
{
	plat_set_term_color(TermColor_red);

	va_list a;
	va_start(a, format);
	vprintf(format, a);
	va_end(a);

	plat_set_term_color(TermColor_default);
	printf("\n");

	asm("int $3"); // For mingw breakpoint problems when b exit doesn't work
	abort();
}

