#ifndef REVOLC_GLOBAL_ENV_H
#define REVOLC_GLOBAL_ENV_H

#include "build.h"

struct Device;
struct ResBlob;
struct Renderer;

typedef struct {
	struct Device* device;
	struct Renderer* renderer;
	struct ResBlob* res_blob;
} Env;

extern REVOLC_API Env g_env;

#endif // REVOLC_GLOBAL_ENV_H
