#ifndef REVOLC_CORE_JSON_H
#define REVOLC_CORE_JSON_H

#include "build.h"
#include "core/color.h"
#include "core/math.h"

#ifndef CODEGEN
#	include <jsmn/jsmn.h>
#	include <qc/ast.h>
#endif

typedef enum {
	JsonType_object,
	JsonType_array,
	JsonType_number,
	JsonType_string
} JsonType;

// Read-only json element
typedef struct JsonTok {
	const char *json_path;
	const char *json_dir;
	const char *json; // Whole file json string
	const char *null_json; // Strings and primitives are null-terminated
	U32 json_size;
	jsmntok_t *tok;
} JsonTok;

struct Ator;

typedef struct ParsedJsonFile {
	struct Ator *ator;
	const char *json_path;
	jsmntok_t *tokens;
	char *json;
	char *null_json;
	JsonTok root;
} ParsedJsonFile;

ParsedJsonFile parse_json_file(struct Ator *ator, const char *file);
void free_parsed_json_file(ParsedJsonFile json);

REVOLC_API JsonTok json_value_by_key(JsonTok j, const char *key);
REVOLC_API JsonTok json_member(JsonTok j, U32 i);
REVOLC_API bool json_is_null(JsonTok j);
REVOLC_API bool json_is_string(JsonTok j);
REVOLC_API bool json_is_object(JsonTok j);
REVOLC_API bool json_is_array(JsonTok j);
REVOLC_API bool json_is_number(JsonTok j);
REVOLC_API JsonType json_type(JsonTok j);
REVOLC_API U32 json_member_count(JsonTok j);
REVOLC_API bool json_is_same(JsonTok j, const char *str);
REVOLC_API void json_strcpy(char *dst, U32 max_len, JsonTok j);
REVOLC_API U32 json_tok_len(JsonTok j);
REVOLC_API const char * json_str(JsonTok j);
REVOLC_API F64 json_real(JsonTok j);
REVOLC_API S64 json_integer(JsonTok j);
REVOLC_API bool json_bool(JsonTok j);
REVOLC_API V2d json_v2(JsonTok j);
REVOLC_API V3d json_v3(JsonTok j);
REVOLC_API Color json_color(JsonTok j);
REVOLC_API Qd json_q(JsonTok j);
REVOLC_API T3d json_t3(JsonTok j);

// Api for traversing json-like subset of C99

REVOLC_API QC_AST_Node *cson_key(QC_AST_Node *n, const char *key);
REVOLC_API QC_AST_Node *cson_member(QC_AST_Node *n, U32 i);
REVOLC_API const char *cson_compound_type(QC_AST_Node *n); // NULL for plain initializer list
REVOLC_API bool cson_is_compound(QC_AST_Node *n);
REVOLC_API U32 cson_member_count(QC_AST_Node *n);
// 'err' can only be modified to be true. This allows chaining with the same error variable.
REVOLC_API const char *cson_string(QC_AST_Node *n, bool *err);
REVOLC_API F64 cson_floating(QC_AST_Node *n, bool *err);
REVOLC_API S64 cson_integer(QC_AST_Node *n, bool *err);
REVOLC_API bool cson_boolean(QC_AST_Node *n, bool *err);

// Used to partially update json strings
// If complex manipulation of json files is needed, this should
// probably override JsonTok as read/write structure
typedef struct WJson {
	JsonType type;

	struct WJson *last_member; // Owns
	struct WJson *prev;

	struct WJson *first_member;
	struct WJson *next;

	U32 member_count;

	F64 number;
	char *string; // Owns
} WJson;

REVOLC_API WJson * wjson_object();
REVOLC_API WJson * wjson_array();
REVOLC_API void wjson_destroy(WJson *root);

REVOLC_API void wjson_append(WJson *j, WJson *item);
REVOLC_API WJson * wjson_add_named_member(WJson *j, const char *name, WJson *member);
// Use wjson_add_named_member instead as it works correctly with strings and numbers
// @todo Remove
REVOLC_API WJson * wjson_named_member(WJson *j, JsonType t, const char *name);
REVOLC_API WJson * wjson_str(const char *str);
REVOLC_API WJson * wjson_number(F64 n);
REVOLC_API WJson * wjson_v2(V2d vec);
REVOLC_API WJson * wjson_v3(V3d vec);
REVOLC_API WJson * wjson_q(Qd q);
REVOLC_API WJson * wjson_t3(T3d tf);
REVOLC_API WJson * wjson_color(Color c);
REVOLC_API void wjson_dump(WJson *j);
REVOLC_API void wjson_write_updated(const char *path, JsonTok input, WJson *upd);

#endif // REVOLC_CORE_JSON_H
