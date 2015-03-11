#ifndef REVOLC_PLATFORM_FILE_H
#define REVOLC_PLATFORM_FILE_H

#include "build.h"

REVOLC_API void copy_file(const char *dst, const char *src);
REVOLC_API void delete_file(const char *file);

#endif // REVOLC_PLATFORM_FILE_H
