#ifndef REVOLC_PLATFORM_STDLIB_H
#define REVOLC_PLATFORM_STDLIB_H

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define MEMBER_SIZE(st, m) (sizeof(((st*)0)->m))
#define MEMBER_OFFSET(st, m) (offsetof(st, m))

#endif // REVOLC_PLATFORM_STDLIB_H
