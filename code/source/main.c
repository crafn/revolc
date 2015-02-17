#include <stdio.h>

#include "build.h"
#include "core/ensure.h"
#include "core/debug_print.h"
#include "core/vector.h"
#include "platform/device.h"
#include "platform/gl.h"
#include "resources/resblob.h"
#include "visual/mesh.h"
#include "visual/model.h"
#include "visual/texture.h"

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
 
		const U32 res_count= 2;
		cur_offset += fwrite(&res_count, 1, sizeof(res_count), file);
		BlobOffset res_offsets[2]= {24, 68};
		cur_offset += fwrite(&res_offsets[cur_res], 1, sizeof(res_offsets), file);

		{ // Texture
			debug_print("offset: %i", (int)cur_offset);
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
				{ 0x00, 0xFF, 0x00, 0xFF },
				{ 0xFF, 0x00, 0x00, 0xFF },
				{ 0x13, 0x37, 0x13, 0x37 },
			};
			cur_offset += fwrite(&data[0], 1, sizeof(data), file);
		}

		{ // Mesh
			debug_print("offset: %i", (int)cur_offset);
			Resource res= { ResType_Mesh, "squirrel_mesh" };
			cur_offset += fwrite(&res, 1, sizeof(res), file);

			MeshType type= MeshType_tri;
			cur_offset += fwrite(&type, 1, sizeof(type), file);

			const U32 v_count= 4;
			const U32 i_count= 6;
			cur_offset += fwrite(&v_count, 1, sizeof(v_count), file);
			cur_offset += fwrite(&i_count, 1, sizeof(i_count), file);

			TriMeshVertex data[4];
			data[1].pos.x= 1.0;
			data[2].pos.x= 1.0;
			data[2].pos.y= 1.0;
			data[3].pos.y= 1.0;
			cur_offset += fwrite(&data[0], 1, sizeof(data), file);
		}

		fclose(file);

		debug_print("Ready!");
	} else {
		Device d= plat_init("Revolc engine", 800, 600);

		ResBlob* blob= load_blob("resources.blob");
		print_resources(blob);

		while (!d.quit_requested) {
			plat_update(&d);
			F32 c_gl[2]= {
				2.0*d.cursor_pos[0]/d.win_size[0] - 1.0,
				-2.0*d.cursor_pos[1]/d.win_size[1] + 1.0,
			};

			glViewport(0, 0, d.win_size[0], d.win_size[1]);
			glClear(GL_COLOR_BUFFER_BIT);
			glColor3f(1.0, 0.0, 1.0);
			glLoadIdentity();
			glBegin(GL_QUADS);
				glVertex2f(0.0 + c_gl[0], 0.0 + c_gl[1]);
				glVertex2f(1.0 + c_gl[0], 0.0 + c_gl[1]);
				glVertex2f(1.0 + c_gl[0], 1.0 + c_gl[1]);
				glVertex2f(0.0 + c_gl[0], 1.0 + c_gl[1]);
			glEnd();

			gl_check_errors("loop");

			plat_sleep(1);
		}

		unload_blob(blob);
		plat_quit(&d);
	}

	return 0;
}
