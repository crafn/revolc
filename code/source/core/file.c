#include "core/file.h"

bool file_exists(const char *path)
{
	FILE *file= fopen(path, "rb");
	if (file) {
		fclose(file);
		return true;
	}

	return false;
}

void * malloc_file(const char *path, U32 *file_size)
{
	FILE *file= fopen(path, "rb");
	if (!file)
		fail("Couldn't open file: %s", path);

	fseek(file, 0, SEEK_END);
	U32 size= ftell(file);
	fseek(file, 0, SEEK_SET);

	U8 *buf= malloc(size);
	U64 len= fread(buf, size, 1, file);
	if (len != 1)
		fail("Couldn't fully read file: %s", path);

	fclose(file);

	if (file_size)
		*file_size= size;

	return buf;
}

void file_write(FILE *f, const void *data, U32 size)
{
	int ret= fwrite(data, 1, size, f);
	if ((U32)ret != size)
		fail("fwrite failed");
}

void file_printf(FILE *f, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret= vfprintf(f, fmt, args);
	va_end(args);

	if (ret < 0)
		fail("vfprintf failed");
}
