#include "basic.h"
#include "core/debug.h"

int fmt_str(char *str, U32 size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret= v_fmt_str(str, size, fmt, args);
	va_end(args);
	return ret;
}


void joined_path(char *dst, const char *a, const char *b)
{
	U32 strip_len= 0;
	for (U32 i= strlen(a); i > 0; --i) {
		if (a[i - 1] == '/')
			break;
		++strip_len;
	}
	U32 stripped_a_len= strlen(a) - strip_len;

	U32 total_size= stripped_a_len +
					strlen(b) +
					strlen("/") + 1;
	ensure(total_size < MAX_PATH_SIZE);
	fmt_str(dst, MAX_PATH_SIZE, "%.*s/%s", stripped_a_len, a, b);
}

void path_to_dir(char *dst, const char *path_to_file)
{
	U32 strip_len= 0;
	for (U32 i= strlen(path_to_file); i > 0; --i) {
		if (path_to_file[i - 1] == '/')
			break;
		++strip_len;
	}
	U32 stripped_path_len= strlen(path_to_file) - strip_len;

	U32 total_size= stripped_path_len + 1;
	ensure(total_size < MAX_PATH_SIZE);
	fmt_str(dst, MAX_PATH_SIZE, "%.*s", stripped_path_len, path_to_file);
}

bool is_str_end(const char *str, const char *end)
{
	U32 str_len= strlen(str);
	U32 end_len= strlen(end);

	if (end_len > str_len)
		return false;

	for (U32 i= 0; i < end_len; ++i) {
		if (str[str_len - 1 - i] != end[end_len - 1 - i])
			return false;
	}
	return true;
}

char * frame_str(const char *fmt, ...)
{
	char *text= NULL;
	va_list args;
	va_list args_copy;

	va_start(args, fmt);
	va_copy(args_copy, args);
	U32 size= v_fmt_str(NULL, 0, fmt, args) + 1; 
	text= frame_alloc(size);
	v_fmt_str(text, size, fmt, args_copy);
	va_end(args_copy);
	va_end(args);
	return text;
}


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

#if PLATFORM == PLATFORM_LINUX
#	include <unistd.h>
#endif

void copy_file(const char *dst, const char *src)
{
	FILE *src_file= fopen(src, "rb");
	FILE *dst_file= fopen(dst, "wb");
	if (!src_file || !dst_file)
		goto error;

	char buffer[1024*10];
    U32 n;
    while ((n= fread(buffer, sizeof(char), sizeof(buffer), src_file)) > 0) {
        if (fwrite(buffer, sizeof(char), n, dst_file) != n)
			goto error;
    }

cleanup:
	fclose(src_file);
	fclose(dst_file);
	return;
error:
	fail("copy_file failed with '%s' -> '%s'", src, dst);
	goto cleanup;
}

void delete_file(const char *file)
{ remove(file); }
