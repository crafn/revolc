#ifndef REVOLC_CORE_STRING_H
#define REVOLC_CORE_STRING_H

#include <string.h>

/// e.g. ("../folder/", "foo/bar") == "../folder/foo/bar"
/// e.g. ("../folder/file", "foo") == "../folder/foo"
/// @note	If this seems non-obvious later, make more specific function.
///			Removing file from `a` is required in making paths from relative
///			to json file easily.
char * malloc_joined_path(const char *a, const char *b);

#endif // REVOLC_CORE_STRING_H
