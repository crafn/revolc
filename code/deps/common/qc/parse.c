#include "parse.h"

int str_to_int(QC_Buf_Str text)
{
	const char *c = text.buf;
	const char *end = c + text.len;
	int value = 0;
	int sign = 1;
	if (*c == '+' || *c == '-') {
		if (*c == '-')
			sign = -1;
		c++;
	}
	while (c < end) {
		value *= 10;
		QC_ASSERT(*c >= '0' && *c <= '9');
		value += (int)(*c - '0');
		c++;
	}
	return value*sign;
}

double str_to_float(QC_Buf_Str text)
{
	const char *c = text.buf;
	const char *end = c + text.len;
	double value = 0.0;
	int sign = 1;
	int decimal_div = 10;

	if (*c == '+' || *c == '-') {
		if (*c == '-')
			sign = -1;
		c++;
	}

	while (c < end && *c != '.') {
		value *= 10;
		QC_ASSERT(*c >= '0' && *c <= '9');
		value += (double)(*c - '0');
		++c;
	}

	++c;

	while (c < end) {
		value += (double)(*c - '0')/decimal_div;
		decimal_div *= 10;
		++c;
	}

	return value*sign;
}

#define UOP_PRECEDENCE 100000
/* Not all of the accepted tokens are actually binary operators. They're here for similar precedence handling. */
int qc_biop_prec(QC_Token_Type type)
{
	/* Order should match with C operator precedence */
	int prec = 1;
	switch (type) {

	case QC_Token_open_paren: ++prec; /* Function call */

	case QC_Token_dot:
	case QC_Token_right_arrow: ++prec;

	case QC_Token_mul: 
	case QC_Token_div: ++prec;
	case QC_Token_mod: ++prec;
	case QC_Token_add: 
	case QC_Token_sub: ++prec;

	case QC_Token_leq:
	case QC_Token_geq:
	case QC_Token_less:
	case QC_Token_greater: ++prec;

	case QC_Token_equals: ++prec;

	case QC_Token_and: ++prec;
	case QC_Token_or: ++prec;

	case QC_Token_assign: ++prec;
	case QC_Token_add_assign:
	case QC_Token_sub_assign:
	case QC_Token_mul_assign:
	case QC_Token_div_assign: ++prec;

	break;
	default: return -1;
	}
	return prec;
}

/* -1 left, 1 right */
int qc_biop_assoc(QC_Token_Type type)
{
	switch (type) {
		case QC_Token_assign: return 1; /* a = b = c  <=>  (a = (b = c)) */
		default: return -1;
	}
}

QC_Bool is_binary_op(QC_Token_Type type)
{
	switch (type) {

	case QC_Token_mul: 
	case QC_Token_div:
	case QC_Token_mod:
	case QC_Token_add: 
	case QC_Token_sub:

	case QC_Token_leq:
	case QC_Token_geq:
	case QC_Token_less:
	case QC_Token_greater:

	case QC_Token_equals:

	case QC_Token_and:
	case QC_Token_or:

	case QC_Token_assign:
	case QC_Token_add_assign:
	case QC_Token_sub_assign:
	case QC_Token_mul_assign:
	case QC_Token_div_assign:
		return QC_true;
	default: return QC_false;
	}
}

QC_Bool is_unary_op(QC_Token_Type type)
{
	switch (type) {
		case QC_Token_add:
		case QC_Token_sub:
		case QC_Token_kw_sizeof:
		case QC_Token_incr:
		case QC_Token_decr:
		case QC_Token_addrof:
		case QC_Token_mul: /* deref */
			return QC_true;
		default: return QC_false;
	}
}


/* Mirrors call stack in parsing */
/* Used in searching, setting parent nodes, and backtracking */
typedef struct Parse_Stack_Frame {
	QC_Token *begin_tok;
	QC_AST_Node *node;
} Parse_Stack_Frame;

QC_DECLARE_ARRAY(Parse_Stack_Frame)
QC_DEFINE_ARRAY(Parse_Stack_Frame)

typedef QC_AST_Type_Decl* QC_AST_Type_Decl_Ptr;
QC_DECLARE_ARRAY(QC_AST_Type_Decl_Ptr)
QC_DEFINE_ARRAY(QC_AST_Type_Decl_Ptr)

#define QC_PRIORITY_UNDECLARED 10
#define QC_PRIORITY_DEFAULT 0

typedef struct Parse_Ctx {
	QC_AST_Scope *root;
	QC_Token *first_tok; /* @todo Consider having some QC_Token_sof, corresponding to QC_Token_eof*/
	QC_Token *tok; /* Access with cur_tok */
	QC_Bool dont_ref_tokens; /* AST will not point to any tokens */
	QC_Bool allow_undeclared; /* Don't error out with undeclared identifiers */
	int expr_depth;

	/* Builtin type and funcs decls are generated while parsing */

	int error_priority;
	QC_Array(char) error_msg;
	QC_Token *error_tok;

	QC_Array(Parse_Stack_Frame) parse_stack;

	QC_AST_Parent_Map parent_map; /* Built incrementally during parsing */
} Parse_Ctx;


/* QC_Token manipulation */

QC_INTERNAL QC_Token *cur_tok(Parse_Ctx *ctx)
{ return ctx->tok; }

QC_INTERNAL void advance_tok(Parse_Ctx *ctx)
{
	QC_ASSERT(ctx->tok->type != QC_Token_eof);
	do {
		++ctx->tok;
	} while (qc_is_comment_tok(ctx->tok->type));
}

QC_INTERNAL QC_Bool accept_tok(Parse_Ctx *ctx, QC_Token_Type type)
{
	if (ctx->tok->type == type) {
		advance_tok(ctx);
		return QC_true;
	}
	return QC_false;
}

/* Backtracking / stack traversing */

QC_INTERNAL void begin_node_parsing(Parse_Ctx *ctx, QC_AST_Node *node)
{
	Parse_Stack_Frame frame = {0};
	QC_ASSERT(node);

#define QC_PARSE_DEBUG 0
#if QC_PARSE_DEBUG
	printf("begin_node_parsing %s\n", qc_node_type_str(node->type));
#endif

	if (ctx->parse_stack.size > 0) {
		QC_AST_Node *parent = ctx->parse_stack.data[ctx->parse_stack.size - 1].node;
		QC_ASSERT(parent && parent != node);
		qc_set_parent_node(&ctx->parent_map, node, parent);
	}

	frame.begin_tok = cur_tok(ctx);
	frame.node = node;
	qc_push_array(Parse_Stack_Frame)(&ctx->parse_stack, frame);
}

QC_INTERNAL QC_Bool can_have_post_comments(QC_AST_Node *node)
{
	/* if or loop without explicit scope can't have post-comments -- only the statement can */
	if (node->type == QC_AST_cond) {
		QC_CASTED_NODE(QC_AST_Cond, cond, node);
		return !cond->implicit_scope;
	} else if (node->type == QC_AST_loop) {
		QC_CASTED_NODE(QC_AST_Loop, loop, node);
		return !loop->implicit_scope;
	}
	return QC_true;
}

