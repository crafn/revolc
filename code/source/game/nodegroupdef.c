#include "global/rtti.h"
#include "nodegroupdef.h"
#include "resources/resblob.h"

#ifndef CODEGEN
#	include <qc/parse.h>
#endif

#define MAX_TOKEN_STR_SIZE 64

internal
U32 node_i_by_name(const NodeGroupDef *def, const char *name)
{
	for (U32 i = 0; i < def->node_count; ++i) {
		if (!strcmp(def->nodes[i].name, name))
			return i;
	}

	fail("Node not found: '%s'", name);
}

// Finds type, offset and size of the sub-member in `foo.bar.pos.x`, where `type_name` is type of `foo`
// The structure of AST is `((foo.bar).pos).x`
internal
void find_member_storage(const char **member_type_name, U32 *offset, U32 *size, const char *type_name, QC_AST_Node *node, bool struct_included)
{
	if (member_type_name)
		*member_type_name = NULL;
	*offset = 0;
	*size = 0;

	/* Recurse through Access(Access(Access(Ident foo), Ident bar), Ident pos , Ident x) */
	if (node->type == QC_AST_ident) {
		QC_CASTED_NODE(QC_AST_Ident, ident, node);

		ensure(type_name);
		StructRtti *s = rtti_struct(type_name);
		ensure(s);
		if (struct_included) {
			/* This is the struct variable */
			*offset = 0;
			*size = s->size;
			if (member_type_name)
				*member_type_name = type_name;
		} else {
			/* This is .something */
			const char *member_name = ident->text.data;
			U32 member_ix = rtti_member_index(type_name, member_name);
			*offset = s->members[member_ix].offset;
			*size = s->members[member_ix].size;
			if (member_type_name)
				*member_type_name = s->members[member_ix].base_type_name;
		}
	} else {
		ensure(node->type == QC_AST_access);
		QC_CASTED_NODE(QC_AST_Access, access, node);

		if (access->is_var_access) {
			find_member_storage(member_type_name, offset, size, type_name, access->base, struct_included);
		} else if (access->is_member_access) {
			ensure(access->args.size == 1);
			U32 b_offset, b_size;
			const char *b_type_name;
			find_member_storage(&b_type_name, &b_offset, &b_size, type_name, access->base, struct_included);

			// This access->args[0] is usually just an ident
			U32 m_offset, m_size;
			find_member_storage(member_type_name, &m_offset, &m_size, b_type_name, access->args.data[0], false);

			*offset = b_offset + m_offset;
			*size = m_size;
		} else {
			fail("Unknown access type");
		}
	}
}

const char *leftmost_name_of_access(QC_AST_Access *access)
{
	QC_AST_Node *node = QC_AST_BASE(access);
	while (node->type == QC_AST_access)
		node = ((QC_AST_Access*)node)->base;
	ensure(node->type == QC_AST_ident);
	return ((QC_AST_Ident*)node)->text.data;
}

