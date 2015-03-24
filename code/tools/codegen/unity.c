#include "main.c"

#include <sparse/cse.c>
//#include <sparse/dissect.c> // Piles of compile errors on MinGW, and doesn't seem necessary
#include <sparse/evaluate.c>
#include <sparse/expand.c>
#include <sparse/expression.c>
#include <sparse/flow.c>
#include <sparse/inline.c>
#include <sparse/liveness.c>
#include <sparse/memops.c>
#include <sparse/pre-process.c>
#include <sparse/ptrlist.c>
#include <sparse/show-parse.c>
#include <sparse/simplify.c>
#include <sparse/sort.c>
#include <sparse/storage.c>
#include <sparse/symbol.c>
#include <sparse/char.c>
#include <sparse/scope.c>
#include <sparse/target.c>
#include <sparse/unssa.c>
#include <sparse/lib.c>
#include <sparse/allocate.c>
#include <sparse/tokenize.c>
#include <sparse/parse.c>
#include <sparse/linearize.c>

#if defined(__linux__)
#	include <sparse/compat-linux.c>
#elif defined(__MINGW32__)
#	include <sparse/compat-mingw.c>
#endif