#ifndef REVOLC_PLATFORM_STDLIB_H
#define REVOLC_PLATFORM_STDLIB_H

#ifndef CODEGEN
#	include <stdarg.h>
#	include <stdlib.h>
#	include <string.h>
#	include <stddef.h>
#endif

#define MEMBER_SIZE(st, m) (sizeof(((st*)0)->m))
#define MEMBER_OFFSET(st, m) (offsetof(st, m))

// Format string
// Will always (when size > 0) insert '\0' at the end of `str`
// Use instead of snprintf which is horribly broken on mingw (due to microsoft CRT)
REVOLC_API int fmt_str(char *str, U32 size, const char *fmt, ...);
REVOLC_API int v_fmt_str(char *str, U32 size, const char *fmt, va_list args);

#endif // REVOLC_PLATFORM_STDLIB_H
