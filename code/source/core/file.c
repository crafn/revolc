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
