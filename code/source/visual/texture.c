#include "core/debug_print.h"
#include "core/ensure.h"
#include "platform/gl.h"
#include "texture.h"

#include <lodepng/lodepng.h>

void init_texture(Texture *tex)
{
	debug_print("Texture init: %s", tex->res.name);

	glGenTextures(1, &tex->gl_id);
	glBindTexture(GL_TEXTURE_2D, tex->gl_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); /// @todo Mipmaps

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.6);

	glTexImage2D(	GL_TEXTURE_2D,
					0,
					GL_RGBA8,
					tex->reso[0],
					tex->reso[1],
					0,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					(void*)tex->texels);
}

void deinit_texture(Texture *tex)
{
	debug_print("Texture deinit: %s", tex->res.name);
	glDeleteTextures(1, &tex->gl_id);
}

int json_texture_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	int return_value= 0;

	U8 *image= NULL;
	U32 width, height;
	int err=
		lodepng_decode32_file(&image, &width, &height, "../../resources/gamedata/barrel_color.png");
	if (err) {
		critical_print("PNG load error: %s", lodepng_error_text(err));
		goto error;
	}

	U16 reso[2]= {(U16)width, (U16)height};
	U32 gl_id= 0; // Cached

	blob_write(blob, offset, reso, sizeof(reso));
	blob_write(blob, offset, &gl_id, sizeof(gl_id));
	blob_write(blob, offset, image, width*height*4);

cleanup:
	free(image);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}

