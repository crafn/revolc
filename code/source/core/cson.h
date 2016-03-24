#ifndef REVOLC_CORE_CSON_H
#define REVOLC_CORE_CSON_H

#include "build.h"
#include "core/color.h"
#include "core/math.h"

// Api for traversing json-like subset of C99

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

#endif // REVOLC_CORE_CSON_H
