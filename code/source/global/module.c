#include "core/basic.h"
#include "global/module.h"

void init_for_modules()
{
	U32 module_count;
	Module **modules = (Module**)all_res_by_type(&module_count,
												g_env.resblob,
												ResType_Module);
	for (U32 i = 0; i < module_count; ++i) {
		if (modules[i]->init)
			modules[i]->init();
	}
}

void deinit_for_modules()
{
	U32 module_count;
	Module **modules = (Module**)all_res_by_type(&module_count,
												g_env.resblob,
												ResType_Module);
	for (U32 i = 0; i < module_count; ++i) {
		if (modules[i]->deinit)
			modules[i]->deinit();
	}
}

void worldgen_for_modules(World *w)
{
	U32 module_count;
	Module **modules = (Module**)all_res_by_type(&module_count,
												g_env.resblob,
												ResType_Module);
	for (U32 i = 0; i < module_count; ++i) {
		if (modules[i]->worldgen)
			modules[i]->worldgen(w);
	}
}

void upd_for_modules()
{
	U32 module_count;
	Module **modules = (Module**)all_res_by_type(&module_count,
												g_env.resblob,
												ResType_Module);
	for (U32 i = 0; i < module_count; ++i) {
		if (modules[i]->upd)
			modules[i]->upd();
	}
}


