#include "global/env.h"
#include "global/module.h"
#include "global/rtti.h"
#include "nodetype.h"
#include "resources/resblob.h"

void init_nodetype(NodeType *node)
{
	bool has_init = node->init_func_name[0] != 0;
	bool has_resurrect = node->resurrect_func_name[0] != 0;
	bool has_overwrite = node->overwrite_func_name[0] != 0;
	bool has_free = node->free_func_name[0] != 0;
	bool has_upd = node->upd_func_name[0] != 0;
	bool has_pack = node->pack_func_name[0] != 0;
	bool has_unpack = node->unpack_func_name[0] != 0;

	if (has_init) {
		node->init = (InitNodeImpl)rtti_func_ptr(node->init_func_name);
		if (!node->init)
			fail("init_func not found: '%s'", node->init_func_name);
	}

	if (has_resurrect) {
		node->resurrect = (ResurrectNodeImpl)rtti_func_ptr(node->resurrect_func_name);
		if (!node->resurrect)
			fail("resurrect_func not found: '%s'", node->resurrect_func_name);
	}

	if (has_overwrite) {
		node->overwrite = (OverwriteNodeImpl)rtti_func_ptr(node->overwrite_func_name);
		if (!node->overwrite)
			fail("overwrite_func not found: '%s'", node->overwrite_func_name);
	}

	if (has_free) {
		node->free = (FreeNodeImpl)rtti_func_ptr(node->free_func_name);
		if (!node->free)
			fail("free_func not found: '%s'", node->free_func_name);
	}

	if (has_upd) {
		node->upd = (UpdNodeImpl)rtti_func_ptr(node->upd_func_name);
		if (!node->upd)
			fail("upd_func not found: '%s'", node->upd_func_name);
	}

	if (has_pack) {
		node->pack = (PackNodeImpl)rtti_func_ptr(node->pack_func_name);
		if (!node->pack)
			fail("pack_func not found: '%s'", node->pack_func_name);
	}

	if (has_unpack) {
		node->unpack = (UnpackNodeImpl)rtti_func_ptr(node->unpack_func_name);
		if (!node->unpack)
			fail("unpack_func not found: '%s'", node->unpack_func_name);
	}

	if (node->auto_impl_mgmt == false) {
		node->storage = (StorageNodeImpl)rtti_func_ptr(node->storage_func_name);
		if (!node->storage)
			fail("Func not found: %s", node->storage_func_name);
	}

	StructRtti *s = rtti_struct(node->res.name);
	ensure(s);
	node->size = s->size;
	if (!node->size)
		fail("Couldn't find struct %s size. Has codegen run?", node->res.name);
}

int json_nodetype_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_impl_mgmt = json_value_by_key(j, "impl_mgmt");
	JsonTok j_max_count = json_value_by_key(j, "max_count");
	JsonTok j_init = json_value_by_key(j, "init_func");
	JsonTok j_resurrect = json_value_by_key(j, "resurrect_func");
	JsonTok j_overwrite = json_value_by_key(j, "overwrite_func");
	JsonTok j_free = json_value_by_key(j, "free_func");
	JsonTok j_upd = json_value_by_key(j, "upd_func");
	JsonTok j_storage = json_value_by_key(j, "storage_func");
	JsonTok j_pack = json_value_by_key(j, "pack_func");
	JsonTok j_unpack = json_value_by_key(j, "unpack_func");
	JsonTok j_packsync = json_value_by_key(j, "packsync");

	if (json_is_null(j_impl_mgmt))
		RES_ATTRIB_MISSING("impl_mgmt");
	if (json_is_null(j_init))
		RES_ATTRIB_MISSING("init_func");
	if (json_is_null(j_resurrect))
		RES_ATTRIB_MISSING("resurrect_func");
	if (json_is_null(j_overwrite))
		RES_ATTRIB_MISSING("overwrite_func");
	if (json_is_null(j_upd))
		RES_ATTRIB_MISSING("upd_func");
	if (json_is_null(j_free))
		RES_ATTRIB_MISSING("free_func");
	if (json_is_null(j_pack))
		RES_ATTRIB_MISSING("pack_func");
	if (json_is_null(j_unpack))
		RES_ATTRIB_MISSING("unpack_func");

	bool auto_impl_mgmt = !strcmp(json_str(j_impl_mgmt), "auto");
	U32 max_count = 0;
	if (auto_impl_mgmt == false) {
		if (json_is_null(j_storage) || strlen(json_str(j_storage)) == 0)
			RES_ATTRIB_MISSING("storage_func");
		if (json_is_null(j_resurrect) || strlen(json_str(j_resurrect)) == 0)
			RES_ATTRIB_MISSING("resurrect_func");
	} else {
		if (json_is_null(j_max_count))
			RES_ATTRIB_MISSING("max_count");
		if (!json_is_null(j_storage)) {
			critical_print("Shouldn't have 'storage_func', impl_mgmt is 'auto'");
			goto error;
		}
	}
	if (!json_is_null(j_max_count))
		max_count = json_integer(j_max_count);

	NodeType n = {
		.auto_impl_mgmt = auto_impl_mgmt,
		.max_count = max_count,
	};
	fmt_str(n.init_func_name, sizeof(n.init_func_name), "%s", json_str(j_init));
	fmt_str(n.resurrect_func_name, sizeof(n.resurrect_func_name), "%s", json_str(j_resurrect));
	fmt_str(n.overwrite_func_name, sizeof(n.overwrite_func_name), "%s", json_str(j_overwrite));
	fmt_str(n.upd_func_name, sizeof(n.upd_func_name), "%s", json_str(j_upd));
	fmt_str(n.free_func_name, sizeof(n.free_func_name), "%s", json_str(j_free));
	fmt_str(n.pack_func_name, sizeof(n.pack_func_name), "%s", json_str(j_pack));
	fmt_str(n.unpack_func_name, sizeof(n.unpack_func_name), "%s", json_str(j_unpack));

	if (n.auto_impl_mgmt == false) {
		fmt_str(n.storage_func_name, sizeof(n.storage_func_name), "%s", json_str(j_storage));
	}

	if (json_is_null(j_packsync)) {
		n.packsync = PackSync_full;
	} else {
		const char *net = json_str(j_packsync);
		if (!strcmp(net, "presence")) {
			n.packsync = PackSync_presence;
		} else if (!strcmp(net, "full")) {
			n.packsync = PackSync_full;
		} else {
			critical_print("Invalid packsync: %s", net);
			goto error;
		}
	}

	blob_write(buf, &n, sizeof(n));

	return 0;

error:
	return 1;
}
