#include "ast.h"

QC_DEFINE_ARRAY(QC_AST_Node_Ptr)
QC_DEFINE_ARRAY(QC_AST_Var_Decl_Ptr)
QC_DEFINE_ARRAY(QC_Token_Ptr)
DEFINE_HASH_TABLE(QC_AST_Node_Ptr, QC_AST_Node_Ptr)

QC_Bool type_node_equals(QC_AST_Type a, QC_AST_Type b)
{
	if (	a.ptr_depth != b.ptr_depth ||
			a.array_size != b.array_size ||
			a.is_const != b.is_const ||
			a.base_type_decl != b.base_type_decl)
		return QC_false;
	return QC_true;
}

QC_Bool builtin_type_equals(QC_Builtin_Type a, QC_Builtin_Type b)
{
	QC_Bool is_same_matrix = (a.is_matrix == b.is_matrix);
	QC_Bool is_same_field = (a.is_field == b.is_field);
	if (is_same_matrix && a.is_matrix)
		is_same_matrix = !memcmp(a.matrix_dim, b.matrix_dim, sizeof(a.matrix_dim));
	if (is_same_field && a.is_field)
		is_same_field = (a.field_dim == b.field_dim);

	return	a.is_void == b.is_void &&
			a.is_integer == b.is_integer &&
			a.is_float == b.is_float &&
			a.bitness == b.bitness &&
			a.is_unsigned == b.is_unsigned &&
			is_same_matrix &&
			is_same_field;
}

QC_INTERNAL QC_AST_Node *qc_create_node_impl(QC_AST_Node_Type type, int size)
{
	QC_AST_Node *n = QC_MALLOC(size);
	memset(n, 0, size);
	n->type = type;
	n->pre_comments = qc_create_array(QC_Token_Ptr)(0);
	n->post_comments = qc_create_array(QC_Token_Ptr)(0);
	return n;
}
#define CREATE_NODE(type, type_enum) ((type*)qc_create_node_impl(type_enum, sizeof(type)))

QC_AST_Node *qc_create_ast_node(QC_AST_Node_Type type)
{
	switch (type) {
		case QC_AST_scope: return QC_AST_BASE(qc_create_scope_node());
		case QC_AST_ident: return QC_AST_BASE(qc_create_ident_node());
		case QC_AST_type: return QC_AST_BASE(qc_create_type_node());
		case QC_AST_type_decl: return QC_AST_BASE(qc_create_type_decl_node());
		case QC_AST_var_decl: return QC_AST_BASE(qc_create_var_decl_node());
		case QC_AST_func_decl: return QC_AST_BASE(qc_create_func_decl_node());
		case QC_AST_literal: return QC_AST_BASE(qc_create_literal_node());
		case QC_AST_biop: return QC_AST_BASE(qc_create_biop_node());
		case QC_AST_control: return QC_AST_BASE(qc_create_control_node());
		case QC_AST_call: return QC_AST_BASE(qc_create_call_node());
		case QC_AST_access: return QC_AST_BASE(qc_create_access_node());
		case QC_AST_cond: return QC_AST_BASE(qc_create_cond_node());
		case QC_AST_loop: return QC_AST_BASE(qc_create_loop_node());
		case QC_AST_cast: return QC_AST_BASE(qc_create_cast_node());
		case QC_AST_typedef: return QC_AST_BASE(qc_create_typedef_node());
		case QC_AST_parallel: return QC_AST_BASE(qc_create_parallel_node());
		default: QC_FAIL(("qc_create_ast_node: Unknown node type %i", type));
	}
}

QC_AST_Scope *qc_create_scope_node()
{
	QC_AST_Scope *scope = CREATE_NODE(QC_AST_Scope, QC_AST_scope);
	scope->nodes = qc_create_array(QC_AST_Node_Ptr)(8);
	return scope;
}

QC_AST_Ident *qc_create_ident_node()
{
	QC_AST_Ident * ident = CREATE_NODE(QC_AST_Ident, QC_AST_ident);
	ident->text = qc_create_array(char)(1);
	qc_append_str(&ident->text, "");
	return ident;
}

QC_AST_Type *qc_create_type_node()
{ return CREATE_NODE(QC_AST_Type, QC_AST_type); }

QC_AST_Type_Decl *qc_create_type_decl_node()
{ return CREATE_NODE(QC_AST_Type_Decl, QC_AST_type_decl); }

QC_AST_Var_Decl *qc_create_var_decl_node()
{ return CREATE_NODE(QC_AST_Var_Decl, QC_AST_var_decl); }

QC_AST_Func_Decl *qc_create_func_decl_node()
{ return CREATE_NODE(QC_AST_Func_Decl, QC_AST_func_decl); }

QC_AST_Literal *qc_create_literal_node()
{ return CREATE_NODE(QC_AST_Literal, QC_AST_literal); }

QC_AST_Biop *qc_create_biop_node()
{ return CREATE_NODE(QC_AST_Biop, QC_AST_biop); }

QC_AST_Control *qc_create_control_node()
{ return CREATE_NODE(QC_AST_Control, QC_AST_control); }

QC_AST_Call *qc_create_call_node()
{
	QC_AST_Call *call = CREATE_NODE(QC_AST_Call, QC_AST_call);
	call->args = qc_create_array(QC_AST_Node_Ptr)(0);
	return call;
}

QC_AST_Access *qc_create_access_node()
{
	QC_AST_Access *access = CREATE_NODE(QC_AST_Access, QC_AST_access);
	access->args = qc_create_array(QC_AST_Node_Ptr)(1);
	return access;
}

QC_AST_Cond *qc_create_cond_node()
{ return CREATE_NODE(QC_AST_Cond, QC_AST_cond); }

QC_AST_Loop *qc_create_loop_node()
{ return CREATE_NODE(QC_AST_Loop, QC_AST_loop); }

QC_AST_Cast *qc_create_cast_node()
{ return CREATE_NODE(QC_AST_Cast, QC_AST_cast); }

QC_AST_Typedef *qc_create_typedef_node()
{ return CREATE_NODE(QC_AST_Typedef, QC_AST_typedef); }

QC_AST_Parallel *qc_create_parallel_node()
{ return CREATE_NODE(QC_AST_Parallel, QC_AST_parallel); }


/* Node copying */

void qc_copy_ast_node_base(QC_AST_Node *dst, QC_AST_Node *src)
{
	int i;
	if (dst == src)
		return;
	/* Type of a node can not be changed */
	/*dst->type = src->type;*/
	dst->begin_tok = src->begin_tok;
	for (i = 0; i < src->pre_comments.size; ++i)
		qc_push_array(QC_Token_Ptr)(&dst->pre_comments, src->pre_comments.data[i]);
	for (i = 0; i < src->post_comments.size; ++i)
		qc_push_array(QC_Token_Ptr)(&dst->post_comments, src->post_comments.data[i]);
	dst->attribute = src->attribute;
}

void qc_copy_ast_node(QC_AST_Node *copy, QC_AST_Node *node, QC_AST_Node **subnodes, int subnode_count, QC_AST_Node **refnodes, int refnode_count)
{
	QC_ASSERT(copy->type == node->type);
	switch (node->type) {
		case QC_AST_scope:
			QC_ASSERT(refnode_count == 0);
			qc_copy_scope_node((QC_AST_Scope*)copy, (QC_AST_Scope*)node, subnodes, subnode_count);
		break;

		case QC_AST_ident: {
			QC_ASSERT(subnode_count == 0 && refnode_count == 1);
			qc_copy_ident_node((QC_AST_Ident*)copy, (QC_AST_Ident*)node, refnodes[0]);
		} break;

		case QC_AST_type: {
			QC_ASSERT(subnode_count == 0 && refnode_count == 2);
			qc_copy_type_node((QC_AST_Type*)copy, (QC_AST_Type*)node, refnodes[0], refnodes[1]);
		} break;

		case QC_AST_type_decl: {
			QC_ASSERT(subnode_count == 2 && refnode_count == 2);
			qc_copy_type_decl_node((QC_AST_Type_Decl*)copy, (QC_AST_Type_Decl*)node, subnodes[0], subnodes[1], refnodes[0], refnodes[1]);
		} break;

		case QC_AST_var_decl: {
			QC_ASSERT(subnode_count == 3 && refnode_count == 0);
			qc_copy_var_decl_node((QC_AST_Var_Decl*)copy, (QC_AST_Var_Decl*)node, subnodes[0], subnodes[1], subnodes[2]);
		} break;

		case QC_AST_func_decl: {
			QC_ASSERT(subnode_count >= 3 && refnode_count == 1);
			qc_copy_func_decl_node((QC_AST_Func_Decl*)copy, (QC_AST_Func_Decl*)node, subnodes[0], subnodes[1], subnodes[2], &subnodes[3], subnode_count - 3, refnodes[0]);
		} break;

		case QC_AST_literal: {
			QC_CASTED_NODE(QC_AST_Literal, literal_node, node);
			if (literal_node->type == QC_Literal_compound) {
				QC_ASSERT(subnode_count >= 1 && refnode_count == 1);
				qc_copy_literal_node((QC_AST_Literal*)copy, (QC_AST_Literal*)node, subnodes[0], &subnodes[1], subnode_count - 1, refnodes[0]);
			} else {
				QC_ASSERT(subnode_count == 0 && refnode_count == 1);
				qc_copy_literal_node((QC_AST_Literal*)copy, (QC_AST_Literal*)node, NULL, NULL, 0, refnodes[0]);
			}
		} break;

		case QC_AST_biop: {
			QC_ASSERT(subnode_count == 2 && refnode_count == 0);
			qc_copy_biop_node((QC_AST_Biop*)copy, (QC_AST_Biop*)node, subnodes[0], subnodes[1]);
		} break;

		case QC_AST_control: {
			QC_ASSERT(subnode_count == 1 && refnode_count == 0);
			qc_copy_control_node((QC_AST_Control*)copy, (QC_AST_Control*)node, subnodes[0]);
		} break;

		case QC_AST_call: {
			QC_ASSERT(subnode_count >= 1 && refnode_count == 0);
			qc_copy_call_node((QC_AST_Call*)copy, (QC_AST_Call*)node, subnodes[0], &subnodes[1], subnode_count - 1);
		} break;

		case QC_AST_access: {
			QC_ASSERT(subnode_count >= 1 && refnode_count == 0);
			qc_copy_access_node((QC_AST_Access*)copy, (QC_AST_Access*)node, subnodes[0], &subnodes[1], subnode_count - 1);
		} break;

		case QC_AST_cond: {
			QC_ASSERT(subnode_count == 3 && refnode_count == 0);
			qc_copy_cond_node((QC_AST_Cond*)copy, (QC_AST_Cond*)node, subnodes[0], subnodes[1], subnodes[2]);
		} break;

		case QC_AST_loop: {
			QC_ASSERT(subnode_count == 4 && refnode_count == 0);
			qc_copy_loop_node((QC_AST_Loop*)copy, (QC_AST_Loop*)node, subnodes[0], subnodes[1], subnodes[2], subnodes[3]);
		} break;

		case QC_AST_cast: {
			QC_ASSERT(subnode_count == 2 && refnode_count == 0);
			qc_copy_cast_node((QC_AST_Cast*)copy, (QC_AST_Cast*)node, subnodes[0], subnodes[1]);
		} break;

		case QC_AST_typedef: {
			QC_ASSERT(subnode_count == 2 && refnode_count == 0);
			qc_copy_typedef_node((QC_AST_Typedef*)copy, (QC_AST_Typedef*)node, subnodes[0], subnodes[1]);
		} break;

		case QC_AST_parallel: {
			QC_ASSERT(subnode_count == 3 && refnode_count == 0);
			qc_copy_parallel_node((QC_AST_Parallel*)copy, (QC_AST_Parallel*)node, subnodes[0], subnodes[1], subnodes[2]);
		} break;
		default: QC_FAIL(("qc_copy_ast_node: Unknown node type %i", node->type));
	}
}

