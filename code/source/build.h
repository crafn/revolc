#ifndef REVOLC_BUILD_H
#define REVOLC_BUILD_H

#include <stdbool.h>

/// @todo Other operating systems
#define PLATFORM_LINUX 1
#define PLATFORM PLATFORM_LINUX
/// @todo Other compilers
#define REVOLC_API __attribute__ ((visibility ("default")))

#define internal static
#define local_persist static

#define ALIGNED(x) __attribute__((aligned(x)))
#define PACKED __attribute__((packed))
#define WARN_UNUSED __attribute__((warn_unused_result))

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef signed char S8;
typedef short S16;
typedef	int S32;
typedef	long long S64;

typedef float F32;
typedef double F64;

#endif // REVOLC_BUILD_H
