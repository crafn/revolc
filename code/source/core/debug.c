#include "basic.h"
#include "debug.h"
#include "device.h"
#include "visual/renderer.h"

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

Debug *create_debug()
{
	Debug *d = ZERO_ALLOC(gen_ator(), sizeof(*d), "debug");
	return d;
}

void destroy_debug(Debug *d)
{
	FREE(gen_ator(), d);
}

void upd_debug(Debug *d)
{
	memmove(d->dt_history, d->dt_history + 1, sizeof(*d->dt_history)*(DT_HISTORY_COUNT - 1));
	d->dt_history[DT_HISTORY_COUNT - 1] = g_env.device->dt;

	F32 max = 0;
	for (U32 i = 0; i < DT_HISTORY_COUNT; ++i) {
		if (d->dt_history[i] > max)
			max = d->dt_history[i];
	}

	int height = 128;
	for (U32 i = 0; i < DT_HISTORY_COUNT; ++i) {
		V2i size = {1, d->dt_history[i]/max*height};
		V2i pos = {i, height - size.y};
		drawcmd_px_quad(pos, size, (Color) {1.f, 0.f, 0.f, 0.9f}, 9999999);
	}
}