void qc_shallow_copy_ast_node(QC_AST_Node *copy, QC_AST_Node* node)
{
	QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
	QC_Array(QC_AST_Node_Ptr) refnodes = qc_create_array(QC_AST_Node_Ptr)(0);

	qc_push_immediate_subnodes(&subnodes, node);
	qc_push_immediate_refnodes(&refnodes, node);
	qc_copy_ast_node(copy, node, subnodes.data, subnodes.size, refnodes.data, refnodes.size);

	qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
	qc_destroy_array(QC_AST_Node_Ptr)(&refnodes);
}

void qc_copy_scope_node(QC_AST_Scope *copy, QC_AST_Scope *scope, QC_AST_Node **subnodes, int subnode_count)
{
	int i;
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(scope));

	qc_clear_array(QC_AST_Node_Ptr)(&copy->nodes);
	for (i = 0; i < subnode_count; ++i) {
		if (!subnodes[i])
			continue;
		qc_push_array(QC_AST_Node_Ptr)(&copy->nodes, subnodes[i]);
	}
	copy->is_root = scope->is_root;
}

void qc_copy_ident_node(QC_AST_Ident *copy, QC_AST_Ident *ident, QC_AST_Node *ref_to_decl)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(ident));
	if (copy != ident) {
		qc_destroy_array(char)(&copy->text);
		copy->text = qc_copy_array(char)(&ident->text);
	}
	copy->designated = ident->designated;
	copy->decl = ref_to_decl;
}

void qc_copy_type_node(QC_AST_Type *copy, QC_AST_Type *type, QC_AST_Node *ref_to_base_type_decl, QC_AST_Node *ref_to_base_typedef)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(type));
	QC_ASSERT(!ref_to_base_type_decl || ref_to_base_type_decl->type == QC_AST_type_decl);
	QC_ASSERT(!ref_to_base_typedef || ref_to_base_typedef->type == QC_AST_typedef);
	copy->base_type_decl = (QC_AST_Type_Decl*)ref_to_base_type_decl;
	copy->base_typedef = (QC_AST_Typedef*)ref_to_base_typedef;
	copy->ptr_depth = type->ptr_depth;
	copy->array_size = type->array_size;
	copy->is_const = type->is_const;
}

void qc_copy_type_decl_node(QC_AST_Type_Decl *copy, QC_AST_Type_Decl *decl, QC_AST_Node *ident, QC_AST_Node *body, QC_AST_Node *builtin_sub_decl_ref, QC_AST_Node *builtin_decl_ref)
{
	QC_ASSERT(!ident || ident->type == QC_AST_ident);
	QC_ASSERT(!body || body->type == QC_AST_scope);
	QC_ASSERT(!builtin_decl_ref || builtin_decl_ref->type == QC_AST_type_decl);
	QC_ASSERT(!builtin_sub_decl_ref || builtin_sub_decl_ref->type == QC_AST_type_decl);
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(decl));
	copy->ident = (QC_AST_Ident*)ident;
	copy->body = (QC_AST_Scope*)body;
	copy->is_builtin = decl->is_builtin;
	copy->builtin_type = decl->builtin_type;
	copy->builtin_sub_type_decl = (QC_AST_Type_Decl*)builtin_sub_decl_ref;
	copy->builtin_concrete_decl = (QC_AST_Type_Decl*)builtin_decl_ref;
}

void qc_copy_var_decl_node(QC_AST_Var_Decl *copy, QC_AST_Var_Decl *decl, QC_AST_Node *type, QC_AST_Node *ident, QC_AST_Node *value)
{
	QC_ASSERT(type->type == QC_AST_type);
	QC_ASSERT(ident->type == QC_AST_ident);
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(decl));
	copy->type = (QC_AST_Type*)type;
	copy->ident = (QC_AST_Ident*)ident;
	copy->value = value;
}

void qc_copy_func_decl_node(QC_AST_Func_Decl *copy, QC_AST_Func_Decl *decl, QC_AST_Node *return_type, QC_AST_Node *ident, QC_AST_Node *body, QC_AST_Node **params, int param_count, QC_AST_Node *backend_decl_ref)
{
	int i;
	QC_ASSERT(ident->type == QC_AST_ident);
	QC_ASSERT(return_type->type == QC_AST_type);
	QC_ASSERT(!body || body->type == QC_AST_scope);
	QC_ASSERT(!backend_decl_ref || backend_decl_ref->type == QC_AST_func_decl);
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(decl));
	copy->return_type = (QC_AST_Type*)return_type;
	copy->ident = (QC_AST_Ident*)ident;
	copy->body = (QC_AST_Scope*)body;
	copy->ellipsis = decl->ellipsis;
	copy->is_builtin = decl->is_builtin;
	copy->builtin_concrete_decl = (QC_AST_Func_Decl*)backend_decl_ref;

	qc_clear_array(QC_AST_Var_Decl_Ptr)(&copy->params);
	for (i = 0; i < param_count; ++i) {
		QC_ASSERT(params[i]->type == QC_AST_var_decl);
		qc_push_array(QC_AST_Var_Decl_Ptr)(&copy->params, (QC_AST_Var_Decl_Ptr)params[i]);
	}
}

void qc_copy_literal_node(QC_AST_Literal *copy, QC_AST_Literal *literal, QC_AST_Node *comp_type, QC_AST_Node **comp_subs, int comp_sub_count, QC_AST_Node *type_decl_ref)
{
	QC_Bool is_same = (copy == literal);
	if (!is_same) {
		if (copy->type == QC_Literal_string) {
			qc_destroy_array(char)(&copy->value.string);
		} else if (copy->type == QC_Literal_compound) {
			qc_destroy_array(QC_AST_Node_Ptr)(&copy->value.compound.subnodes);
		}
	}

	QC_ASSERT(!type_decl_ref || type_decl_ref->type == QC_AST_type_decl);
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(literal));
	copy->type = literal->type;
	copy->value = literal->value;
	copy->base_type_decl = (QC_AST_Type_Decl*)type_decl_ref;

	if (literal->type == QC_Literal_string) {
		if (!is_same)
			copy->value.string = qc_copy_array(char)(&literal->value.string);
	} else if (literal->type == QC_Literal_compound) {
		int i;
		if (is_same)
			qc_clear_array(QC_AST_Node_Ptr)(&copy->value.compound.subnodes);
		else
			copy->value.compound.subnodes = qc_create_array(QC_AST_Node_Ptr)(comp_sub_count);

		QC_ASSERT(!comp_type || comp_type->type == QC_AST_type);
		copy->value.compound.type = (QC_AST_Type*)comp_type;
		for (i = 0; i < comp_sub_count; ++i) {
			qc_push_array(QC_AST_Node_Ptr)(&copy->value.compound.subnodes, comp_subs[i]);
		}
	}
}

void qc_copy_biop_node(QC_AST_Biop *copy, QC_AST_Biop *biop, QC_AST_Node *lhs, QC_AST_Node *rhs)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(biop));
	copy->type = biop->type;
	copy->is_top_level = biop->is_top_level;
	copy->lhs = lhs;
	copy->rhs = rhs;
}

void qc_copy_control_node(QC_AST_Control *copy, QC_AST_Control *control, QC_AST_Node *value)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(control));
	copy->type = control->type;
	copy->value = value;
}

void qc_copy_call_node(QC_AST_Call *copy, QC_AST_Call *call, QC_AST_Node *base, QC_AST_Node **args, int arg_count)
{
	int i;
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(call));
	copy->base = base;
	qc_clear_array(QC_AST_Node_Ptr)(&copy->args);
	for (i = 0; i < arg_count; ++i) {
		qc_push_array(QC_AST_Node_Ptr)(&copy->args, args[i]);
	}
}

