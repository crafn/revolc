#ifndef REVOLC_GLOBAL_MODULE_H
#define REVOLC_GLOBAL_MODULE_H

#include "build.h"
#include "core/json.h"
#include "global/cfg.h"
#include "platform/dll.h"
#include "resources/resource.h"

struct World;
typedef void (*WorldGenModuleImpl)(struct World*);

typedef struct Module {
	Resource res;
	char extless_file[MAX_PATH_SIZE];
	char tmp_file[MAX_PATH_SIZE];
	bool is_main_prog_module; // If true, dll == g_main_program_dll
	DllHandle dll;

	char worldgen_func_name[MAX_FUNC_NAME_SIZE];
	WorldGenModuleImpl worldgen;
} PACKED Module;

REVOLC_API void init_module(Module *mod);
REVOLC_API void deinit_module(Module *mod);

REVOLC_API WARN_UNUSED
int json_module_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GLOBAL_MODULE_H
