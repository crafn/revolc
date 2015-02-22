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

#define DEFAULT_RES_FILE "../../resources/gamedata/test.res"

int main(int argc, const char **argv)
{
	Device *d= plat_init("Revolc engine", 800, 600);

	make_blob("main.blob", DEFAULT_RES_FILE);
	ResBlob* blob= g_env.res_blob= load_blob("main.blob");
	print_blob(blob);

	Renderer *rend= create_renderer();

	Model *model= (Model*)res_by_name(blob, ResType_Model, "woodenbarrel");
#define ENTITY_COUNT 500
	U32 entity_handles[ENTITY_COUNT];
	for (int i= 0; i < ENTITY_COUNT; ++i) {
		U32 h= create_modelentity(rend, model);
		entity_handles[i]= h;
	}

	F32 time= 0;
	while (!d->quit_requested) {
		plat_update(d);
		F32 cursor[2]= {
			2.0*d->cursor_pos[0]/d->win_size[0] - 1.0,
			-2.0*d->cursor_pos[1]/d->win_size[1] + 1.0,
		};
		time += d->dt;

		if (d->lmbDown) {
			make_blob("main.blob", DEFAULT_RES_FILE);
			blob= g_env.res_blob= reload_blob(blob, "main.blob");
		}

		for (int i= 0; i < ENTITY_COUNT; ++i) {
			ModelEntity *e= get_modelentity(rend, entity_handles[i]);
			e->pos.z= 2.0 + i*0.1 + sin(i + time);
			e->pos.x= sin(i + time*0.7)*3.0;
		}

		render_frame(rend, cursor[0]*3.0, cursor[1]*3.0);

		gl_check_errors("loop");

		plat_sleep(1);
	}

	destroy_renderer(rend);

	unload_blob(blob);
	g_env.res_blob= NULL;

	plat_quit(d);

	return 0;
}