void init_module(Module *mod)
{
	if (mod->is_main_prog_module) {
		mod->dll = NULL;
		return;
	}

	char file[MAX_PATH_SIZE] = {};
	fmt_str(file, sizeof(file), "%s.%s", mod->extless_file, plat_dll_ext());

	// On Windows, unique path is needed so that the original can be overwritten
	// when recompiling at runtime
	const char *tmp_exts[] = {"_tmp0", "_tmp1", "_tmp2"};
	U32 tmp_ext_i = 0;
	do {
		fmt_str(	mod->tmp_file, sizeof(mod->tmp_file),	"%s%s",
					file, tmp_exts[tmp_ext_i]);
		++tmp_ext_i;
		if (tmp_ext_i > sizeof(tmp_exts)/sizeof(*tmp_exts))
			fail("Too many temp files present (%s)", mod->tmp_file);
		delete_file(mod->tmp_file);
	} while (file_exists(mod->tmp_file));
	copy_file(mod->tmp_file, file);

	mod->dll = load_dll(mod->tmp_file);
	if (!mod->dll)
		fail("Couldn't load dll '%s': %s", mod->extless_file, dll_error());

	bool has_worldgen = mod->worldgen_func_name[0] != 0;
	bool has_init = mod->init_func_name[0] != 0;
	bool has_deinit = mod->deinit_func_name[0] != 0;
	bool has_upd = mod->upd_func_name[0] != 0;
	if (has_worldgen) {
		mod->worldgen =
			(WorldGenModuleImpl)rtti_func_ptr(mod->worldgen_func_name);
		if (!mod->worldgen)
			fail("worldgen_func not found: '%s'", mod->worldgen_func_name);
	}
	if (has_init) {
		mod->init =
			(InitModuleImpl)rtti_func_ptr(mod->init_func_name);
		if (!mod->init)
			fail("init_func not found: '%s'", mod->init_func_name);
	}
	if (has_deinit) {
		mod->deinit =
			(DeinitModuleImpl)rtti_func_ptr(mod->deinit_func_name);
		if (!mod->deinit)
			fail("deinit_func not found: '%s'", mod->deinit_func_name);
	}
	if (has_upd) {
		mod->upd =
			(UpdModuleImpl)rtti_func_ptr(mod->upd_func_name);
		if (!mod->upd)
			fail("upd_func not found: '%s'", mod->upd_func_name);
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
	JsonTok j_file = json_value_by_key(j, "extless_file");
	JsonTok j_worldgen = json_value_by_key(j, "worldgen_func");
	JsonTok j_init = json_value_by_key(j, "init_func");
	JsonTok j_deinit = json_value_by_key(j, "deinit_func");
	JsonTok j_upd = json_value_by_key(j, "upd_func");

	if (json_is_null(j_file))
		RES_ATTRIB_MISSING("extless_file");

	Module m = {};
	if (!json_is_null(j_worldgen)) {
		fmt_str(m.worldgen_func_name, sizeof(m.worldgen_func_name),
				"%s", json_str(j_worldgen));
	}
	if (!json_is_null(j_init)) {
		fmt_str(m.init_func_name, sizeof(m.init_func_name),
				"%s", json_str(j_init));
	}
	if (!json_is_null(j_deinit)) {
		fmt_str(m.deinit_func_name, sizeof(m.deinit_func_name),
				"%s", json_str(j_deinit));
	}
	if (!json_is_null(j_upd)) {
		fmt_str(m.upd_func_name, sizeof(m.upd_func_name),
				"%s", json_str(j_upd));
	}

	fmt_str(m.rel_extless_file, sizeof(m.rel_extless_file), "%s", json_str(j_file));
	fmt_str(m.extless_file, sizeof(m.extless_file), "%s%s", j.json_dir, json_str(j_file));
	if (!strcmp(json_str(json_value_by_key(j, "name")), "main_prog"))
		m.is_main_prog_module = true;

	blob_write(buf, &m, sizeof(m));

	return 0;
error:
	return 1;
}

Module *blobify_module(struct WArchive *ar, Cson c, bool *err)
{
	Cson c_file = cson_key(c, "extless_file");
	Cson c_worldgen = cson_key(c, "worldgen_func");
	Cson c_init = cson_key(c, "init_func");
	Cson c_deinit = cson_key(c, "deinit_func");
	Cson c_upd = cson_key(c, "upd_func");
	Cson c_name = cson_key(c, "name");

	if (cson_is_null(c_file))
		RES_ATTRIB_MISSING("extless_file");

	Module m = {};
	if (!cson_is_null(c_worldgen)) {
		fmt_str(m.worldgen_func_name, sizeof(m.worldgen_func_name),
				"%s", blobify_string(c_worldgen, err));
	}
	if (!cson_is_null(c_init)) {
		fmt_str(m.init_func_name, sizeof(m.init_func_name),
				"%s", blobify_string(c_init, err));
	}
	if (!cson_is_null(c_deinit)) {
		fmt_str(m.deinit_func_name, sizeof(m.deinit_func_name),
				"%s", blobify_string(c_deinit, err));
	}
	if (!cson_is_null(c_upd)) {
		fmt_str(m.upd_func_name, sizeof(m.upd_func_name),
				"%s", blobify_string(c_upd, err));
	}

	fmt_str(m.rel_extless_file, sizeof(m.rel_extless_file), "%s", blobify_string(c_file, err));
	fmt_str(m.extless_file, sizeof(m.extless_file), "%s%s", c.dir_path, blobify_string(c_file, err));
	const char *name = blobify_string(c_name, err);
	if (name && !strcmp(name, "main_prog"))
		m.is_main_prog_module = true;

	if (err && *err)
		goto error;

	Module *ptr = warchive_ptr(ar);
	pack_buf(ar, &m, sizeof(m));
	return ptr;

error:
	SET_ERROR_FLAG(err);
	return NULL;
}

void deblobify_module(WCson *c, struct RArchive *ar)
{
	Module m;
	unpack_buf(ar, &m, sizeof(m));

	wcson_begin_compound(c, "Module");

	wcson_designated(c, "name");
	deblobify_string(c, m.res.name);

	wcson_designated(c, "extless_file");
	deblobify_string(c, m.rel_extless_file);

	wcson_designated(c, "worldgen_func");
	deblobify_string(c, m.worldgen_func_name);

	wcson_designated(c, "init_func");
	deblobify_string(c, m.init_func_name);

	wcson_designated(c, "deinit_func");
	deblobify_string(c, m.deinit_func_name);

	wcson_designated(c, "upd_func");
	deblobify_string(c, m.upd_func_name);

	wcson_end_compound(c);
}
