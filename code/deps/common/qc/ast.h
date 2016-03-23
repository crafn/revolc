#ifndef QC_AST_H
#define QC_AST_H

#include "core.h"
#include "tokenize.h"

typedef enum {
	QC_AST_none,
	QC_AST_scope,
	QC_AST_ident,
	QC_AST_type,
	QC_AST_type_decl,
	QC_AST_var_decl,
	QC_AST_func_decl,
	QC_AST_literal,
	QC_AST_biop,
	QC_AST_control,
	QC_AST_call,
	QC_AST_access,
	QC_AST_cond,
	QC_AST_loop,
	QC_AST_cast,
	QC_AST_typedef,
	QC_AST_parallel
} QC_AST_Node_Type;

struct QC_AST_Node;
struct QC_AST_Decl;
typedef struct QC_AST_Node *QC_AST_Node_Ptr;
typedef struct QC_Token *QC_Token_Ptr;

QC_DECLARE_ARRAY(QC_AST_Node_Ptr)
QC_DECLARE_ARRAY(QC_Token_Ptr)
QC_DECLARE_HASH_TABLE(QC_AST_Node_Ptr, QC_AST_Node_Ptr)
static QC_U32 qc_hash(QC_AST_Node_Ptr)(QC_AST_Node_Ptr node) { return qc_hash(QC_Void_Ptr)(node); }

/* Base "class" for every QC_AST node type */
typedef struct QC_AST_Node {
	QC_AST_Node_Type type;

	/* Information for human-readable output */

	/* @todo Inline struct with needed info. Then generated code can have vertical spacing etc. */
	QC_Token *begin_tok;

	/* Comments on the previous line(s) (like this comment) */
	QC_Array(QC_Token_Ptr) pre_comments;
	QC_Array(QC_Token_Ptr) post_comments; /* On the same line (like this comment) */

	const char *attribute; /* Just a quick test for __global__ */
} QC_AST_Node;

/* { ... } */
typedef struct QC_AST_Scope {
	QC_AST_Node b;
	QC_Array(QC_AST_Node_Ptr) nodes;
	QC_Bool is_root;
} QC_AST_Scope;

/* Identifier */
typedef struct QC_AST_Ident {
	QC_AST_Node b;
	QC_Array(char) text; /* NULL-terminated */
	QC_Bool is_designated; /* Dot before identifier */

	struct QC_AST_Node *decl; /* Not owned */
} QC_AST_Ident;

struct QC_AST_Type_Decl;
struct QC_AST_Typedef;

typedef struct QC_AST_Type {
	QC_AST_Node b;

	/* 'Foo' of 'Foo **'. NULL for builtin types. */
	/* This is needed for storing the identifier when the type can't be resolved. */
	/* When this is not NULL, 'ident->decl == base_type_decl' */
	QC_AST_Ident *ident;

	/* Pointer to 'struct Foo { ... }' in type 'Foo **' */
	/* Decided to use type decl directly instead of identifiers, because
	 * type names can be multiple keywords long in source code (long int etc.)*/
	struct QC_AST_Type_Decl *base_type_decl; /* Not owned. */
	struct QC_AST_Typedef *base_typedef; /* Not owned. Records the chain of typedefs for backend. */
	int ptr_depth;
	/* @todo 2-dimensional arrays, pointers to arrays, ... (?) */
	int array_size; /* 0 for no array */
	QC_Bool is_const; /* Just to propagate consts to output */

	/* When adding members, remember to update type_node_equals! */
} QC_AST_Type;
QC_Bool type_node_equals(QC_AST_Type a, QC_AST_Type b);

typedef struct QC_Builtin_Type {
	QC_Bool is_void;
	QC_Bool is_integer;
	QC_Bool is_boolean;
	QC_Bool is_char; /* int8_t != char */
	QC_Bool is_float;
	int bitness; /* Zero for "not explicitly specified" */
	QC_Bool is_unsigned;

	QC_Bool is_matrix;
#define QC_MAX_MATRIX_RANK 10 /* Could be made dynamic */
	int matrix_rank;
	int matrix_dim[QC_MAX_MATRIX_RANK];

	QC_Bool is_field;
	int field_dim;

	/* When adding members, remember to update builtin_type_equals! */
} QC_Builtin_Type;
QC_Bool builtin_type_equals(QC_Builtin_Type a, QC_Builtin_Type b);

