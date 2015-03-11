#include "core/ensure.h"
#include "file.h"

#include <unistd.h>

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