void qc_copy_access_node(QC_AST_Access *copy, QC_AST_Access *access, QC_AST_Node *base, QC_AST_Node **args, int arg_count)
{
	int i;
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(access));
	copy->base = base;
	qc_clear_array(QC_AST_Node_Ptr)(&copy->args);
	for (i = 0; i < arg_count; ++i)
		qc_push_array(QC_AST_Node_Ptr)(&copy->args, args[i]);
	copy->is_var_access = access->is_var_access;
	copy->is_member_access = access->is_member_access;
	copy->is_element_access = access->is_element_access;
	copy->is_array_access = access->is_array_access;
	copy->implicit_deref = access->implicit_deref;
}

void qc_copy_cond_node(QC_AST_Cond *copy, QC_AST_Cond *cond, QC_AST_Node *expr, QC_AST_Node *body, QC_AST_Node *after_else)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(cond));
	QC_ASSERT(!body || body->type == QC_AST_scope);
	copy->expr = expr;
	copy->body = (QC_AST_Scope*)body;
	copy->after_else = after_else;
	copy->implicit_scope = cond->implicit_scope;
}

void qc_copy_loop_node(QC_AST_Loop *copy, QC_AST_Loop *loop, QC_AST_Node *init, QC_AST_Node *cond, QC_AST_Node *incr, QC_AST_Node *body)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(loop));
	QC_ASSERT(!body || body->type == QC_AST_scope);
	copy->init = init;
	copy->cond = cond;
	copy->incr = incr;
	copy->body = (QC_AST_Scope*)body;
	copy->implicit_scope = loop->implicit_scope;
}

void qc_copy_cast_node(QC_AST_Cast *copy, QC_AST_Cast *cast, QC_AST_Node *type, QC_AST_Node *target)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(cast));
	QC_ASSERT(!type || type->type == QC_AST_type);
	copy->type = (QC_AST_Type*)type;
	copy->target = target;
}

void qc_copy_typedef_node(QC_AST_Typedef *copy, QC_AST_Typedef *def, QC_AST_Node *type, QC_AST_Node *ident)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(def));
	QC_ASSERT(!type || type->type == QC_AST_type);
	QC_ASSERT(!ident || ident->type == QC_AST_ident);
	copy->type = (QC_AST_Type*)type;
	copy->ident = (QC_AST_Ident*)ident;
}

void qc_copy_parallel_node(QC_AST_Parallel *copy, QC_AST_Parallel *parallel, QC_AST_Node *output, QC_AST_Node *input, QC_AST_Node *body)
{
	qc_copy_ast_node_base(QC_AST_BASE(copy), QC_AST_BASE(parallel));
	QC_ASSERT(!body || body->type == QC_AST_scope);
	copy->output = output;
	copy->input = input;
	copy->body = (QC_AST_Scope*)body;
	copy->dim = parallel->dim;
}

void qc_destroy_node(QC_AST_Node *node)
{
	int i;
	if (!node)
		return;

	switch (node->type) {
	case QC_AST_scope: {
		QC_CASTED_NODE(QC_AST_Scope, scope, node);
		for (i = 0; i < scope->nodes.size; ++i)
			qc_destroy_node(scope->nodes.data[i]);
	} break;

	case QC_AST_ident: {
	} break;

	case QC_AST_type: {
	} break;

	case QC_AST_type_decl: {
		QC_CASTED_NODE(QC_AST_Type_Decl, decl, node);
		qc_destroy_node(QC_AST_BASE(decl->ident));
		qc_destroy_node(QC_AST_BASE(decl->body));
	} break;

	case QC_AST_var_decl: {
		QC_CASTED_NODE(QC_AST_Var_Decl, decl, node);
		qc_destroy_node(QC_AST_BASE(decl->type));
		qc_destroy_node(QC_AST_BASE(decl->ident));
		qc_destroy_node(decl->value);
	} break;

	case QC_AST_func_decl: {
		QC_CASTED_NODE(QC_AST_Func_Decl, decl, node);
		qc_destroy_node(QC_AST_BASE(decl->return_type));
		qc_destroy_node(QC_AST_BASE(decl->ident));
		for (i = 0; i < decl->params.size; ++i)
			qc_destroy_node(QC_AST_BASE(decl->params.data[i]));
		qc_destroy_node(QC_AST_BASE(decl->body));
	} break;

	case QC_AST_literal: {
		QC_CASTED_NODE(QC_AST_Literal, literal, node);
		if (literal->type == QC_Literal_compound) {
			qc_destroy_node(QC_AST_BASE(literal->value.compound.type));
			for (i = 0; i < literal->value.compound.subnodes.size; ++i)
				qc_destroy_node(literal->value.compound.subnodes.data[i]);
		}
	} break;

	case QC_AST_biop: {
		QC_CASTED_NODE(QC_AST_Biop, op, node);
		qc_destroy_node(op->lhs);
		qc_destroy_node(op->rhs);
	} break;

	case QC_AST_control: {
		QC_CASTED_NODE(QC_AST_Control, control, node);
		qc_destroy_node(control->value);
	} break;

	case QC_AST_call: {
		QC_CASTED_NODE(QC_AST_Call, call, node);
		qc_destroy_node(call->base);
		for (i = 0; i < call->args.size; ++i)
			qc_destroy_node(call->args.data[i]);
	} break;

	case QC_AST_access: {
		QC_CASTED_NODE(QC_AST_Access, access, node);
		qc_destroy_node(access->base);
		for (i = 0; i < access->args.size; ++i)
			qc_destroy_node(access->args.data[i]);
	} break;

	case QC_AST_cond: {
		QC_CASTED_NODE(QC_AST_Cond, cond, node);
		qc_destroy_node(cond->expr);
		qc_destroy_node(QC_AST_BASE(cond->body));
		qc_destroy_node(cond->after_else);
	} break;

	case QC_AST_loop: {
		QC_CASTED_NODE(QC_AST_Loop, loop, node);
		qc_destroy_node(loop->init);
		qc_destroy_node(loop->cond);
		qc_destroy_node(loop->incr);
		qc_destroy_node(QC_AST_BASE(loop->body));
	} break;

	case QC_AST_cast: {
		QC_CASTED_NODE(QC_AST_Cast, cast, node);
		qc_destroy_node(QC_AST_BASE(cast->type));
		qc_destroy_node(cast->target);
	} break;

	case QC_AST_typedef: {
		QC_CASTED_NODE(QC_AST_Typedef, def, node);
		qc_destroy_node(QC_AST_BASE(def->type));
		qc_destroy_node(QC_AST_BASE(def->ident));
	} break;

	case QC_AST_parallel: {
		QC_CASTED_NODE(QC_AST_Parallel, parallel, node);
		qc_destroy_node(parallel->output);
		qc_destroy_node(parallel->input);
		qc_destroy_node(QC_AST_BASE(parallel->body));
	} break;

	default: QC_FAIL(("qc_destroy_node: Unknown node type %i", node->type));
	}
	qc_shallow_destroy_node(node);
}

void qc_shallow_destroy_node(QC_AST_Node *node)
{
	switch (node->type) {
	case QC_AST_scope: {
		QC_CASTED_NODE(QC_AST_Scope, scope, node);
		qc_destroy_array(QC_AST_Node_Ptr)(&scope->nodes);
	} break;

	case QC_AST_ident: {
		QC_CASTED_NODE(QC_AST_Ident, ident, node);
		qc_destroy_array(char)(&ident->text);
	} break;

	case QC_AST_type: break;
	case QC_AST_type_decl: break;
	case QC_AST_var_decl: break;

	case QC_AST_func_decl: {
		QC_CASTED_NODE(QC_AST_Func_Decl, decl, node);
		qc_destroy_array(QC_AST_Var_Decl_Ptr)(&decl->params);
	} break;

	case QC_AST_literal: {
		QC_CASTED_NODE(QC_AST_Literal, literal, node);
		if (literal->type == QC_Literal_string) {
			qc_destroy_array(char)(&literal->value.string);
		} else if (literal->type == QC_Literal_compound) {
			qc_destroy_array(QC_AST_Node_Ptr)(&literal->value.compound.subnodes);
		}
	} break;
	case QC_AST_biop: break;
	case QC_AST_control: break;

	case QC_AST_call: {
		QC_CASTED_NODE(QC_AST_Call, call, node);
		qc_destroy_array(QC_AST_Node_Ptr)(&call->args);
	} break;

	case QC_AST_access: {
		QC_CASTED_NODE(QC_AST_Access, access, node);
		qc_destroy_array(QC_AST_Node_Ptr)(&access->args);
	} break;

	case QC_AST_cond: break;
	case QC_AST_loop: break;
	case QC_AST_cast: break;
	case QC_AST_typedef: break;
	case QC_AST_parallel: break;

	default: QC_FAIL(("qc_shallow_destroy_node: Unknown node type %i", node->type));
	};

	qc_destroy_array(QC_Token_Ptr)(&node->pre_comments);
	qc_destroy_array(QC_Token_Ptr)(&node->post_comments);
	QC_FREE(node);
}

QC_Bool qc_expr_type(QC_AST_Type *ret, QC_AST_Node *expr)
{
	QC_Bool success = QC_false;
	memset(ret, 0, sizeof(*ret));

	switch (expr->type) {
	case QC_AST_ident: {
		QC_CASTED_NODE(QC_AST_Ident, ident, expr);
		if (!ident->decl)
			break;
		QC_ASSERT(ident->decl->type == QC_AST_var_decl);
		{
			QC_CASTED_NODE(QC_AST_Var_Decl, decl, ident->decl);
			*ret = *decl->type;
			success = QC_true;
		}
	} break;

	case QC_AST_literal: {
		QC_CASTED_NODE(QC_AST_Literal, literal, expr);
		ret->base_type_decl = literal->base_type_decl;
		QC_ASSERT(ret->base_type_decl);
		if (literal->type == QC_Literal_string)
			++ret->ptr_depth;
		success = QC_true;
	} break;

	case QC_AST_access: {
		QC_CASTED_NODE(QC_AST_Access, access, expr);
		if (access->is_var_access) {
			/* Plain variable access */
			QC_ASSERT(access->args.size == 0);
			success = qc_expr_type(ret, access->base);
		} else if (access->is_member_access) {
			QC_ASSERT(access->args.size == 1);
			success = qc_expr_type(ret, access->args.data[0]);
		} else if (access->is_element_access) {
			success = qc_expr_type(ret, access->base);
			/* "Dereference" field -> matrix/scalar, matrix -> scalar */
			ret->base_type_decl = ret->base_type_decl->builtin_sub_type_decl;
		} else if (access->is_array_access) {
			success = qc_expr_type(ret, access->base);
			--ret->ptr_depth;
		} else {
			QC_FAIL(("Unknown access type"));
		}
	} break;

	case QC_AST_biop: {
		QC_CASTED_NODE(QC_AST_Biop, biop, expr);
		/* @todo Operation can yield different types than either of operands (2x1 * 1x2 matrices for example) */
		if (biop->lhs && biop->rhs) {
			success = qc_expr_type(ret, biop->rhs);
		} else if (biop->rhs) {
			/* op value */
			success = qc_expr_type(ret, biop->rhs);
			if (biop->type == QC_Token_mul) { /* Deref */
				--ret->ptr_depth;
			}
		} else if (biop->lhs) {
			/* value op */
			success = qc_expr_type(ret, biop->lhs);
		}
	} break;

	default:;
	}

	return success;
}

