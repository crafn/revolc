#include "global/rtti.h"
#include "nodegroupdef.h"
#include "resources/resblob.h"

int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_nodes= json_value_by_key(j, "nodes");
	if (json_is_null(j_nodes))
		RES_ATTRIB_MISSING("nodes");

	NodeGroupDef def= {};
	for (U32 node_i= 0; node_i < json_member_count(j_nodes);
			++node_i, ++def.node_count) {
		JsonTok j_node= json_member(j_nodes, node_i);

		JsonTok j_type_name= json_value_by_key(j_node, "type");
		JsonTok j_name= json_value_by_key(j_node, "name");
		if (json_is_null(j_type_name))
			RES_ATTRIB_MISSING("type");
		if (json_is_null(j_name))
			RES_ATTRIB_MISSING("name");

		snprintf(	def.nodes[node_i].type_name,
					sizeof(def.nodes[node_i].type_name),
					"%s", json_str(j_type_name));

		snprintf(	def.nodes[node_i].name,
					sizeof(def.nodes[node_i].name),
					"%s", json_str(j_name));

		/// @todo Default struct value
		const char *node_type_name= json_str(j_type_name);
		def.nodes[node_i].default_struct_size= struct_size(node_type_name);

		// Outputs
		JsonTok j_outputs= json_value_by_key(j_node, "outputs");
		if (json_is_null(j_outputs))
			continue;

		for (U32 out_i= 0; out_i < json_member_count(j_outputs);
				++out_i, ++def.nodes[node_i].output_count) {
			JsonTok j_out= json_member(j_outputs, out_i);
			ensure(json_is_object(j_out));

			JsonTok j_src= json_member(j_out, 0);

			const char *src_slot= json_str(j_src);

			JsonTok j_dst= json_member(j_src, 0);
			ensure(json_is_array(j_dst));
			ensure(json_member_count(j_dst) == 2); /// @todo Proper errors

			const char *dst_node_name= json_str(json_member(j_dst, 0));
			const char *dst_slot= json_str(json_member(j_dst, 1));

			def.nodes[node_i].outputs[out_i]
				.src_offset= member_offset(node_type_name, src_slot);
			def.nodes[node_i].outputs[out_i]
				.src_size= member_size(node_type_name, src_slot);
/*
			def.nodes[node_i].outputs[out_i]
				.src_offset= member_offset(dst_node_name, dst_slot);
			def.nodes[node_i].outputs[out_i]
				.src_size= member_size(dst_node_name, dst_slot);
				*/
		}
	}

	blob_write(	buf,
				(U8*)&def + sizeof(Resource),
				sizeof(def) - sizeof(Resource));
	return 0;
}
