#ifndef REVOLC_GLOBAL_ENV_H
#define REVOLC_GLOBAL_ENV_H

#include "build.h"

struct ResBlob;

typedef struct {
	struct ResBlob* res_blob;
} Env;

extern REVOLC_API Env g_env;

#endif // REVOLC_GLOBAL_ENV_H