QC_AST_Literal *qc_eval_const_expr(QC_AST_Node *expr)
{
	QC_AST_Literal *ret = NULL;
	QC_AST_Literal *lhs = NULL, *rhs = NULL;

	switch (expr->type) {
	case QC_AST_literal: {
		ret = (QC_AST_Literal*)qc_copy_ast(expr);
	} break;

	case QC_AST_biop: {
		QC_CASTED_NODE(QC_AST_Biop, biop, expr);
		/* @todo Operation can yield different types than either of operands (2x1 * 1x2 matrices for example) */
		if (biop->lhs && biop->rhs) {
			/* Binary expr */
			lhs = qc_eval_const_expr(biop->lhs);
			rhs = qc_eval_const_expr(biop->rhs);
			if (!lhs || !rhs || lhs->type != rhs->type)
				break;
			ret = (QC_AST_Literal*)qc_copy_ast(QC_AST_BASE(rhs));

			switch (rhs->type) {
			case QC_Literal_int: {
				int result;
				switch (biop->type) {
				case QC_Token_add: result = lhs->value.integer + rhs->value.integer; break;
				case QC_Token_sub: result = lhs->value.integer - rhs->value.integer; break;
				default: QC_FAIL(("Unhandled biop type"));
				}
				ret->value.integer = result;
			} break;
			case QC_Literal_float: {
				double result;
				switch (biop->type) {
				case QC_Token_add: result = lhs->value.floating + rhs->value.floating; break;
				case QC_Token_sub: result = lhs->value.floating - rhs->value.floating; break;
				default: QC_FAIL(("Unhandled biop type"));
				}
				ret->value.floating = result;
			} break;
			default: QC_FAIL(("Unhandled expr type %i", rhs->type));
			}
		} else if (biop->rhs) {
			/* Unary expr */
			rhs = qc_eval_const_expr(biop->rhs);
			if (!rhs)
				break;
			ret = (QC_AST_Literal*)qc_copy_ast(QC_AST_BASE(rhs));

			switch (rhs->type) {
			case QC_Literal_int: {
				int result;
				switch (biop->type) {
				case QC_Token_add: result = rhs->value.integer; break;
				case QC_Token_sub: result = -rhs->value.integer; break;
				default: QC_FAIL(("Unhandled biop type"));
				}
				ret->value.integer = result;
			} break;
			case QC_Literal_float: {
				double result;
				switch (biop->type) {
				case QC_Token_add: result = rhs->value.floating; break;
				case QC_Token_sub: result = -rhs->value.floating; break;
				default: QC_FAIL(("Unhandled biop type"));
				}
				ret->value.floating = result;
			} break;
			default: QC_FAIL(("Unhandled expr type %i", rhs->type));
			}
		} else {
			QC_FAIL(("Invalid biop"));
		}
	} break;

	default:;
	}

	qc_destroy_node(QC_AST_BASE(lhs));
	qc_destroy_node(QC_AST_BASE(rhs));
	return ret;
}

QC_Bool qc_is_decl(QC_AST_Node *node)
{ return	node->type == QC_AST_type_decl ||
			node->type == QC_AST_var_decl ||
			node->type == QC_AST_func_decl ||
			node->type == QC_AST_typedef; }

QC_AST_Ident *qc_decl_ident(QC_AST_Node *node)
{
	QC_ASSERT(qc_is_decl(node));
	switch (node->type) {
		case QC_AST_type_decl: {
			QC_CASTED_NODE(QC_AST_Type_Decl, decl, node);
			return decl->ident;
		} break;
		case QC_AST_var_decl: {
			QC_CASTED_NODE(QC_AST_Var_Decl, decl, node);
			return decl->ident;
		} break;
		case QC_AST_func_decl: {
			QC_CASTED_NODE(QC_AST_Func_Decl, decl, node);
			return decl->ident;
		} break;
		case QC_AST_typedef: {
			QC_CASTED_NODE(QC_AST_Typedef, def, node);
			return def->ident;
		} break;
		default: QC_FAIL(("qc_decl_ident: invalid node type %i", node->type));
	}
}

QC_AST_Ident *qc_access_ident(QC_AST_Access *access)
{
	if (access->base->type == QC_AST_ident)
		return (QC_AST_Ident*)access->base;

	QC_ASSERT(access->base->type == QC_AST_access);
	return qc_access_ident((QC_AST_Access*)access->base);
}

QC_AST_Ident *qc_unwrap_ident(QC_AST_Node *node)
{
	if (node->type == QC_AST_ident)
		return (QC_AST_Ident*)node;

	if (node->type == QC_AST_access) {
		QC_CASTED_NODE(QC_AST_Access, access, node);
		if (!access->is_var_access)
			return NULL;

		QC_ASSERT(access->base->type == QC_AST_ident);
		return (QC_AST_Ident*)access->base;
	}

	return NULL;
}

const char *qc_node_type_str(QC_AST_Node_Type type)
{
	switch (type) {
	case QC_AST_none: return "none";
	case QC_AST_scope: return "scope";
	case QC_AST_ident: return "ident";
	case QC_AST_type: return "type";
	case QC_AST_type_decl: return "type_decl";
	case QC_AST_var_decl: return "var_decl";
	case QC_AST_func_decl: return "func_decl";
	case QC_AST_literal: return "literal";
	case QC_AST_biop: return "biop";
	case QC_AST_control: return "control";
	case QC_AST_call: return "call";
	case QC_AST_access: return "access";
	case QC_AST_cond: return "cond";
	case QC_AST_loop: return "loop";
	case QC_AST_cast: return "cast";
	case QC_AST_typedef: return "typedef";
	case QC_AST_parallel: return "parallel";
	default: QC_FAIL(("Unhandled node type %i", type));
	}
}


QC_AST_Scope *qc_create_ast()
{
	QC_AST_Scope *root = qc_create_scope_node();
	root->is_root = QC_true;
	return root;
}

void qc_destroy_ast(QC_AST_Scope *node)
{ qc_destroy_node((QC_AST_Node*)node); }

typedef struct Copy_Ctx {
	QC_Hash_Table(QC_AST_Node_Ptr, QC_AST_Node_Ptr) src_to_dst;
} Copy_Ctx;

QC_AST_Node *qc_copy_ast_impl(Copy_Ctx *ctx, QC_AST_Node *node)
{
	/* @todo backend_c qc_copy_excluding_types_and_funcs has almost identical code */
	if (node) {
		int i;
		QC_AST_Node *copy = qc_create_ast_node(node->type);

		/* @todo Do something for the massive number of allocations */
		QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
		QC_Array(QC_AST_Node_Ptr) refnodes = qc_create_array(QC_AST_Node_Ptr)(0);

		qc_set_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(&ctx->src_to_dst, node, copy);

		qc_push_immediate_subnodes(&subnodes, node);
		qc_push_immediate_refnodes(&refnodes, node);

		for (i = 0; i < subnodes.size; ++i) {
			subnodes.data[i] = qc_copy_ast_impl(ctx, subnodes.data[i]);
		}
		for (i = 0; i < refnodes.size; ++i) {
			QC_AST_Node *remapped = qc_get_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(&ctx->src_to_dst, refnodes.data[i]);
			/* If referenced node is outside branch we're copying, don't change it */
			if (!remapped)
				remapped = refnodes.data[i]; 
			refnodes.data[i] = remapped;
		}

		qc_copy_ast_node(	copy, node,
						subnodes.data, subnodes.size,
						refnodes.data, refnodes.size);

		qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
		qc_destroy_array(QC_AST_Node_Ptr)(&refnodes);

		return copy;
	}
	return NULL;
}

QC_AST_Node *qc_copy_ast(QC_AST_Node *node)
{
	Copy_Ctx ctx = {{0}};
	QC_AST_Node *ret;
	/* @todo Size should be something like TOTAL_NODE_COUNT*2 */
	ctx.src_to_dst = qc_create_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(NULL, NULL, 1024);
	ret = qc_copy_ast_impl(&ctx, node);
	qc_destroy_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(&ctx.src_to_dst);
	return ret;
}

QC_AST_Node *qc_shallow_copy_ast(QC_AST_Node *node)
{
	QC_AST_Node *copy = qc_create_ast_node(node->type);
	qc_shallow_copy_ast_node(copy, node);
	return copy;
}

void qc_move_ast(QC_AST_Scope *dst, QC_AST_Scope *src)
{
	/* Substitute subnodes in dst with subnodes of src, and destroy src */
	int i;
	for (i = 0; i < dst->nodes.size; ++i)
		qc_destroy_node(dst->nodes.data[i]);

	qc_clear_array(QC_AST_Node_Ptr)(&dst->nodes);
	qc_insert_array(QC_AST_Node_Ptr)(&dst->nodes, 0, src->nodes.data, src->nodes.size);

	/* Don't destroy subnodes because they were moved */
	qc_shallow_destroy_node(QC_AST_BASE(src));
}

