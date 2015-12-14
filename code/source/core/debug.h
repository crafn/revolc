#ifndef REVOLC_CORE_DEBUG_PRINT_H
#define REVOLC_CORE_DEBUG_PRINT_H

#include "build.h"

REVOLC_API void debug_print(const char *format, ...);
REVOLC_API void critical_print(const char *format, ...);

// @todo release/debug ensure
// @todo ensure -> ASSERT
#define ensure(x) \
	do { if (!(x)) fail("ensure failed (%s: %i): %s", __FILE__, __LINE__, #x); } while(0)

REVOLC_API NORETURN
void fail(const char *format, ...);

typedef enum {
	TermColor_default,
	TermColor_red,
} TermColor;

void plat_set_term_color(TermColor c);

#endif // REVOLC_CORE_DEBUG_PRINT_H
