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

	{ // Vertex
		*vs= glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(*vs, 1, &vs_src, NULL);
		glCompileShader(*vs);
		gl_check_shader_status(*vs, "vertex shader");
	}
	if (strlen(gs_src) > 0) { // Geometry
		*gs= glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(*gs, 1, &gs_src, NULL);
		glCompileShader(*gs);
		gl_check_shader_status(*gs, "geometry shader");
	}
	if (strlen(fs_src) > 0) { // Fragment
		*fs= glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(*fs, 1, &fs_src, NULL);
		glCompileShader(*fs);
		gl_check_shader_status(*fs, "fragment shader");
	}
	{ // Shader program
		*prog= glCreateProgram();
		glAttachShader(*prog, *vs);
		if (strlen(gs_src) > 0)
			glAttachShader(*prog, *gs);
		if (strlen(fs_src) > 0)
			glAttachShader(*prog, *fs);

		for (U32 i= 0; i < attrib_count; ++i)
			glBindAttribLocation(*prog, i, attribs[i].name);

		const char *varyings[MAX_SHADER_VARYING_COUNT];
		U32 varying_count= 0;
		for (U32 i= 0; i < MAX_SHADER_VARYING_COUNT; ++i) {
			if (strlen(shd->feedback_varyings[i]) == 0)
				continue;
			varyings[varying_count++]= shd->feedback_varyings[i];
		}
		glTransformFeedbackVaryings(
						*prog,
						varying_count,
						varyings,
						GL_INTERLEAVED_ATTRIBS);

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

int json_shadersource_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	char *vs_src= NULL; /// @warning Not null-terminated!
	char *gs_src= NULL; /// @warning Not null-terminated!
	char *fs_src= NULL; /// @warning Not null-terminated!

	JsonTok j_vs_file= json_value_by_key(j, "vs_file");
	JsonTok j_gs_file= json_value_by_key(j, "gs_file");
	JsonTok j_fs_file= json_value_by_key(j, "fs_file");
	JsonTok j_varyings= json_value_by_key(j, "feedback_varyings");

	if (json_is_null(j_vs_file)) {
		critical_print("Attrib 'vs_file' missing");
		goto error;
	}

	char varyings[MAX_SHADER_VARYING_COUNT][RES_NAME_SIZE]= {};
	if (!json_is_null(j_varyings)) {
		ensure(json_member_count(j_varyings) < MAX_SHADER_VARYING_COUNT);
		for (U32 i= 0; i < json_member_count(j_varyings); ++i) {
			JsonTok j_str= json_member(j_varyings, i);
			fmt_str(varyings[i], sizeof(varyings[i]), "%s", json_str(j_str));
		}
	}

	char vs_total_path[MAX_PATH_SIZE];
	char gs_total_path[MAX_PATH_SIZE];
	char fs_total_path[MAX_PATH_SIZE];
	joined_path(	vs_total_path,
					j.json_path,
					json_str(j_vs_file));
	if (!json_is_null(j_gs_file))
		joined_path(	gs_total_path,
						j.json_path,
						json_str(j_gs_file));
	if (!json_is_null(j_fs_file))
		joined_path(	fs_total_path,
						j.json_path,
						json_str(j_fs_file));

	U32 vs_src_len= 0;
	U32 gs_src_len= 0;
	U32 fs_src_len= 0;
	vs_src= malloc_file(vs_total_path, &vs_src_len);
	if (!json_is_null(j_gs_file))
		gs_src= malloc_file(gs_total_path, &gs_src_len);
	if (!json_is_null(j_fs_file))
		fs_src= malloc_file(fs_total_path, &fs_src_len);

	BlobOffset vs_src_offset= buf->offset + sizeof(ShaderSource) - sizeof(Resource);
	BlobOffset gs_src_offset= vs_src_offset + vs_src_len + 1;
	BlobOffset fs_src_offset= gs_src_offset + gs_src_len + 1;
	MeshType mesh_type= MeshType_tri;
	U32 cached= 0;
	U8 null_byte= 0;

	blob_write(buf, &vs_src_offset, sizeof(vs_src_offset));
	blob_write(buf, &gs_src_offset, sizeof(gs_src_offset));
	blob_write(buf, &fs_src_offset, sizeof(fs_src_offset));
	blob_write(buf, &mesh_type, sizeof(mesh_type));
	blob_write(buf, &varyings, sizeof(varyings));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, &cached, sizeof(cached));
	blob_write(buf, vs_src, vs_src_len);
	blob_write(buf, &null_byte, 1);
	blob_write(buf, gs_src, gs_src_len);
	blob_write(buf, &null_byte, 1);
	blob_write(buf, fs_src, fs_src_len);
	blob_write(buf, &null_byte, 1);

cleanup:
	free(vs_src);
	free(gs_src);
	free(fs_src);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}

