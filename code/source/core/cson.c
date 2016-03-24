#include "cson.h"
#include "basic.h"

#ifndef CODEGEN
#	include <qc/ast.h>
#	include <qc/parse.h>
#endif

internal
Cson cson_mod(Cson n, QC_AST_Node *ast_node)
{
	n.ast_node = ast_node;
	return n;
}

Cson cson_create(const char *text, const char *dir_path)
{
	QC_AST_Node *expr;
	QC_AST_Scope *root = qc_parse_string(&expr, text);
	if (!root)
		fail("Failed parsing %s", MISSING_RES_FILE);

	Cson cson = {
		.ast_node = expr,
		.root = root,
		.dir_path = dir_path,
	};
	return cson;
}

void cson_destroy(Cson c)
{
	qc_destroy_ast(c.root);
}

Cson cson_key(Cson c, const char *key)
{
	QC_AST_Node *n = c.ast_node;

	if (!qc_is_literal_node(n, QC_Literal_compound))
		return cson_null();

	QC_CASTED_NODE(QC_AST_Literal, literal, n);

	QC_Array(QC_AST_Node_Ptr) *nodes = &literal->value.compound.subnodes;
	for (int i = 0; i < nodes->size; ++i) {
		QC_AST_Node *node = nodes->data[i];
		if (node->type != QC_AST_biop)
			continue;

		QC_CASTED_NODE(QC_AST_Biop, biop, node);
		if (!biop->lhs || !biop->rhs)
			continue;

		QC_AST_Ident *ident = qc_unwrap_ident(biop->lhs);
		if (!ident)
			continue;
		if (!ident->is_designated)
			continue;
		if (strcmp(ident->text.data, key))
			continue;

		return cson_mod(c, biop->rhs);
	}

	return cson_null();
}

Cson cson_member(Cson c, U32 i)
{
	QC_AST_Node *n = c.ast_node;

	if (!cson_is_compound(c))
		return cson_null();

	QC_CASTED_NODE(QC_AST_Literal, literal, n);
	QC_Array(QC_AST_Node_Ptr) *nodes = &literal->value.compound.subnodes;
	ensure((int)i < nodes->size);
	return cson_mod(c, nodes->data[i]);
}

Cson cson_null()
{
	Cson cson = {
		.ast_node = NULL,
		.dir_path = "<none>",
	};
	return cson;
}

const char *cson_compound_type(Cson c)
{
	QC_AST_Node *n = c.ast_node;

	if (!cson_is_compound(c))
		return "";

	QC_CASTED_NODE(QC_AST_Literal, literal, n);
	QC_AST_Type *type = literal->value.compound.type;
	if (!type)
		return "";
	
	return type->ident->text.data;
}

bool cson_is_compound(Cson c)
{
	QC_AST_Node *n = c.ast_node;

	if (!n || n->type != QC_AST_literal)
		return false;

	QC_CASTED_NODE(QC_AST_Literal, literal, n);
	if (literal->type != QC_Literal_compound)
		return false;
	
	return true;
}

bool cson_is_null(Cson c)
{ return c.ast_node == NULL; }

U32 cson_member_count(Cson c)
{
	QC_AST_Node *n = c.ast_node;

	if (!cson_is_compound(c))
		return 0;

	QC_CASTED_NODE(QC_AST_Literal, literal, n);
	return literal->value.compound.subnodes.size;
}

const char *blobify_string(Cson c, bool *err)
{
	QC_AST_Node *n = c.ast_node;

	if (!qc_is_literal_node(n, QC_Literal_string)) {
		if (err)
			*err = true;
		return ""; // Should return valid value, NULL is not
	}

	QC_CASTED_NODE(QC_AST_Literal, literal, n);
	return literal->value.string.data;
}

F64 blobify_floating(Cson c, bool *err)
{
	QC_AST_Node *n = c.ast_node;

	QC_AST_Literal *eval = qc_eval_const_expr(n);
	F64 val = 0.0;
	if (!eval || (eval->type != QC_Literal_integer && eval->type != QC_Literal_floating)) {
		if (err)
			*err = true;
		goto exit;
	}

	if (eval->type == QC_Literal_floating)
		val = eval->value.floating;
	else
		val = eval->value.integer;

exit:
	qc_destroy_node(QC_AST_BASE(eval));
	return val;
}

// @todo
S64 blobify_integer(Cson c, bool *err)
{ return (S64)blobify_floating(c, err); }

