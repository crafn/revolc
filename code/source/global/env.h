#ifndef REVOLC_GLOBAL_ENV_H
#define REVOLC_GLOBAL_ENV_H

#include "build.h"

struct AudioSystem;
struct Device;
struct PhysWorld;
struct ResBlob;
struct Renderer;
struct World;

typedef struct Env {
	struct AudioSystem* audiosystem;
	struct Device* device;
	struct PhysWorld* physworld;
	struct Renderer* renderer;
	struct ResBlob* resblob;
	struct World* world;

	U8 *frame_mem_begin;
	U8 *frame_mem_end;
	U8 *frame_mem;
} Env;

extern REVOLC_API Env g_env;

// Allocates memory valid only for the current frame
REVOLC_API void * frame_alloc(U32 size);

REVOLC_API void init_frame_alloc(U32 size);
REVOLC_API void reset_frame_alloc();

#endif // REVOLC_GLOBAL_ENV_H
