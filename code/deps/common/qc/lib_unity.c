/*
	Include this file wherever you want the implementation of the library to be.
	Define wanted output backends (QC_BACKEND_* macros) before including.
*/

/* Library settings -- override defaults by defining them before including this file */

#ifndef QC_MALLOC
#	define QC_MALLOC malloc
#endif

#ifndef QC_FREE
#	define QC_FREE free
#endif

/* Implementation */

#include "ast.c"

#if defined(QC_BACKEND_C) || defined(QC_BACKEND_CUDA)
#	include "backend_c.c"
#endif

#if defined(QC_BACKEND_CUDA)
#	include "backend_cuda.c"
#endif

#include "core.c"
#include "parse.c"
#include "tokenize.c"
