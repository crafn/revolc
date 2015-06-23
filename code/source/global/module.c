#include "platform/file.h"
#include "global/module.h"

void init_module(Module *mod)
{
	if (mod->is_main_prog_module) {
		mod->dll= NULL;
		return;
	}

	char file[MAX_PATH_SIZE]= {};
	fmt_str(file, sizeof(file), "%s.%s", mod->extless_file, plat_dll_ext());

	// On Windows, unique path is needed so that the original can be overwritten
	// when recompiling at runtime
	const char *tmp_exts[]= {"_tmp0", "_tmp1", "_tmp2"};
	U32 tmp_ext_i= 0;
	do {
		fmt_str(	mod->tmp_file, sizeof(mod->tmp_file),	"%s%s",
					file, tmp_exts[tmp_ext_i]);
		++tmp_ext_i;
		if (tmp_ext_i > sizeof(tmp_exts)/sizeof(*tmp_exts))
			fail("Too many temp files present (%s)", mod->tmp_file);
		delete_file(mod->tmp_file);
	} while (file_exists(mod->tmp_file));
	copy_file(mod->tmp_file, file);

	mod->dll= load_dll(mod->tmp_file);
	if (!mod->dll)
		fail("Couldn't load dll '%s': %s", mod->extless_file, dll_error());

	bool has_worldgen= mod->worldgen_func_name[0] != 0;
	if (has_worldgen) {
		mod->worldgen=
			(WorldGenModuleImpl)rtti_func_ptr(mod->worldgen_func_name);
		if (!mod->worldgen)
			fail("worldgen_func not found: '%s'", mod->worldgen_func_name);
	}
	
	debug_print("DLL loaded: %s", mod->extless_file);
}

void deinit_module(Module *mod)
{
	if (mod->dll) {
		unload_dll(mod->dll);
		debug_print("DLL unloaded: %s", mod->extless_file);
		delete_file(mod->tmp_file);
	}
}

int json_module_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_file= json_value_by_key(j, "extless_file");
	JsonTok j_worldgen= json_value_by_key(j, "worldgen_func");

	if (json_is_null(j_file))
		RES_ATTRIB_MISSING("extless_file");

	Module m= {};
	if (!json_is_null(j_worldgen)) {
		fmt_str(m.worldgen_func_name, sizeof(m.worldgen_func_name),
				"%s", json_str(j_worldgen));
	}

	fmt_str(m.extless_file, sizeof(m.extless_file), "%s%s", j.json_dir, json_str(j_file));
	if (!strcmp(json_str(json_value_by_key(j, "name")), "main_prog"))
		m.is_main_prog_module= true;
	blob_write(buf, (U8*)&m + sizeof(Resource), sizeof(m) - sizeof(Resource));

	return 0;
error:
	return 1;
}