/* Type declaration / definition */
typedef struct QC_AST_Type_Decl {
	QC_AST_Node b;
	QC_AST_Ident *ident;
	QC_AST_Scope *body;

	/* 'body' and 'ident' are NULL for builtin types */
	QC_Bool is_builtin; /* void, int, char etc. */
	QC_Builtin_Type builtin_type;
	struct QC_AST_Type_Decl *builtin_sub_type_decl; /* Not owned. Matrix/scalar for a field. Scalar for a matrix. */
	struct QC_AST_Type_Decl *builtin_concrete_decl; /* Not owned. Backend can use this to point generated types. */
} QC_AST_Type_Decl;

/* Variable declaration / definition */
typedef struct QC_AST_Var_Decl {
	QC_AST_Node b;

	QC_AST_Type *type;
	QC_AST_Ident *ident;
	QC_AST_Node *value;
} QC_AST_Var_Decl;

typedef QC_AST_Var_Decl *QC_AST_Var_Decl_Ptr;
QC_DECLARE_ARRAY(QC_AST_Var_Decl_Ptr)

/* Function declaration / definition */
typedef struct QC_AST_Func_Decl {
	QC_AST_Node b;
	QC_AST_Type *return_type;
	QC_AST_Ident *ident;
	QC_Array(QC_AST_Var_Decl_Ptr) params;
	QC_Bool ellipsis;
	QC_AST_Scope *body;

	QC_Bool is_builtin; /* Field allocation and deallocation functions */
	struct QC_AST_Func_Decl *builtin_concrete_decl; /* Not owned. Backend can use this to point generated types. */
} QC_AST_Func_Decl;

typedef enum {
	QC_Literal_integer,
	QC_Literal_floating,
	QC_Literal_boolean,
	QC_Literal_string,
	QC_Literal_null,
	QC_Literal_compound /* (Type) {1, 2} or just {1, 2} */
} QC_Literal_Type;

typedef struct QC_AST_Literal {
	QC_AST_Node b;
	QC_Literal_Type type; /* @todo Could be removed by changing base_type_decl -> type */
	union {
		/* @todo Different integer sizes etc */
		int integer;
		double floating;
		QC_Bool boolean;
		QC_Array(char) string;
		struct {
			QC_AST_Type *type; /* NULL for initializer list */
			QC_Array(QC_AST_Node_Ptr) subnodes;
		} compound;
	} value;

	struct QC_AST_Type_Decl *base_type_decl; /* Not owned. 'qc_expr_type' needs this. */
} QC_AST_Literal;

QC_Bool qc_is_literal_node(QC_AST_Node *node, QC_Literal_Type literal_type);

/* Binary operation */
/* @todo Rename to QC_AST_Expr */
typedef struct QC_AST_Biop {
	QC_AST_Node b;
	QC_Token_Type type;
	QC_AST_Node *lhs; /* NULL for unary operations like '-5' */
	QC_AST_Node *rhs; /* NULL for unary operations like 'i++' */

	QC_Bool is_top_level; /* This is not part of another expression */
} QC_AST_Biop;

/* return, goto, continue, break */
typedef struct QC_AST_Control {
	QC_AST_Node b;
	QC_Token_Type type;
	/* For return and goto */
	QC_AST_Node *value;
} QC_AST_Control;

/* Function call */
typedef struct QC_AST_Call {
	QC_AST_Node b;
	QC_AST_Node *base; /* Access or expr yielding function */
	QC_Array(QC_AST_Node_Ptr) args;
} QC_AST_Call;

/* Returns NULL if call is not simple (e.g. foo[0](5) or (foo + 0)(5)) */
QC_AST_Ident *qc_call_ident(QC_AST_Call *call);

/* Variable/array/member access */
typedef struct QC_AST_Access {
	QC_AST_Node b;

	/* base -- base.arg -- base->arg -- base[arg] -- some_matrix(arg1, arg2)*/
	QC_AST_Node *base;
	QC_Array(QC_AST_Node_Ptr) args;
	/* 'base.sub' -> Access(Access(Ident(base)), Ident(sub))
	 * 'base' is wrapped in an extra Access, because then '(base + 0)->sub' and '(base)->sub' and 'base.sub'
	 * are handled uniformly (every expression has two Access nodes) */

	/* @todo Reconsider if wrapping plain idents in Accesses is worth it. */
	QC_Bool is_var_access; /* Plain variable access */
	QC_Bool is_member_access;
	QC_Bool is_element_access; /* Matrix or field element access */
	QC_Bool is_array_access;

	QC_Bool implicit_deref; /* 'a->b' or 'field_ptr(1, 2)' */

	/* @todo Field access etc. */
} QC_AST_Access;

