#include "core/ensure.h"

#include <stdlib.h>
#include <stdio.h>

void fail(const char *msg)
{
	printf("fail: %s\n", msg);
	abort();
}