QC_INTERNAL void print_indent(int indent)
{ printf("%*s", indent, ""); }

QC_INTERNAL void populate_parent_map(QC_AST_Parent_Map *map, QC_AST_Node *root)
{
	int i;
	QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
	qc_push_immediate_subnodes(&subnodes, root);

	for (i = 0; i < subnodes.size; ++i) {
		if (!subnodes.data[i])
			continue;

		/* @todo Add builtin decls to 'builtin_decls' */
		qc_set_parent_node(map, subnodes.data[i], root);
		populate_parent_map(map, subnodes.data[i]);
	}

	qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
}

QC_AST_Parent_Map qc_create_parent_map(QC_AST_Node *root)
{
	QC_AST_Parent_Map map;
	memset(&map, 0, sizeof(map));
	map.table = qc_create_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(NULL, NULL, 2048);
	map.builtin_decls = qc_create_array(QC_AST_Node_Ptr)(32);

	populate_parent_map(&map, root);

	return map;
}

void qc_destroy_parent_map(QC_AST_Parent_Map *map)
{
	qc_destroy_array(QC_AST_Node_Ptr)(&map->builtin_decls);
	qc_destroy_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(&map->table);
}


/* Part of 'qc_find_decls_scoped' */
QC_INTERNAL void match_and_add(QC_Array(QC_AST_Node_Ptr) *ret, QC_Buf_Str name, QC_AST_Node *decl)
{
	QC_AST_Ident *ident = qc_decl_ident(decl);
	if (!ident || !qc_buf_str_equals(qc_c_str_to_buf_str(ident->text.data), name))
		return;

	/* Found declaration for the name */
	qc_push_array(QC_AST_Node_Ptr)(ret, decl);
}

/* @todo Not very elegant with 'hint' */
void qc_find_decls_scoped(QC_AST_Parent_Map *map, QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node *node, QC_Buf_Str name, QC_AST_Type *hint)
{
	int i;
	QC_AST_Node *stack_node = node;
	while ((stack_node = qc_find_parent_node(map, stack_node))) {
		switch (stack_node->type) {
		case QC_AST_scope: {
			QC_CASTED_NODE(QC_AST_Scope, scope, stack_node);
			for (i = 0; i < scope->nodes.size; ++i) {
				QC_AST_Node *node = scope->nodes.data[i];
				if (!qc_is_decl(node))
					continue;

				match_and_add(ret, name, node);
			}
		} break;
		case QC_AST_loop: {
			QC_CASTED_NODE(QC_AST_Loop, loop, stack_node);
			if (!loop->init || !qc_is_decl(loop->init))
				continue;

			match_and_add(ret, name, loop->init);
		} break;
		case QC_AST_func_decl: {
			QC_CASTED_NODE(QC_AST_Func_Decl, decl, stack_node);
			for (i = 0; i < decl->params.size; ++i) {
				QC_AST_Node *node = QC_AST_BASE(decl->params.data[i]);
				if (!qc_is_decl(node))
					continue;

				match_and_add(ret, name, node);
			}
		} break;
		default:;
		}
	}

	/* Look from builtin funcs. */
	/* This doesn't find types, only functions, because e.g. decl
	 * 'float matrix(2,2)' doesn't have an identifier */
	for (i = 0; i < map->builtin_decls.size; ++i) {
		QC_AST_Node *node = map->builtin_decls.data[i];
		if (node->type != QC_AST_func_decl)
			continue;

		{
			QC_CASTED_NODE(QC_AST_Func_Decl, decl, node);
			if (!qc_buf_str_equals(qc_c_str_to_buf_str(decl->ident->text.data), name))
				continue;

			if (hint && !type_node_equals(*hint, *decl->return_type))
				continue;

			qc_push_array(QC_AST_Node_Ptr)(ret, node);
		}
	}
}

QC_AST_Node *qc_find_parent_node(QC_AST_Parent_Map *map, QC_AST_Node *node)
{
	QC_AST_Node *parent = qc_get_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(&map->table, node);
	QC_ASSERT(!parent || parent != node);
	return parent;
}

void qc_set_parent_node(QC_AST_Parent_Map *map, QC_AST_Node *sub, QC_AST_Node *parent)
{
	QC_ASSERT(sub != parent);
	QC_ASSERT(qc_find_parent_node(map, parent) != sub && "QC_AST turning cyclic");
	qc_set_tbl(QC_AST_Node_Ptr, QC_AST_Node_Ptr)(&map->table, sub, parent);
}

int qc_find_in_scope(QC_AST_Scope *scope, QC_AST_Node *needle)
{
	int i;
	for (i = 0; i < scope->nodes.size; ++i) {
		if (scope->nodes.data[i] == needle)
			return i;
	}
	return -1;
}

QC_AST_Func_Decl *qc_find_enclosing_func(QC_AST_Parent_Map *map, QC_AST_Node *node)
{
	QC_AST_Node *parent = qc_find_parent_node(map, node);
	while (parent && parent->type != QC_AST_func_decl)
		parent = qc_find_parent_node(map, parent);
	return (QC_AST_Func_Decl*)parent;
}

QC_Bool qc_is_subnode(QC_AST_Parent_Map *map, QC_AST_Node *parent, QC_AST_Node *sub)
{
	QC_AST_Node *p = qc_find_parent_node(map, sub);
	while (p) {
		if (p == parent)
			return QC_true;
		p = qc_find_parent_node(map, p);
	}
	return QC_false;
}

/* @todo Use in resolve_node and not in ast parsing directly */
QC_INTERNAL QC_Bool qc_resolve_ident_in_scope(QC_AST_Ident *ident, QC_AST_Scope *search_scope)
{
	/* Search from given scope */
	int i;
	for (i = 0; i < search_scope->nodes.size; ++i) {
		QC_AST_Node *subnode = search_scope->nodes.data[i];
		if (!qc_is_decl(subnode))
			continue;
		if (strcmp(qc_decl_ident(subnode)->text.data, ident->text.data))
			continue;

		ident->decl = subnode;
		return QC_true;
	}

	return QC_false;
}

/* @todo Split to multiple functions */
/* Use 'arg_count = -1' for non-function identifier resolution */
QC_INTERNAL QC_Bool resolve_node(QC_AST_Parent_Map *map, QC_AST_Ident *ident, QC_AST_Type *hint, QC_AST_Type *arg_types, int arg_count)
{
	QC_Array(QC_AST_Node_Ptr) decls = qc_create_array(QC_AST_Node_Ptr)(0);
	int i, k;

	ident->decl = NULL;

	{ /* Designated initializer ident search from enclosing compound literal type */
		QC_AST_Node *parent = qc_find_parent_node(map, QC_AST_BASE(ident));
		if (parent->type == QC_AST_literal) {
			QC_CASTED_NODE(QC_AST_Literal, literal, parent);
			if (	literal->type == QC_Literal_compound &&
					literal->value.compound.type) {
				qc_resolve_ident_in_scope(ident, literal->value.compound.type->base_type_decl->body);
			}
		}
	}

	/* Search from names visible from identifier position */
	if (!ident->decl) {
		QC_AST_Node *best_match = NULL;

		qc_find_decls_scoped(map, &decls, QC_AST_BASE(ident), qc_c_str_to_buf_str(ident->text.data), hint);
		for (i = 0; i < decls.size; ++i) {
			QC_AST_Node *decl = decls.data[i];
			if (!best_match) {
				best_match = decl;
				continue;
			}

			/* Match decl type with 'hint' */
			if (hint && decl->type == QC_AST_var_decl && arg_count == -1) {
				QC_CASTED_NODE(QC_AST_Var_Decl, var_decl, decl);
				if (!type_node_equals(*hint, *var_decl->type))
					continue;

				best_match = decl;
				break;
			} else if (decl->type == QC_AST_func_decl) {
				QC_CASTED_NODE(QC_AST_Func_Decl, func_decl, decl);
				QC_Bool arg_types_matched = QC_true;

				if (hint && !type_node_equals(*hint, *func_decl->return_type))
					continue;

				if (func_decl->params.size != arg_count)
					continue;

				/* Match argument types */
				for (k = 0; k < arg_count; ++k) {
					if (!type_node_equals(*func_decl->params.data[k]->type, arg_types[k])) {
						arg_types_matched = QC_false;
						break;
					}
				}
				if (!arg_types_matched)
					continue;

				best_match = decl;
				break;
			} else {
			}
		}

		ident->decl = best_match;
	}

	qc_destroy_array(QC_AST_Node_Ptr)(&decls);
	return ident->decl != NULL;
}

QC_AST_Ident *qc_resolve_ident(QC_AST_Parent_Map *map, QC_AST_Ident *ident)
{
	if (resolve_node(map, ident, NULL, NULL, -1))
		return ident;
	else
		return NULL;
}

QC_DECLARE_ARRAY(QC_AST_Type)
QC_DEFINE_ARRAY(QC_AST_Type)

QC_AST_Ident *qc_call_ident(QC_AST_Call *call)
{
	QC_ASSERT(call->base->type == QC_AST_access);

	return qc_unwrap_ident(call->base);
}

/* Resolves overloaded calls like in `x = foo(5)`. Doesn't resolve `foo_arr[0](5)` */
QC_AST_Call *qc_resolve_overloaded_call(QC_AST_Parent_Map *map, QC_AST_Call *call, QC_AST_Type *return_type_hint)
{
	int i;
	QC_Bool success = QC_true;
	QC_Array(QC_AST_Type) types = qc_create_array(QC_AST_Type)(call->args.size);
	QC_AST_Ident *ident;
	for (i = 0; i < call->args.size; ++i) {
		QC_AST_Type type;
		if (!qc_expr_type(&type, call->args.data[i])) {
			success = QC_false;
			break;
		}
		qc_push_array(QC_AST_Type)(&types, type);
	}

	ident = qc_call_ident(call);
	if (ident) {
		/* Return value overloading supported for plain function calls */
		QC_CASTED_NODE(QC_AST_Access, access, call->base);
		QC_AST_Node *ident = access->base;
		QC_ASSERT(ident->type == QC_AST_ident);
		if (!resolve_node(map, (QC_AST_Ident*)ident, return_type_hint, types.data, types.size))
			success = QC_false;
	} else {
		success = QC_false;
	}

	qc_destroy_array(QC_AST_Type)(&types);

	if (success)
		return call;
	else
		return NULL;
}

