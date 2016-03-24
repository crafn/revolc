#include "tokenize.h"

QC_DEFINE_ARRAY(QC_Token)

QC_INTERNAL QC_Bool whitespace(char ch)
{ return ch == ' ' || ch == '\t' || ch == '\n'; }

QC_INTERNAL QC_Bool linebreak(char ch)
{ return ch == '\n'; }

QC_INTERNAL QC_Token_Type single_char_tokentype(char ch)
{
	switch (ch) {
		case '=': return QC_Token_assign;
		case ';': return QC_Token_semi;
		case ',': return QC_Token_comma;
		case '(': return QC_Token_open_paren;
		case ')': return QC_Token_close_paren;
		case '{': return QC_Token_open_brace;
		case '}': return QC_Token_close_brace;
		case '[': return QC_Token_open_square;
		case ']': return QC_Token_close_square;
		case '<': return QC_Token_less;
		case '>': return QC_Token_greater;
		case '+': return QC_Token_add;
		case '-': return QC_Token_sub;
		case '*': return QC_Token_mul;
		case '/': return QC_Token_div;
		case '\\': return QC_Token_rdiv;
		case '%': return QC_Token_mod;
		case '.': return QC_Token_dot;
		case '&': return QC_Token_addrof;
		case '^': return QC_Token_hat;
		case '?': return QC_Token_question;
		case '~': return QC_Token_tilde;
		case '\'': return QC_Token_squote;
		default: return QC_Token_unknown;
	}
}

QC_INTERNAL QC_Token_Type double_char_tokentype(char ch1, char ch2)
{
	if (ch1 == '-' && ch2 == '>')
		return QC_Token_right_arrow;
	if (ch1 == '=' && ch2 == '=')
		return QC_Token_equals;
	if (ch1 == '!' && ch2 == '=')
		return QC_Token_nequals;
	if (ch1 == '.' && ch2 == '.')
		return QC_Token_ellipsis;
	if (ch1 == '/' && ch2 == '/')
		return QC_Token_line_comment;
	if (ch1 == '/' && ch2 == '*')
		return QC_Token_block_comment;
	if (ch1 == '<' && ch2 == '=')
		return QC_Token_leq;
	if (ch1 == '>' && ch2 == '=')
		return QC_Token_geq;
	if (ch2 == '=') {
		switch (ch1) {
			case '+': return QC_Token_add_assign;
			case '-': return QC_Token_sub_assign;
			case '*': return QC_Token_mul_assign;
			case '/': return QC_Token_div_assign;
			default:;
		}
	}
	if (ch1 == '+' && ch2 == '+')
		return QC_Token_incr;
	if (ch1 == '-' && ch2 == '-')
		return QC_Token_decr;
	if (ch1 == '&' && ch2 == '&')
		return QC_Token_and;
	if (ch1 == '|' && ch2 == '|')
		return QC_Token_or;

	return QC_Token_unknown;
}

/* Differs from strncmp by str_equals_buf("ab", "ab", 1) == QC_false */
QC_INTERNAL QC_Bool str_equals_buf(const char *c_str, const char *buf, int buf_size)
{
	int i;
	for (i = 0; i < buf_size && c_str[i] != '\0'; ++i) {
		if (c_str[i] != buf[i])
			return QC_false;
	}
	return c_str[i] == '\0' && i == buf_size;
}

QC_INTERNAL QC_Token_Type kw_tokentype(const char *buf, int size)
{
	if (str_equals_buf("struct", buf, size))
		return QC_Token_kw_struct;
	if (str_equals_buf("return", buf, size))
		return QC_Token_kw_return;
	if (str_equals_buf("goto", buf, size))
		return QC_Token_kw_goto;
	if (str_equals_buf("break", buf, size))
		return QC_Token_kw_break;
	if (str_equals_buf("continue", buf, size))
		return QC_Token_kw_continue;
	if (str_equals_buf("else", buf, size))
		return QC_Token_kw_else;
	if (str_equals_buf("NULL", buf, size))
		return QC_Token_kw_null;
	if (str_equals_buf("for", buf, size))
		return QC_Token_kw_for;
	if (str_equals_buf("while", buf, size))
		return QC_Token_kw_while;
	if (str_equals_buf("if", buf, size))
		return QC_Token_kw_if;
	if (str_equals_buf("true", buf, size))
		return QC_Token_kw_true;
	if (str_equals_buf("false", buf, size))
		return QC_Token_kw_false;
	if (str_equals_buf("sizeof", buf, size))
		return QC_Token_kw_sizeof;
	if (str_equals_buf("typedef", buf, size))
		return QC_Token_kw_typedef;
	if (str_equals_buf("for_field", buf, size))
		return QC_Token_kw_parallel;
	if (str_equals_buf("void", buf, size))
		return QC_Token_kw_void;
	if (str_equals_buf("int", buf, size))
		return QC_Token_kw_int;
	if (str_equals_buf("bool", buf, size))
		return QC_Token_kw_bool;
	if (str_equals_buf("size_t", buf, size))
		return QC_Token_kw_size_t;
	if (str_equals_buf("char", buf, size))
		return QC_Token_kw_char;
	if (str_equals_buf("float", buf, size))
		return QC_Token_kw_float;
	if (str_equals_buf("matrix", buf, size))
		return QC_Token_kw_matrix;
	if (str_equals_buf("field", buf, size))
		return QC_Token_kw_field;
	if (str_equals_buf("const", buf, size))
		return QC_Token_kw_const;
	return QC_Token_unknown;
}

