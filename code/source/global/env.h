#ifndef REVOLC_GLOBAL_ENV_H
#define REVOLC_GLOBAL_ENV_H

#include "build.h"
#include "core/memory.h"

struct AudioSystem;
struct Debug;
struct Editor;
struct Device;
struct PhysWorld;
struct ResBlob;
struct Renderer;
struct NetState;
struct UiContext;
struct World;

struct SymbolTable;

typedef struct Env {
	struct AudioSystem *audiosystem;
	struct Debug *debug;
	struct Editor *editor;
	struct Device *device;
	struct PhysWorld *physworld;
	struct Renderer *renderer;
	struct ResBlob *resblob;
	struct NetState *netstate;
	struct UiContext *uicontext;
	struct World *world;

	U32 argc;
	const char **argv;

	F64 time_from_start;
	F64 dt;

	void *game_data; // Pointer to be used by the game module

	Ator frame_ator;

	U32 prod_heap_alloc_count; // Not thread-safe
	bool os_allocs_forbidden;

	// All symbols which have been queried using rtti
	// This is used to relocate pointers to dll on reload, see `rtti_relocate`
	struct SymbolTable *used_rtti_symbols;
} Env;

extern REVOLC_API Env g_env;

REVOLC_API void init_env(U32 argc, const char **argv);
REVOLC_API void deinit_env();


#endif // REVOLC_GLOBAL_ENV_H