/* if */
typedef struct QC_AST_Cond {
	QC_AST_Node b;

	QC_AST_Node *expr;
	QC_AST_Scope *body;
	QC_Bool implicit_scope; /* Original source had no { } */

	/* Must be QC_AST_Scope or QC_AST_Cond or NULL */
	QC_AST_Node *after_else;
} QC_AST_Cond;

/* while/for/do-while */
typedef struct QC_AST_Loop {
	QC_AST_Node b;

	/* while-loop has only 'cond' */
	QC_AST_Node *init;
	QC_AST_Node *cond;
	QC_AST_Node *incr;

	QC_AST_Scope *body;
	QC_Bool implicit_scope; /* Original source had no { } */

	/* @todo do-while */
} QC_AST_Loop;

typedef struct QC_AST_Cast {
	QC_AST_Node b;

	QC_AST_Type *type;
	QC_AST_Node *target;
} QC_AST_Cast;

typedef struct QC_AST_Typedef {
	QC_AST_Node b;

	QC_AST_Type *type;
	QC_AST_Ident *ident;
} QC_AST_Typedef;

/* for_field */
typedef struct QC_AST_Parallel {
	QC_AST_Node b;

	QC_AST_Node *output;
	QC_AST_Node *input;

	QC_AST_Scope *body;

	int dim; /* Essentially output field dimension */
} QC_AST_Parallel;


const char *qc_node_type_str(QC_AST_Node_Type type);


/* Usage: QC_CASTED_NODE(QC_AST_Ident, ident, generic_node); printf("%c", ident->text_buf[0]); */
#define QC_CASTED_NODE(type, name, assign) \
	type *name = (type*)assign
#define QC_AST_BASE(node) (&(node)->b)

QC_AST_Scope *qc_create_ast();
void qc_destroy_ast(QC_AST_Scope *node);
QC_AST_Node *qc_copy_ast(QC_AST_Node *node);
QC_AST_Node *qc_shallow_copy_ast(QC_AST_Node *node);
void qc_move_ast(QC_AST_Scope *dst, QC_AST_Scope *src);

QC_AST_Node *qc_create_ast_node(QC_AST_Node_Type type);
QC_AST_Scope *qc_create_scope_node();
QC_AST_Ident *qc_create_ident_node();
QC_AST_Type *qc_create_type_node();
QC_AST_Type_Decl *qc_create_type_decl_node();
QC_AST_Var_Decl *qc_create_var_decl_node();
QC_AST_Func_Decl *qc_create_func_decl_node();
QC_AST_Literal *qc_create_literal_node();
QC_AST_Biop *qc_create_biop_node();
QC_AST_Control *qc_create_control_node();
QC_AST_Call *qc_create_call_node();
QC_AST_Access *qc_create_access_node();
QC_AST_Cond *qc_create_cond_node();
QC_AST_Loop *qc_create_loop_node();
QC_AST_Cast *qc_create_cast_node();
QC_AST_Typedef *qc_create_typedef_node();
QC_AST_Parallel *qc_create_parallel_node();

/* Copies only stuff in QC_AST_Node structure. Useful for copying comments to another node, for example. */
void qc_copy_ast_node_base(QC_AST_Node *dst, QC_AST_Node *src);
/* Calls a specific qc_copy_*_node */
/* 'subnodes' and 'refnodes' should contain same nodes as a specific qc_copy_*_node */
void qc_copy_ast_node(QC_AST_Node *copy, QC_AST_Node *node, QC_AST_Node **subnodes, int subnode_count, QC_AST_Node **refnodes, int refnode_count);
void qc_shallow_copy_ast_node(QC_AST_Node *copy, QC_AST_Node* node);
/* Copy and source nodes can be the same in copying functions. This just updates the subnodes. */
/* First param: destination
 * Second param: source
 * Subnode params: new subnodes for destination
 * Refnode params: new refnodes for destination */
