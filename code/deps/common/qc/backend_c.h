#ifndef QC_BACKEND_C_H
#define QC_BACKEND_C_H

#include "config.h"
#include "core.h"
#include "parse.h"

QC_API QC_AST_Var_Decl *qc_is_device_field_member_decl(QC_AST_Type_Decl *field_decl);

/* Utils for other c-like backends */
QC_API void qc_lift_var_decls(QC_AST_Scope *root);
QC_API void qc_parallel_loops_to_ordinary(QC_AST_Scope *root);
QC_API void qc_lift_types_and_funcs_to_global_scope(QC_AST_Scope *root);
QC_API void qc_add_builtin_c_decls_to_global_scope(QC_AST_Scope *root, QC_Bool cpu_device_impl);
QC_API void qc_apply_c_operator_overloading(QC_AST_Scope *root, QC_Bool convert_mat_expr);
/* Type name for builtin type */
QC_API void qc_append_builtin_type_c_str(QC_Array(char) *buf, QC_Builtin_Type bt);
/* Function name for expression */
QC_API void qc_append_expr_c_func_name(QC_Array(char) *buf, QC_AST_Node *expr);
QC_API void qc_append_c_stdlib_includes(QC_Array(char) *buf);

QC_API QC_Bool qc_ast_to_c_str(QC_Array(char) *buf, int indent, QC_AST_Node *node);

/* @todo Flag determining C99 or C89 */
QC_API QC_Array(char) qc_gen_c_code(QC_AST_Scope *root);


#endif