bool blobify_boolean(Cson c, bool *err)
{
	QC_AST_Node *n = c.ast_node;

	QC_AST_Literal *eval = qc_eval_const_expr(n);
	bool val = false;
	if (!eval || eval->type != QC_Literal_boolean) {
		if (err)
			*err = true;
		goto exit;
	}

	val = eval->value.boolean;

exit:
	qc_destroy_node(QC_AST_BASE(eval));
	return val;
}

V2d blobify_v2(Cson c, bool *err)
{
	V2d value = {};
	value.x = blobify_floating(cson_member(c, 0), err);
	value.y = blobify_floating(cson_member(c, 1), err);
	return value;
}

V3d blobify_v3(Cson c, bool *err)
{
	V3d value = {};
	value.x = blobify_floating(cson_member(c, 0), err);
	value.y = blobify_floating(cson_member(c, 1), err);
	value.z = blobify_floating(cson_member(c, 2), err);
	return value;
}

Color blobify_color(Cson c, bool *err)
{
	Color value = {};
	value.r = blobify_floating(cson_member(c, 0), err);
	value.g = blobify_floating(cson_member(c, 1), err);
	value.b = blobify_floating(cson_member(c, 2), err);
	value.a = blobify_floating(cson_member(c, 3), err);
	return value;
}

Qd blobify_q(Cson j, bool *err)
{
	V3d axis = {
		blobify_floating(cson_member(j, 0), err),
		blobify_floating(cson_member(j, 1), err),
		blobify_floating(cson_member(j, 2), err)
	};
	F64 angle = blobify_floating(cson_member(j, 3), err);
	ensure(	isfinite(axis.x) &&
			isfinite(axis.y) &&
			isfinite(axis.z) &&
			isfinite(angle));
	return qd_by_axis(axis, angle);
}

T3d blobify_t3(Cson c, bool *err)
{
	return (T3d) {
		.scale = blobify_v3(cson_key(c, "scale"), err),
		.rot = blobify_q(cson_key(c, "rot"), err),
		.pos = blobify_v3(cson_key(c, "pos"), err)
	};
}

//
// WCson
//

WCson *wcson_create()
{ return qc_create_write_context(); }
void wcson_destroy(WCson *c)
{ return qc_destroy_write_context(c); }

void wcson_begin_initializer(WCson *c)
{ qc_begin_initializer(c); }
void wcson_end_initializer(WCson *c)
{ qc_end_initializer(c); }

void wcson_begin_compound(WCson *c, const char *type_name)
{ qc_begin_compound(c, type_name); }
void wcson_end_compound(WCson *c)
{ qc_end_compound(c); }

void wcson_designated(WCson *c, const char *var_name)
{ qc_add_designated(c, var_name); }

void deblobify_string(WCson *c, const char *str)
{ qc_add_string(c, str); }
void deblobify_integer(WCson *c, S64 value)
{ qc_add_integer(c, value); }
void deblobify_floating(WCson *c, double value)
{ qc_add_floating(c, value); }
void deblobify_boolean(WCson *c, bool boolean)
{ qc_add_boolean(c, boolean); }

void deblobify_v2(WCson *c, V2d v)
{
	wcson_begin_compound(c, "V2d");
	deblobify_floating(c, v.x);
	deblobify_floating(c, v.y);
	wcson_end_compound(c);
}

void deblobify_v3(WCson *c, V3d v)
{
	wcson_begin_compound(c, "V3d");
	deblobify_floating(c, v.x);
	deblobify_floating(c, v.y);
	deblobify_floating(c, v.z);
	wcson_end_compound(c);
}

void deblobify_color(WCson *c, Color v)
{
	wcson_begin_compound(c, "Color");
	deblobify_floating(c, v.r);
	deblobify_floating(c, v.g);
	deblobify_floating(c, v.b);
	deblobify_floating(c, v.a);
	wcson_end_compound(c);
}

void deblobify_q(WCson *c, Qd v)
{
	V3d axis = axis_qd(v);
	F64 angle = angle_qd(v);

	wcson_begin_compound(c, "Qd");

	deblobify_floating(c, axis.x);
	deblobify_floating(c, axis.y);
	deblobify_floating(c, axis.z);
	deblobify_floating(c, angle);

	wcson_end_compound(c);
}

void deblobify_t3(WCson *c, T3d v)
{
	wcson_begin_compound(c, "T3d");

	wcson_designated(c, "scale");
	deblobify_v3(c, v.scale);

	wcson_designated(c, "rot");
	deblobify_q(c, v.rot);

	wcson_designated(c, "pos");
	deblobify_v3(c, v.pos);

	wcson_end_compound(c);
}

