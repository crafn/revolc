#include "json.h"

#include <stdlib.h>

JsonTok json_value_by_key(JsonTok obj, const char *key)
{
	ensure(obj.tok->type == JSMN_OBJECT);
	jsmntok_t *m= obj.tok + 1;
	for (U32 i= 0; i < obj.tok->size; ++i, m += m->deep_size + 1) {
		if (m->type != JSMN_STRING)
			continue;

		if (!strcmp(key, obj.json + m->start)) {
			JsonTok ret= obj;
			ret.tok= m + 1;
			return ret;
		}
	}

	JsonTok null= {};
	return null;
}

bool json_is_null(JsonTok obj)
{ return obj.tok == NULL; }

bool json_is_string(JsonTok obj)
{ return obj.tok->type == JSMN_STRING; }

bool json_is_object(JsonTok obj)
{ return obj.tok->type == JSMN_OBJECT; }

bool json_is_array(JsonTok obj)
{ return obj.tok->type == JSMN_ARRAY; }

U32 json_member_count(JsonTok j)
{
	if (json_is_null(j))
		return 0;
	return j.tok->size;
}

JsonTok json_member(JsonTok j, U32 i)
{
	ensure(i < j.tok->size);
	JsonTok ret= j;
	ret.tok += 1;
	for (U32 k= 0; k < i; ++k)
		ret.tok += ret.tok->deep_size + 1;
	return ret;
}

bool json_is_same(JsonTok j, const char *str)
{
	ensure(	j.tok->type == JSMN_STRING ||
			j.tok->type == JSMN_PRIMITIVE);

	U32 i= 0;
	while (str[i] != 0 && i < j.tok->end - j.tok->start) {
		if (str[i] != j.json[j.tok->start + i])
			return false;
		++i;
	}
	return true;
}

void json_strcpy(char *dst, U32 max_len, JsonTok j)
{
	U32 i= 0;
	for (; i < j.tok->end - j.tok->start && i < max_len; ++i)
		dst[i]= j.json[i + j.tok->start];

	if (i < max_len)
		dst[i]= '\0';
	else
		dst[max_len - 1]= '\0';
}

U32 json_tok_len(JsonTok j)
{ return j.tok->end - j.tok->start; }

REVOLC_API const char * json_str(JsonTok j)
{
	if (j.tok->type != JSMN_STRING)
		return "<JSON OBJ>";
	return j.json + j.tok->start;
}

F64 json_real(JsonTok j)
{ return atof(j.json + j.tok->start); }

S64 json_integer(JsonTok j)
{ return atoll(j.json + j.tok->start); };

V2d json_v2(JsonTok j)
{
	return (V2d) {
		json_real(json_member(j, 0)),
		json_real(json_member(j, 1))
	};
}

V3d json_v3(JsonTok j)
{
	return (V3d) {
		json_real(json_member(j, 0)),
		json_real(json_member(j, 1)),
		json_real(json_member(j, 2))
	};
}

Qd json_q(JsonTok j)
{
	V3d axis= {
		json_real(json_member(j, 0)),
		json_real(json_member(j, 1)),
		json_real(json_member(j, 2))
	};
	F64 angle= json_real(json_member(j, 3));
	ensure(	isfinite(axis.x) &&
			isfinite(axis.y) &&
			isfinite(axis.z) &&
			isfinite(angle));
	return qd_by_axis(axis, angle);
}

T3d json_t3(JsonTok j)
{
	return (T3d) {
		json_v3(json_value_by_key(j, "scale")),
		json_q(json_value_by_key(j, "rot")),
		json_v3(json_value_by_key(j, "pos"))
	};
}
