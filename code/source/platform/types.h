#ifndef REVOLC_PLATFORM_TYPES_H
#define REVOLC_PLATFORM_TYPES_H

#ifndef CODEGEN
#	include <stdbool.h>
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

typedef void (*VoidFunc)();

#define U8_MAX 255
#define U32_MAX 4294967295
#define S32_MAX 2147483647
#define S32_MIN (-2147483648)

#endif // REVOLC_PLATFORM_TYPES_H
