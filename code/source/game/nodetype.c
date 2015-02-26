#include "nodetype.h"
#include "platform/dll.h"
#include "resources/resblob.h"

void init_nodetype(NodeType *node)
{
	node->alloc=
		(AllocNodeImpl)query_dll_sym(main_program_dll, node->alloc_func_name);
	node->free=
		(FreeNodeImpl)query_dll_sym(main_program_dll, node->free_func_name);
	node->storage=
		(StorageNodeImpl)query_dll_sym(main_program_dll, node->storage_func_name);
	node->upd=
		(UpdNodeImpl)query_dll_sym(main_program_dll, node->upd_func_name);

	if (!node->alloc)
		critical_print("Func not found: %s", node->alloc_func_name);
	if (!node->free)
		critical_print("Func not found: %s", node->free_func_name);
	if (!node->storage)
		critical_print("Func not found: %s", node->storage_func_name);
	if (!node->upd)
		critical_print("Func not found: %s", node->upd_func_name);
}

int json_nodetype_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_alloc= json_value_by_key(j, "alloc_func");
	JsonTok j_free= json_value_by_key(j, "free_func");
	JsonTok j_upd= json_value_by_key(j, "upd_func");
	JsonTok j_storage= json_value_by_key(j, "storage_func");

	if (json_is_null(j_alloc)) {
		critical_print("Attrib 'alloc_func' missing");
		return 1;
	}
	if (json_is_null(j_free)) {
		critical_print("Attrib 'free_func' missing");
		return 1;
	}
	if (json_is_null(j_upd)) {
		critical_print("Attrib 'upd_func' missing");
		return 1;
	}
	if (json_is_null(j_storage)) {
		critical_print("Attrib 'storage_func' missing");
		return 1;
	}

	NodeType n= {};
	snprintf(
		n.alloc_func_name, sizeof(n.alloc_func_name), "%s", json_str(j_alloc));
	snprintf(
		n.free_func_name, sizeof(n.free_func_name), "%s", json_str(j_free));
	snprintf(
		n.upd_func_name, sizeof(n.upd_func_name), "%s", json_str(j_upd));
	snprintf(
		n.storage_func_name, sizeof(n.storage_func_name), "%s", json_str(j_storage));
	blob_write(	buf,
				(U8*)&n + sizeof(Resource),
				sizeof(n) - sizeof(Resource));
	return 0;
}
