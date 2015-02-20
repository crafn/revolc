#include "core/debug_print.h"
#include "core/ensure.h"
#include "platform/gl.h"
#include "texture.h"

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

