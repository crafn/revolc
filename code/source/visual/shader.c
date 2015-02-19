#include "core/debug_print.h"
#include "global/env.h"
#include "platform/gl.h"
#include "resources/resblob.h"
#include "shader.h"

void init_shader(Shader *shd)
{
	const GLchar* vs_src= blob_ptr(g_env.res_blob, shd->vs_src_offset);
	const GLchar* gs_src= blob_ptr(g_env.res_blob, shd->gs_src_offset);
	const GLchar* fs_src= blob_ptr(g_env.res_blob, shd->fs_src_offset);

	U32 attrib_count;
	const VertexAttrib *attribs;
	vertex_attributes(
			shd->mesh_type,
			&attribs,
			&attrib_count);

	U32* prog= &shd->prog_gl_id;
	U32* vs= &shd->vs_gl_id;
	U32* gs= &shd->gs_gl_id;
	U32* fs= &shd->fs_gl_id;
	U32 vs_count= 1;
	U32 gs_count= shd->gs_src_offset != 0;
	U32 fs_count= 1;

	{ // Vertex
		*vs= glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(*vs, vs_count, &vs_src, NULL);
		glCompileShader(*vs);
		gl_check_shader_status(*vs);
	}
	if (gs_count > 0) { // Geometry
		*gs= glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(*gs, gs_count, &gs_src, NULL);
		glCompileShader(*gs);
		gl_check_shader_status(*gs);
	}
	{ // Fragment
		*fs= glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(*fs, fs_count, &fs_src, NULL);
		glCompileShader(*fs);
		gl_check_shader_status(*fs);
	}
	{ // Shader program
		*prog= glCreateProgram();
		glAttachShader(*prog, *vs);
		if (gs_count > 0)
			glAttachShader(*prog, *gs);
		glAttachShader(*prog, *fs);

		for (U32 i= 0; i < attrib_count; ++i)
			glBindAttribLocation(*prog, i, attribs[i].name);

		glLinkProgram(*prog);
		gl_check_program_status(*prog);
	}
}

void deinit_shader(Shader *shd)
{
	gl_destroy_shader_prog(
			&shd->prog_gl_id,
			&shd->vs_gl_id,
			&shd->gs_gl_id,
			&shd->fs_gl_id);
}
