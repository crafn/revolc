#ifndef REVOLC_CORE_ENSURE_H
#define REVOLC_CORE_ENSURE_H

#include "build.h"

#include <assert.h>

/// @todo ensure
#define ensure assert

REVOLC_API
void fail(const char *msg);

#endif // REVOLC_CORE_ENSURE_H
