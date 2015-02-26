#ifndef REVOLC_GLOBAL_ENV_H
#define REVOLC_GLOBAL_ENV_H

#include "build.h"

struct Device;
struct PhysWorld;
struct ResBlob;
struct Renderer;

typedef struct Env {
	struct Device* device;
	struct PhysWorld* phys_world;
	struct Renderer* renderer;
	struct ResBlob* res_blob;
} Env;

extern REVOLC_API Env g_env;

#endif // REVOLC_GLOBAL_ENV_H