typedef enum {
	Tok_State_none,
	Tok_State_maybe_single_char,
	Tok_State_number,
	Tok_State_number_after_dot,
	Tok_State_name,
	Tok_State_str,
	Tok_State_line_comment,
	Tok_State_block_comment
} Tok_State;

typedef struct QC_Tokenize_Ctx {
	Tok_State state;
	char string_begin_char;
	int block_comment_depth;
	const char *end;
	int cur_line;
	QC_Bool last_line_was_empty;
	int tokens_on_line;
	int comments_on_line;
	QC_Array(QC_Token) tokens;
} QC_Tokenize_Ctx;

QC_INTERNAL void commit_token(QC_Tokenize_Ctx *t, const char *b, const char *e, QC_Token_Type type)
{
	if (e >= b) {
		QC_Token tok = {0};
		QC_Bool last_on_line = e + 1 < t->end && linebreak(*e);
		if (type == QC_Token_name) {
			QC_Token_Type kw = kw_tokentype(b, e - b);
			if (kw != QC_Token_unknown)
				type = kw;
		}
		tok.type = type;
		tok.text.buf = b;
		tok.text.len = e - b;
		tok.line = t->cur_line;
		tok.empty_line_before = (t->tokens_on_line == 0 && t->last_line_was_empty);
		tok.last_on_line = last_on_line;

		if (qc_is_comment_tok(type)) {
			if (t->tokens_on_line  == t->comments_on_line)
				tok.comment_bound_to = 1; /* If line is only comments, bound to next token */
			else
				tok.comment_bound_to = -1; /* Else bound to token left to comment */
			++t->comments_on_line;
		}

		qc_push_array(QC_Token)(&t->tokens, tok);
		t->state = Tok_State_none;
		++t->tokens_on_line;
	}
}

QC_INTERNAL void on_linebreak(QC_Tokenize_Ctx *t)
{
	++t->cur_line;
	t->last_line_was_empty = (t->tokens_on_line == 0);
	t->tokens_on_line = 0;
	t->comments_on_line = 0;
}

