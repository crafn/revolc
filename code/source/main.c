#include "build.h"
#include "core/ensure.h"
#include "core/debug_print.h"
#include "core/vector.h"
#include "global/env.h"
#include "platform/device.h"
#include "platform/gl.h"
#include "resources/resblob.h"
#include "visual/mesh.h"
#include "visual/model.h"
#include "visual/texture.h"
#include "visual/shader.h"
#include "visual/renderer.h"
#include "visual/vao.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

int main(int argc, const char **argv)
{
	if (argc > 1) {
		// Generate resources
		debug_print("Generating temp resources..");

		U32 cur_offset= 0;
		U32 cur_res= 0;

		FILE* file= fopen("resources.blob", "wb");

		// Header
		const U32 version= 2;
		cur_offset += fwrite(&version, 1, sizeof(version), file);
 
		const U32 res_count= 4;
		cur_offset += fwrite(&res_count, 1, sizeof(res_count), file);
		BlobOffset res_offsets[4]= {40, 84, 412, 464};
		cur_offset += fwrite(&res_offsets[cur_res], 1, sizeof(res_offsets), file);

		{ // Texture
			debug_print("offset: %i", (int)cur_offset);
			ensure(cur_offset == res_offsets[0]);
			Resource res= { ResType_Texture, "test_tex" };
			cur_offset += fwrite(&res, 1, sizeof(res), file);

			U16 width= 2;
			U16 height= 2;
			U32 cached= 0;
			cur_offset += fwrite(&width, 1, sizeof(width), file);
			cur_offset += fwrite(&height, 1, sizeof(height), file);
			cur_offset += fwrite(&cached, 1, sizeof(cached), file);

			Texel data[4]= {
				{ 0xFF, 0x00, 0x00, 0xFF },
				{ 0x00, 0x00, 0xFF, 0xFF },
				{ 0x00, 0x00, 0xFF, 0xFF },
				{ 0xFF, 0x00, 0x00, 0xFF },
			};
			cur_offset += fwrite(&data[0], 1, sizeof(data), file);
		}

		{ // Mesh
			debug_print("offset: %i", (int)cur_offset);
			ensure(cur_offset == res_offsets[1]);
			Resource res= { ResType_Mesh, "squirrel_mesh" };
			cur_offset += fwrite(&res, 1, sizeof(res), file);

			MeshType type= MeshType_tri;
			cur_offset += fwrite(&type, 1, sizeof(type), file);

			const U32 v_count= 4;
			const U32 i_count= 6;
			cur_offset += fwrite(&v_count, 1, sizeof(v_count), file);
			cur_offset += fwrite(&i_count, 1, sizeof(i_count), file);

			BlobOffset v_offset= 132;
			BlobOffset i_offset= 388;
			cur_offset += fwrite(&v_offset, 1, sizeof(v_offset), file);
			cur_offset += fwrite(&i_offset, 1, sizeof(i_offset), file);

			TriMeshVertex vertices[4]= {};
			vertices[1].pos.x= 0.7;
			vertices[1].uv.x= 1.0;

			vertices[2].pos.x= 1.0;
			vertices[2].pos.y= 0.7;
			vertices[2].uv.x= 1.0;
			vertices[2].uv.y= 1.0;

			vertices[3].pos.y= 1.0;
			vertices[3].uv.y= 1.0;

			debug_print("  mesh data offset: %i", (int)cur_offset);
			ensure(cur_offset == v_offset);
			cur_offset += fwrite(&vertices[0], 1, sizeof(vertices), file);

			MeshIndexType indices[6]= {
				0, 1, 2, 0, 2, 3
			};

			debug_print("  mesh data offset: %i", (int)cur_offset);
			ensure(cur_offset == i_offset);
			cur_offset += fwrite(&indices[0], 1, sizeof(indices), file);
		}

		{ // Model

			debug_print("offset: %i", (int)cur_offset);
			ensure(cur_offset == res_offsets[2]);
			Resource res= { ResType_Model, "squirrel_model" };
			cur_offset += fwrite(&res, 1, sizeof(res), file);

			BlobOffset texs[3]= {res_offsets[0], 0, 0};
			BlobOffset mesh= res_offsets[1];
			cur_offset += fwrite(&texs[0], 1, sizeof(texs), file);
			cur_offset += fwrite(&mesh, 1, sizeof(mesh), file);
		}

		{ // Shader
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

			BlobOffset vs_src_offset= cur_offset + sizeof(Shader);
			BlobOffset gs_src_offset= 0;
			BlobOffset fs_src_offset= vs_src_offset + strlen(vs_src) + 1;

			debug_print("offset: %i", (int)cur_offset);
			ensure(cur_offset == res_offsets[3]);
			Resource res= { ResType_Shader, "gen_shader" };
			cur_offset += fwrite(&res, 1, sizeof(res), file);

			cur_offset += fwrite(&vs_src_offset, 1, sizeof(vs_src_offset), file);
			cur_offset += fwrite(&gs_src_offset, 1, sizeof(gs_src_offset), file);
			cur_offset += fwrite(&fs_src_offset, 1, sizeof(fs_src_offset), file);

			MeshType mesh_type= MeshType_tri;
			cur_offset += fwrite(&mesh_type, 1, sizeof(mesh_type), file);

			U32 cached= 0;
			cur_offset += fwrite(&cached, 1, sizeof(cached), file);
			cur_offset += fwrite(&cached, 1, sizeof(cached), file);
			cur_offset += fwrite(&cached, 1, sizeof(cached), file);
			cur_offset += fwrite(&cached, 1, sizeof(cached), file);

			ensure(cur_offset == vs_src_offset);
			cur_offset += fwrite(vs_src, 1, strlen(vs_src) + 1, file);

			ensure(cur_offset == fs_src_offset);
			cur_offset += fwrite(fs_src, 1, strlen(fs_src) + 1, file);
		}

		fclose(file);

		debug_print("Ready!");
	} else {
		Device d= plat_init("Revolc engine", 800, 600);

		ResBlob* blob= load_blob("resources.blob");
		print_blob(blob);

		Renderer* rend= create_renderer();

		Model* model= (Model*)resource_by_name(blob, ResType_Model, "squirrel_model");
#define ENTITY_COUNT 10
		U32 entity_handles[ENTITY_COUNT];
		for (int i= 0; i < ENTITY_COUNT; ++i) {
			U32 h= create_modelentity(rend, model);
			entity_handles[i]= h;
		}

		F32 time= 0;
		while (!d.quit_requested) {
			plat_update(&d);
			F32 c_gl[2]= {
				2.0*d.cursor_pos[0]/d.win_size[0] - 1.0,
				-2.0*d.cursor_pos[1]/d.win_size[1] + 1.0,
			};
			time += d.dt;

			for (int i= 0; i < ENTITY_COUNT; ++i) {
				ModelEntity *e= get_modelentity(rend, entity_handles[i]);
				e->pos.z= 2.0 + i*0.5 + sin(i + time);
				e->pos.x= sin(i + time*0.7)*2.0;
			}

			Texture* tex= (Texture*)resource_by_name(blob, ResType_Texture, "test_tex");
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex->gl_id);

			Shader* shd= (Shader*)resource_by_name(blob, ResType_Shader, "gen_shader");
			glUseProgram(shd->prog_gl_id);
			glUniform1i(glGetUniformLocation(shd->prog_gl_id, "u_tex_color"), 0);
			glUniform2f(glGetUniformLocation(shd->prog_gl_id, "u_cursor"), c_gl[0], c_gl[1]);

			glViewport(0, 0, d.win_size[0], d.win_size[1]);
			glClear(GL_COLOR_BUFFER_BIT);

			render_frame(rend);

			gl_check_errors("loop");

			plat_sleep(1);
		}

		destroy_renderer(rend);

		unload_blob(blob);
		plat_quit(&d);
	}

	return 0;
}
