#ifndef REVOLC_CORE_JSON_H
#define REVOLC_CORE_JSON_H

#include "build.h"
#include "core/quaternion.h"
#include "core/transform.h"
#include "core/vector.h"

#include <jsmn/jsmn.h>

typedef struct JsonTok {
	const char *json_path;
	const char *json; // Strings and primitives are null-terminated
	jsmntok_t *tok;
} JsonTok;

REVOLC_API JsonTok json_value_by_key(JsonTok j, const char *key);
REVOLC_API JsonTok json_member(JsonTok j, U32 i);
REVOLC_API bool json_is_null(JsonTok j);
REVOLC_API bool json_is_string(JsonTok j);
REVOLC_API bool json_is_object(JsonTok j);
REVOLC_API bool json_is_array(JsonTok j);
REVOLC_API U32 json_member_count(JsonTok j);
REVOLC_API bool json_is_same(JsonTok j, const char *str);
REVOLC_API void json_strcpy(char *dst, U32 max_len, JsonTok j);
REVOLC_API U32 json_tok_len(JsonTok j);
REVOLC_API const char * json_str(JsonTok j);
REVOLC_API F64 json_real(JsonTok j);
REVOLC_API S64 json_integer(JsonTok j);
REVOLC_API V2d json_v2(JsonTok j);
REVOLC_API V3d json_v3(JsonTok j);
REVOLC_API Qd json_q(JsonTok j);
REVOLC_API T3d json_t3(JsonTok j);

#endif // REVOLC_CORE_JSON_H
