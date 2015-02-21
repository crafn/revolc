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
	Device d= plat_init("Revolc engine", 800, 600);

	make_blob("main.blob", "../../resources/gamedata/test.res");

	ResBlob* blob= load_blob("main.blob");
	print_blob(blob);

	Renderer* rend= create_renderer();

	Model* model= (Model*)resource_by_name(blob, ResType_Model, "woodenbarrel");
#define ENTITY_COUNT 100
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
			e->pos.x= sin(i + time*0.7)*3.0;
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

	return 0;
}
