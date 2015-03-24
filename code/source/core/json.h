#ifndef REVOLC_CORE_JSON_H
#define REVOLC_CORE_JSON_H

#include "build.h"
#include "core/color.h"
#include "core/quaternion.h"
#include "core/transform.h"
#include "core/vector.h"

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

typedef struct ParsedJsonFile {
	const char *json_path;
	jsmntok_t *tokens;
	char *json;
	char *null_json;
	JsonTok root;
} ParsedJsonFile;

ParsedJsonFile malloc_parsed_json_file(const char *file);
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
REVOLC_API V2d json_v2(JsonTok j);
REVOLC_API V3d json_v3(JsonTok j);
REVOLC_API Color json_color(JsonTok j);
REVOLC_API Qd json_q(JsonTok j);
REVOLC_API T3d json_t3(JsonTok j);

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

REVOLC_API WJson *wjson_create(JsonType type);
REVOLC_API void wjson_destroy(WJson *j);

REVOLC_API void wjson_append(WJson *j, WJson *item);
REVOLC_API WJson * wjson_named_member(WJson *j, JsonType t, const char *name);
REVOLC_API WJson * wjson_str(const char *str);
REVOLC_API WJson * wjson_number(F64 n);
REVOLC_API WJson * wjson_v3(V3d vec);
REVOLC_API WJson * wjson_q(Qd q);
REVOLC_API WJson * wjson_t3(T3d tf);
REVOLC_API void wjson_dump(WJson *j);
REVOLC_API void wjson_write_updated(const char *path, JsonTok input, WJson *upd);

#endif // REVOLC_CORE_JSON_H
