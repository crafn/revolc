#ifndef REVOLC_CORE_ENSURE_H
#define REVOLC_CORE_ENSURE_H

#include "build.h"
#include "platform/assert.h"

// @todo ensure
// @todo CAPS
#define ensure(x) \
	do { if (!(x)) fail("ensure failed: %s", #x); } while(0)

REVOLC_API NORETURN
void fail(const char *format, ...);

#endif // REVOLC_CORE_ENSURE_H
