#include "string.h"
#include "core/malloc.h"

char * malloc_joined_path(const char *a, const char *b)
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
	char *path= malloc(total_size);
	snprintf(path, total_size, "%.*s/%s", stripped_a_len, a, b);
	return path;
}
