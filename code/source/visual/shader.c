#include "core/debug_print.h"
#include "platform/gl.h"
#include "resources/resblob.h"
#include "shader.h"

void init_shader(Shader *shd, ResBlob *blob)
{
	const GLchar* vs_src= blob_ptr(blob, shd->vs_src_offset);
	const GLchar* gs_src= blob_ptr(blob, shd->gs_src_offset);
	const GLchar* fs_src= blob_ptr(blob, shd->fs_src_offset);

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

int json_shader_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	const char* vs_src=
		"#version 150 core\n"
		"in vec3 a_pos;"
		"in vec2 a_uv;"
		"uniform vec2 u_cursor;"
		"out vec2 v_uv;"
		"void main() {"
		"	v_uv= a_uv;"
		"	gl_Position= vec4((a_pos.xy + u_cursor)/(1.0 + a_pos.z), 0.0, 1.0);"
		"}\n";
	const char* fs_src=
		"#version 150 core\n"
		"uniform sampler2D u_tex_color;"
		"in vec2 v_uv;"
		"void main() { gl_FragColor= texture2D(u_tex_color, v_uv); }\n";

	BlobOffset vs_src_offset= *offset + sizeof(Shader) - sizeof(Resource);
	BlobOffset gs_src_offset= 0;
	BlobOffset fs_src_offset= vs_src_offset + strlen(vs_src) + 1;
	MeshType mesh_type= MeshType_tri;
	U32 cached= 0;

	blob_write(blob, offset, &vs_src_offset, sizeof(vs_src_offset));
	blob_write(blob, offset, &gs_src_offset, sizeof(gs_src_offset));
	blob_write(blob, offset, &fs_src_offset, sizeof(fs_src_offset));
	blob_write(blob, offset, &mesh_type, sizeof(mesh_type));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, vs_src, strlen(vs_src) + 1);
	blob_write(blob, offset, fs_src, strlen(fs_src) + 1);

	return 0;
}

