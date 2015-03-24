#ifndef REVOLC_CORE_FILE_H
#define REVOLC_CORE_FILE_H

#include "build.h"
#include "platform/io.h"

bool file_exists(const char *path);
void * malloc_file(const char *path, U32 *file_size);

void file_write(FILE *f, const void *data, U32 size);
void file_printf(FILE *f, const char *fmt, ...);

#endif // REVOLC_CORE_FILE_H