QC_Array(QC_Token) qc_tokenize(const char* src, int src_size)
{
	const char *cur;
	const char *tok_begin;
	QC_Tokenize_Ctx t = {0};
	
	cur = src;
	tok_begin = src;
	t.end = src + src_size;
	t.cur_line = 1;
	t.tokens = qc_create_array(QC_Token)(src_size/4); /* Estimate token count */

	while (cur <= t.end && tok_begin < t.end) {
		char ch = cur < t.end ? *cur : '\n';
		switch (t.state) {
			case Tok_State_none:
				if (single_char_tokentype(ch) != QC_Token_unknown) {
					t.state = Tok_State_maybe_single_char;
				} else if (ch >= '0' && ch <= '9') {
					t.state = Tok_State_number;
				} else if (	(ch >= 'a' && ch <= 'z') ||
							(ch >= 'A' && ch <= 'Z') ||
							(ch == '_')) {
					t.state = Tok_State_name;
				} else if (ch == '\"' || ch == '@') { /* @todo Remove temp hack of @ */
					t.string_begin_char = ch;
					t.state = Tok_State_str;
				} else if (linebreak(ch)) {
					on_linebreak(&t);
				}
				tok_begin = cur;
			break;
			case Tok_State_maybe_single_char: {
				QC_Token_Type type = double_char_tokentype(*tok_begin, ch);
				if (type == QC_Token_unknown) {
					commit_token(&t, tok_begin, cur, single_char_tokentype(*tok_begin));
					--cur;
				} else {
					if (type == QC_Token_line_comment) {
						t.state = Tok_State_line_comment;
						tok_begin += 2;
					} else if (type == QC_Token_block_comment) {
						t.state = Tok_State_block_comment;
						t.block_comment_depth = 1;
						tok_begin += 2;
					} else {
						commit_token(&t, tok_begin, cur + 1, type);
					}
				}
			}
			break;
			case Tok_State_number_after_dot:
			case Tok_State_number:
				if ((ch < '0' || ch > '9') && ch != '.') {
					QC_Token_Type type = QC_Token_int;
					if (t.state == Tok_State_number_after_dot)
						type = QC_Token_float;

					commit_token(&t, tok_begin, cur, type);
					--cur;
					break;
				}

				if (ch == '.')
					t.state = Tok_State_number_after_dot;
				else if (t.state != Tok_State_number_after_dot)
					t.state = Tok_State_number;
			break;
			case Tok_State_name:
				if (	whitespace(ch) ||
						single_char_tokentype(ch) != QC_Token_unknown) {
					commit_token(&t, tok_begin, cur, QC_Token_name);
					--cur;
				}
			break;
			case Tok_State_str:
				if (ch == t.string_begin_char)
					commit_token(&t, tok_begin + 1, cur, QC_Token_string);
			break;
			case Tok_State_line_comment:
				if (linebreak(ch)) {
					commit_token(&t, tok_begin, cur, QC_Token_line_comment);
					on_linebreak(&t);
				}
			case Tok_State_block_comment: {
				char a = *(cur - 1);
				char b = *(cur);
				if (double_char_tokentype(a, b) == QC_Token_block_comment) {
					++t.block_comment_depth;
				} else if (a == '*' && b == '/') {
					--t.block_comment_depth;
					if (t.block_comment_depth <= 0)
						commit_token(&t, tok_begin, cur - 1, QC_Token_block_comment);
				}
			} break;
			default:;
		}
		++cur;
	}

	{ /* Append eof */
		QC_Token eof = {0};
		eof.text.buf = "eof";
		eof.text.len = strlen(eof.text.buf);
		eof.line = t.cur_line;
		eof.last_on_line = QC_true;
		qc_push_array(QC_Token)(&t.tokens, eof);
	}
	return t.tokens;
}

const char* qc_tokentype_str(QC_Token_Type type)
{
	switch (type) {
		case QC_Token_eof: return "eof";
		case QC_Token_name: return "name";
		case QC_Token_int: return "int";
		case QC_Token_float: return "float";
		case QC_Token_string: return "string";
		case QC_Token_assign: return "assign";
		case QC_Token_semi: return "semi";
		case QC_Token_comma: return "comma";
		case QC_Token_open_paren: return "open_paren";
		case QC_Token_close_paren: return "close_paren";
		case QC_Token_open_brace: return "open_brace";
		case QC_Token_close_brace: return "close_brace";
		case QC_Token_open_square: return "open_square";
		case QC_Token_close_square: return "close_square";
		case QC_Token_right_arrow: return "right_arrow";
		case QC_Token_equals: return "equals";
		case QC_Token_nequals: return "nequals";
		case QC_Token_less: return "less";
		case QC_Token_greater: return "greater";
		case QC_Token_leq: return "leq";
		case QC_Token_geq: return "geq";
		case QC_Token_add_assign: return "add_assign";
		case QC_Token_sub_assign: return "sub_assign";
		case QC_Token_mul_assign: return "mul_assign";
		case QC_Token_div_assign: return "div_assign";
		case QC_Token_add: return "add";
		case QC_Token_sub: return "sub";
		case QC_Token_mul: return "mul";
		case QC_Token_div: return "div";
		case QC_Token_incr: return "incr";
		case QC_Token_decr: return "decr";
		case QC_Token_and: return "and";
		case QC_Token_or: return "or";
		case QC_Token_rdiv: return "rdiv";
		case QC_Token_mod: return "mod";
		case QC_Token_dot: return "dot";
		case QC_Token_addrof: return "addrof";
		case QC_Token_hat: return "hat";
		case QC_Token_question: return "question";
		case QC_Token_tilde: return "tilde";
		case QC_Token_squote: return "squote";
		case QC_Token_line_comment: return "line_comment";
		case QC_Token_block_comment: return "block_comment";
		case QC_Token_kw_struct: return "kw_struct";
		case QC_Token_kw_return: return "kw_return";
		case QC_Token_kw_goto: return "kw_goto";
		case QC_Token_kw_break: return "kw_break";
		case QC_Token_kw_continue: return "kw_continue";
		case QC_Token_kw_else: return "kw_else";
		case QC_Token_kw_null: return "kw_null";
		case QC_Token_kw_for: return "kw_for";
		case QC_Token_kw_if: return "kw_if";
		case QC_Token_kw_true: return "kw_true";
		case QC_Token_kw_false: return "kw_false";
		case QC_Token_kw_sizeof: return "kw_sizeof";
		case QC_Token_kw_typedef: return "kw_typedef";
		case QC_Token_kw_parallel: return "kw_parallel";
		case QC_Token_kw_void: return "kw_void";
		case QC_Token_kw_int: return "kw_int";
		case QC_Token_kw_bool: return "kw_bool";
		case QC_Token_kw_size_t: return "kw_size_t";
		case QC_Token_kw_char: return "kw_char";
		case QC_Token_kw_float: return "kw_float";
		case QC_Token_kw_matrix: return "kw_matrix";
		case QC_Token_kw_field: return "kw_field";
		case QC_Token_kw_const: return "kw_const";
		case QC_Token_unknown:
		default: return "unknown";
	}
}