void qc_resolve_ast(QC_AST_Scope *root)
{
	int i;
	QC_AST_Parent_Map parent_map = qc_create_parent_map(QC_AST_BASE(root));
	QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
	qc_push_subnodes(&subnodes, QC_AST_BASE(root), QC_false);

	for (i = 0; i < subnodes.size; ++i) {
		QC_AST_Node *node = subnodes.data[i];
		if (node->type != QC_AST_ident)
			continue;
		/* @todo Other nodes might also need resolving (literals at least) */
		{
			QC_CASTED_NODE(QC_AST_Ident, ident, node);
			if (ident->decl)
				continue; /* Already resolved */
			{
				QC_AST_Node *parent = qc_find_parent_node(&parent_map, node);
				QC_ASSERT(parent);
				switch (parent->type) {
				case QC_AST_call: {
					QC_CASTED_NODE(QC_AST_Call, call, parent);
					/* @todo Return type hint (bake into parent map?) */
					qc_resolve_overloaded_call(&parent_map, call, NULL);
				} break;
				default:;
					qc_resolve_ident(&parent_map, ident);
				}
			}
		}
	}

	qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
	qc_destroy_parent_map(&parent_map);
}

void qc_unresolve_ast(QC_AST_Node *root)
{
	int i;
	QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
	qc_push_subnodes(&subnodes, root, QC_false);

	for (i = 0; i < subnodes.size; ++i) {
		QC_AST_Node *node = subnodes.data[i];
		if (!node)
			continue;
		switch (node->type) {
			case QC_AST_ident: {
				QC_CASTED_NODE(QC_AST_Ident, ident, node);
				ident->decl = NULL;
			} break;
			/* @todo Unresolve everything, literal base types etc. */
			default:;
		}
	}

	qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
}

void qc_push_immediate_subnodes(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node *node)
{
	int i;
	if (!node)
		return;

	switch (node->type) {
	case QC_AST_scope: {
		QC_CASTED_NODE(QC_AST_Scope, scope, node);
		for (i = 0; i < scope->nodes.size; ++i)
			qc_push_array(QC_AST_Node_Ptr)(ret, scope->nodes.data[i]);
	} break;

	case QC_AST_ident: {
	} break;

	case QC_AST_type: {
	} break;

	case QC_AST_type_decl: {
		QC_CASTED_NODE(QC_AST_Type_Decl, decl, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->ident));
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->body));
	} break;

	case QC_AST_var_decl: {
		QC_CASTED_NODE(QC_AST_Var_Decl, decl, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->type));
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->ident));
		qc_push_array(QC_AST_Node_Ptr)(ret, decl->value);
	} break;

	case QC_AST_func_decl: {
		QC_CASTED_NODE(QC_AST_Func_Decl, decl, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->return_type));
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->ident));
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->body));
		for (i = 0; i < decl->params.size; ++i)
			qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->params.data[i]));
	} break;

	case QC_AST_literal: {
		QC_CASTED_NODE(QC_AST_Literal, literal, node);
		if (literal->type == QC_Literal_compound) {
			qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(literal->value.compound.type));
			for (i = 0; i < literal->value.compound.subnodes.size; ++i)
				qc_push_array(QC_AST_Node_Ptr)(ret, literal->value.compound.subnodes.data[i]);
		}
	} break;

	case QC_AST_biop: {
		QC_CASTED_NODE(QC_AST_Biop, biop, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, biop->lhs);
		qc_push_array(QC_AST_Node_Ptr)(ret, biop->rhs);
	} break;

	case QC_AST_control: {
		QC_CASTED_NODE(QC_AST_Control, control, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, control->value);
	} break;

	case QC_AST_call: {
		QC_CASTED_NODE(QC_AST_Call, call, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, call->base);
		for (i = 0; i < call->args.size; ++i)
			qc_push_array(QC_AST_Node_Ptr)(ret, call->args.data[i]);
	} break;

	case QC_AST_access: {
		QC_CASTED_NODE(QC_AST_Access, access, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, access->base);
		qc_insert_array(QC_AST_Node_Ptr)(ret, ret->size, access->args.data, access->args.size);
	} break;

	case QC_AST_cond: {
		QC_CASTED_NODE(QC_AST_Cond, cond, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, cond->expr);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(cond->body));
		qc_push_array(QC_AST_Node_Ptr)(ret, cond->after_else);
	} break;

	case QC_AST_loop: {
		QC_CASTED_NODE(QC_AST_Loop, loop, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, loop->init);
		qc_push_array(QC_AST_Node_Ptr)(ret, loop->cond);
		qc_push_array(QC_AST_Node_Ptr)(ret, loop->incr);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(loop->body));
	} break;

	case QC_AST_cast: {
		QC_CASTED_NODE(QC_AST_Cast, cast, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(cast->type));
		qc_push_array(QC_AST_Node_Ptr)(ret, cast->target);
	} break;

	case QC_AST_typedef: {
		QC_CASTED_NODE(QC_AST_Typedef, def, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(def->type));
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(def->ident));
	} break;

	case QC_AST_parallel: {
		QC_CASTED_NODE(QC_AST_Parallel, parallel, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, parallel->output);
		qc_push_array(QC_AST_Node_Ptr)(ret, parallel->input);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(parallel->body));
	} break;

	default: QC_FAIL(("qc_push_immediate_subnodes: Unknown node type: %i", node->type));
	}
}

void qc_push_immediate_refnodes(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node *node)
{
	if (!node)
		return;

	switch (node->type) {
	case QC_AST_scope: break;

	case QC_AST_ident: {
		QC_CASTED_NODE(QC_AST_Ident, ident, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, ident->decl);
	} break;

	case QC_AST_type: {
		QC_CASTED_NODE(QC_AST_Type, type, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(type->base_type_decl));
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(type->base_typedef));
	} break;

	case QC_AST_type_decl: {
		QC_CASTED_NODE(QC_AST_Type_Decl, decl, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->builtin_sub_type_decl));
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->builtin_concrete_decl));
	} break;

	case QC_AST_var_decl: break;

	case QC_AST_func_decl: {
		QC_CASTED_NODE(QC_AST_Func_Decl, decl, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(decl->builtin_concrete_decl));
	} break;

	case QC_AST_literal: {
		QC_CASTED_NODE(QC_AST_Literal, literal, node);
		qc_push_array(QC_AST_Node_Ptr)(ret, QC_AST_BASE(literal->base_type_decl));
	} break;

	case QC_AST_biop: break;
	case QC_AST_control: break;
	case QC_AST_call: break;
	case QC_AST_access: break;
	case QC_AST_cond: break;
	case QC_AST_loop: break;
	case QC_AST_cast: break;
	case QC_AST_typedef: break;
	case QC_AST_parallel: break;

	default: QC_FAIL(("qc_push_immediate_refnodes: Unknown node type: %i", node->type));
	}
}

void qc_push_subnodes(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node *node, QC_Bool qc_push_before_recursing)
{
	int i;
	QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
	qc_push_immediate_subnodes(&subnodes, node);

	for (i = 0; i < subnodes.size; ++i) {
		if (!subnodes.data[i])
			continue;

		if (qc_push_before_recursing)
			qc_push_array(QC_AST_Node_Ptr)(ret, subnodes.data[i]);

		qc_push_subnodes(ret, subnodes.data[i], qc_push_before_recursing);

		if (!qc_push_before_recursing)
			qc_push_array(QC_AST_Node_Ptr)(ret, subnodes.data[i]);
	}

	qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
}

QC_AST_Node *qc_replace_nodes_in_ast(QC_AST_Node *node, QC_AST_Node **old_nodes, QC_AST_Node **new_nodes, int node_count)
{
	int i, k;

	if (!node || node_count == 0)
		return node;

	/* @todo Use QC_Hash_Table to eliminate O(n^2) */
	/* Replacing happens before recursing, so that old_nodes contained in new_nodes are also replaced */
	for (i = 0; i < node_count; ++i) {
		if (node == old_nodes[i]) {
			node = new_nodes[i];
			break;
		}
	}

	{
		/* @todo Do something for the massive number of allocations */
		QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
		QC_Array(QC_AST_Node_Ptr) refnodes = qc_create_array(QC_AST_Node_Ptr)(0);

		qc_push_immediate_subnodes(&subnodes, node);
		qc_push_immediate_refnodes(&refnodes, node);

		/* Replace subnodes */
		for (i = 0; i < subnodes.size; ++i) {
			subnodes.data[i] = qc_replace_nodes_in_ast(subnodes.data[i], old_nodes, new_nodes, node_count);
		}
		/* Replace referenced nodes */
		for (i = 0; i < refnodes.size; ++i) {
			for (k = 0; k < node_count; ++k) {
				if (refnodes.data[i] == old_nodes[k]) {
					refnodes.data[i] = new_nodes[k];
					break;
				}
			}
		}

		/* Update replaced pointers to node */
		qc_copy_ast_node(	node, node,
						subnodes.data, subnodes.size,
						refnodes.data, refnodes.size);

		qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
		qc_destroy_array(QC_AST_Node_Ptr)(&refnodes);
	}

	return node;
}

void qc_find_subnodes_of_type(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node_Type type, QC_AST_Node *node)
{
	int i;
	QC_Array(QC_AST_Node_Ptr) subnodes = qc_create_array(QC_AST_Node_Ptr)(0);
	qc_push_subnodes(&subnodes, node, QC_false);

	for (i = 0; i < subnodes.size; ++i) {
		if (subnodes.data[i]->type == type)
			qc_push_array(QC_AST_Node_Ptr)(ret, subnodes.data[i]);
	}

	qc_destroy_array(QC_AST_Node_Ptr)(&subnodes);
}


