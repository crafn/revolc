#include "core/debug_print.h"
#include "global/env.h"
#include "platform/gl.h"
#include "resources/resblob.h"
#include "shader.h"

void init_Shader(Shader *shd)
{
	const GLchar* vs_src= blob_ptr(g_env.res_blob, shd->vs_src_offset);
	const GLchar* gs_src= blob_ptr(g_env.res_blob, shd->gs_src_offset);
	const GLchar* fs_src= blob_ptr(g_env.res_blob, shd->fs_src_offset);

	debug_print("%s", vs_src);
	debug_print("%s", fs_src);

	gl_create_shader_prog(
		&shd->prog_gl_id, &shd->vs_gl_id, &shd->gs_gl_id, &shd->fs_gl_id,
		1,							&vs_src,
		shd->gs_src_offset != 0,	&gs_src,
		1,							&fs_src);
}

void deinit_Shader(Shader *shd)
{
	gl_destroy_shader_prog(
			&shd->prog_gl_id,
			&shd->vs_gl_id,
			&shd->gs_gl_id,
			&shd->fs_gl_id);
}
