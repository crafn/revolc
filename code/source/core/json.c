#include "json.h"

#include <stdlib.h>

ParsedJsonFile malloc_parsed_json_file(const char *file)
{
	ParsedJsonFile ret= {};
	U32 file_size;
	ret.json_path= file;
	ret.json= (char*)malloc_file(file, &file_size);
	ret.null_json= (char*)malloc_file(file, &file_size);
	ret.root.json_size= file_size;

	{ // Parse json
		U32 token_count= file_size/4 + 64; // Intuition
		ret.tokens= malloc(sizeof(jsmntok_t)*token_count);
		jsmn_parser parser;
		jsmn_init(&parser);
		int r= jsmn_parse(&parser, ret.json, file_size,
				ret.tokens, token_count);
		switch (r) {
			case JSMN_ERROR_NOMEM:
				critical_print("Too large JSON file (engine problem): %s",
						file);
				goto error;
			break;
			case JSMN_ERROR_INVAL:
				critical_print("JSON syntax error: %s",
						file);
				goto error;
			break;
			case JSMN_ERROR_PART:
				critical_print("Unexpected JSON end: %s",
						file);
				goto error;
			break;
			case 0:
				critical_print("Empty JSON file: %s",
						file);
				goto error;
			break;
			default: ensure(r > 0);
		}

		ret.root.json_path= file;
		ret.root.json_dir= malloc_path_to_dir(file);
		ret.root.json= ret.json;
		ret.root.null_json= ret.null_json;
		ret.root.tok= ret.tokens;

		{ // Terminate strings and primitives so that they're easier to handle
			for (U32 i= 1; i < r; ++i) {
				if (	ret.tokens[i].type == JSMN_STRING ||
						ret.tokens[i].type == JSMN_PRIMITIVE)
					ret.null_json[ret.tokens[i].end]= '\0';
			}
		}
	}

	return ret;

error:
	free_parsed_json_file(ret);

	ParsedJsonFile null= {};
	return null;
}

void free_parsed_json_file(ParsedJsonFile json)
{
	free(json.json);
	free(json.null_json);
	free(json.tokens);
	free((char*)json.root.json_dir);
}