void qc_print_ast(QC_AST_Node *node, int indent)
{
	int i;
	if (!node)
		return;

	print_indent(indent);

	switch (node->type) {
	case QC_AST_scope: {
		QC_CASTED_NODE(QC_AST_Scope, scope, node);
		printf("scope %i\n", scope->nodes.size);
		for (i = 0; i < scope->nodes.size; ++i)
			qc_print_ast(scope->nodes.data[i], indent + 2);
	} break;

	case QC_AST_ident: {
		QC_CASTED_NODE(QC_AST_Ident, ident, node);
		printf("ident: %s\n", ident->text.data);
	} break;

	case QC_AST_type: {
		QC_CASTED_NODE(QC_AST_Type, type, node);
		if (type->base_type_decl->is_builtin) {
			printf("builtin_type\n");
		} else {
			printf("type %s %i\n", type->base_type_decl->ident->text.data, type->ptr_depth);
		}
	} break;

	case QC_AST_type_decl: {
		QC_CASTED_NODE(QC_AST_Type_Decl, decl, node);
		if (decl->is_builtin) {
			QC_Builtin_Type bt = decl->builtin_type;
			printf("builtin_type_decl: is_field: %i dim: %i  is_matrix: %i rank: %i  is_int: %i  is_float: %i\n",
					bt.is_field, bt.field_dim,
					bt.is_matrix, bt.matrix_rank,
					bt.is_integer, bt.is_float);
		} else {
			printf("type_decl\n");
		}
		qc_print_ast(QC_AST_BASE(decl->ident), indent + 2);
		qc_print_ast(QC_AST_BASE(decl->body), indent + 2);
	} break;

	case QC_AST_var_decl: {
		QC_CASTED_NODE(QC_AST_Var_Decl, decl, node);
		printf("var_decl\n");
		qc_print_ast(QC_AST_BASE(decl->type), indent + 2);
		qc_print_ast(QC_AST_BASE(decl->ident), indent + 2);
		qc_print_ast(decl->value, indent + 2);
	} break;

	case QC_AST_func_decl: {
		QC_CASTED_NODE(QC_AST_Func_Decl, decl, node);
		printf("func_decl\n");
		qc_print_ast(QC_AST_BASE(decl->return_type), indent + 2);
		qc_print_ast(QC_AST_BASE(decl->ident), indent + 2);
		for (i = 0; i < decl->params.size; ++i)
			qc_print_ast(QC_AST_BASE(decl->params.data[i]), indent + 2);
		qc_print_ast(QC_AST_BASE(decl->body), indent + 2);
	} break;

	case QC_AST_literal: {
		QC_CASTED_NODE(QC_AST_Literal, literal, node);
		printf("literal: ");
		switch (literal->type) {
			case QC_Literal_int: printf("int: %i\n", literal->value.integer); break;
			case QC_Literal_float: printf("float: %f\n", literal->value.floating); break;
			case QC_Literal_string: printf("str: %s\n", literal->value.string.data); break;
			case QC_Literal_null: printf("NULL\n"); break;
			case QC_Literal_compound: printf("COMPOUND\n"); break;
			default: QC_FAIL(("Unknown literal type: %i", literal->type));
		}
	} break;

	case QC_AST_biop: {
		QC_CASTED_NODE(QC_AST_Biop, op, node);
		if (op->lhs && op->rhs) {
			printf("biop %s\n", qc_tokentype_str(op->type));
			qc_print_ast(op->lhs, indent + 2);
			qc_print_ast(op->rhs, indent + 2);
		} else {
			printf("uop %s\n", qc_tokentype_str(op->type));
			qc_print_ast(op->rhs, indent + 2);
		}
	} break;

	case QC_AST_control: {
		QC_CASTED_NODE(QC_AST_Control, control, node);
		printf("control %s\n", qc_tokentype_str(control->type));
		qc_print_ast(control->value, indent + 2);
	} break;

	case QC_AST_call: {
		QC_CASTED_NODE(QC_AST_Call, call, node);
		printf("call\n");
		qc_print_ast(call->base, indent + 2);
		for (i = 0; i < call->args.size; ++i)
			qc_print_ast(call->args.data[i], indent + 2);
	} break;

	case QC_AST_access: {
		QC_CASTED_NODE(QC_AST_Access, access, node);
		printf("access ");
		if (access->is_member_access)
			printf("member");
		if (access->is_element_access)
			printf("element");
		if (access->is_array_access)
			printf("array");
		printf("\n");
		qc_print_ast(access->base, indent + 2);
		for (i = 0; i < access->args.size; ++i)
			qc_print_ast(access->args.data[i], indent + 2);
	} break;

	case QC_AST_cond: {
		QC_CASTED_NODE(QC_AST_Cond, cond, node);
		printf("cond\n");
		qc_print_ast(cond->expr, indent + 2);
		qc_print_ast(QC_AST_BASE(cond->body), indent + 2);
		qc_print_ast(cond->after_else, indent + 2);
	} break;

	case QC_AST_loop: {
		QC_CASTED_NODE(QC_AST_Loop, loop, node);
		printf("loop\n");
		qc_print_ast(loop->init, indent + 2);
		qc_print_ast(loop->cond, indent + 2);
		qc_print_ast(loop->incr, indent + 2);
		qc_print_ast(QC_AST_BASE(loop->body), indent + 2);
	} break;

	case QC_AST_cast: {
		QC_CASTED_NODE(QC_AST_Cast, cast, node);
		printf("cast\n");
		qc_print_ast(QC_AST_BASE(cast->type), indent + 2);
		qc_print_ast(cast->target, indent + 2);
	} break;

	case QC_AST_typedef: {
		QC_CASTED_NODE(QC_AST_Typedef, def, node);
		printf("typedef\n");
		qc_print_ast(QC_AST_BASE(def->type), indent + 2);
		qc_print_ast(QC_AST_BASE(def->ident), indent + 2);
	} break;

	case QC_AST_parallel: {
		QC_CASTED_NODE(QC_AST_Parallel, parallel, node);
		printf("parallel\n");
		qc_print_ast(parallel->input, indent + 2);
		qc_print_ast(parallel->output, indent + 2);
		qc_print_ast(QC_AST_BASE(parallel->body), indent + 2);
	} break;

	default: QC_FAIL(("qc_print_ast: Unknown node type %i", node->type));
	};
}

QC_AST_Ident *qc_create_ident_with_text(QC_AST_Node *decl, const char *fmt, ...)
{
	QC_AST_Ident *ident = qc_create_ident_node();
	va_list args;
	va_start(args, fmt);
	qc_safe_vsprintf(&ident->text, fmt, args);
	va_end(args);

	ident->decl = decl;
	return ident;
}

QC_AST_Var_Decl *qc_create_simple_var_decl(QC_AST_Type_Decl *type_decl, const char *ident)
{ return qc_create_var_decl(type_decl, qc_create_ident_with_text(NULL, ident), NULL); }

QC_AST_Var_Decl *qc_create_var_decl(QC_AST_Type_Decl *type_decl, QC_AST_Ident *ident, QC_AST_Node *value)
{
	QC_AST_Var_Decl *decl = qc_create_var_decl_node();
	decl->type = qc_create_type_node();
	decl->type->base_type_decl = type_decl;
	decl->ident = ident;
	decl->ident->decl = QC_AST_BASE(decl);
	decl->value = value;
	return decl;
}

QC_AST_Type_Decl *qc_find_builtin_type_decl(QC_Builtin_Type bt, QC_AST_Scope *root)
{
	int i;
	for (i = 0; i < root->nodes.size; ++i) {
		QC_AST_Node *node = root->nodes.data[i];
		if (node->type != QC_AST_type_decl)
			continue;
		{
			QC_CASTED_NODE(QC_AST_Type_Decl, type_decl, node);
			if (!type_decl->is_builtin)
				continue;

			if (builtin_type_equals(type_decl->builtin_type, bt))
				return type_decl;
		}
	}
	QC_FAIL(("qc_find_builtin_type_decl: Builtin type not found"));
	return NULL;
}

QC_AST_Literal *qc_create_integer_literal(int value, QC_AST_Scope *root)
{
	QC_AST_Literal *literal = qc_create_literal_node();
	literal->type = QC_Literal_int;
	literal->value.integer = value;
	if (root) /* @todo Don't accept NULL */
		literal->base_type_decl = qc_find_builtin_type_decl(qc_int_builtin_type(), root);
	return literal;
}

QC_AST_Literal *qc_create_floating_literal(double value, QC_AST_Scope *root)
{
	QC_AST_Literal *literal = qc_create_literal_node();
	literal->type = QC_Literal_float;
	literal->value.floating = value;
	if (root) /* @todo Don't accept NULL */
		literal->base_type_decl = qc_find_builtin_type_decl(qc_float_builtin_type(), root);
	return literal;
}

QC_AST_Call *qc_create_call_1(QC_AST_Ident *ident, QC_AST_Node *arg)
{
	QC_AST_Call *call = qc_create_call_node();
	call->base = qc_try_create_access(QC_AST_BASE(ident));
	qc_push_array(QC_AST_Node_Ptr)(&call->args, arg);
	return call;
}

QC_AST_Call *qc_create_call_2(QC_AST_Ident *ident, QC_AST_Node *arg1, QC_AST_Node *arg2)
{
	QC_AST_Call *call = qc_create_call_1(ident, arg1);
	qc_push_array(QC_AST_Node_Ptr)(&call->args, arg2);
	return call;
}

QC_AST_Call *qc_create_call_3(QC_AST_Ident *ident, QC_AST_Node *arg1, QC_AST_Node *arg2, QC_AST_Node *arg3)
{
	QC_AST_Call *call = qc_create_call_2(ident, arg1, arg2);
	qc_push_array(QC_AST_Node_Ptr)(&call->args, arg3);
	return call;
}