internal
void parse_cmd(NodeGroupDef_Cmd *cmd, QC_AST_Node *node, const NodeGroupDef *def)
{
	if (node->type == QC_AST_cond) {
		QC_CASTED_NODE(QC_AST_Cond, cond, node);
		// Just ignoring condition for now. It's not that useful feature, should be removed maybe.
/*
		const char *cond_node_name = toks[2].str;
		const char *cond_member_name = toks[4].str;
		U32 cond_node_i = node_i_by_name(def, cond_node_name);
		const char *cond_node_type_name = def->nodes[cond_node_i].type_name;
		U32 cond_member_ix = rtti_member_index(cond_node_type_name, cond_member_name);
		StructRtti *s = rtti_struct(cond_node_type_name);
		ensure(s);

		cmd->has_condition = true;
		cmd->cond_node_i = cond_node_i;
		cmd->cond_offset = s->members[cond_member_ix].offset;
		cmd->cond_size = s->members[cond_member_ix].size;
		*/
 
 		ensure(cond->body->nodes.size == 1);
		parse_cmd(cmd, cond->body->nodes.data[0], def);
	} else if (node->type == QC_AST_biop) {
		QC_CASTED_NODE(QC_AST_Biop, biop, node);
		if (biop->type != QC_Token_assign) {
			// @todo Error message, no crash
			fail("Only assignment supported");
		}

		// @todo Proper error messages
		ensure(biop->lhs->type == QC_AST_access);
		ensure(biop->rhs->type == QC_AST_access);
		QC_CASTED_NODE(QC_AST_Access, lhs, biop->lhs);
		QC_CASTED_NODE(QC_AST_Access, rhs, biop->rhs);

		const char *dst_node_name = leftmost_name_of_access(lhs);
		const char *src_node_name = leftmost_name_of_access(rhs);

		U32 src_node_i = node_i_by_name(def, src_node_name);
		U32 dst_node_i = node_i_by_name(def, dst_node_name);

		const char *src_type_name = def->nodes[src_node_i].type_name;
		const char *dst_type_name = def->nodes[dst_node_i].type_name;

		cmd->type = CmdType_memcpy;
		cmd->src_node_i = src_node_i;
		cmd->dst_node_i = dst_node_i;
		U32 dst_size;
		U32 src_size;
		find_member_storage(NULL, &cmd->dst_offset, &dst_size, dst_type_name, QC_AST_BASE(lhs), true);
		find_member_storage(NULL, &cmd->src_offset, &src_size, src_type_name, QC_AST_BASE(rhs), true);
		ensure(dst_size >= src_size); // Allow memcpying V2f to V3f
		cmd->size = src_size;
	} else if (node->type == QC_AST_call) {
		QC_CASTED_NODE(QC_AST_Call, call, node);
		QC_AST_Ident *call_ident = qc_unwrap_ident(call->base);
		ensure(call_ident); // @todo Error message

		cmd->type = CmdType_call;
		fmt_str(cmd->func_name, sizeof(cmd->func_name), "%s", call_ident->text.data);

		/// @todo	Check that there's correct number of params,
		///			and that they're correct type!!!

		// Params should be format "node_name" (for now)
		for (int i = 0; i < call->args.size; ++i) {
			QC_AST_Ident *ident = qc_unwrap_ident(call->args.data[i]);
			ensure(ident); // @todo Error message

			U32 p_node_i = node_i_by_name(def, ident->text.data);

			ensure(cmd->p_count < MAX_CMD_CALL_PARAMS);
			cmd->p_node_i[cmd->p_count] = p_node_i;
			++cmd->p_count;
		}
	} else {
		fail("Unknown cmd");
	}
}

internal void eval_to_bits(void *dst, U32 dst_size, const char *dst_type_hint, QC_AST_Node *expr)
{
	// Deserialize value of an expression to memory
	// @todo Need some generic way to transform text/numeric data to bits (and maybe back).
	//       Something that can be used here, commands, and in program state editor. 
	//       Probably want to stay in C-like syntax, like "foo.bar.x = 0.5" and not in JSON-like
	//       "foo" : { "bar" : { "x" : 0.5 }}. Better to move to the direction of one single language
	//       for engine, game, and data than to the opposite direction. (JSON is also very clumsy for
	//       presenting function calls. It's not designed for presenting code.)

	QC_AST_Literal *eval = qc_eval_const_expr(expr);

	if (eval->type == QC_Literal_string) {
		fmt_str(dst, dst_size, "%s", eval->value.string.data);
	} else if (eval->type == QC_Literal_integer || eval->type == QC_Literal_floating) {
		F64 value = 0.0;
		if (eval->type == QC_Literal_integer) {
			value = eval->value.integer;
		} else if (eval->type == QC_Literal_floating) {
			value = eval->value.floating;
		} else {
			fail("Unhandled token type");
		}

		if (!strcmp(dst_type_hint, "F32")) {
			F32 v = value;
			ensure(dst_size == sizeof(v));
			memcpy(dst, &v, dst_size);
		} else if (!strcmp(dst_type_hint, "F64")) {
			ensure(dst_size == sizeof(value));
			memcpy(dst, &value, dst_size);
		} else {
			fail("Unhandled value type");
		}
	} else {
		fail("Unhandled literal type");
	}
	qc_destroy_node(QC_AST_BASE(eval));
}

