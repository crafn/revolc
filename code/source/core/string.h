#ifndef REVOLC_CORE_STRING_H
#define REVOLC_CORE_STRING_H

#include <string.h>

/// e.g. ("../folder/", "foo/bar") == "../folder/foo/bar"
/// e.g. ("../folder/file", "foo") == "../folder/foo"
/// @note	If this seems non-obvious later, make more specific function.
///			Removing file from `a` is required in making paths from relative
///			to json file easily.
REVOLC_API char * malloc_joined_path(const char *a, const char *b);

REVOLC_API char * malloc_path_to_dir(const char *path_to_file);

REVOLC_API bool is_str_end(const char *str, const char *end);

#endif // REVOLC_CORE_STRING_H
