#include "global/rtti.h"
#include "nodetype.h"
#include "resources/resblob.h"

void init_nodetype(NodeType *node)
{
	bool resurrect_by_memcpy=
		!strcmp(node->resurrect_func_name, "memcpy");

	node->alloc=
		(AllocNodeImpl)func_ptr(node->alloc_func_name);
	node->free=
		(FreeNodeImpl)func_ptr(node->free_func_name);
	node->upd=
		(UpdNodeImpl)func_ptr(node->upd_func_name);
	node->storage=
		(StorageNodeImpl)func_ptr(node->storage_func_name);
	if (!resurrect_by_memcpy) {
		node->resurrect=
			(ResurrectNodeImpl)func_ptr(node->resurrect_func_name);
	}

	if (!node->alloc)
		fail("Func not found: %s", node->alloc_func_name);
	if (!node->free)
		fail("Func not found: %s", node->free_func_name);
	if (!node->upd && node->upd_func_name[0])
		fail("Func not found: %s", node->upd_func_name);
	if (!node->storage)
		fail("Func not found: %s", node->storage_func_name);
	if (!node->resurrect && !resurrect_by_memcpy)
		fail("Func not found: %s", node->resurrect_func_name);

	// Figure out node struct size by custom RTTI

	node->size= struct_size(node->res.name);
	if (!node->size)
		fail("Couldn't find struct %s size. Has codegen run?",
				node->res.name);
}

int json_nodetype_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_alloc= json_value_by_key(j, "alloc_func");
	JsonTok j_free= json_value_by_key(j, "free_func");
	JsonTok j_upd= json_value_by_key(j, "upd_func");
	JsonTok j_storage= json_value_by_key(j, "storage_func");
	JsonTok j_resurrect= json_value_by_key(j, "resurrect_func");

	if (json_is_null(j_alloc))
		RES_ATTRIB_MISSING("alloc_func");
	if (json_is_null(j_free))
		RES_ATTRIB_MISSING("free_func");
	if (json_is_null(j_upd))
		RES_ATTRIB_MISSING("upd_func");
	if (json_is_null(j_storage))
		RES_ATTRIB_MISSING("storage");
	if (json_is_null(j_resurrect))
		RES_ATTRIB_MISSING("resurrect_func");

	NodeType n= {};
	snprintf(
		n.alloc_func_name, sizeof(n.alloc_func_name), "%s", json_str(j_alloc));
	snprintf(
		n.free_func_name, sizeof(n.free_func_name), "%s", json_str(j_free));
	snprintf(
		n.upd_func_name, sizeof(n.upd_func_name), "%s", json_str(j_upd));
	snprintf(
		n.storage_func_name, sizeof(n.storage_func_name), "%s", json_str(j_storage));
	snprintf(
		n.resurrect_func_name, sizeof(n.resurrect_func_name), "%s", json_str(j_resurrect));
	blob_write(	buf,
				(U8*)&n + sizeof(Resource),
				sizeof(n) - sizeof(Resource));
	return 0;
}