QC_INTERNAL void end_node_parsing(Parse_Ctx *ctx)
{
	Parse_Stack_Frame frame = qc_pop_array(Parse_Stack_Frame)(&ctx->parse_stack);
	QC_ASSERT(frame.node);

	/* Something was succesfully parsed, so recovery from previous error has happened */
	ctx->error_priority = 0;
	ctx->error_tok = NULL;

#if QC_PARSE_DEBUG
	printf("end_node_parsing (%s)\n", qc_node_type_str(frame.node->type));
#endif

	/* frame.node is used in end_node_parsing because node might not yet be created at the begin_node_parsing */
	/* That ^ is QC_false now! Can be moved to begin. */
	if (!ctx->dont_ref_tokens)
		frame.node->begin_tok = frame.begin_tok;

	/*	Gather comments around node if this is the first time calling end_node_parsing with this node.
		It's possible to have multiple begin...end_node_parsing with the same node because of nesting,
		like when parsing statement: 'foo;', which yields parse_expr(parse_ident()). */
	/* That ^ is false now! Not possible to have multiple begin..end with the same node anymore. */
	if (!ctx->dont_ref_tokens) {
		if (frame.node->pre_comments.size == 0) {
			QC_Token *tok = frame.begin_tok - 1;
			/* Rewind first */
			while (tok >= ctx->first_tok && qc_is_comment_tok(tok->type))
				--tok;
			++tok;
			while (qc_is_comment_tok(tok->type) && tok->comment_bound_to == 1) {
				qc_push_array(QC_Token_Ptr)(&frame.node->pre_comments, tok);
				++tok;
			}
		}
		if (can_have_post_comments(frame.node) && frame.node->post_comments.size == 0) {
			QC_Token *tok = cur_tok(ctx);
			/* Cursor has been advanced past following comments, rewind */
			--tok;
			while (tok >= ctx->first_tok && qc_is_comment_tok(tok->type))
				--tok;
			++tok;
			while (qc_is_comment_tok(tok->type) && tok->comment_bound_to == -1) {
				qc_push_array(QC_Token_Ptr)(&frame.node->post_comments, tok);
				++tok;
			}
		}
	}
}

QC_INTERNAL void cancel_node_parsing(Parse_Ctx *ctx)
{
	Parse_Stack_Frame frame = qc_pop_array(Parse_Stack_Frame)(&ctx->parse_stack);
	QC_ASSERT(frame.node);

#if QC_PARSE_DEBUG
	printf("cancel_node_parsing (%s)\n", qc_node_type_str(frame.node->type));
#endif

	/* Backtrack */
	ctx->tok = frame.begin_tok;
	qc_set_parent_node(&ctx->parent_map, frame.node, NULL);

	qc_destroy_node(frame.node);
}

QC_INTERNAL void report_error(Parse_Ctx *ctx, int priority, const char *fmt, ...)
{
	QC_Array(char) msg;
	va_list args;

	if (cur_tok(ctx) <= ctx->error_tok)
		return; /* Don't overwrite error generated from less succesfull parsing (heuristic) */

	va_start(args, fmt);
	msg = qc_create_array(char)(0);
	qc_safe_vsprintf(&msg, fmt, args);
	va_end(args);

	if (priority >= ctx->error_priority) {
#if QC_PARSE_DEBUG
		printf("ERROR %s\n", msg.data);
#endif
		qc_destroy_array(char)(&ctx->error_msg);
		ctx->error_priority = priority;
		ctx->error_msg = msg;
		ctx->error_tok = cur_tok(ctx);
	}
}

QC_INTERNAL void report_error_expected(Parse_Ctx *ctx, const char *expected, QC_Token *got)
{
	report_error(ctx, QC_PRIORITY_DEFAULT, "Expected %s, got '%.*s'", expected, QC_BUF_STR_ARGS(got->text));
}

/* Parsing */

