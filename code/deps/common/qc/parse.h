#ifndef QC_PARSE_H
#define QC_PARSE_H

#include "ast.h"
#include "tokenize.h"

/* @todo To *precedence and *associativity */
int qc_biop_prec(QC_Token_Type type);
int qc_biop_assoc(QC_Token_Type type);

/* dont_reference_tokens: Allowing AST to reference tokens will give e.g. line number for each node */
/* allow_undeclared: Don't mind if types/identifiers stay unresolved. Can result in wrong AST though */
/* @todo dont_reference_tokens -> self_contained (no token refs, no source refs) */
/* @todo Don't print error messages, return them */
QC_AST_Scope *qc_parse_tokens(	QC_Token *toks,
								QC_Bool dont_reference_tokens,
								QC_Bool allow_undeclared);


/* Convenience functions */

/* `string` can be like "foo.bar.x = 123" or a whole program.
   Note that parsing pieces of a program doesn't necessarily result in
   a full or even correct AST because C is context sensitive. */
/* Return value is the scope which contains the expr and builtin type decls */
/* expr is the expression corresponding to the string */ 
QC_AST_Scope *qc_parse_string(QC_AST_Node **expr, const char *string);

#endif
