#include "core/basic.h"
#include "core/debug.h"
#include "core/gl.h"
#include "resources/resblob.h"
#include "shadersource.h"

void init_shadersource(ShaderSource *shd)
{
	const GLchar* vs_src = rel_ptr(&shd->vs_src_offset);
	const GLchar* gs_src = rel_ptr(&shd->gs_src_offset);
	const GLchar* fs_src = rel_ptr(&shd->fs_src_offset);

	U32 attrib_count;
	const VertexAttrib *attribs;
	vertex_attributes(
			shd->mesh_type,
			&attribs,
			&attrib_count);

	U32* prog = &shd->prog_gl_id;
	U32* vs = &shd->vs_gl_id;
	U32* gs = &shd->gs_gl_id;
	U32* fs = &shd->fs_gl_id;

	{ // Vertex
		*vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(*vs, 1, &vs_src, NULL);
		glCompileShader(*vs);
		gl_check_shader_status(*vs, "vertex shader");
	}
	if (strlen(gs_src) > 0) { // Geometry
		*gs = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(*gs, 1, &gs_src, NULL);
		glCompileShader(*gs);
		gl_check_shader_status(*gs, "geometry shader");
	}
	if (strlen(fs_src) > 0) { // Fragment
		*fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(*fs, 1, &fs_src, NULL);
		glCompileShader(*fs);
		gl_check_shader_status(*fs, "fragment shader");
	}
	{ // Shader program
		*prog = glCreateProgram();
		glAttachShader(*prog, *vs);
		if (strlen(gs_src) > 0)
			glAttachShader(*prog, *gs);
		if (strlen(fs_src) > 0)
			glAttachShader(*prog, *fs);

		for (U32 i = 0; i < attrib_count; ++i)
			glBindAttribLocation(*prog, i, attribs[i].name);

		const char *varyings[MAX_SHADER_VARYING_COUNT];
		U32 varying_count = 0;
		for (U32 i = 0; i < MAX_SHADER_VARYING_COUNT; ++i) {
			if (strlen(shd->feedback_varyings[i]) == 0)
				continue;
			varyings[varying_count++] = shd->feedback_varyings[i];
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

ShaderSource *blobify_shadersource(struct WArchive *ar, Cson c, bool *err)
{
	ShaderSource *ptr = warchive_ptr(ar);
	char *vs_src = NULL; /// @warning Not null-terminated!
	char *gs_src = NULL; /// @warning Not null-terminated!
	char *fs_src = NULL; /// @warning Not null-terminated!

	Cson c_vs_file = cson_key(c, "vs_file");
	Cson c_gs_file = cson_key(c, "gs_file");
	Cson c_fs_file = cson_key(c, "fs_file");
	Cson c_varyings = cson_key(c, "feedback_varyings");

	if (cson_is_null(c_vs_file)) {
		critical_print("Attrib 'vs_file' missing");
		goto error;
	}

	char varyings[MAX_SHADER_VARYING_COUNT][RES_NAME_SIZE] = {};
	if (!cson_is_null(c_varyings)) {
		ensure(cson_member_count(c_varyings) < MAX_SHADER_VARYING_COUNT);
		for (U32 i = 0; i < cson_member_count(c_varyings); ++i) {
			Cson c_str = cson_member(c_varyings, i);
			fmt_str(varyings[i], sizeof(varyings[i]), "%s", blobify_string(c_str, err));
		}
	}

	char rel_vs_path[MAX_PATH_SIZE] = {0};
	char rel_gs_path[MAX_PATH_SIZE] = {0};
	char rel_fs_path[MAX_PATH_SIZE] = {0};
	fmt_str(rel_vs_path, sizeof(rel_vs_path), "%s", blobify_string(c_vs_file, err));
	if (!cson_is_null(c_gs_file))
		fmt_str(rel_gs_path, sizeof(rel_gs_path), "%s", blobify_string(c_gs_file, err));
	if (!cson_is_null(c_fs_file))
		fmt_str(rel_fs_path, sizeof(rel_fs_path), "%s", blobify_string(c_fs_file, err));

	char vs_total_path[MAX_PATH_SIZE];
	char gs_total_path[MAX_PATH_SIZE];
	char fs_total_path[MAX_PATH_SIZE];
	joined_path(vs_total_path, c.dir_path, blobify_string(c_vs_file, err));
	if (!cson_is_null(c_gs_file))
		joined_path(gs_total_path, c.dir_path, blobify_string(c_gs_file, err));
	if (!cson_is_null(c_fs_file))
		joined_path(fs_total_path, c.dir_path, blobify_string(c_fs_file, err));

	U32 vs_src_len = 0;
	U32 gs_src_len = 0;
	U32 fs_src_len = 0;
	vs_src = read_file(gen_ator(), vs_total_path, &vs_src_len);
	if (!cson_is_null(c_gs_file))
		gs_src = read_file(gen_ator(), gs_total_path, &gs_src_len);
	if (!cson_is_null(c_fs_file))
		fs_src = read_file(gen_ator(), fs_total_path, &fs_src_len);

	U64 vs_src_offset = ar->data_size + offsetof(ShaderSource, vs_src_offset);
	U64 gs_src_offset = ar->data_size + offsetof(ShaderSource, gs_src_offset);
	U64 fs_src_offset = ar->data_size + offsetof(ShaderSource, fs_src_offset);
	MeshType mesh_type = MeshType_tri;
	U32 cached = 0;
	U8 null_byte = 0;

	if (err && *err)
		goto error;

	// @todo Fill ShaderSource and write that instead of separate members
	Resource res;

	pack_buf(ar, &res, sizeof(res));
	pack_strbuf(ar, rel_vs_path, sizeof(rel_vs_path));
	pack_strbuf(ar, rel_gs_path, sizeof(rel_gs_path));
	pack_strbuf(ar, rel_fs_path, sizeof(rel_fs_path));
	pack_buf(ar, &vs_src_offset, sizeof(vs_src_offset));
	pack_buf(ar, &gs_src_offset, sizeof(gs_src_offset));
	pack_buf(ar, &fs_src_offset, sizeof(fs_src_offset));
	pack_buf(ar, &mesh_type, sizeof(mesh_type));
	pack_buf(ar, &varyings, sizeof(varyings));
	pack_buf(ar, &cached, sizeof(cached));
	pack_buf(ar, &cached, sizeof(cached));
	pack_buf(ar, &cached, sizeof(cached));
	pack_buf(ar, &cached, sizeof(cached));
	pack_patch_rel_ptr(ar, vs_src_offset);
	pack_buf(ar, vs_src, vs_src_len);
	pack_buf(ar, &null_byte, 1);
	pack_patch_rel_ptr(ar, gs_src_offset);
	pack_buf(ar, gs_src, gs_src_len);
	pack_buf(ar, &null_byte, 1);
	pack_patch_rel_ptr(ar, fs_src_offset);
	pack_buf(ar, fs_src, fs_src_len);
	pack_buf(ar, &null_byte, 1);

cleanup:
	free(vs_src);
	free(gs_src);
	free(fs_src);
	return ptr;

error:
	ptr = NULL;
	SET_ERROR_FLAG(err);
	goto cleanup;
}

void deblobify_shadersource(WCson *c, struct RArchive *ar)
{
	ShaderSource *shd = rarchive_ptr(ar, sizeof(*shd));
	unpack_advance(ar, shd->res.size);

	wcson_begin_compound(c, "ShaderSource");

	wcson_designated(c, "name");
	deblobify_string(c, shd->res.name);

	if (shd->rel_vs_file[0]) {
		wcson_designated(c, "vs_file");
		deblobify_string(c, shd->rel_vs_file);
	}

	if (shd->rel_gs_file[0]) {
		wcson_designated(c, "gs_file");
		deblobify_string(c, shd->rel_gs_file);
	}

	if (shd->rel_fs_file[0]) {
		wcson_designated(c, "fs_file");
		deblobify_string(c, shd->rel_fs_file);
	}

	wcson_end_compound(c);
}

