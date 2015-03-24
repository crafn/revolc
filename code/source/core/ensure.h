#ifndef REVOLC_CORE_ENSURE_H
#define REVOLC_CORE_ENSURE_H

#include "build.h"
#include "platform/assert.h"

/// @todo ensure
#define ensure assert

REVOLC_API NORETURN
void fail(const char *format, ...);

#endif // REVOLC_CORE_ENSURE_H
