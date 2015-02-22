#ifndef REVOLC_CORE_FILE_H
#define REVOLC_CORE_FILE_H

#include <stdlib.h>

bool file_exists(const char *path);
void * malloc_file(const char *path, U32 *file_size);

#endif // REVOLC_CORE_FILE_H
