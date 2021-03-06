#ifndef REVOLC_GLOBAL_MODULE_H
#define REVOLC_GLOBAL_MODULE_H

#include "build.h"
#include "core/dll.h"
#include "core/cson.h"
#include "global/cfg.h"
#include "resources/resource.h"

struct World;
typedef void (*InitModuleImpl)();
typedef void (*DeinitModuleImpl)();
typedef void (*WorldGenModuleImpl)(struct World*);
typedef void (*UpdModuleImpl)();

typedef struct Module {
	Resource res;
	char extless_file[MAX_PATH_SIZE];
	char rel_extless_file[MAX_PATH_SIZE];
	char tmp_file[MAX_PATH_SIZE];
	bool is_main_prog_module; // If true, dll == g_main_program_dll
	DllHandle dll;

	char worldgen_func_name[MAX_FUNC_NAME_SIZE];
	char init_func_name[MAX_FUNC_NAME_SIZE];
	char deinit_func_name[MAX_FUNC_NAME_SIZE];
	char upd_func_name[MAX_FUNC_NAME_SIZE];
	InitModuleImpl init; // Called during resource load
	DeinitModuleImpl deinit;
	WorldGenModuleImpl worldgen;
	UpdModuleImpl upd;
} PACKED Module;

// Call module init/deinit for all modules.
// Must be separated from res init because module init is after
// subsystem init, which is after resource init.
REVOLC_API void init_for_modules();
REVOLC_API void deinit_for_modules();
REVOLC_API void worldgen_for_modules(World *w);
REVOLC_API void upd_for_modules();

// Module _resource_ init/deinit
REVOLC_API void init_module(Module *mod);
REVOLC_API void deinit_module(Module *mod);

REVOLC_API WARN_UNUSED
Module *blobify_module(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_module(WCson *c, struct RArchive *ar);

#endif // REVOLC_GLOBAL_MODULE_H
