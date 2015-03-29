#include "string.h"
#include "core/malloc.h"

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
