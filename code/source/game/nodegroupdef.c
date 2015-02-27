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

	return (U32)-1;
}

int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_nodes= json_value_by_key(j, "nodes");
	if (json_is_null(j_nodes))
		RES_ATTRIB_MISSING("nodes");

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

			ensure(offset + size < MAX_DEFAULT_STRUCT_SIZE);
			ensure(strlen(value_str) <= offset);
			memcpy(node->default_struct, value_str, strlen(value_str));
		}

		++def.node_count;
	}

	// Outputs in separate pass -- they can refer to arbitrary nodes
	for (U32 node_i= 0; node_i < def.node_count; ++node_i) {
		JsonTok j_node= json_member(j_nodes, node_i);
		NodeGroupDef_Node *node= &def.nodes[node_i];

		// Outputs
		JsonTok j_outputs= json_value_by_key(j_node, "outputs");
		for (U32 out_i= 0; out_i < json_member_count(j_outputs); ++out_i) {
			JsonTok j_out= json_member(j_outputs, out_i);
			ensure(json_is_object(j_out));
			NodeGroupDef_Node_Output *out= &node->outputs[out_i];

			JsonTok j_src= json_member(j_out, 0);

			const char *src_slot= json_str(j_src);

			JsonTok j_dst= json_member(j_src, 0);
			ensure(json_is_array(j_dst));
			ensure(json_member_count(j_dst) == 2); /// @todo Proper errors

			U32 dst_node_i= node_i_by_name(&def, json_str(json_member(j_dst, 0)));
			ensure(dst_node_i < def.node_count);
			const char *dst_type_name= def.nodes[dst_node_i].type_name;
			const char *dst_slot= json_str(json_member(j_dst, 1));

			*out= (NodeGroupDef_Node_Output) {
				.src_offset= member_offset(node->type_name, src_slot),
				.size= member_size(node->type_name, src_slot),
				.dst_node_i= dst_node_i,
				.dst_offset= member_offset(dst_type_name, dst_slot),
			};

			// Src should be same size as dst
			ensure(out->size == member_size(dst_type_name, dst_slot));

			++node->output_count;
		}
	}

	blob_write(	buf,
				(U8*)&def + sizeof(Resource),
				sizeof(def) - sizeof(Resource));
	return 0;
}
