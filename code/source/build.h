#ifndef REVOLC_BUILD_H
#define REVOLC_BUILD_H

#define PLATFORM_LINUX 1
#define PLATFORM_WINDOWS 2

#if defined(__linux__)
#	define PLATFORM PLATFORM_LINUX
#	define PLATFORM_BITNESS 64
#else
#	define PLATFORM PLATFORM_WINDOWS
#	if	defined(__WIN64)
#		define PLATFORM_BITNESS 64
#	else
#		define PLATFORM_BITNESS 32
#	endif
#endif

#define DLL_EXPORT __attribute__((visibility ("default")))
#define DLL_IMPORT __attribute__((visibility ("default")))

#ifdef MOD_DLL_BUILD
#	define REVOLC_API DLL_IMPORT
#	define MOD_API DLL_EXPORT
#else
#	define REVOLC_API DLL_EXPORT
#endif  

#if PLATFORM == PLATFORM_WINDOWS
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
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


#include "platform/types.h"

#endif // REVOLC_BUILD_H
