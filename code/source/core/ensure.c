#include "core/ensure.h"
#include "platform/io.h"
#include "platform/stdlib.h"
#include "platform/term.h"

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