void init_nodegroupdef(NodeGroupDef *def)
{
	for (U32 node_i = 0; node_i < def->node_count; ++node_i) {
		NodeGroupDef_Node *node = &def->nodes[node_i];
		StructRtti *s = rtti_struct(node->type_name);
		ensure(s);
		node->default_struct_size = s->size;
		ensure(node->default_struct_size > 0);
		node->default_struct =
			ZERO_ALLOC(gen_ator(), node->default_struct_size, "default_struct");
		node->default_struct_set_bytes =
			ZERO_ALLOC(gen_ator(), node->default_struct_size, "default_struct_set_bytes");
	}

	for (U32 node_i = 0; node_i < def->node_count; ++node_i) {
		NodeGroupDef_Node *node = &def->nodes[node_i];

		// Default values
		for (U32 i = 0; i < node->defaults_count; ++i) {
			const char *str = node->defaults[i].str;

			//debug_print("PARSE DEFAULT %s", str);
			QC_AST_Node *expr;
			QC_AST_Scope *root = qc_parse_string(&expr, str); 
			//qc_print_ast(expr, 4);
			ensure(expr->type == QC_AST_biop);
			QC_CASTED_NODE(QC_AST_Biop, biop, expr);

			U32 dst_offset;
			U32 dst_size;
			const char *type_name;
			find_member_storage(&type_name, &dst_offset, &dst_size, node->type_name, biop->lhs, false);
			void *dst_ptr = &node->default_struct[dst_offset];
			memset(node->default_struct_set_bytes + dst_offset, 1, dst_size);

			eval_to_bits(dst_ptr, dst_size, type_name, biop->rhs);

			qc_destroy_ast(root);
		}
	}

	// cmds
	for (U32 cmd_i = 0; cmd_i < def->cmd_count; ++cmd_i) {
		NodeGroupDef_Cmd *cmd = &def->cmds[cmd_i];

		QC_AST_Node *expr;
		//debug_print("PARSE CMD %s", cmd->str);
		QC_AST_Scope *root = qc_parse_string(&expr, cmd->str);
		parse_cmd(cmd, expr, def);
		qc_destroy_ast(root);

		if (cmd->type == CmdType_call)
			cmd->fptr = rtti_func_ptr(cmd->func_name);
	}
}

void deinit_nodegroupdef(NodeGroupDef *def)
{
	for (U32 i = 0; i < def->node_count; ++i) {
		FREE(gen_ator(), def->nodes[i].default_struct);
		FREE(gen_ator(), def->nodes[i].default_struct_set_bytes);
	}
}

int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_nodes = json_value_by_key(j, "nodes");
	JsonTok j_cmds = json_value_by_key(j, "cmds");
	if (json_is_null(j_nodes))
		RES_ATTRIB_MISSING("nodes");
	if (json_is_null(j_cmds))
		RES_ATTRIB_MISSING("cmds");

	// nodes
	NodeGroupDef def = {};
	for (U32 node_i = 0; node_i < json_member_count(j_nodes); ++node_i) {
		JsonTok j_node = json_member(j_nodes, node_i);
		NodeGroupDef_Node *node = &def.nodes[node_i];

		JsonTok j_type_name = json_value_by_key(j_node, "type");
		JsonTok j_name = json_value_by_key(j_node, "name");
		if (json_is_null(j_type_name))
			RES_ATTRIB_MISSING("type");
		if (json_is_null(j_name))
			RES_ATTRIB_MISSING("name");

		fmt_str(	node->type_name,
					sizeof(def.nodes[node_i].type_name),
					"%s", json_str(j_type_name));

		fmt_str(	node->name,
					sizeof(def.nodes[node_i].name),
					"%s", json_str(j_name));

		JsonTok j_defaults = json_value_by_key(j_node, "defaults");
		for (U32 i = 0; i < json_member_count(j_defaults); ++i) {
			JsonTok j_default = json_member(j_defaults, i);
			ensure(json_is_string(j_default));

			const char *str = json_str(j_default);
			NodeGroupDef_Node_Defaults *defaults =
				&node->defaults[node->defaults_count++];
			fmt_str(defaults->str, sizeof(defaults->str), "%s", str);
		}

		++def.node_count;
	}

	// cmds
	for (U32 cmd_i = 0; cmd_i < json_member_count(j_cmds); ++cmd_i) {
		JsonTok j_cmd = json_member(j_cmds, cmd_i);
		const char *cmd_str = json_str(j_cmd);
		NodeGroupDef_Cmd *cmd = &def.cmds[def.cmd_count];
		fmt_str(cmd->str, sizeof(cmd->str), "%s", cmd_str);
		++def.cmd_count;
	}

	blob_write(buf, &def, sizeof(def));

