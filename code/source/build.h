#ifndef REVOLC_BUILD_H
#define REVOLC_BUILD_H

#define PLATFORM_LINUX 1
#define PLATFORM_WINDOWS 2

#if defined(__linux__)
#	define PLATFORM PLATFORM_LINUX
#else
#	define PLATFORM PLATFORM_WINDOWS
#endif

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
#define CLAMP(v, a, b) (MAX(MIN(v, b), a))
#define ABS(v) ((v) > 0 ? (v) : -(v))
#define NULL_HANDLE ((U32)-1)

#include "platform/types.h"

#endif // REVOLC_BUILD_H
