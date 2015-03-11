#include "global/module.h"

void init_module(Module *mod)
{
	if (mod->is_main_prog_module) {
		mod->dll= NULL;
		return;
	}

	mod->dll= load_dll(mod->file);
	if (!mod->dll)
		fail("Couldn't load dll '%s': %s", mod->file, dll_error());
	debug_print("DLL loaded: %s", mod->file);
}

void deinit_module(Module *mod)
{
	if (mod->dll)
		unload_dll(mod->dll);
}

int json_module_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_file= json_value_by_key(j, "file");
	if (json_is_null(j_file))
		RES_ATTRIB_MISSING("file");

	Module m= {};
	snprintf(m.file, sizeof(m.file), "%s", json_str(j_file));
	if (!strcmp(json_str(json_value_by_key(j, "name")), "main_prog"))
		m.is_main_prog_module= true;
	blob_write(buf, (U8*)&m + sizeof(Resource), sizeof(m) - sizeof(Resource));

	return 0;
error:
	return 1;
}