void qc_copy_scope_node(QC_AST_Scope *copy, QC_AST_Scope *scope, QC_AST_Node **subnodes, int subnode_count);
void qc_copy_ident_node(QC_AST_Ident *copy, QC_AST_Ident *ident, QC_AST_Node *ref_to_decl);
void qc_copy_type_node(QC_AST_Type *copy, QC_AST_Type *type, QC_AST_Node *ident, QC_AST_Node *ref_to_base_type_decl, QC_AST_Node *ref_to_base_typedef);
void qc_copy_type_decl_node(QC_AST_Type_Decl *copy, QC_AST_Type_Decl *decl, QC_AST_Node *ident, QC_AST_Node *body, QC_AST_Node *builtin_sub_decl_ref, QC_AST_Node *backend_decl_ref);
void qc_copy_var_decl_node(QC_AST_Var_Decl *copy, QC_AST_Var_Decl *decl, QC_AST_Node *type, QC_AST_Node *ident, QC_AST_Node *value);
void qc_copy_func_decl_node(QC_AST_Func_Decl *copy, QC_AST_Func_Decl *decl, QC_AST_Node *type, QC_AST_Node *ident, QC_AST_Node *body, QC_AST_Node **params, int param_count, QC_AST_Node *backend_decl_ref);
void qc_copy_literal_node(QC_AST_Literal *copy, QC_AST_Literal *literal, QC_AST_Node *comp_type, QC_AST_Node **comp_subs, int comp_sub_count, QC_AST_Node *type_decl_ref);
void qc_copy_biop_node(QC_AST_Biop *copy, QC_AST_Biop *biop, QC_AST_Node *lhs, QC_AST_Node *rhs);
void qc_copy_control_node(QC_AST_Control *copy, QC_AST_Control *control, QC_AST_Node *value);
void qc_copy_call_node(QC_AST_Call *copy, QC_AST_Call *call, QC_AST_Node *base, QC_AST_Node **args, int arg_count);
void qc_copy_access_node(QC_AST_Access *copy, QC_AST_Access *access, QC_AST_Node *base, QC_AST_Node **args, int arg_count);
void qc_copy_cond_node(QC_AST_Cond *copy, QC_AST_Cond *cond, QC_AST_Node *expr, QC_AST_Node *body, QC_AST_Node *after_else);
void qc_copy_loop_node(QC_AST_Loop *copy, QC_AST_Loop *loop, QC_AST_Node *init, QC_AST_Node *cond, QC_AST_Node *incr, QC_AST_Node *body);
void qc_copy_cast_node(QC_AST_Cast *copy, QC_AST_Cast *cast, QC_AST_Node *type, QC_AST_Node *target);
void qc_copy_typedef_node(QC_AST_Typedef *copy, QC_AST_Typedef *def, QC_AST_Node *type, QC_AST_Node *ident);
void qc_copy_parallel_node(QC_AST_Parallel *copy, QC_AST_Parallel *parallel, QC_AST_Node *output, QC_AST_Node *input, QC_AST_Node *body);

/* Recursive */
void qc_destroy_node(QC_AST_Node *node);
/* Use this to destroy the original node after qc_shallow_copy_ast_node.
 * Doesn't destroy any owned nodes. */
void qc_shallow_destroy_node(QC_AST_Node *node);

/* Evaluation */

/* Don't destroy *ret, it's not constructed by qc_create_*, just container of data */
QC_Bool qc_expr_type(QC_AST_Type *ret, QC_AST_Node *expr);
/* Return value needs to be qc_destroy_node'd */
QC_AST_Literal *qc_eval_const_expr(QC_AST_Node *expr);

/* @todo Move these near corresponding types */
QC_Bool qc_is_decl(QC_AST_Node *node);
QC_AST_Ident *qc_decl_ident(QC_AST_Node *node);
QC_AST_Ident *qc_access_ident(QC_AST_Access *access);
QC_AST_Ident *qc_unwrap_ident(QC_AST_Node *node);


/* QC_AST traversing utils */

typedef struct QC_AST_Parent_Map {
	QC_Hash_Table(QC_AST_Node_Ptr, QC_AST_Node_Ptr) table; /* Use qc_find_- and qc_set_parent_node to access */
	QC_Array(QC_AST_Node_Ptr) builtin_decls; /* Builtin decls are separate, because they're created during parsing */
	/* @todo That ^ could maybe be removed. */
} QC_AST_Parent_Map;

