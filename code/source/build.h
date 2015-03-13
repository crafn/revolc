#ifndef REVOLC_BUILD_H
#define REVOLC_BUILD_H

#include <stdbool.h>

/// @todo Other operating systems
/// @todo Other compilers

#define PLATFORM_LINUX 1
#define PLATFORM PLATFORM_LINUX

#define DLL_EXPORT __attribute__((visibility ("default")))
#define DLL_IMPORT __attribute__((visibility ("default")))

#ifdef MOD_DLL_BUILD
#	define REVOLC_API DLL_IMPORT
#	define MOD_API DLL_EXPORT
#else
#	define REVOLC_API DLL_EXPORT
#endif

#define internal static
#define local_persist static

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
#define U8_MAX 255
#define U32_MAX 4294967295
#define S32_MAX 2147483647
#define S32_MIN (-2147483648)
#define NULL_HANDLE ((U32)-1)

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

#endif // REVOLC_BUILD_H
