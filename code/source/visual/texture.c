#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/string.h"
#include "platform/gl.h"
#include "texture.h"

#include <lodepng/lodepng.h>

int json_texture_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	U8 *image= NULL;
	U8 *flipped_image= NULL;
	char *total_path= NULL;

	JsonTok j_file= json_value_by_key(j, "file");
	if (json_is_null(j_file)) {
		critical_print("Attrib 'file' missing");
		goto error;
	}

	total_path= malloc_joined_path(j.json_path, json_str(j_file));

	const int comps= 4;
	U32 width, height;
	int err=
		lodepng_decode32_file(&image, &width, &height, total_path);
	if (err) {
		critical_print("PNG load error: %s", lodepng_error_text(err));
		goto error;
	}
	ensure(width > 0 && height > 0);

	// Invert y-axis
	
	flipped_image= malloc(width*height*comps);
	for (U32 y= 0; y < height; ++y) {
		memcpy(	flipped_image + y*width*comps,
				image + (height - 1 - y)*width*comps,
				width*comps);
	}	

	V2i reso= {width, height};
	V3f atlas_uv= {};

	blob_write(buf, &reso, sizeof(reso));
	blob_write(buf, &atlas_uv, sizeof(atlas_uv));
	blob_write(buf, flipped_image, width*height*comps);

cleanup:
	free(image);
	free(flipped_image);
	free(total_path);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}