QC_AST_Parent_Map qc_create_parent_map(QC_AST_Node *root);
void qc_destroy_parent_map(QC_AST_Parent_Map *map);
QC_AST_Node *qc_find_parent_node(QC_AST_Parent_Map *map, QC_AST_Node *node);
void qc_set_parent_node(QC_AST_Parent_Map *map, QC_AST_Node *sub, QC_AST_Node *parent);
int qc_find_in_scope(QC_AST_Scope *scope, QC_AST_Node *needle);
QC_AST_Func_Decl *qc_find_enclosing_func(QC_AST_Parent_Map *map, QC_AST_Node *node);
QC_Bool qc_is_subnode(QC_AST_Parent_Map *map, QC_AST_Node *parent, QC_AST_Node *sub);

QC_AST_Ident *qc_resolve_ident(QC_AST_Parent_Map *map, QC_AST_Ident *ident);
/* Resolves call to specific overload */
QC_AST_Call *qc_resolve_overloaded_call(QC_AST_Parent_Map *map, QC_AST_Call *call, QC_AST_Type *return_type_hint);
/* Resove all unresolved things in QC_AST. Call this after inserting unresolved nodes into QC_AST. */
void qc_resolve_ast(QC_AST_Scope *root);
/* Break all resolved things. Call this when e.g. moving blocks of code around. */
void qc_unresolve_ast(QC_AST_Node *node);


void qc_push_immediate_subnodes(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node *node);
/* Gathers immediate referenced (not owned) nodes */
void qc_push_immediate_refnodes(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node *node);
/* Gathers the whole subnode tree to array */
void qc_push_subnodes(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node *node, QC_Bool qc_push_before_recursing);
/* Rewrites nodes in tree, old_nodes[i] -> new_nodes[i]
 * Doesn't free or allocate any nodes.
 * Doesn't recurse into old_nodes. They can be dangling.
 * Is recursive, so if some new_nodes[i] contain old_nodes[k], it will also be replaced. */
QC_AST_Node *qc_replace_nodes_in_ast(QC_AST_Node *node, QC_AST_Node **old_nodes, QC_AST_Node **new_nodes, int node_count);

/* Innermost first */
void qc_find_subnodes_of_type(QC_Array(QC_AST_Node_Ptr) *ret, QC_AST_Node_Type type, QC_AST_Node *node);

/* Debug */
void qc_print_ast(QC_AST_Node *node, int indent);

/* Convenience functions */

