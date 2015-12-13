#ifndef REVOLC_PLATFORM_STDLIB_H
#define REVOLC_PLATFORM_STDLIB_H

// C standard library plus some related utilities

#ifndef CODEGEN
#	include <math.h>
#	include <stdarg.h>
#	include <stdbool.h>
#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>
#	include <stddef.h>
#	include <time.h>
#endif

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef signed char S8;
typedef signed short S16;
typedef	signed int S32;
typedef	signed long long S64;

typedef float F32;
typedef double F64;

// Typically 4bn handles (index) is enough
typedef U32 Handle;
// Typically 4bn ids (unique) is not enough
typedef U64 Id;

// Usage HANDLE(Sprite) foo;
#define HANDLE(type) U32

typedef void (*VoidFunc)();

#define U8_MAX 255
#define U16_MAX 65535
#define U32_MAX 4294967295
#define S32_MAX 2147483647
#define S32_MIN (-2147483648)

#define ORIG_TYPE_U32 U32
#define ORIG_TYPE_U64 U64

#if PLATFORM_BITNESS == 32
	typedef U32 PtrInt;
#elif PLATFORM_BITNESS == 64
	typedef U64 PtrInt;
#else
#	error Unknown platform bitness
#endif

#define internal static
#define local_persist static

#define MAX_ALIGNMENT 16
#define ALIGNED(x) __attribute__((aligned(x)))
#define PACKED __attribute__((packed))
#define WARN_UNUSED __attribute__((warn_unused_result))
#define NORETURN __attribute__((noreturn))

#define WITH_DEREF_SIZEOF(x) x, sizeof(*(x))
#define WITH_STR_SIZE(x) x, (strlen(x) + 1)
#define WITH_SIZEOF(x) x, (sizeof(x))
#define ARRAY_COUNT(x) (sizeof(x)/sizeof(*x))
#define WITH_ARRAY_COUNT(x) x, (sizeof(x)/sizeof(*x))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(v, a, b) (MAX(MIN(v, b), a))
#define ABS(v) ((v) > 0 ? (v) : -(v))
#define SQR(x) ((x)*(x))
#define SIGN(x) ((x) > 0 ? 1 : -1)
#define NULL_HANDLE ((U32)-1)
#define NULL_ID ((U64)-1)

#define TO_STRING_IND(X) #X
#define TO_STRING(X) TO_STRING_IND(X)

#define JOIN2_IND(A, B) A##B
#define JOIN2(A, B) JOIN2_IND(A, B)

#define JOIN3_IND(A, B, C) A##B##C
#define JOIN3(A, B, C) JOIN3_IND(A, B, C)

// @todo Generate from every typedef
#define ORIG_TYPE_Id U64
#define ORIG_TYPE_Handle U32
// @todo Generate
#define LC_U32 u32
#define LC_U64 u64

// ORIG_TYPE(typedeffed_int) -> int
#define ORIG_TYPE(T) JOIN2(ORIG_TYPE_, T)

// LC(U64) -> u64
#define LC(T) JOIN2(LC_, T)

#define MEMBER_SIZE(st, m) (sizeof(((st*)0)->m))
#define MEMBER_OFFSET(st, m) (offsetof(st, m))

// Format string
// Will always (when size > 0) insert '\0' at the end of `str`
// Use instead of snprintf which is horribly broken on mingw (due to microsoft CRT)
REVOLC_API int fmt_str(char *str, U32 size, const char *fmt, ...);
REVOLC_API int v_fmt_str(char *str, U32 size, const char *fmt, va_list args);

/// e.g. ("../folder/", "foo/bar") == "../folder/foo/bar"
/// e.g. ("../folder/file", "foo") == "../folder/foo"
/// @note	If this seems non-obvious later, make more specific function.
///			Removing file from `a` is required in making paths from relative
///			to json file easily.
REVOLC_API void joined_path(char *dst, const char *a, const char *b);

REVOLC_API void path_to_dir(char *dst, const char *path_to_file);

REVOLC_API bool is_str_end(const char *str, const char *end);

// Creates a string which exists only this frame
REVOLC_API char * frame_str(const char *fmt, ...);

static void toggle_bool(bool *b)
{ *b = !*b; }

// Declares a "pointer" relative to its own address, handy in binary blobs. 
// Macro because we'll want to have type-safe custom preprocessing soon
#define REL_PTR(type) RelPtr

typedef struct RelPtr {
	U64 value;
} RelPtr;

static void set_rel_ptr(RelPtr *rel_ptr, const void *ptr)
{ rel_ptr->value = (U8*)ptr - (U8*)rel_ptr; }

static void * rel_ptr(const RelPtr *rel_ptr)
{ return (U8*)rel_ptr + rel_ptr->value; }

// File stuff

REVOLC_API bool file_exists(const char *path);
REVOLC_API void * malloc_file(const char *path, U32 *file_size);

REVOLC_API void file_write(FILE *f, const void *data, U32 size);
REVOLC_API void file_printf(FILE *f, const char *fmt, ...);

REVOLC_API void copy_file(const char *dst, const char *src);
REVOLC_API void delete_file(const char *file);

#endif // REVOLC_PLATFORM_STDLIB_H
