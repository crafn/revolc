#ifndef REVOLC_CORE_JSON_H
#define REVOLC_CORE_JSON_H

#include "build.h"
#include "core/color.h"
#include "core/math.h"

#ifndef CODEGEN
#	include <jsmn/jsmn.h>
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
// Json usage should be eventually substituted with C99

struct QC_AST_Node;
struct QC_AST_Scope;
typedef struct Cson {
	struct QC_AST_Node *ast_node;
	struct QC_AST_Scope *root;
	const char *dir_path; // Directory of parsed file
} Cson;

// @todo Swap word order (create_cson etc)
REVOLC_API Cson cson_create(const char *text, const char *dir_path);
REVOLC_API void cson_destroy(Cson c);

REVOLC_API Cson cson_key(Cson c, const char *key);
REVOLC_API Cson cson_member(Cson c, U32 i);
REVOLC_API Cson cson_null();
REVOLC_API const char *cson_compound_type(Cson c); // NULL for plain initializer list
REVOLC_API bool cson_is_compound(Cson c);
REVOLC_API bool cson_is_null(Cson c);
REVOLC_API U32 cson_member_count(Cson c);

// Conversion op from cson to dead binary format is called "blobify"

// 'err' can only be modified to be true. This allows chaining with the same error variable.
REVOLC_API const char *blobify_string(Cson c, bool *err);
REVOLC_API F64 blobify_floating(Cson c, bool *err);
REVOLC_API S64 blobify_integer(Cson c, bool *err);
REVOLC_API bool blobify_boolean(Cson c, bool *err);
REVOLC_API V2d blobify_v2(Cson c, bool *err);
REVOLC_API V3d blobify_v3(Cson c, bool *err);
REVOLC_API Color blobify_color(Cson c, bool *err);
REVOLC_API Qd blobify_q(Cson c, bool *err);
REVOLC_API T3d blobify_t3(Cson c, bool *err);

// Api for creating json-like subset of C99

struct QC_Write_Context;
typedef struct QC_Write_Context WCson;

REVOLC_API WCson *wcson_create();
REVOLC_API void wcson_destroy(WCson *c);

// { ... }
REVOLC_API void wcson_begin_initializer(WCson *c);
REVOLC_API void wcson_end_initializer(WCson *c);

// (type_name) { ... }
REVOLC_API void wcson_begin_compound(WCson *c, const char *type_name);
REVOLC_API void wcson_end_compound(WCson *c);

// .var_name = ...
REVOLC_API void wcson_designated(WCson *c, const char *var_name);

REVOLC_API void deblobify_string(WCson *c, const char *str);
REVOLC_API void deblobify_integer(WCson *c, S64 value);
REVOLC_API void deblobify_floating(WCson *c, double value);
REVOLC_API void deblobify_boolean(WCson *c, bool boolean);
REVOLC_API void deblobify_v2(WCson *c, V2d v);
REVOLC_API void deblobify_v3(WCson *c, V3d v);
REVOLC_API void deblobify_color(WCson *c, Color v);
REVOLC_API void deblobify_q(WCson *c, Qd v);
REVOLC_API void deblobify_t3(WCson *c, T3d v);


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
