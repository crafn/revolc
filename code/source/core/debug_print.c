#include "debug_print.h"
#include "platform/stdlib.h"
#include "platform/term.h"

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