JsonTok json_value_by_key(JsonTok obj, const char *key)
{
	ensure(obj.tok->type == JSMN_OBJECT);
	jsmntok_t *m= obj.tok + 1;
	for (U32 i= 0; i < obj.tok->size; ++i, m += m->deep_size + 1) {
		if (m->type != JSMN_STRING)
			continue;

		if (!strcmp(key, obj.null_json + m->start)) {
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

bool json_is_number(JsonTok obj)
{ return	obj.tok &&
			obj.json[obj.tok->start] >= '0' &&
			obj.json[obj.tok->start] <= '9'; }

JsonType json_type(JsonTok j)
{
	if (json_is_string(j))
		return JsonType_string;
	if (json_is_object(j))
		return JsonType_object;
	if (json_is_array(j))
		return JsonType_array;
	if (json_is_number(j))
		return JsonType_number;

	fail("Unknown JsonTok type");
}

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
	return j.null_json + j.tok->start;
}

F64 json_real(JsonTok j)
{ return atof(j.null_json + j.tok->start); }

S64 json_integer(JsonTok j)
{ return atoll(j.null_json + j.tok->start); };

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

Color json_color(JsonTok j)
{
	return (Color) {
		json_real(json_member(j, 0)),
		json_real(json_member(j, 1)),
		json_real(json_member(j, 2)),
		json_real(json_member(j, 3)),
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

//
// wjson
//

WJson *wjson_create(JsonType type)
{
	WJson *new_member= dev_malloc(sizeof(*new_member));
	*new_member= (WJson) {.type= type};
	return new_member;
}

void wjson_destroy(WJson *j)
{
	if (!j)
		return;

	WJson *member= j->last_member;
	while (member) {
		WJson* prev= member->prev;
		wjson_destroy(member);
		member= prev;
	}

	if (j->type == JsonType_string) {
		dev_free(j->string);
		j->string= NULL;
	}

	dev_free(j);
}

internal
void * wjson_init_string(WJson *j_str, const char *str)
{
	ensure(j_str->string == NULL);
	j_str->type= JsonType_string;
	U32 size= strlen(str) + 1;
	j_str->string= dev_malloc(size);
	snprintf(j_str->string, size, "%s", str);
	return j_str;
}

void wjson_append(WJson *j, WJson *item)
{
	item->prev= j->last_member;
	j->last_member= item;

	if (item->prev)
		item->prev->next= item;

	if (!j->first_member)
		j->first_member= item;

	++j->member_count;
}

internal
WJson * wjson_new_member(WJson *j)
{
	WJson *new_member= wjson_create(JsonType_object);
	wjson_append(j, new_member);
	return new_member;
}

WJson * wjson_named_member(WJson *j, JsonType t, const char *name)
{
	WJson *j_str= wjson_new_member(j);
	wjson_init_string(j_str, name);
	WJson *j_member= wjson_new_member(j_str);
	j_member->type= t;
	return j_member;
}

WJson * wjson_number(F64 n)
{
	WJson *j_number= wjson_create(JsonType_number);
	j_number->number= n;
	return j_number;
}

WJson * wjson_v3(V3d vec)
{
	WJson *j_v3= wjson_create(JsonType_array);
	wjson_append(j_v3, wjson_number(vec.x));
	wjson_append(j_v3, wjson_number(vec.y));
	wjson_append(j_v3, wjson_number(vec.z));
	return j_v3;
}

internal
void wjson_dump_recurse(WJson *j, U32 depth)
{
	for (U32 i= 0; i < depth; ++i)
		printf("  ");
	switch (j->type) {
	case JsonType_object: debug_print("{"); break;
	case JsonType_array: debug_print("["); break;
	case JsonType_string:
		if (j->last_member)
			debug_print("\"%s\" : ", j->string);
		else
			debug_print("\"%s\",", j->string);
	break;
	case JsonType_number: debug_print("%g,", j->number); break;
	default: fail("Unknown value");
	}

	WJson *member= j->first_member;
	while (member) {
		wjson_dump_recurse(member, depth + 1);
		member= member->next;
	}

	if (	j->type == JsonType_object ||
			j->type == JsonType_array) {
		for (U32 i= 0; i < depth; ++i)
			printf("  ");

		if (j->type == JsonType_object)
			debug_print("},");
		else
			debug_print("],");
	}
}

void wjson_dump(WJson *j)
{
	debug_print("WJson dump");
	wjson_dump_recurse(j, 0);
}

// Single substitution to json string
typedef struct JsonSubs {
	U32 dst_begin, dst_end; // Indices to json string
	WJson *src;
} JsonSubs;

// Adds substitution
internal
void wjson_add_sub(	JsonSubs *subs, U32 *count, U32 max_count,
					JsonTok in, WJson *upd)
{
	ensure(max_count > *count);

	// Substituting strings should chomp the whole `"str" : [1, 2],`
	/// @todo This probably disappears when fields have partial substitution
	int begin_offset= 0;
	int end_offset= 0;
	if (json_is_string(in)) {
		begin_offset= -1;
		if (json_member_count(in) == 1)
			end_offset= json_member(in, 0).tok->end - in.tok->end + 1;
	}

	JsonSubs s= {
		.dst_begin= in.tok->start + begin_offset,
		.dst_end= in.tok->end + end_offset,
		.src= upd,
	};
	subs[*count]= s;
	++*count;
}

// Find ranges which will be substituted by modified values
internal
void wjson_find_subs(	JsonSubs *subs,
						U32 *sub_count,
						U32 max_sub_count,
						JsonTok in, WJson *upd)
{
	ensure(upd->type == json_type(in) && "Json type mismatch");

	switch (upd->type) {
	case JsonType_object: {
		WJson *upd_member= upd->first_member;
		while (upd_member) {
			ensure(upd_member->type == JsonType_string);
			bool found= false;
			for (U32 i= 0; i < json_member_count(in); ++i) {
				JsonTok in_member= json_member(in, i);
				ensure(json_is_string(in_member));

				if (!strcmp(upd_member->string, json_str(in_member))) {
					wjson_find_subs(subs, sub_count, max_sub_count,
									in_member, upd_member);
					found= true;
					break;
				}
			}
			if (!found) {
				// Add "substitution" to the end of object
				ensure(max_sub_count > *sub_count);
				JsonSubs s= {
					.dst_begin= in.tok->end - 1,
					.dst_end= in.tok->end - 1,
					.src= upd_member,
				};
				subs[*sub_count]= s;
				++*sub_count;
			}
			upd_member= upd_member->next;
		}
	} break;
	case JsonType_array: {
		if (json_member_count(in) == upd->member_count) {
			WJson *upd_member= upd->first_member;
			U32 i= 0;
			while (upd_member) {
				wjson_find_subs(subs, sub_count, max_sub_count,
								json_member(in, i), upd_member);
				upd_member= upd_member->next;
				++i;
			}
		} else {
			// Just overwrite everything in the array if count has changed
			wjson_add_sub(subs, sub_count, max_sub_count, in, upd);
		}
	} break;
	case JsonType_number: {
		wjson_add_sub(subs, sub_count, max_sub_count, in, upd);
	} break;
	case JsonType_string: {
		/// @todo Don't just overwrite string members, should inspect
		wjson_add_sub(subs, sub_count, max_sub_count, in, upd);
	} break;
	default: fail("Unknown JsonType: %i", upd->type);
	}
}

internal
void wjson_write(FILE *file, WJson *j)
{
	switch (j->type) {
	case JsonType_object: file_printf(file, "{"); break;
	case JsonType_array: file_printf(file, "["); break;
	case JsonType_string:
		if (j->last_member)
			file_printf(file, "\"%s\" : ", j->string);
		else
			file_printf(file, "\"%s\"", j->string);
	break;
	case JsonType_number: file_printf(file, "%g", j->number); break;
	default: fail("Unknown value");
	}

	WJson *member= j->first_member;
	while (member) {
		wjson_write(file, member);
		member= member->next;
	}

	if (	j->type == JsonType_object ||
			j->type == JsonType_array) {
		if (j->type == JsonType_object)
			file_printf(file, "}");
		else
			file_printf(file, "]");
	}
	if (j->type == JsonType_string && j->first_member) {
		// Nop, comma is printed by member
	} else {
		file_printf(file, ",");
	}
	if (j->next)
		file_printf(file, " ");
}

internal
int sub_index_cmp(const void *a_, const void *b_)
{
	JsonSubs *a= (JsonSubs*)a_;
	JsonSubs *b= (JsonSubs*)b_;
	return (a->dst_begin > b->dst_begin) - (a->dst_begin < b->dst_begin);
}

internal
void wjson_write_with_subs(FILE *file, const char *json, U32 json_size, JsonSubs *subs, U32 sub_count)
{
	qsort(subs, sub_count, sizeof(*subs), sub_index_cmp);

	U32 last_write_end= 0;
	for (U32 i= 0; i < sub_count; ++i) {
		U32 pre_sub_count= subs[i].dst_begin - last_write_end;

		// Stuff before the substitution
		file_write(file, json + last_write_end, pre_sub_count);

		// Substitution
		wjson_write(file, subs[i].src);
		last_write_end= subs[i].dst_end;
	}

	// Rest of the original file
	ensure(last_write_end <= json_size);
	file_write(file, json + last_write_end, json_size - last_write_end);
}

void wjson_write_updated(const char *path, JsonTok input, WJson *upd)
{
	const U32 max_subs= 1024*10;
	JsonSubs subs[max_subs];
	U32 sub_count= 0;
	wjson_find_subs(subs, &sub_count, max_subs, input, upd);

	FILE *file= fopen(path, "wb");
	ensure(file);
	wjson_write_with_subs(file, input.json, input.json_size, subs, sub_count);
	fclose(file);
}

