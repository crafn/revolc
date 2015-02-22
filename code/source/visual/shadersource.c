#include "core/debug_print.h"
#include "core/string.h"
#include "core/file.h"
#include "platform/gl.h"
#include "resources/resblob.h"
#include "shadersource.h"

void init_shadersource(ShaderSource *shd)
{
	const GLchar* vs_src= blob_ptr(&shd->res, shd->vs_src_offset);
	const GLchar* gs_src= blob_ptr(&shd->res, shd->gs_src_offset);
	const GLchar* fs_src= blob_ptr(&shd->res, shd->fs_src_offset);

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

void deinit_shadersource(ShaderSource *shd)
{
	gl_destroy_shader_prog(
			&shd->prog_gl_id,
			&shd->vs_gl_id,
			&shd->gs_gl_id,
			&shd->fs_gl_id);
}

int json_shadersource_to_blob(BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	char *vs_src= NULL; /// @warning Not null-terminated!
	char *fs_src= NULL; /// @warning Not null-terminated!
	char *vs_total_path= NULL;
	char *fs_total_path= NULL;

	JsonTok j_vs_file= json_value_by_key(j, "vs_file");
	JsonTok j_fs_file= json_value_by_key(j, "fs_file");

	if (json_is_null(j_vs_file)) {
		critical_print("Attrib 'vs_file' missing");
		goto error;
	}

	if (json_is_null(j_fs_file)) {
		critical_print("Attrib 'fs_file' missing");
		goto error;
	}

	vs_total_path= malloc_joined_path(j.json_path, json_str(j_vs_file));
	fs_total_path= malloc_joined_path(j.json_path, json_str(j_fs_file));

	U32 vs_src_len;
	U32 fs_src_len;
	vs_src= malloc_file(vs_total_path, &vs_src_len);
	fs_src= malloc_file(fs_total_path, &fs_src_len);

	BlobOffset vs_src_offset= buf->offset + sizeof(ShaderSource) - sizeof(Resource);
	BlobOffset gs_src_offset= 0;
	BlobOffset fs_src_offset= vs_src_offset + vs_src_len + 1;
	MeshType mesh_type= MeshType_tri;
	U32 cached= 0;
	U8 null_byte= 0;

	blob_write(buf, &vs_src_offset, sizeof(vs_src_offset));
	blob_write(buf, &gs_src_offset, sizeof(gs_src_offset));
	blob_write(buf, &fs_src_offset, sizeof(fs_src_offset));
	blob_write(buf, &mesh_type, sizeof(mesh_type));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, vs_src, vs_src_len);
	blob_write(buf, &null_byte, 1);
	blob_write(buf, fs_src, fs_src_len);
	blob_write(buf, &null_byte, 1);

cleanup:
	free(vs_total_path);
	free(fs_total_path);
	free(vs_src);
	free(fs_src);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}

