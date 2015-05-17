#ifndef REVOLC_CORE_DEBUG_PRINT_H
#define REVOLC_CORE_DEBUG_PRINT_H

#include "build.h"

REVOLC_API void debug_print(const char *format, ...);
REVOLC_API void critical_print(const char *format, ...);

#endif // REVOLC_CORE_DEBUG_PRINT_H