/* @todo QC_AST_Node -> specific node type. <-- not so sure about that.. */
QC_INTERNAL QC_Bool parse_ident(Parse_Ctx *ctx, QC_AST_Ident **ret, QC_AST_Node *decl, QC_Bool allow_designated);
QC_INTERNAL QC_Bool resolve_parsed_ident(Parse_Ctx *ctx, QC_AST_Ident *ident);
QC_INTERNAL QC_Bool parse_type_decl(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_typedef(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_var_decl(Parse_Ctx *ctx, QC_AST_Node **ret, QC_Bool is_param_decl);
QC_INTERNAL QC_Bool parse_type(Parse_Ctx *ctx, QC_AST_Type **ret_type);
QC_INTERNAL QC_Bool parse_type_and_ident(Parse_Ctx *ctx, QC_AST_Type **ret_type, QC_AST_Ident **ret_ident, QC_AST_Node *enclosing_decl);
QC_INTERNAL QC_Bool parse_func_decl(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_scope(Parse_Ctx *ctx, QC_AST_Scope **ret, QC_Bool already_created);
QC_INTERNAL QC_Bool parse_literal(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_uop(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_arg_list(Parse_Ctx *ctx, QC_Array(QC_AST_Node_Ptr) *ret, QC_Token_Type ending);
QC_INTERNAL QC_Bool parse_expr_inside_parens(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_expr(Parse_Ctx *ctx, QC_AST_Node **ret, int min_prec, QC_AST_Type *type_hint, QC_Bool semi);
QC_INTERNAL QC_Bool parse_control(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_cond(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_loop(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_parallel(Parse_Ctx *ctx, QC_AST_Node **ret);
QC_INTERNAL QC_Bool parse_statement(Parse_Ctx *ctx, QC_AST_Node **ret);

/* If decl is NULL, then declaration is searched. */
/* Parse example: foo */
QC_INTERNAL QC_Bool parse_ident(Parse_Ctx *ctx, QC_AST_Ident **ret, QC_AST_Node *decl, QC_Bool allow_designated)
{
	QC_AST_Ident *ident = qc_create_ident_node();

	begin_node_parsing(ctx, QC_AST_BASE(ident));

	if (allow_designated) {
		/* Consume dot before designated initializer identifier */
		if (accept_tok(ctx, QC_Token_dot)) {
			ident->is_designated = QC_true;
		}
	}

	qc_append_str(&ident->text, "%.*s", QC_BUF_STR_ARGS(cur_tok(ctx)->text));

	if (cur_tok(ctx)->type != QC_Token_name) {
		report_error(ctx, QC_PRIORITY_DEFAULT, "'%.*s' is not an identifier", QC_BUF_STR_ARGS(cur_tok(ctx)->text));
		goto mismatch;
	}

	/* Might be NULL -- should be resolved shortly after */
	ident->decl = decl;

	advance_tok(ctx);

	end_node_parsing(ctx);

	*ret = ident;
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_Bool resolve_parsed_ident(Parse_Ctx *ctx, QC_AST_Ident *ident)
{
	if (qc_resolve_ident(&ctx->parent_map, ident))
		return QC_true;

	if (ctx->allow_undeclared)
		return QC_true;

	report_error(ctx, QC_PRIORITY_UNDECLARED, "Undeclared identifier '%s'", ident->text.data);
	return QC_false;
}

/* Parse examples: int test -- int func() { } -- struct foo { } */
QC_INTERNAL QC_Bool parse_type_decl(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Type_Decl *decl = qc_create_type_decl_node();
	begin_node_parsing(ctx, QC_AST_BASE(decl));

	if (!accept_tok(ctx, QC_Token_kw_struct))
		goto mismatch;

	if (!parse_ident(ctx, &decl->ident, QC_AST_BASE(decl), QC_false)) {
		report_error_expected(ctx, "type name", cur_tok(ctx));
		goto mismatch;
	}

	if (parse_scope(ctx, &decl->body, QC_false))
		;
	else {
		report_error_expected(ctx, "'{'", cur_tok(ctx));
		goto mismatch;
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(decl);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_typedef(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Typedef *def = qc_create_typedef_node();
	begin_node_parsing(ctx, QC_AST_BASE(def));

	if (!accept_tok(ctx, QC_Token_kw_typedef)) {
		report_error_expected(ctx, "typedef", cur_tok(ctx));
		goto mismatch;
	}

	if (!parse_type_and_ident(ctx, &def->type, &def->ident, QC_AST_BASE(def)))
		goto mismatch;

	if (!accept_tok(ctx, QC_Token_semi)) {
		report_error_expected(ctx, ";", cur_tok(ctx));
		goto mismatch;
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(def);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_AST_Type_Decl *qc_create_builtin_decl(Parse_Ctx *ctx, QC_Builtin_Type bt)
{
	int i;
	/* Search for builtin declaration from existing decls */
	for (i = 0; i < ctx->parent_map.builtin_decls.size; ++i) {
		QC_AST_Node *node = ctx->parent_map.builtin_decls.data[i];
		if (node->type == QC_AST_type_decl) {
			QC_CASTED_NODE(QC_AST_Type_Decl, decl, node);
			QC_Builtin_Type t = decl->builtin_type;
			if (builtin_type_equals(t, bt))
				return decl;
		}
	}

	{ /* Create new builtin decl if not found */
		QC_AST_Type_Decl *tdecl = qc_create_type_decl_node();
		/* Note that the declaration doesn't have ident -- it's up to backend to generate it */
		tdecl->is_builtin = QC_true;
		tdecl->builtin_type = bt;
		qc_push_array(QC_AST_Node_Ptr)(&ctx->parent_map.builtin_decls, QC_AST_BASE(tdecl));

		if (bt.is_field) {
			int type;
			/* Create field alloc funcs */
			for (type = 0; type < 2; ++type) { 
				const char *names[2] = { "alloc_field", "alloc_device_field" };
				QC_AST_Func_Decl *fdecl = qc_create_func_decl_node();
				fdecl->is_builtin = QC_true;

				fdecl->return_type = qc_create_type_node();
				fdecl->return_type->base_type_decl = tdecl;

				fdecl->ident = qc_create_ident_with_text(QC_AST_BASE(fdecl), names[type]);

				for (i = 0; i < bt.field_dim; ++i) {
					QC_AST_Var_Decl *param = qc_create_var_decl_node();
					param->type = qc_create_type_node();
					param->type->base_type_decl = qc_create_builtin_decl(ctx, qc_int_builtin_type());
					param->ident = qc_create_ident_with_text(QC_AST_BASE(param), "size_%i", i);
					qc_push_array(QC_AST_Var_Decl_Ptr)(&fdecl->params, param);
				}

				qc_push_array(QC_AST_Node_Ptr)(&ctx->parent_map.builtin_decls, QC_AST_BASE(fdecl));
			}

			/* Create field free funcs */
			for (type = 0; type < 2; ++type) { 
				const char *names[2] = { "free_field", "free_device_field" };
				QC_AST_Func_Decl *fdecl = qc_create_func_decl_node();
				fdecl->is_builtin = QC_true;

				fdecl->return_type = qc_create_type_node();
				fdecl->return_type->base_type_decl = qc_create_builtin_decl(ctx, qc_void_builtin_type());

				fdecl->ident = qc_create_ident_with_text(QC_AST_BASE(fdecl), names[type]);

				{
					QC_AST_Var_Decl *param = qc_create_var_decl_node();
					param->type = qc_create_type_node();
					param->type->base_type_decl = tdecl;
					param->ident = qc_create_ident_with_text(QC_AST_BASE(param), "field");
					qc_push_array(QC_AST_Var_Decl_Ptr)(&fdecl->params, param);
				}

				qc_push_array(QC_AST_Node_Ptr)(&ctx->parent_map.builtin_decls, QC_AST_BASE(fdecl));
			}

			{ /* Create field memcpy func */
				QC_AST_Func_Decl *fdecl = qc_create_func_decl_node();
				fdecl->is_builtin = QC_true;

				fdecl->return_type = qc_create_type_node();
				fdecl->return_type->base_type_decl = qc_create_builtin_decl(ctx, qc_void_builtin_type());

				fdecl->ident = qc_create_ident_with_text(QC_AST_BASE(fdecl), "memcpy_field");

				{
					QC_AST_Var_Decl *dst = qc_create_simple_var_decl(tdecl, "dst");
					QC_AST_Var_Decl *src = qc_create_simple_var_decl(tdecl, "src");
					qc_push_array(QC_AST_Var_Decl_Ptr)(&fdecl->params, dst);
					qc_push_array(QC_AST_Var_Decl_Ptr)(&fdecl->params, src);
				}

				qc_push_array(QC_AST_Node_Ptr)(&ctx->parent_map.builtin_decls, QC_AST_BASE(fdecl));
			}

			{ /* Create field size query func */
				QC_AST_Func_Decl *fdecl = qc_create_func_decl_node();
				fdecl->is_builtin = QC_true;

				fdecl->return_type = qc_create_simple_type(qc_create_builtin_decl(ctx, qc_int_builtin_type()));
				fdecl->ident = qc_create_ident_with_text(QC_AST_BASE(fdecl), "size");

				{
					QC_AST_Var_Decl *param1 = qc_create_simple_var_decl(tdecl, "field");
					QC_AST_Var_Decl *param2 =
						qc_create_simple_var_decl(
								qc_create_builtin_decl(ctx, qc_int_builtin_type()),
								"index");
					qc_push_array(QC_AST_Var_Decl_Ptr)(&fdecl->params, param1);
					qc_push_array(QC_AST_Var_Decl_Ptr)(&fdecl->params, param2);
				}

				qc_push_array(QC_AST_Node_Ptr)(&ctx->parent_map.builtin_decls, QC_AST_BASE(fdecl));
			}
		}

		return tdecl;
	}
}

QC_INTERNAL QC_Bool parse_type(Parse_Ctx *ctx, QC_AST_Type **ret_type)
{
	return parse_type_and_ident(ctx, ret_type, NULL, NULL);
}

/* Type and ident in same function because of cases like 'int (*foo)()' */
QC_INTERNAL QC_Bool parse_type_and_ident(Parse_Ctx *ctx, QC_AST_Type **ret_type, QC_AST_Ident **ret_ident, QC_AST_Node *enclosing_decl)
{
	QC_AST_Type *type = qc_create_type_node();
	begin_node_parsing(ctx, QC_AST_BASE(type));

	/* @todo ptr-to-funcs, types with multiple identifiers... */

	{ /* Type */
		QC_AST_Node *found_decl = NULL;
		QC_Bool is_builtin = QC_false;
		QC_Builtin_Type bt = {0};
		QC_Bool recognized = QC_true;

		if (accept_tok(ctx, QC_Token_kw_const))
			type->is_const = QC_true;

		/* Gather all builtin type specifiers (like int, matrix(), field()) */
		while (recognized) {
			QC_Token *tok = cur_tok(ctx);

			switch (tok->type) {
			case QC_Token_kw_void:
				bt.is_void = QC_true;
				advance_tok(ctx);
			break;
			case QC_Token_kw_int:
				bt.is_integer = QC_true;
				bt.bitness = 0; /* Not specified */
				advance_tok(ctx);
			break;
			case QC_Token_kw_bool:
				bt.is_boolean = QC_true;
				advance_tok(ctx);
			break;
			case QC_Token_kw_size_t:
				bt.is_integer = QC_true;
				bt.bitness = sizeof(size_t)*8; /* @todo Assuming target is same architecture than host */
				bt.is_unsigned = QC_true;
				advance_tok(ctx);
			break;
			case QC_Token_kw_char:
				bt.is_char = QC_true;
				bt.bitness = 8;
				advance_tok(ctx);
			break;
			case QC_Token_kw_float:
				bt.is_float = QC_true;
				bt.bitness = 32;
				advance_tok(ctx);
			break;
			case QC_Token_kw_matrix: {
				bt.is_matrix = QC_true;
				advance_tok(ctx);

				if (!accept_tok(ctx, QC_Token_open_paren)) {
					report_error_expected(ctx, "'('", cur_tok(ctx));
					goto mismatch;
				}

				{ /* Parse dimension list */
					/* @todo Support constant expressions */
					while (cur_tok(ctx)->type == QC_Token_int) {
						int dim = str_to_int(cur_tok(ctx)->text);
						bt.matrix_dim[bt.matrix_rank++] = dim;
						advance_tok(ctx);

						if (bt.matrix_rank > QC_MAX_MATRIX_RANK) {
							report_error(ctx, QC_PRIORITY_DEFAULT, "Too high rank for a matrix. Max is %i", QC_MAX_MATRIX_RANK);
							goto mismatch;
						}

						if (!accept_tok(ctx, QC_Token_comma))
							break;
					}
				}

				if (!accept_tok(ctx, QC_Token_close_paren)) {
					report_error_expected(ctx, "')'", cur_tok(ctx));
					goto mismatch;
				}
			} break;
			case QC_Token_kw_field: {
				QC_AST_Node *dim_expr = NULL;
				QC_AST_Literal *dim_value;
				bt.is_field = QC_true;
				advance_tok(ctx);

				if (!parse_expr_inside_parens(ctx, &dim_expr))
					goto mismatch;
				dim_value = qc_eval_const_expr(dim_expr);
				if (!dim_value) {
					report_error(ctx, QC_PRIORITY_DEFAULT, "Field dimension must be a constant expression");
					goto mismatch;
				}
				if (dim_value->type != QC_Literal_integer) {
					report_error(ctx, QC_PRIORITY_DEFAULT, "Field dimension must be an integer");
					goto mismatch;
				}
				bt.field_dim = dim_value->value.integer;
				qc_destroy_node(QC_AST_BASE(dim_value));
				/* Constant expression is removed */
				qc_destroy_node(dim_expr);
			} break;
			default: recognized = QC_false;
			}
			if (recognized)
				is_builtin = QC_true;
		}

		/* Create builtin decls for types included in the type */
		if (is_builtin) {
			QC_AST_Type_Decl *builtin_type = NULL;
			QC_AST_Type_Decl *field_sub_type = NULL;
			QC_AST_Type_Decl *matrix_sub_type = NULL;

			if (bt.is_field) {
				QC_Builtin_Type sub_type = bt;
				sub_type.is_field = QC_false;
				field_sub_type = qc_create_builtin_decl(ctx, sub_type);
			}

			if (bt.is_matrix) {
				QC_Builtin_Type sub_type = bt;
				sub_type.is_matrix = QC_false;
				sub_type.is_field = QC_false;
				matrix_sub_type = qc_create_builtin_decl(ctx, sub_type);

				/* 'int matrix field' generates int, matrix, and field type decls.
				 * This sets matrix->builtin_sub_type_decl to the int type decl. */
				if (field_sub_type)
					field_sub_type->builtin_sub_type_decl = matrix_sub_type;
			}

			builtin_type = qc_create_builtin_decl(ctx, bt);

			if (field_sub_type) {
				builtin_type->builtin_sub_type_decl = field_sub_type;
			} else if (matrix_sub_type) {
				builtin_type->builtin_sub_type_decl = matrix_sub_type;
			}

			found_decl = QC_AST_BASE(builtin_type);
		} else {
			QC_AST_Ident *ident = NULL;
			if (!parse_ident(ctx, &ident, NULL, QC_false))
				goto mismatch;
			if (!resolve_parsed_ident(ctx, ident)) {
				qc_destroy_node(QC_AST_BASE(ident));
				goto mismatch;
			}

			found_decl = ident->decl;
			type->ident = ident;
		}

		if (found_decl || !ctx->allow_undeclared) {
			QC_ASSERT(found_decl);
			if (found_decl->type == QC_AST_type_decl) {
				type->base_type_decl = (QC_AST_Type_Decl*)found_decl;
			} else if (found_decl->type == QC_AST_typedef) {
				QC_CASTED_NODE(QC_AST_Typedef, def, found_decl);
				type->base_typedef = def;
				type->base_type_decl = def->type->base_type_decl;
			} else {
				report_error_expected(ctx, "type name", cur_tok(ctx));
				goto mismatch;
			}
		}
	}

	/* Pointer * */
	while (accept_tok(ctx, QC_Token_mul))
		++type->ptr_depth;

	if (ret_ident && enclosing_decl) {
		/* Variable name */
		if (!parse_ident(ctx, ret_ident, enclosing_decl, QC_false)) {
			report_error_expected(ctx, "identifier", cur_tok(ctx));
			goto mismatch;
		}
	}

	end_node_parsing(ctx);

	*ret_type = type;
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_var_decl(Parse_Ctx *ctx, QC_AST_Node **ret, QC_Bool is_param_decl)
{
	QC_AST_Var_Decl *decl = qc_create_var_decl_node();
	begin_node_parsing(ctx, QC_AST_BASE(decl));

	if (!parse_type_and_ident(ctx, &decl->type, &decl->ident, QC_AST_BASE(decl)))
		goto mismatch;

	if (!is_param_decl) {
		if (accept_tok(ctx, QC_Token_assign)) {
			if (!parse_expr(ctx, &decl->value, 0, NULL, QC_true))
				goto mismatch;
		} else if (!accept_tok(ctx, QC_Token_semi)) {
			report_error_expected(ctx, "';'", cur_tok(ctx));
			goto mismatch;
		}
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(decl);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_func_decl(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Func_Decl *decl = qc_create_func_decl_node();
	begin_node_parsing(ctx, QC_AST_BASE(decl));

	if (!parse_type_and_ident(ctx, &decl->return_type, &decl->ident, QC_AST_BASE(decl)))
		goto mismatch;

	if (!accept_tok(ctx, QC_Token_open_paren)) {
		report_error_expected(ctx, "'('", cur_tok(ctx));
		goto mismatch;
	}

	{ /* Parse parameter declaration list */
		while (cur_tok(ctx)->type != QC_Token_close_paren) {
			QC_AST_Var_Decl *param_decl = NULL;
			if (cur_tok(ctx)->type == QC_Token_comma)
				advance_tok(ctx);

			if (accept_tok(ctx, QC_Token_ellipsis)) {
				decl->ellipsis = QC_true;
				continue;
			}

			if (!parse_var_decl(ctx, (QC_AST_Node**)&param_decl, QC_true))
				goto mismatch;

			qc_push_array(QC_AST_Var_Decl_Ptr)(&decl->params, param_decl);
		}
	}

	if (!accept_tok(ctx, QC_Token_close_paren)) {
		report_error_expected(ctx, "')'", cur_tok(ctx));
		goto mismatch;
	}

	if (cur_tok(ctx)->type == QC_Token_semi) {
		/* No body */
		accept_tok(ctx, QC_Token_semi);
	} else if (parse_scope(ctx, &decl->body, QC_false)) {
		;
	} else {
		goto mismatch;
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(decl);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

/* Parse example: { .... } */
QC_INTERNAL QC_Bool parse_scope(Parse_Ctx *ctx, QC_AST_Scope **ret, QC_Bool already_created)
{
	QC_AST_Scope *scope = (already_created ? *ret : qc_create_scope_node());

	begin_node_parsing(ctx, QC_AST_BASE(scope));

	if (!accept_tok(ctx, QC_Token_open_brace)) {
		report_error_expected(ctx, "'{'", cur_tok(ctx));
		goto mismatch;
	}

	while (!accept_tok(ctx, QC_Token_close_brace)) {
		QC_AST_Node *element = NULL;
		if (!parse_statement(ctx, &element))
			goto mismatch;
		qc_push_array(QC_AST_Node_Ptr)(&scope->nodes, element);
	}

	end_node_parsing(ctx);

	*ret = scope;
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

/* Parse example: 1234 */
QC_INTERNAL QC_Bool parse_literal(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Literal *literal = qc_create_literal_node();
	QC_Token *tok = cur_tok(ctx);

	begin_node_parsing(ctx, QC_AST_BASE(literal));

	switch (tok->type) {
		case QC_Token_int:
			literal->type = QC_Literal_integer;
			literal->value.integer = str_to_int(tok->text);
			literal->base_type_decl = qc_create_builtin_decl(ctx, qc_int_builtin_type());
			advance_tok(ctx);
		break;
		case QC_Token_float:
			literal->type = QC_Literal_floating;
			literal->value.floating = str_to_float(tok->text);
			literal->base_type_decl = qc_create_builtin_decl(ctx, qc_float_builtin_type());
			advance_tok(ctx);
		break;
		case QC_Token_kw_true:
			literal->type = QC_Literal_boolean;
			literal->value.boolean = QC_true;
			literal->base_type_decl = qc_create_builtin_decl(ctx, qc_boolean_builtin_type());
			advance_tok(ctx);
		break;
		case QC_Token_kw_false:
			literal->type = QC_Literal_boolean;
			literal->value.boolean = QC_false;
			literal->base_type_decl = qc_create_builtin_decl(ctx, qc_boolean_builtin_type());
			advance_tok(ctx);
		break;
		case QC_Token_string:
			literal->type = QC_Literal_string;
			literal->value.string = qc_create_array(char)(tok->text.len + 1);
			qc_append_str(&literal->value.string, "%.*s", QC_BUF_STR_ARGS(tok->text));
			literal->base_type_decl = qc_create_builtin_decl(ctx, qc_char_builtin_type());
			advance_tok(ctx);
		break;
		case QC_Token_kw_null:
			literal->type = QC_Literal_null;
			advance_tok(ctx);
		break;
		case QC_Token_open_paren:
			/* Compound literal */
			literal->type = QC_Literal_compound;
			literal->value.compound.subnodes = qc_create_array(QC_AST_Node_Ptr)(2);

			/* (Type) part */
			advance_tok(ctx);
			if (!parse_type(ctx, &literal->value.compound.type))
				goto mismatch;
			if (!accept_tok(ctx, QC_Token_close_paren)) {
				report_error_expected(ctx, "')'", cur_tok(ctx));
				goto mismatch;
			}

			/* {...} part */
			if (!accept_tok(ctx, QC_Token_open_brace)) {
				report_error_expected(ctx, "'{'", cur_tok(ctx));
				goto mismatch;
			}
			if (!parse_arg_list(ctx, &literal->value.compound.subnodes, QC_Token_close_brace))
				goto mismatch;
		break;
		case QC_Token_open_brace:
			/* Initializer list */

			literal->type = QC_Literal_compound;
			literal->value.compound.subnodes = qc_create_array(QC_AST_Node_Ptr)(2);

			advance_tok(ctx); /* Skip '{' */
			if (!parse_arg_list(ctx, &literal->value.compound.subnodes, QC_Token_close_brace))
				goto mismatch;
		break;
		default:
			report_error_expected(ctx, "literal", cur_tok(ctx));
			goto mismatch;
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(literal);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_uop(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Biop *biop = qc_create_biop_node();

	begin_node_parsing(ctx, QC_AST_BASE(biop));

	if (!is_unary_op(cur_tok(ctx)->type))
		goto mismatch;

	biop->type = cur_tok(ctx)->type;
	biop->is_top_level = (ctx->expr_depth == 1);
	advance_tok(ctx);

	/* The precedence should be higher than any binary operation, because -1 op 2 != -(1 op 2) */
	if (!parse_expr(ctx, &biop->rhs, UOP_PRECEDENCE, NULL, QC_false)) {
		goto mismatch;
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(biop);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}


/* Parses 'foo, bar)' */
QC_INTERNAL QC_Bool parse_arg_list(Parse_Ctx *ctx, QC_Array(QC_AST_Node_Ptr) *ret, QC_Token_Type ending)
{
	while (cur_tok(ctx)->type != ending) {
		QC_AST_Node *arg = NULL;
		if (cur_tok(ctx)->type == QC_Token_comma)
			advance_tok(ctx);

		/* Normal expression */
		if (!parse_expr(ctx, (QC_AST_Node**)&arg, 0, NULL, QC_false))
			goto mismatch;

		qc_push_array(QC_AST_Node_Ptr)(ret, arg);
	}

	if (!accept_tok(ctx, ending)) {
		report_error(ctx, QC_PRIORITY_DEFAULT, "List didn't end with '%s', but '%s'",
				qc_tokentype_str(ending), QC_BUF_STR_ARGS(cur_tok(ctx)->text));
		goto mismatch;
	}

	return QC_true;

mismatch:
	return QC_false;
}

/* Parse example: (4 * 2) */
QC_INTERNAL QC_Bool parse_expr_inside_parens(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	if (!accept_tok(ctx, QC_Token_open_paren)) {
		report_error_expected(ctx, "'('", cur_tok(ctx));
		goto mismatch;
	}

	if (!parse_expr(ctx, ret, 0, NULL, QC_false))
		goto mismatch;

	if (!accept_tok(ctx, QC_Token_close_paren)) {
		report_error_expected(ctx, "')'", cur_tok(ctx));
		goto mismatch;
	}

	return QC_true;

mismatch:
	return QC_false;
}


/* Functions '*_expr' are part of 'parse_expr', and therefore take lhs as a param */

/* Parse example: pre-parsed-expr + '(a, b, c)' */
QC_INTERNAL QC_Bool parse_call_expr(Parse_Ctx *ctx, QC_AST_Node **ret, QC_AST_Node *expr, QC_AST_Type *hint)
{
	QC_AST_Call *call = qc_create_call_node();
	QC_AST_Ident *ident = qc_unwrap_ident(expr);
	QC_Array(QC_AST_Type) types = {0};
	begin_node_parsing(ctx, QC_AST_BASE(call));

	if (ident && ident->decl && ident->decl->type != QC_AST_func_decl) {
		/* e.g. Discard e.g. field accessing with () */
		goto mismatch;
	}

	call->base = qc_try_create_access(expr);

	if (!accept_tok(ctx, QC_Token_open_paren)) {
		report_error_expected(ctx, "'('", cur_tok(ctx));
		goto mismatch;
	}

	if (!parse_arg_list(ctx, &call->args, QC_Token_close_paren))
		goto mismatch;

	if (!qc_resolve_overloaded_call(&ctx->parent_map, call, hint) && !ctx->allow_undeclared)
		goto mismatch;

	end_node_parsing(ctx);
	qc_destroy_array(QC_AST_Type)(&types);

	*ret = QC_AST_BASE(call);
	return QC_true;

mismatch:
	qc_destroy_array(QC_AST_Type)(&types);
	call->base = NULL;
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_var_access_expr(Parse_Ctx *ctx, QC_AST_Node **ret, QC_AST_Node *expr)
{
	QC_AST_Access *access = qc_create_access_node();

	begin_node_parsing(ctx, QC_AST_BASE(access));

	if (expr->type != QC_AST_ident) {
		report_error_expected(ctx, "identifier", expr->begin_tok);
		goto mismatch;
	}

	{
		QC_CASTED_NODE(QC_AST_Ident, ident, expr);
		if (!resolve_parsed_ident(ctx, ident))
			goto mismatch;

		if (ctx->allow_undeclared || ident->decl->type == QC_AST_var_decl) {
			access->base = QC_AST_BASE(ident);
			access->is_var_access = QC_true;
		} else {
			ident->decl = NULL;
			goto mismatch;
		}
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(access);
	return QC_true;

mismatch:
	access->base = NULL;
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_biop_expr(Parse_Ctx *ctx, QC_AST_Node **ret, QC_AST_Node *lhs)
{
	QC_AST_Biop *biop = qc_create_biop_node();
	QC_AST_Node *rhs = NULL;
	QC_Token *tok = cur_tok(ctx);
	QC_AST_Type new_type_hint;
	QC_AST_Type *new_type_hint_ptr = NULL;
	int prec = qc_biop_prec(tok->type);
	int assoc = qc_biop_assoc(tok->type);
	int next_min_prec;

	begin_node_parsing(ctx, QC_AST_BASE(biop));

	if (!is_binary_op(cur_tok(ctx)->type))
		goto mismatch;

	if (assoc == -1)
		next_min_prec = prec + 1;
	else
		next_min_prec = prec;
	advance_tok(ctx);

	/* E.g. LHS of 'field = alloc_field(1);' chooses the allocation function through type hint */ 
	if (qc_expr_type(&new_type_hint, lhs))
		new_type_hint_ptr = &new_type_hint;
	if (!parse_expr(ctx, &rhs, next_min_prec, new_type_hint_ptr, QC_false))
		goto mismatch;

	/* @todo Type checking */
	/* @todo Operator overloading */
	biop->type = tok->type;
	biop->lhs = lhs;
	biop->rhs = rhs;
	biop->is_top_level = (ctx->expr_depth == 1);

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(biop);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_member_access_expr(Parse_Ctx *ctx, QC_AST_Node **ret, QC_AST_Node *base)
{
	QC_AST_Access *access = qc_create_access_node();
	QC_AST_Type base_type;

	begin_node_parsing(ctx, QC_AST_BASE(access));

	if (!accept_tok(ctx, QC_Token_dot) && !accept_tok(ctx, QC_Token_right_arrow))
		goto mismatch;

	if (!qc_expr_type(&base_type, base) && !ctx->allow_undeclared) {
		report_error(ctx, QC_PRIORITY_DEFAULT, "Expression does not have accessible members");
		goto mismatch;
	}

	{
		QC_AST_Ident *sub = NULL;
		access->base = base;
		access->is_member_access = QC_true;

		if (!parse_ident(ctx, &sub, NULL, QC_false))
			goto mismatch;

		if (!ctx->allow_undeclared) {
			QC_AST_Scope *base_type_scope = base_type.base_type_decl->body;
			QC_ASSERT(base_type_scope);
			if (!qc_resolve_ident_in_scope(sub, base_type_scope))
				goto mismatch;
		}
		qc_push_array(QC_AST_Node_Ptr)(&access->args, QC_AST_BASE(sub));
	}

	*ret = QC_AST_BASE(access);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

QC_INTERNAL QC_Bool parse_element_access_expr(Parse_Ctx *ctx, QC_AST_Node **ret, QC_AST_Node *base)
{
	QC_AST_Access *access = qc_create_access_node();
	QC_AST_Type base_type;

	begin_node_parsing(ctx, QC_AST_BASE(access));

	if (!accept_tok(ctx, QC_Token_open_paren))
		goto mismatch;

	access->is_element_access = QC_true;
	access->base = base;

	if (!qc_expr_type(&base_type, access->base)) {
		report_error(ctx, QC_PRIORITY_DEFAULT, "Invalid type");
		goto mismatch;
	}

	if (base_type.ptr_depth == 1) {
		access->implicit_deref = QC_true;
	} else if (base_type.ptr_depth > 1) {
		report_error(ctx, QC_PRIORITY_DEFAULT, "Trying to access elements of ptr (depth %i). Only 1 level of implicit dereferencing allowed.", base_type.ptr_depth);
		goto mismatch;
	}

	if (!parse_arg_list(ctx, &access->args, QC_Token_close_paren))
		goto mismatch;

	*ret = QC_AST_BASE(access);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}


/* Parse example: var = 5 + 3 * 2; */
QC_INTERNAL QC_Bool parse_expr(Parse_Ctx *ctx, QC_AST_Node **ret, int min_prec, QC_AST_Type *type_hint, QC_Bool semi)
{
	QC_AST_Node *expr = NULL;

	++ctx->expr_depth;

	if (	parse_ident(ctx, (QC_AST_Ident**)&expr, NULL, QC_true) &&
			resolve_parsed_ident(ctx, (QC_AST_Ident*)expr)) {
		/* If ident is var, wrap it in QC_AST_Access */
		if (parse_var_access_expr(ctx, &expr, expr)) {
			;
		}
	} else if (parse_uop(ctx, &expr)) {
		;
	} else if (parse_literal(ctx, &expr)) {
		;
	} else if (parse_expr_inside_parens(ctx, &expr)) {
		;
	} else {
		report_error_expected(ctx, "identifier or literal", cur_tok(ctx));
		goto mismatch;
	}

	while (qc_biop_prec(cur_tok(ctx)->type) >= min_prec) {

		if (parse_biop_expr(ctx, &expr, expr)) {
			;
		} else if (parse_call_expr(ctx, &expr, expr, type_hint)) {
			;
		} else if (parse_member_access_expr(ctx, &expr, expr)) {
			;
		} else if (parse_element_access_expr(ctx, &expr, expr)) {
			;
		} else {
			QC_FAIL(("Expression parsing logic failed"));
		}
	}

	/* @todo Think about accept_tok(;) -> accept_stmt_end, which also checks for eof and ending scope */
	if (semi && !accept_tok(ctx, QC_Token_semi) && cur_tok(ctx)->type != QC_Token_eof) {
		report_error_expected(ctx, "';'", cur_tok(ctx));
		goto mismatch;
	}

	/* @todo This leaks memory (?) */
	if (expr->type == QC_AST_ident) {
		QC_CASTED_NODE(QC_AST_Ident, ident, expr);
		if (!ident->decl) {
			report_error(ctx, QC_PRIORITY_UNDECLARED, "'%s' not declared in this scope", ident->text.data);
			goto mismatch;
		}
	}

	--ctx->expr_depth;

	QC_ASSERT(expr);
	*ret = expr;
	return QC_true;

mismatch:

	--ctx->expr_depth;
	return QC_false;
}

/* Parse example: return 42; */
QC_INTERNAL QC_Bool parse_control(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_Token *tok = cur_tok(ctx);
	QC_AST_Control *control = qc_create_control_node();
	control->type = tok->type;

	begin_node_parsing(ctx, QC_AST_BASE(control));

	switch (tok->type) {
		case QC_Token_kw_return: {
			advance_tok(ctx);
			if (!parse_statement(ctx, &control->value)) {
				report_error_expected(ctx, "return value", cur_tok(ctx));
				goto mismatch;
			}
		} break;
		case QC_Token_kw_goto: {
			advance_tok(ctx);
			if (!parse_statement(ctx, &control->value)) {
				report_error_expected(ctx, "goto label", cur_tok(ctx));
				goto mismatch;
			}
		} break;
		case QC_Token_kw_continue:
		case QC_Token_kw_break:
			advance_tok(ctx);
		break;
		default:
			report_error_expected(ctx, "control statement", cur_tok(ctx));
			goto mismatch;
	}

	end_node_parsing(ctx);

	*ret = QC_AST_BASE(control);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

/* Parse example: if (..) { .. } else { .. } */
QC_INTERNAL QC_Bool parse_cond(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Cond *cond = qc_create_cond_node();
	begin_node_parsing(ctx, QC_AST_BASE(cond));

	if (!accept_tok(ctx, QC_Token_kw_if)) {
		report_error_expected(ctx, "'if'", cur_tok(ctx));
		goto mismatch;
	}

	if (!parse_expr_inside_parens(ctx, &cond->expr))
		goto mismatch;

	if (!accept_tok(ctx, QC_Token_semi)) {
		if (parse_scope(ctx, &cond->body, QC_false)) {
			;
		} else {
			/* Always have scope, because then we can expand one statement to multiple ones without worry */
			QC_AST_Scope *scope = qc_create_scope_node();
			QC_AST_Node *elem = NULL;
			cond->body = scope;
			cond->implicit_scope = QC_true;

			if (parse_statement(ctx, &elem)) {
				qc_push_array(QC_AST_Node_Ptr)(&scope->nodes, elem);
			} else {
				goto mismatch;
			}
		}
	}

	if (accept_tok(ctx, QC_Token_kw_else)) {
		if (!accept_tok(ctx, QC_Token_semi)) {
			if (!parse_statement(ctx, &cond->after_else))
				goto mismatch;
		}

		if (cond->after_else) {
			if (	cond->after_else->type != QC_AST_scope ||
					cond->after_else->type != QC_AST_cond) {
				report_error_expected(ctx, "'if', '{' or ';'", cur_tok(ctx));
			}
		}
	}

	end_node_parsing(ctx);
	*ret = QC_AST_BASE(cond);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

/* Parse example: while(1) { .. } -- for (i = 0; i < 10; ++i) { .. } */
QC_INTERNAL QC_Bool parse_loop(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Loop *loop = qc_create_loop_node();
	begin_node_parsing(ctx, QC_AST_BASE(loop));

	if (accept_tok(ctx, QC_Token_kw_for)) {
		if (!accept_tok(ctx, QC_Token_open_paren)) {
			report_error_expected(ctx, "'('", cur_tok(ctx));
			goto mismatch;
		}

		if (!parse_statement(ctx, &loop->init))
			goto mismatch;

		if (!parse_expr(ctx, &loop->cond, 0, NULL, QC_true))
			goto mismatch;

		if (!parse_expr(ctx, &loop->incr, 0, NULL, QC_false)) {
			goto mismatch;
		}

		if (!accept_tok(ctx, QC_Token_close_paren)) {
			report_error_expected(ctx, "')'", cur_tok(ctx));
			goto mismatch;
		}
	} else if (accept_tok(ctx, QC_Token_kw_while)) {
		if (!parse_expr_inside_parens(ctx, &loop->cond))
			goto mismatch;
	} else {
		report_error_expected(ctx, "beginning of a loop", cur_tok(ctx));
		goto mismatch;
	}

	if (!accept_tok(ctx, QC_Token_semi)) {
		if (parse_scope(ctx, &loop->body, QC_false)) {
			;
		} else {
			/* Always have scope, because then we can expand one statement to multiple ones without worry */
			QC_AST_Scope *scope = qc_create_scope_node();
			QC_AST_Node *elem = NULL;
			loop->body = scope;
			loop->implicit_scope = QC_true;

			if (parse_statement(ctx, &elem)) {
				qc_push_array(QC_AST_Node_Ptr)(&scope->nodes, elem);
			} else {
				goto mismatch;
			}
		}
	}

	end_node_parsing(ctx);
	*ret = QC_AST_BASE(loop);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

/* Parse example: for_field (output; input) { .. } */
QC_INTERNAL QC_Bool parse_parallel(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Parallel *parallel = qc_create_parallel_node();
	begin_node_parsing(ctx, QC_AST_BASE(parallel));

	if (!accept_tok(ctx, QC_Token_kw_parallel))
		goto mismatch;

	if (!accept_tok(ctx, QC_Token_open_paren)) {
		report_error_expected(ctx, "'('", cur_tok(ctx));
		goto mismatch;
	}

	if (!parse_expr(ctx, &parallel->output, 0, NULL, QC_true))
		goto mismatch;

	{ /* Fetch dimension from output type */
		QC_AST_Type out_type;
		if (!qc_expr_type(&out_type, parallel->output) || !out_type.base_type_decl->is_builtin) {
			report_error(ctx, QC_PRIORITY_DEFAULT, "Parallel loop has invalid output type");
			goto mismatch;
		}

		parallel->dim = out_type.base_type_decl->builtin_type.field_dim;
	}

	if (!parse_expr(ctx, &parallel->input, 0, NULL, QC_false))
		goto mismatch;

	if (!accept_tok(ctx, QC_Token_close_paren)) {
		report_error_expected(ctx, "')'", cur_tok(ctx));
		goto mismatch;
	}

	{
		QC_AST_Scope *scope = qc_create_scope_node();
		{ /* Builtin declaration of 'int matrix(dim) id;' to the beginning of the loop */
			QC_AST_Type_Decl *bt_decl;
			QC_AST_Var_Decl *id_decl;
			QC_Builtin_Type bt = {0};
			bt.is_integer = QC_true;
			bt.is_matrix = QC_true;
			bt.matrix_rank = 1;
			bt.matrix_dim[0] = parallel->dim;
			bt_decl = qc_create_builtin_decl(ctx, bt);

			id_decl = qc_create_var_decl(bt_decl, qc_create_ident_with_text(NULL, "id"), NULL);
			qc_push_array(QC_AST_Node_Ptr)(&scope->nodes, QC_AST_BASE(id_decl));
		}

		if (!parse_scope(ctx, &scope, QC_true))
			goto mismatch;

		parallel->body = scope;
	}

	end_node_parsing(ctx);
	*ret = QC_AST_BASE(parallel);
	return QC_true;

mismatch:
	cancel_node_parsing(ctx);
	return QC_false;
}

/* Parse the next self-contained thing - var decl, function decl, statement, expr... */
QC_INTERNAL QC_Bool parse_statement(Parse_Ctx *ctx, QC_AST_Node **ret)
{
	QC_AST_Node *result = NULL;

	/* @todo Heuristic */
	if (accept_tok(ctx, QC_Token_semi)) {
		report_error(ctx, QC_PRIORITY_DEFAULT, "Unexpected ';'");
		goto mismatch;
	} else if (parse_type_decl(ctx, &result))
		;
	else if (parse_typedef(ctx, &result))
		;
	else if (parse_var_decl(ctx, &result, QC_false))
		;
	else if (parse_func_decl(ctx, &result))
		;
	else if (parse_expr(ctx, &result, 0, NULL, QC_true))
		;
	else if (parse_control(ctx, &result))
		;
	else if (parse_scope(ctx, (QC_AST_Scope**)&result, QC_false))
		;
	else if (parse_cond(ctx, &result))
		;
	else if (parse_loop(ctx, &result))
		;
	else if (parse_parallel(ctx, &result))
		;
	else 
		goto mismatch;

	*ret = result;
	return QC_true;

mismatch:
	return QC_false;
}

QC_AST_Scope *qc_parse_tokens(QC_Token *toks, QC_Bool dont_reference_tokens, QC_Bool allow_undeclared)
{
	QC_Bool failure = QC_false;
	Parse_Ctx ctx = {0};
	QC_AST_Scope *root = qc_create_ast();

	ctx.root = root;
	ctx.parse_stack = qc_create_array(Parse_Stack_Frame)(32);
	ctx.tok = ctx.first_tok = toks;
	ctx.parent_map = qc_create_parent_map(NULL);
	ctx.dont_ref_tokens = dont_reference_tokens;
	ctx.allow_undeclared = allow_undeclared;
	if (qc_is_comment_tok(ctx.tok->type))
		advance_tok(&ctx);

	{ /* Create C builtin types (backends should be able to rely that they exist in the QC_AST) */
		qc_create_builtin_decl(&ctx, qc_void_builtin_type());
		qc_create_builtin_decl(&ctx, qc_int_builtin_type());
		qc_create_builtin_decl(&ctx, qc_char_builtin_type());
		qc_create_builtin_decl(&ctx, qc_boolean_builtin_type());
		/* @todo Rest */
	}

	begin_node_parsing(&ctx, QC_AST_BASE(root));
	while (ctx.tok->type != QC_Token_eof) {
		QC_AST_Node *elem = NULL;
		if (!parse_statement(&ctx, &elem)) {
			failure = QC_true;
			cancel_node_parsing(&ctx);
			root = NULL;
			break;
		}
		qc_push_array(QC_AST_Node_Ptr)(&root->nodes, elem); 
	}
	if (!failure)
		end_node_parsing(&ctx);

	if (!failure) { /* Insert builtin declarations to beginning of root node */
		QC_Array(QC_AST_Node_Ptr) new_nodes = qc_create_array(QC_AST_Node_Ptr)(ctx.parent_map.builtin_decls.size + root->nodes.size);
		int i;
		/* @todo QC_Array insert function */
		for (i = 0; i < ctx.parent_map.builtin_decls.size; ++i) {
			qc_push_array(QC_AST_Node_Ptr)(&new_nodes, ctx.parent_map.builtin_decls.data[i]);
		}
		for (i = 0; i < root->nodes.size; ++i) {
			qc_push_array(QC_AST_Node_Ptr)(&new_nodes, root->nodes.data[i]);
		}
		qc_destroy_array(QC_AST_Node_Ptr)(&root->nodes);
		root->nodes = new_nodes;
	}

	if (failure) {
		QC_Token *tok = ctx.error_tok;
		const char *msg = ctx.error_msg.data;
		if (tok && msg) {
			printf("Error at line %i near token '%.*s':\n   %s\n",
					tok->line, QC_BUF_STR_ARGS(tok->text), msg);
		} else {
			printf("Internal parser error: no error message (this is a bug)\n");
		}
		printf("Compilation failed\n");
		qc_destroy_ast(root);
		root = NULL;
	}

	qc_destroy_array(char)(&ctx.error_msg);
	qc_destroy_parent_map(&ctx.parent_map);
	qc_destroy_array(Parse_Stack_Frame)(&ctx.parse_stack);

	return root;
}

QC_AST_Scope *qc_parse_string(QC_AST_Node **expr, const char *string)
{
	QC_Array(QC_Token) tokens = {0};
	QC_AST_Scope *root = NULL;
	*expr = NULL;

	tokens = qc_tokenize(string, strlen(string));
	if (!tokens.data)
		goto cleanup;

	root = qc_parse_tokens(tokens.data, QC_true, QC_true);

cleanup:
	qc_destroy_array(QC_Token)(&tokens);

	QC_ASSERT(root->nodes.size > 0);

	/* Last node in the global scope is the expr. There are builtin type decls before it. */
	*expr = root->nodes.data[root->nodes.size - 1];	

	return root;
}
