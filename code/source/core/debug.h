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

REVOLC_API void plat_set_term_color(TermColor c);

// Misc debug and dev info
typedef struct Debug {
#	define DT_HISTORY_COUNT (60*10)
	F32 dt_history[DT_HISTORY_COUNT];
} Debug;

REVOLC_API Debug *create_debug();
REVOLC_API void destroy_debug(Debug *d);
REVOLC_API void upd_debug(Debug *d);

#endif // REVOLC_CORE_DEBUG_PRINT_H
