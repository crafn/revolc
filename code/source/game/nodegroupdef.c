#include "global/rtti.h"
#include "nodegroupdef.h"
#include "resources/resblob.h"

internal
U32 node_i_by_name(NodeGroupDef *def, const char *name)
{
	for (U32 i= 0; i < def->node_count; ++i) {
		if (!strcmp(def->nodes[i].name, name))
			return i;
	}

	fail("Node not found: %s", name);
	return 0;
}

// e.g. "asd .fgh" -> { "asd", "fgh" }
internal
void split_str(char **dst, U32 dst_array_size, U32 dst_str_size,
		const char separator, const char *input)
{
	U32 i= 0;
	const char *begin= input;
	bool end_reached= false;
	while (i < dst_array_size) {
		const char *end= strchr(begin, separator);
		if (!end && end_reached)
			break;

		if (!end) {
			end= begin + strlen(begin);
			end_reached= true;
		}

		const char *sep_tok= end;

		while (*begin == ' ')
			++begin;
		while (end > begin && *(end - 1) == ' ')
			--end;

		U32 count= end - begin + 1; // Account null-byte
		if (count > dst_str_size)
			count= dst_str_size;
		snprintf(dst[i], count, "%s", begin);
		begin= sep_tok + 1;
		++i;
	}
}

int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_nodes= json_value_by_key(j, "nodes");
	JsonTok j_cmds= json_value_by_key(j, "cmds");
	if (json_is_null(j_nodes))
		RES_ATTRIB_MISSING("nodes");
	if (json_is_null(j_cmds))
		RES_ATTRIB_MISSING("cmds");

	NodeGroupDef def= {};
	for (U32 node_i= 0; node_i < json_member_count(j_nodes); ++node_i) {
		JsonTok j_node= json_member(j_nodes, node_i);
		NodeGroupDef_Node *node= &def.nodes[node_i];

		JsonTok j_type_name= json_value_by_key(j_node, "type");
		JsonTok j_name= json_value_by_key(j_node, "name");
		if (json_is_null(j_type_name))
			RES_ATTRIB_MISSING("type");
		if (json_is_null(j_name))
			RES_ATTRIB_MISSING("name");

		snprintf(	node->type_name,
					sizeof(def.nodes[node_i].type_name),
					"%s", json_str(j_type_name));

		snprintf(	node->name,
					sizeof(def.nodes[node_i].name),
					"%s", json_str(j_name));

		node->default_struct_size= struct_size(node->type_name);
		ensure(node->default_struct_size < MAX_DEFAULT_STRUCT_SIZE);

		JsonTok j_defaults= json_value_by_key(j_node, "defaults");
		for (U32 i= 0; i < json_member_count(j_defaults); ++i) {
			JsonTok j_d_obj= json_member(j_defaults, i);
			ensure(json_member_count(j_d_obj) == 1);

			JsonTok j_default_field= json_member(j_d_obj, 0);
			JsonTok j_default_value= json_member(j_default_field, 0);
			ensure(json_is_string(j_default_field));
			ensure(json_is_string(j_default_value)); /// @todo All types

			const char *field_str= json_str(j_default_field);
			const char *value_str= json_str(j_default_value);

			U32 size= member_size(node->type_name, field_str);
			U32 offset= member_offset(node->type_name, field_str);

			const U32 value_size= strlen(value_str) + 1;
			ensure(offset + size < MAX_DEFAULT_STRUCT_SIZE);
			ensure(value_size <= size);
			memcpy(node->default_struct + offset, value_str, value_size);
			memset(node->default_struct_set_bytes + offset, 1, value_size);
		}

		++def.node_count;
	}

	for (U32 cmd_i= 0; cmd_i < json_member_count(j_cmds); ++cmd_i) {
		JsonTok j_cmd= json_member(j_cmds, cmd_i);
		const char *cmd_str= json_str(j_cmd);

		// Split "a.b = c.d"
		char src_str[RES_NAME_SIZE*2]= {};
		char dst_str[RES_NAME_SIZE*2]= {};
		split_str(
				(char*[]) {src_str, dst_str}, 2, RES_NAME_SIZE*2,
				'=', cmd_str);

		// Split "a.b"
		char src_node_name[RES_NAME_SIZE]= {};
		char src_member_name[RES_NAME_SIZE]= {};
		split_str(
				(char*[]) {src_node_name, src_member_name}, 2, RES_NAME_SIZE,
				'.', src_str);

		// Split "c.d"
		char dst_node_name[RES_NAME_SIZE]= {};
		char dst_member_name[RES_NAME_SIZE]= {};
		split_str(
				(char*[]) {dst_node_name, dst_member_name}, 2, RES_NAME_SIZE,
				'.', dst_str);

		U32 src_node_i= node_i_by_name(&def, src_node_name);
		U32 dst_node_i= node_i_by_name(&def, dst_node_name);
		ensure(src_node_i < def.node_count);
		ensure(dst_node_i < def.node_count);

		const char *src_type_name= def.nodes[src_node_i].type_name;
		const char *dst_type_name= def.nodes[dst_node_i].type_name;

		NodeGroupDef_Node *src_node= &def.nodes[src_node_i];
		NodeGroupDef_Node_Output *out= &src_node->outputs[src_node->output_count];

		*out= (NodeGroupDef_Node_Output) {
			.src_offset= member_offset(src_type_name, src_member_name),
			.dst_offset= member_offset(dst_type_name, dst_member_name),
			.dst_node_i= dst_node_i,
			.size= member_size(src_type_name, src_member_name),
		};

		// Src should be the same size as dst
		ensure(out->size == member_size(dst_type_name, dst_member_name));

		++src_node->output_count;
	}

	blob_write(	buf,
				(U8*)&def + sizeof(Resource),
				sizeof(def) - sizeof(Resource));
	return 0;
}
