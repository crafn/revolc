#include "global/rtti.h"
#include "nodegroupdef.h"
#include "resources/resblob.h"

#define MAX_TOKEN_STR_SIZE 64

bool whitespace(char ch)
{ return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\0'; }

typedef enum {
	TokType_eof,
	TokType_name, // single_word_like_this
	TokType_assign, // =
	TokType_comma, // ,
	TokType_dot, // .
	TokType_open_paren, // (
	TokType_close_paren, // )
	TokType_number,
	TokType_kw_if, // if
	TokType_unknown
} TokType;

TokType single_char_token_type(char ch)
{
	switch (ch) {
		case '=': return TokType_assign;
		case ',': return TokType_comma;
		case '(': return TokType_open_paren;
		case ')': return TokType_close_paren;
		case '.': return TokType_dot;
		default: return TokType_unknown;
	}
}

TokType kw_token_type(const char* str, U32 len)
{
	if (!strncmp(str, "if", len))
		return TokType_kw_if;
	return TokType_unknown;
}

typedef struct Token {
	TokType type;
	char str[MAX_TOKEN_STR_SIZE];
} Token;

typedef enum {
	TokState_none,
	TokState_maybe_single_char,
	TokState_number,
	TokState_number_after_dot,
	TokState_name,
	TokState_str,
} TokState; // Tokenization state

internal
void add_tok(Token *dst_tokens, U32 *next_tok, U32 max_tokens, Token t)
{
	if (*next_tok >= max_tokens)
		fail("Too many tokens");
	dst_tokens[(*next_tok)++]= t;
}

internal
void commit(	TokState *state,
				Token *tokens,
				U32 *next_tok,
				U32 max_token_count,
				const char* b, const char* e,
				TokType t,
				const char *end)
{
	if (e > b) {
		if (t == TokType_name) {
			TokType kw= kw_token_type(b, e - b);
			if (kw != TokType_unknown)
				t= kw;
		}
		Token tok= { .type= t };
		U32 size= e - b + 1;
		if (size > sizeof(tok.str))
			fail("Too long token");
		fmt_str(tok.str, size, "%s", b);
		add_tok(tokens, next_tok, max_token_count, tok);
		*state= TokState_none;
	}
}

internal
void tokenize(Token *dst_tokens, U32 *next_tok, U32 max_toks, const char* contents)
{
	U32 contents_size= strlen(contents) + 1;
	*next_tok= 0;

	TokState state= TokState_none;
	const char* cur= contents;
	const char* tok_begin= contents;
	const char* end= contents + contents_size;

	while (cur < end + 1 && tok_begin < end) {
		switch (state) {
			case TokState_none:
				if (single_char_token_type(*cur) != TokType_unknown)
					state= TokState_maybe_single_char;
				else if (*cur >= '0' && *cur <= '9')
					state= TokState_number;
				else if (	(*cur >= 'a' && *cur <= 'z') ||
							(*cur >= 'A' && *cur <= 'Z') ||
							(*cur == '_'))
					state= TokState_name;
				else if (*cur == '\"')
					state= TokState_str;
				tok_begin= cur;
			break;
			case TokState_maybe_single_char: {
				TokType t= TokType_unknown; //double_char_tok_type(*tok_begin, *cur);
				if (t == TokType_unknown) {
					commit(&state, dst_tokens, next_tok, max_toks, tok_begin, cur, single_char_token_type(*tok_begin), end);
					--cur;
				} else {
					commit(&state, dst_tokens, next_tok, max_toks, tok_begin, cur + 1, t, end);
				}
			}
			break;
			case TokState_number_after_dot:
			case TokState_number:
				if (	whitespace(*cur) ||
						single_char_token_type(*cur) != TokType_unknown) {
					if (state == TokState_number_after_dot) {
						// `123.` <- last dot is detected and removed,
						commit(&state, dst_tokens, next_tok, max_toks, tok_begin, cur - 1, TokType_number, end);
						cur -= 2;
						break;
					} else if (*cur != '.') {
						commit(&state, dst_tokens, next_tok, max_toks, tok_begin, cur, TokType_number, end);
						--cur;
						break;
					}
				}

				if (*cur == '.')
					state= TokState_number_after_dot;
				else
					state= TokState_number;
			break;
			case TokState_name:
				if (	whitespace(*cur) ||
						single_char_token_type(*cur) != TokType_unknown) {
					commit(&state, dst_tokens, next_tok, max_toks, tok_begin, cur, TokType_name, end);
					--cur;
				}
			break;
			case TokState_str:
				if (*cur == '\"')
					commit(&state, dst_tokens, next_tok, max_toks, tok_begin + 1, cur, TokType_name, end);
			break;
			default:;
		}
		++cur;
	}
	ensure(*cur == '\0');
	ensure(*next_tok < max_toks);
	dst_tokens[*(next_tok++)]= (Token) { .type= TokType_eof };
}

internal
U32 node_i_by_name(const NodeGroupDef *def, const char *name)
{
	for (U32 i= 0; i < def->node_count; ++i) {
		if (!strcmp(def->nodes[i].name, name))
			return i;
	}

	fail("Node not found: %s", name);
}

// Finds offset and size of the sub-member in "foo.bar.asdfg.hsdg"
internal
void find_member_storage(U32 *offset, U32 *size, const char *type_name, const Token *tok)
{
	*offset= 0;
	*size= 0;

	// Skip over "struct_var_name." as we already have `type_name`
	ensure(tok->type == TokType_name);
	++tok;
	ensure(tok->type == TokType_dot);
	++tok;

	while (tok->type != TokType_eof) {
		ensure(tok->type == TokType_name);

		const char *member_name= tok->str;
		*offset += rtti_member_offset(type_name, member_name);
		const char *next_type_name= rtti_member_type_name(type_name, member_name);

		++tok;

		if (tok->type == TokType_dot) {
			++tok;
		} else {
			// That was the last name in the chain of "foo.bar.asdfg.hsdg"
			*size= rtti_member_size(type_name, member_name);
			break;
		}
		type_name= next_type_name;
	}
}

internal
void parse_cmd(NodeGroupDef_Cmd *cmd, const Token *toks, U32 tok_count, const NodeGroupDef *def)
{
	ensure(tok_count >= 3);
	if (toks[0].type == TokType_kw_if) {
		// if
		ensure(tok_count >= 6);

		// Expecting "if (node.member) <assignment or call>"
		ensure(toks[1].type == TokType_open_paren);
		ensure(toks[2].type == TokType_name);
		ensure(toks[3].type == TokType_dot);
		ensure(toks[4].type == TokType_name);
		ensure(toks[5].type == TokType_close_paren);

		const char *cond_node_name= toks[2].str;
		const char *cond_member_name= toks[4].str;
		U32 cond_node_i= node_i_by_name(def, cond_node_name);
		const char *cond_node_type_name= def->nodes[cond_node_i].type_name;

		cmd->has_condition= true;
		cmd->cond_node_i= cond_node_i;
		cmd->cond_offset= rtti_member_offset(cond_node_type_name, cond_member_name);
		cmd->cond_size= rtti_member_size(cond_node_type_name, cond_member_name);

		// Parse command
		parse_cmd(cmd, toks + 6, tok_count - 6, def);
	} else if (toks[1].type == TokType_dot) {
		// Assignment

		// Dst node token is always first
		ensure(toks[0].type == TokType_name);
		const char *dst_node_name= toks[0].str;

		// Find src node token after =
		U32 src_node_tok_i= 1;
		while (	src_node_tok_i < tok_count &&
				toks[src_node_tok_i].type != TokType_assign)
			++src_node_tok_i;
		++src_node_tok_i;
		ensure(src_node_tok_i < tok_count);
		ensure(toks[src_node_tok_i].type == TokType_name);
		const char *src_node_name= toks[src_node_tok_i].str;

		U32 src_node_i= node_i_by_name(def, src_node_name);
		U32 dst_node_i= node_i_by_name(def, dst_node_name);

		const char *src_type_name= def->nodes[src_node_i].type_name;
		const char *dst_type_name= def->nodes[dst_node_i].type_name;

		cmd->type= CmdType_memcpy;
		cmd->src_node_i= src_node_i;
		cmd->dst_node_i= dst_node_i;
		U32 dst_size;
		U32 src_size;
		find_member_storage(&cmd->dst_offset, &dst_size, dst_type_name, &toks[0]);
		find_member_storage(&cmd->src_offset, &src_size, src_type_name, &toks[src_node_tok_i]);
		ensure(dst_size == src_size);
		cmd->size= src_size;
	} else if (toks[1].type == TokType_open_paren) {
		// Call
		ensure(toks[0].type == TokType_name);

		const char *func_name= toks[0].str;
		cmd->type= CmdType_call;
		fmt_str(cmd->func_name, sizeof(cmd->func_name), "%s", func_name);

		/// @todo	Check that there's correct number of params,
		///			and that they're correct type!!!

		// Params should be format "node_name" (for now)
		for (U32 i= 2; i < tok_count; ++i) {
			if (toks[i].type == TokType_comma)
				continue;
			if (toks[i].type == TokType_close_paren)
				break;

			ensure(toks[i].type == TokType_name);
			U32 p_node_i= node_i_by_name(def, toks[i].str);

			ensure(cmd->p_count < MAX_CMD_CALL_PARAMS);
			cmd->p_node_i[cmd->p_count]= p_node_i;
			++cmd->p_count;
		}
	}
}

void init_nodegroupdef(NodeGroupDef *def)
{
	for (U32 i= 0; i < def->node_count; ++i) {
		NodeGroupDef_Node *node= &def->nodes[i];

		node->default_struct_size= rtti_struct_size(node->type_name);
		ensure(node->default_struct_size < MAX_DEFAULT_STRUCT_SIZE);
	}

	for (U32 node_i= 0; node_i < def->node_count; ++node_i) {
		NodeGroupDef_Node *node= &def->nodes[node_i];

		for (U32 i= 0; i < node->defaults_count; ++i) {
			const char *field_str= node->defaults[i].dst;
			const char *value_str= node->defaults[i].src;

			U32 size= rtti_member_size(node->type_name, field_str);
			U32 offset= rtti_member_offset(node->type_name, field_str);

			const U32 value_size= strlen(value_str) + 1;
			ensure(offset + size < MAX_DEFAULT_STRUCT_SIZE);
			ensure(value_size <= size);
			memcpy(node->default_struct + offset, value_str, value_size);
			memset(node->default_struct_set_bytes + offset, 1, value_size);
		}
	}

	// cmds
	for (U32 cmd_i= 0; cmd_i < def->cmd_count; ++cmd_i) {
		NodeGroupDef_Cmd *cmd= &def->cmds[cmd_i];

		const U32 max_tokens= 64;
		Token toks[max_tokens];
		U32 tok_count;
		tokenize(toks, &tok_count, max_tokens, cmd->str);

		parse_cmd(cmd, toks, tok_count, def);
		if (cmd->type == CmdType_call)
			cmd->fptr= rtti_func_ptr(cmd->func_name);
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

	// nodes
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

		fmt_str(	node->type_name,
					sizeof(def.nodes[node_i].type_name),
					"%s", json_str(j_type_name));

		fmt_str(	node->name,
					sizeof(def.nodes[node_i].name),
					"%s", json_str(j_name));

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

			NodeGroupDef_Node_Defaults *defaults=
				&node->defaults[node->defaults_count++];
			fmt_str(defaults->dst, sizeof(defaults->dst), "%s", field_str);
			fmt_str(defaults->src, sizeof(defaults->src), "%s", value_str);
		}

		++def.node_count;
	}

	// cmds
	for (U32 cmd_i= 0; cmd_i < json_member_count(j_cmds); ++cmd_i) {
		JsonTok j_cmd= json_member(j_cmds, cmd_i);
		const char *cmd_str= json_str(j_cmd);
		NodeGroupDef_Cmd *cmd= &def.cmds[def.cmd_count];
		fmt_str(cmd->str, sizeof(cmd->str), "%s", cmd_str);
		++def.cmd_count;
	}

	blob_write(	buf,
				(U8*)&def + sizeof(Resource),
				sizeof(def) - sizeof(Resource));
	return 0;

error:
	return 1;
}