QC_AST_Call *qc_create_call_4(QC_AST_Ident *ident, QC_AST_Node *arg1, QC_AST_Node *arg2, QC_AST_Node *arg3, QC_AST_Node *arg4)
{
	QC_AST_Call *call = qc_create_call_3(ident, arg1, arg2, arg3);
	qc_push_array(QC_AST_Node_Ptr)(&call->args, arg4);
	return call;
}

QC_AST_Control *qc_create_return(QC_AST_Node *expr)
{
	QC_AST_Control *ret = qc_create_control_node();
	ret->type = QC_Token_kw_return;
	ret->value = expr;
	return ret;
}

QC_AST_Biop *qc_create_sizeof(QC_AST_Node *expr)
{
	QC_AST_Biop *op = qc_create_biop_node();
	op->type = QC_Token_kw_sizeof;
	op->rhs = expr;
	return op;
}

QC_AST_Biop *qc_create_deref(QC_AST_Node *expr)
{
	QC_AST_Biop *op = qc_create_biop_node();
	op->type = QC_Token_mul;
	op->rhs = expr;
	return op;
}

QC_AST_Biop *qc_create_addrof(QC_AST_Node *expr)
{
	QC_AST_Biop *op = qc_create_biop_node();
	op->type = QC_Token_addrof;
	op->rhs = expr;
	return op;
}

QC_AST_Biop *qc_create_biop(QC_Token_Type type, QC_AST_Node *lhs, QC_AST_Node *rhs)
{
	QC_AST_Biop *op = qc_create_biop_node();
	op->type = type;
	op->lhs = lhs;
	op->rhs = rhs;
	return op;
}

QC_AST_Biop *qc_create_assign(QC_AST_Node *lhs, QC_AST_Node *rhs)
{ return qc_create_biop(QC_Token_assign, lhs, rhs); }

QC_AST_Biop *qc_create_mul(QC_AST_Node *lhs, QC_AST_Node *rhs)
{ return qc_create_biop(QC_Token_mul, lhs, rhs); }

QC_AST_Biop *qc_create_less_than(QC_AST_Node *lhs, QC_AST_Node *rhs)
{
	QC_ASSERT(lhs && rhs);
	return qc_create_biop(QC_Token_less, lhs, rhs);
}

QC_AST_Biop *qc_create_equals(QC_AST_Node *lhs, QC_AST_Node *rhs)
{ return qc_create_biop(QC_Token_equals, lhs, rhs); }

QC_AST_Biop *qc_create_and(QC_AST_Node *lhs, QC_AST_Node *rhs)
{ return qc_create_biop(QC_Token_and, lhs, rhs); }

QC_AST_Biop *qc_create_pre_increment(QC_AST_Node *expr)
{ return qc_create_biop(QC_Token_incr, NULL, expr); }

QC_AST_Cast *qc_create_cast(QC_AST_Type *type, QC_AST_Node *target)
{
	QC_AST_Cast *cast = qc_create_cast_node();
	cast->type = type;
	cast->target = target;
	return cast;
}

QC_AST_Type *qc_create_builtin_type(QC_Builtin_Type bt, int ptr_depth, QC_AST_Scope *root)
{
	QC_AST_Type *type = qc_create_type_node();
	type->base_type_decl = qc_find_builtin_type_decl(bt, root);
	type->ptr_depth = ptr_depth;
	QC_ASSERT(type->base_type_decl);
	return type;
}

QC_AST_Type *qc_copy_and_modify_type(QC_AST_Type *type, int delta_ptr_depth)
{
	QC_AST_Type *copy = (QC_AST_Type*)qc_copy_ast(QC_AST_BASE(type));
	copy->ptr_depth += delta_ptr_depth;
	return copy;
}

QC_AST_Type *qc_create_simple_type(QC_AST_Type_Decl *type_decl)
{
	QC_AST_Type *type = qc_create_type_node();
	type->base_type_decl = type_decl;
	return type;
}

QC_AST_Loop *qc_create_for_loop(QC_AST_Var_Decl *index, QC_AST_Node *max_expr, QC_AST_Scope *body)
{
	QC_AST_Loop *loop = qc_create_loop_node();
	loop->init = QC_AST_BASE(index);
	loop->cond = QC_AST_BASE(qc_create_less_than(qc_copy_ast(QC_AST_BASE(index->ident)), max_expr));
	loop->incr = QC_AST_BASE(qc_create_pre_increment(qc_copy_ast(QC_AST_BASE(index->ident))));
	loop->body = body;
	return loop;
}

QC_AST_Node *qc_try_create_access(QC_AST_Node *node)
{
	if (node->type == QC_AST_ident) {
		QC_AST_Access *access = qc_create_access_node();
		access->base = node;
		access->is_var_access = QC_true;
		return QC_AST_BASE(access);
	}
	return node;
}

QC_AST_Access *qc_create_element_access_1(QC_AST_Node *base, QC_AST_Node *arg)
{
	QC_AST_Access *access = qc_create_access_node();
	access->base = base;
	qc_push_array(QC_AST_Node_Ptr)(&access->args, arg);
	access->is_element_access = QC_true;
	return access;
}

QC_AST_Access *qc_create_simple_access(QC_AST_Var_Decl *var)
{
	QC_AST_Access *access = qc_create_access_node();
	access->base = qc_shallow_copy_ast(QC_AST_BASE(var->ident));
	access->is_var_access = QC_true;
	return access;
}

QC_AST_Access *qc_create_simple_member_access(QC_AST_Var_Decl *base, QC_AST_Var_Decl *member)
{
	QC_AST_Access *access = qc_create_access_node();
	QC_AST_Access *base_access = qc_create_access_node();
	QC_AST_Ident *member_ident = (QC_AST_Ident*)qc_shallow_copy_ast(QC_AST_BASE(member->ident));

	base_access->base = qc_shallow_copy_ast(QC_AST_BASE(base->ident));
	base_access->is_var_access = QC_true;

	access->base = QC_AST_BASE(base_access);
	qc_push_array(QC_AST_Node_Ptr)(&access->args, QC_AST_BASE(member_ident));
	access->is_member_access = QC_true;

	return access;
}

QC_AST_Scope *qc_create_scope_1(QC_AST_Node *expr)
{
	QC_AST_Scope *scope = qc_create_scope_node();
	qc_push_array(QC_AST_Node_Ptr)(&scope->nodes, expr);
	return scope;
}

QC_AST_Cond *qc_create_if_1(QC_AST_Node *expr, QC_AST_Node *body_expr_1)
{
	QC_AST_Cond *cond = qc_create_cond_node();
	cond->expr = expr;
	cond->body = qc_create_scope_node();
	qc_push_array(QC_AST_Node_Ptr)(&cond->body->nodes, body_expr_1);
	return cond;
}

QC_AST_Node *qc_create_full_deref(QC_AST_Node *expr)
{
	QC_AST_Type type;
	if (!qc_expr_type(&type, expr))
		QC_FAIL(("qc_create_full_deref: qc_expr_type failed"));

	while (type.ptr_depth-- > 0)
		expr = QC_AST_BASE(qc_create_deref(expr));

	return expr;
}

QC_Builtin_Type qc_void_builtin_type()
{
	QC_Builtin_Type bt = {0};
	bt.is_void = QC_true;
	return bt;
}

QC_Builtin_Type qc_int_builtin_type()
{
	QC_Builtin_Type bt = {0};
	bt.is_integer = QC_true;
	return bt;
}

QC_Builtin_Type qc_float_builtin_type()
{
	QC_Builtin_Type bt = {0};
	bt.is_float = QC_true;
	bt.bitness = 32;
	return bt;
}

QC_Builtin_Type qc_char_builtin_type()
{
	QC_Builtin_Type bt = {0};
	bt.is_char = QC_true;
	return bt;
}

QC_AST_Node *qc_create_chained_expr(QC_AST_Node **elems, int elem_count, QC_Token_Type chainop)
{
	int i;
	QC_AST_Node *ret = NULL;
	for (i = 0; i < elem_count; ++i) {
		if (!ret) {
			ret = elems[i];
		} else {
			QC_AST_Biop *chain = qc_create_biop_node();
			chain->type = chainop;
			chain->lhs = ret;
			chain->rhs = elems[i];

			ret = QC_AST_BASE(chain);
		}
	}
	return ret;
}

QC_AST_Node *qc_create_chained_expr_2(QC_AST_Node **lhs_elems, QC_AST_Node **rhs_elems, int elem_count, QC_Token_Type biop, QC_Token_Type chainop)
{
	int i;
	QC_AST_Node *ret = NULL;
	for (i = 0; i < elem_count; ++i) {
		QC_AST_Biop *inner = qc_create_biop_node();
		inner->type = biop;
		inner->lhs = lhs_elems[i];
		inner->rhs = rhs_elems[i];

		if (!ret) {
			ret = QC_AST_BASE(inner);
		} else {
			QC_AST_Biop *outer = qc_create_biop_node();
			outer->type = chainop;
			outer->lhs = ret;
			outer->rhs = QC_AST_BASE(inner);

			ret = QC_AST_BASE(outer);
		}
	}
	return ret;
}

void qc_add_parallel_id_init(QC_AST_Scope *root, QC_AST_Parallel *parallel, int ix, QC_AST_Node *value)
{
	/* Insert init of 'id' var right after its declaration */
	QC_ASSERT(parallel->body->nodes.size >= 1);
	QC_ASSERT(parallel->body->nodes.data[0]->type == QC_AST_var_decl);
	{
		QC_CASTED_NODE(QC_AST_Var_Decl, id_decl, parallel->body->nodes.data[0]);
		QC_AST_Biop *assign =
			qc_create_assign(
				QC_AST_BASE(qc_create_element_access_1(
					qc_try_create_access(qc_copy_ast(QC_AST_BASE(id_decl->ident))),
					QC_AST_BASE(qc_create_integer_literal(ix, root))
				)),
				value
			);

		qc_insert_array(QC_AST_Node_Ptr)(&parallel->body->nodes, 1, (QC_AST_Node**)&assign, 1);
	}
}
