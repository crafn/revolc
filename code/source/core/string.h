#ifndef REVOLC_CORE_STRING_H
#define REVOLC_CORE_STRING_H

#include "build.h"
#include "platform/stdlib.h"

/// e.g. ("../folder/", "foo/bar") == "../folder/foo/bar"
/// e.g. ("../folder/file", "foo") == "../folder/foo"
/// @note	If this seems non-obvious later, make more specific function.
///			Removing file from `a` is required in making paths from relative
///			to json file easily.
REVOLC_API void joined_path(char *dst, const char *a, const char *b);

REVOLC_API void path_to_dir(char *dst, const char *path_to_file);

REVOLC_API bool is_str_end(const char *str, const char *end);

// Creates a string which exists only this frame
REVOLC_API char * frame_str(const char *fmt, ...);

#endif // REVOLC_CORE_STRING_H