QC_AST_Ident *qc_create_ident_with_text(QC_AST_Node *decl, const char *fmt, ...);
QC_AST_Var_Decl *qc_create_simple_var_decl(QC_AST_Type_Decl *type_decl, const char *ident);
QC_AST_Var_Decl *qc_create_var_decl(QC_AST_Type_Decl *type_decl, QC_AST_Ident *ident, QC_AST_Node *value);
QC_AST_Type_Decl *qc_find_builtin_type_decl(QC_Builtin_Type bt, QC_AST_Scope *root);
QC_AST_Literal *qc_create_integer_literal(int value, QC_AST_Scope *root);
QC_AST_Literal *qc_create_floating_literal(double value, QC_AST_Scope *root);
QC_AST_Literal *qc_create_boolean_literal(QC_Bool value, QC_AST_Scope *root);
QC_AST_Literal *qc_create_string_literal(const char *value, QC_AST_Scope *root);
QC_AST_Call *qc_create_call_1(QC_AST_Ident *ident, QC_AST_Node *arg);
QC_AST_Call *qc_create_call_2(QC_AST_Ident *ident, QC_AST_Node *arg1, QC_AST_Node *arg2);
QC_AST_Call *qc_create_call_3(QC_AST_Ident *ident, QC_AST_Node *arg1, QC_AST_Node *arg2, QC_AST_Node *arg3);
QC_AST_Call *qc_create_call_4(QC_AST_Ident *ident, QC_AST_Node *arg1, QC_AST_Node *arg2, QC_AST_Node *arg3, QC_AST_Node *arg4);
QC_AST_Control *qc_create_return(QC_AST_Node *expr);
QC_AST_Biop *qc_create_sizeof(QC_AST_Node *expr);
QC_AST_Biop *qc_create_deref(QC_AST_Node *expr);
QC_AST_Biop *qc_create_addrof(QC_AST_Node *expr);
QC_AST_Biop *qc_create_biop(QC_Token_Type type, QC_AST_Node *lhs, QC_AST_Node *rhs);
QC_AST_Biop *qc_create_assign(QC_AST_Node *lhs, QC_AST_Node *rhs);
QC_AST_Biop *qc_create_mul(QC_AST_Node *lhs, QC_AST_Node *rhs);
QC_AST_Biop *qc_create_less_than(QC_AST_Node *lhs, QC_AST_Node *rhs);
QC_AST_Biop *qc_create_equals(QC_AST_Node *lhs, QC_AST_Node *rhs);
QC_AST_Biop *qc_create_and(QC_AST_Node *lhs, QC_AST_Node *rhs);
QC_AST_Biop *qc_create_pre_increment(QC_AST_Node *expr);
QC_AST_Cast *qc_create_cast(QC_AST_Type *type, QC_AST_Node *target);
QC_AST_Type *qc_create_builtin_type(QC_Builtin_Type bt, int ptr_depth, QC_AST_Scope *root);
QC_AST_Type *qc_copy_and_modify_type(QC_AST_Type *type, int delta_ptr_depth);
QC_AST_Type *qc_create_simple_type(QC_AST_Type_Decl *type_decl);
QC_AST_Loop *qc_create_for_loop(QC_AST_Var_Decl *index, QC_AST_Node *max_expr, QC_AST_Scope *body);
QC_AST_Node *qc_try_create_access(QC_AST_Node *node);
QC_AST_Access *qc_create_element_access_1(QC_AST_Node *base, QC_AST_Node *arg);
QC_AST_Access *qc_create_simple_access(QC_AST_Var_Decl *var);
QC_AST_Access *qc_create_simple_member_access(QC_AST_Var_Decl *base, QC_AST_Var_Decl *member);
QC_AST_Scope *qc_create_scope_1(QC_AST_Node *expr);
QC_AST_Cond *qc_create_if_1(QC_AST_Node *expr, QC_AST_Node *body_expr_1);
QC_AST_Node *qc_create_full_deref(QC_AST_Node *expr);

QC_Builtin_Type qc_void_builtin_type();
/* @todo int -> integer + bitness and signedness flags */
QC_Builtin_Type qc_int_builtin_type();
/* @todo float -> floating + bitness flag */
QC_Builtin_Type qc_float_builtin_type();
QC_Builtin_Type qc_boolean_builtin_type();
QC_Builtin_Type qc_char_builtin_type();

/* elem[0] chainop elem[1] */
QC_AST_Node *qc_create_chained_expr(QC_AST_Node **elems, int elem_count, QC_Token_Type chainop);

/* (lhs[0] biop rhs[0]) chainop (lhs[1] biop rhs[1]) */
QC_AST_Node *qc_create_chained_expr_2(QC_AST_Node **lhs_elems, QC_AST_Node **rhs_elems, int elem_count, QC_Token_Type biop, QC_Token_Type chainop);

void qc_add_parallel_id_init(QC_AST_Scope *root, QC_AST_Parallel *parallel, int ix, QC_AST_Node *value);


/* Immediate-mode AST generation */
/* Functions beginning with qc_add, qc_begin, or qc_end are dedicated to the immediate mode api */

typedef struct QC_Write_Context {
	QC_AST_Scope *root;
	QC_Array(QC_AST_Node_Ptr) stack;
	/* Nodes not included in output, but required for complete AST. Like type declarations. */
	QC_Array(QC_AST_Node_Ptr) loose_nodes;
} QC_Write_Context;

QC_Write_Context *qc_create_write_context();
void qc_destroy_write_context(QC_Write_Context *ctx);

/* { ... } */
void qc_begin_initializer(QC_Write_Context *ctx);
void qc_end_initializer(QC_Write_Context *ctx);

/* (type_name) { ... } */
void qc_begin_compound(QC_Write_Context *ctx, const char *type_name);
void qc_end_compound(QC_Write_Context *ctx);

/* .var_name = ... */
void qc_add_designated(QC_Write_Context *ctx, const char *var_name);

void qc_add_string(QC_Write_Context *ctx, const char *str);
void qc_add_integer(QC_Write_Context *ctx, int value);
void qc_add_floating(QC_Write_Context *ctx, double value);
void qc_add_boolean(QC_Write_Context *ctx, QC_Bool value);


#endif
