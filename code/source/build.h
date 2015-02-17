#ifndef REVOLC_BUILD_H
#define REVOLC_BUILD_H

/// @todo Other operating systems
/// @todo Other compilers
#define REVOLC_API __attribute__ ((visibility ("default")))

#define internal static
#define local_persist static

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

typedef enum { false, true } Bool;

#endif // REVOLC_BUILD_H