/*
	for (U32 i = 0; i < def.node_count; ++i) {
		U32 default_struct_offset =
			res_offset + offsetof(NodeGroupDef, nodes[i])
			+ offsetof(NodeGroupDef_Node, default_struct);
		blob_patch_rel_ptr(buf, default_struct_offset);
		blob_write(buf, NULL, def.nodes[i].default_struct_size);

		U32 default_struct_set_bytes_offset =
			res_offset + offsetof(NodeGroupDef, nodes[i])
			+ offsetof(NodeGroupDef_Node, default_struct_set_bytes);
		blob_patch_rel_ptr(buf, default_struct_set_bytes_offset);
		blob_write(buf, NULL, def.nodes[i].default_struct_size);
	}
*/

	return 0;

error:
	return 1;
}

NodeGroupDef *blobify_nodegroupdef(struct WArchive *ar, Cson c, bool *err)
{
	Cson c_nodes = cson_key(c, "nodes");
	Cson c_cmds = cson_key(c, "cmds");
	if (cson_is_null(c_nodes))
		RES_ATTRIB_MISSING("nodes");
	if (cson_is_null(c_cmds))
		RES_ATTRIB_MISSING("cmds");

	// nodes
	NodeGroupDef def = {};
	for (U32 node_i = 0; node_i < cson_member_count(c_nodes); ++node_i) {
		Cson c_node = cson_member(c_nodes, node_i);
		NodeGroupDef_Node *node = &def.nodes[node_i];

		Cson c_type_name = cson_key(c_node, "type");
		Cson c_name = cson_key(c_node, "name");
		if (cson_is_null(c_type_name))
			RES_ATTRIB_MISSING("type");
		if (cson_is_null(c_name))
			RES_ATTRIB_MISSING("name");

		fmt_str(	node->type_name,
					sizeof(def.nodes[node_i].type_name),
					"%s", blobify_string(c_type_name, err));

		fmt_str(	node->name,
					sizeof(def.nodes[node_i].name),
					"%s", blobify_string(c_name, err));

		ensure(!*err);
		Cson c_defaults = cson_key(c_node, "defaults");
		for (U32 i = 0; i < cson_member_count(c_defaults); ++i) {
			Cson c_default = cson_member(c_defaults, i);

			const char *str = blobify_string(c_default, err);
			debug_print("foo %i %s", i, str);
			NodeGroupDef_Node_Defaults *defaults =
				&node->defaults[node->defaults_count++];
			fmt_str(defaults->str, sizeof(defaults->str), "%s", str);
		}

		ensure(!*err);
		++def.node_count;
	}

	// cmds
	for (U32 cmd_i = 0; cmd_i < cson_member_count(c_cmds); ++cmd_i) {
		Cson c_cmd = cson_member(c_cmds, cmd_i);
		const char *cmd_str = blobify_string(c_cmd, err);
		NodeGroupDef_Cmd *cmd = &def.cmds[def.cmd_count];
		fmt_str(cmd->str, sizeof(cmd->str), "%s", cmd_str);
		++def.cmd_count;
	}

	if (err && *err)
		goto error;

	NodeGroupDef *ptr = warchive_ptr(ar);
	pack_buf(ar, &def, sizeof(def));
	return ptr;

error:
	SET_ERROR_FLAG(err);
	return NULL;
}

void deblobify_nodegroupdef(WCson *c, struct RArchive *ar)
{
	NodeGroupDef *def = rarchive_ptr(ar, sizeof(*def));
	unpack_advance(ar, sizeof(*def));

	wcson_begin_compound(c, "NodeGroupDef");

	wcson_designated(c, "name");
	deblobify_string(c, def->res.name);

	wcson_designated(c, "nodes");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < def->node_count; ++i) {
		NodeGroupDef_Node node = def->nodes[i];

		wcson_begin_initializer(c);

		wcson_designated(c, "type");
		deblobify_string(c, node.type_name);

		wcson_designated(c, "name");
		deblobify_string(c, node.name);

		// @todo string -> code
		wcson_designated(c, "defaults");
		wcson_begin_initializer(c);
		for (U32 k = 0; k < node.defaults_count; ++k)
			deblobify_string(c, node.defaults[k].str);
		wcson_end_initializer(c);

		wcson_end_initializer(c);
	}
	wcson_end_initializer(c);

	wcson_designated(c, "cmds");
	wcson_begin_initializer(c);
	// @todo string -> code
	for (U32 i = 0; i < def->cmd_count; ++i)
		deblobify_string(c, def->cmds[i].str);
	wcson_end_initializer(c);

	wcson_end_compound(c);
}

