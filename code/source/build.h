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

// BUILD macro defined in compile command
#define BUILD_DEBUG 1
#define BUILD_DEV 2
#define BUILD_RELEASE 3

#define GUI_API REVOLC_API
#define GUI_BOOL bool

#define QC_API REVOLC_API

// @todo Don't include here
#include "core/basic.h"

#endif // REVOLC_BUILD_H
