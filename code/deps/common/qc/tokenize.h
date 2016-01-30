#ifndef QC_TOKENIZE_H
#define QC_TOKENIZE_H

#include "core.h"

typedef enum {
	QC_Token_eof,
	QC_Token_name, /* single_word_like_this */
	QC_Token_int, /* 2538 */
	QC_Token_float, /* 2538.0 */
	QC_Token_string, /* "something" */
	QC_Token_assign, /* = */
	QC_Token_semi, /* ; */
	QC_Token_comma, /* , */
	QC_Token_open_paren, /* ( */
	QC_Token_close_paren, /* ) */
	QC_Token_open_brace, /* { */
	QC_Token_close_brace, /* } */
	QC_Token_open_square, /* [ */
	QC_Token_close_square, /* ] */
	QC_Token_rdiv, /* \ */
	QC_Token_right_arrow, /* -> */
	QC_Token_equals, /* == */
	QC_Token_nequals, /* != */
	QC_Token_less, /* < */
	QC_Token_greater, /* > */
	QC_Token_leq, /* <= */
	QC_Token_geq, /* >= */
	QC_Token_add_assign, /* += */
	QC_Token_sub_assign, /* -= */
	QC_Token_mul_assign, /* *= */
	QC_Token_div_assign, /* /= */
	QC_Token_add, /* + */
	QC_Token_sub, /* - */
	QC_Token_mul, /* * */
	QC_Token_div, /* / */
	QC_Token_incr, /* ++ */
	QC_Token_decr, /* -- */
	QC_Token_and, /* && */
	QC_Token_or, /* || */
	QC_Token_mod, /* % */
	QC_Token_dot, /* . */
	QC_Token_addrof, /* & */
	QC_Token_hat, /* ^ */
	QC_Token_tilde, /* ~ */
	QC_Token_ellipsis, /* .. */ /* @todo Three dots */
	QC_Token_question, /* ? */
	QC_Token_squote, /* ' */
	QC_Token_line_comment, /* // this is comment */
	QC_Token_block_comment, /* this is block comment */
	QC_Token_kw_struct, /* struct */
	QC_Token_kw_return, /* return */
	QC_Token_kw_goto, /* goto */
	QC_Token_kw_break, /* break */
	QC_Token_kw_continue, /* continue */
	QC_Token_kw_else, /* else */
	QC_Token_kw_null, /* NULL */
	QC_Token_kw_for, /* for */
	QC_Token_kw_while, /* while */
	QC_Token_kw_if, /* if */
	QC_Token_kw_QC_true, /* QC_true */
	QC_Token_kw_QC_false, /* QC_false */
	QC_Token_kw_sizeof, /* sizeof */
	QC_Token_kw_typedef, /* typedef */
	QC_Token_kw_parallel, /* for_field */
	/* Type-related */
	QC_Token_kw_void,
	QC_Token_kw_int,
	QC_Token_kw_size_t,
	QC_Token_kw_char,
	QC_Token_kw_float,
	QC_Token_kw_matrix,
	QC_Token_kw_field,
	QC_Token_kw_const,
	QC_Token_unknown
} QC_Token_Type;

const char* qc_tokentype_str(QC_Token_Type type);
const char* qc_tokentype_codestr(QC_Token_Type type);

typedef struct QC_Token {
	QC_Token_Type type;
	QC_Buf_Str text; /* Not terminated! */
	int line;

	QC_Bool empty_line_before;
	QC_Bool last_on_line;

	/* Used only for comments */
	int comment_bound_to; /* -1 == prev token, 1 == next_token */
	int comment_ast_depth; /* Used by parser */
} QC_Token;

static QC_Bool qc_is_comment_tok(QC_Token_Type type) { return type == QC_Token_line_comment || type == QC_Token_block_comment; }

QC_DECLARE_ARRAY(QC_Token)

/* QC_Tokens will be pointing to the 'src' string */
QC_Array(QC_Token) qc_tokenize(const char* src, int src_size);

void qc_print_tokens(QC_Token *tokens, int token_count);


#endif