const char* qc_tokentype_codestr(QC_Token_Type type)
{
	switch (type) {
		case QC_Token_eof: return "";
		case QC_Token_name: return "";
		case QC_Token_int: return "";
		case QC_Token_float: return "";
		case QC_Token_string: return "";
		case QC_Token_assign: return "=";
		case QC_Token_semi: return ";";
		case QC_Token_comma: return ",";
		case QC_Token_open_paren: return "(";
		case QC_Token_close_paren: return ")";
		case QC_Token_open_brace: return "{";
		case QC_Token_close_brace: return "}";
		case QC_Token_open_square: return "[";
		case QC_Token_close_square: return "]";
		case QC_Token_right_arrow: return "->";
		case QC_Token_equals: return "==";
		case QC_Token_nequals: return "!=";
		case QC_Token_less: return "<";
		case QC_Token_greater: return ">";
		case QC_Token_leq: return "<=";
		case QC_Token_geq: return ">=";
		case QC_Token_add_assign: return "+=";
		case QC_Token_sub_assign: return "-=";
		case QC_Token_mul_assign: return "*=";
		case QC_Token_div_assign: return "/=";
		case QC_Token_add: return "+";
		case QC_Token_sub: return "-";
		case QC_Token_mul: return "*";
		case QC_Token_div: return "/";
		case QC_Token_incr: return "++";
		case QC_Token_decr: return "--";
		case QC_Token_and: return "&&";
		case QC_Token_or: return "||";
		case QC_Token_rdiv: return "\\";
		case QC_Token_mod: return "%";
		case QC_Token_dot: return ".";
		case QC_Token_addrof: return "&";
		case QC_Token_hat: return "^";
		case QC_Token_question: return "?";
		case QC_Token_tilde: return "~";
		case QC_Token_squote: return "'";
		case QC_Token_line_comment: return "// ...";
		case QC_Token_block_comment: return "/* ... */";
		case QC_Token_kw_struct: return "struct";
		case QC_Token_kw_return: return "return";
		case QC_Token_kw_goto: return "goto";
		case QC_Token_kw_break: return "break";
		case QC_Token_kw_continue: return "continue";
		case QC_Token_kw_else: return "else";
		case QC_Token_kw_null: return "NULL";
		case QC_Token_kw_for: return "for";
		case QC_Token_kw_while: return "while";
		case QC_Token_kw_if: return "if";
		case QC_Token_kw_true: return "true";
		case QC_Token_kw_false: return "false";
		case QC_Token_kw_sizeof: return "sizeof";
		case QC_Token_kw_typedef: return "typedef";
		case QC_Token_kw_parallel: return "for_field";
		case QC_Token_kw_void: return "void";
		case QC_Token_kw_int: return "int";
		case QC_Token_kw_bool: return "bool";
		case QC_Token_kw_size_t: return "size_t";
		case QC_Token_kw_char: return "char";
		case QC_Token_kw_float: return "float";
		case QC_Token_kw_matrix: return "matrix";
		case QC_Token_kw_field: return "field";
		case QC_Token_kw_const: return "const";
		case QC_Token_unknown:
		default: return "???";
	}
}

void qc_print_tokens(QC_Token *tokens, int token_count)
{
	int i;
	for (i = 0; i < token_count; ++i) {
		QC_Token tok = tokens[i];
		int text_len = QC_MIN(tok.text.len, 20);
		printf("%14s: %20.*s %8i last_on_line: %i empty_line_before: %i\n",
				qc_tokentype_str(tok.type), text_len, tok.text.buf, tok.line, tok.last_on_line, tok.empty_line_before);
	}
}
