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

internal
void trim_whitespace(char *str_begin)
{
	char *str_end= str_begin + strlen(str_begin);

	char *begin= str_begin;
	while (begin < str_end && *begin == ' ')
		++begin;
	char *end= str_end;
	while (end > str_begin && *(end - 1) == ' ')
		--end;

	for (U32 i= 0; i < end - begin; ++i)
		str_begin[i]= begin[i];
	str_begin[end - begin]= '\0';
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

internal void split_func_call(
		char *dst_func_name, char dst_param_strs[][RES_NAME_SIZE],
		U32 *param_count,
		U32 dst_func_name_size,
		U32 dst_array_size, U32 dst_str_size,
		const char *input)
{
	const char *input_end= input + strlen(input);
	const char *paren_tok= input;
	while (paren_tok < input_end && *paren_tok != '(')
		++paren_tok;

	const U32 func_name_size= paren_tok - input + 1; // Remembering null-byte
	if (func_name_size > dst_func_name_size)
		fail("Too long function name: %s", input);
	snprintf(dst_func_name, func_name_size, "%s", input);

	// Params
	const char *param_begin= paren_tok + 1;
	while (param_begin < input_end && *param_begin != ')') {
		// Read next param
		const char *comma_tok= param_begin;
		while (	comma_tok < input_end &&
				*comma_tok != ')' &&
				*comma_tok != ',')
			++comma_tok;

		const U32 param_str_size= comma_tok - param_begin + 1;
		if (param_str_size > dst_str_size)
			fail("Too long parameter name: %s", input);
		snprintf(	dst_param_strs[*param_count],
					param_str_size, "%s",
					param_begin);
		trim_whitespace(dst_param_strs[*param_count]);

		param_begin= comma_tok + 1;
		++*param_count;
	}
}


internal
bool has_char(char ch, const char *input)
{
	U32 len= strlen(input);
	for (U32 i= 0; i < len; ++i) {
		if (input[i] == ch)
			return true;
	}
	return false;
}

int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_nodes= json_value_by_key(j, "nodes");
	JsonTok j_cmds= json_value_by_key(j, "cmds");
	if (json_is_null(j_nodes))
		RES_ATTRIB_MISSING("nodes");
	if (json_is_null(j_cmds))
		RES_ATTRIB_MISSING("cmds");

	// cmds
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

	// cmds
	for (U32 cmd_i= 0; cmd_i < json_member_count(j_cmds); ++cmd_i) {
		JsonTok j_cmd= json_member(j_cmds, cmd_i);
		const char *cmd_str= json_str(j_cmd);
		NodeGroupDef_Node_Cmd *cmd= &def.cmds[def.cmd_count];

		if (has_char('=', cmd_str)) {
			// This is clearly CmdType_memcpy!

			// Split "dst.m = src.m"
			char dst_str[RES_NAME_SIZE*2]= {};
			char src_str[RES_NAME_SIZE*2]= {};
			split_str(
					(char*[]) {dst_str, src_str}, 2, RES_NAME_SIZE*2,
					'=', cmd_str);

			// Split "dst.m"
			char dst_node_name[RES_NAME_SIZE]= {};
			char dst_member_name[RES_NAME_SIZE]= {};
			split_str(
					(char*[]) {dst_node_name, dst_member_name}, 2, RES_NAME_SIZE,
					'.', dst_str);

			// Split "src.m"
			char src_node_name[RES_NAME_SIZE]= {};
			char src_member_name[RES_NAME_SIZE]= {};
			split_str(
					(char*[]) {src_node_name, src_member_name}, 2, RES_NAME_SIZE,
					'.', src_str);

			U32 src_node_i= node_i_by_name(&def, src_node_name);
			U32 dst_node_i= node_i_by_name(&def, dst_node_name);

			const char *src_type_name= def.nodes[src_node_i].type_name;
			const char *dst_type_name= def.nodes[dst_node_i].type_name;

			*cmd= (NodeGroupDef_Node_Cmd) {
				.type= CmdType_memcpy,
				.src_offset= member_offset(src_type_name, src_member_name),
				.dst_offset= member_offset(dst_type_name, dst_member_name),
				.dst_node_i= dst_node_i,
				.size= member_size(src_type_name, src_member_name),
			};

			// Src should be the same size as dst
			ensure(cmd->size == member_size(dst_type_name, dst_member_name));
		} else { // Must be a call
			char func_name[MAX_FUNC_NAME_SIZE]= {};
			char func_param_strs[MAX_CMD_CALL_PARAMS][RES_NAME_SIZE]= {};

			U32 param_count= 0;
			split_func_call(func_name, func_param_strs,
					&param_count,
					MAX_FUNC_NAME_SIZE, MAX_CMD_CALL_PARAMS, RES_NAME_SIZE,
					cmd_str);

			debug_print("FUNC: %s", func_name);
			for (U32 i= 0; i < param_count; ++i) {
				debug_print("PARAM: %s", func_param_strs[i]);
			}

			ensure(param_count > 0);
			*cmd= (NodeGroupDef_Node_Cmd) {
				.type= CmdType_call,
				.fptr= func_ptr(func_name),
				.p_count= param_count,
			};

			/// @todo	Check that there's correct number of params,
			///			and that they're correct type!!!

			// Params should be format "node_name" (for now)
			for (U32 i= 0; i < param_count; ++i) {
				/*
				char p_node_name[RES_NAME_SIZE]= {};
				char p_member_name[RES_NAME_SIZE]= {};
				split_str(
						(char*[]) {p_node_name, p_member_name}, 2, RES_NAME_SIZE,
						'.', func_param_strs[i]);
				U32 p_node_i= node_i_by_name(&def, p_node_name);
				*/

				U32 p_node_i= node_i_by_name(&def, func_param_strs[i]);
				//const char *p_node_type_name= def.nodes[p_node_i].type_name;

				cmd->p_node_i[i]= p_node_i;
				//cmd->p_sizes[i]= struct_size(p_node_type_name);
				//cmd->p_sizes[i]= member_size(p_node_type_name, p_member_name);
				//cmd->p_offsets[i]= member_offset(p_node_type_name, p_member_name);
			}

			if (!cmd->fptr)
				fail("Func ptr not found: %s", func_name);
		}

		++def.cmd_count;
	}

	blob_write(	buf,
				(U8*)&def + sizeof(Resource),
				sizeof(def) - sizeof(Resource));
	return 0;
}
