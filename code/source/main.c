#include "build.h"
#include "core/ensure.h"
#include "core/file.h"
#include "core/debug_print.h"
#include "core/vector.h"
#include "game/world.h"
#include "global/env.h"
#include "physics/physworld.h"
#include "platform/device.h"
#include "resources/resblob.h"
#include "visual/model.h"
#include "visual/renderer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define DEFAULT_RES_ROOT "../../resources/gamedata/"

internal
void make_main_blob()
{
	char **res_paths= plat_find_paths_with_end(DEFAULT_RES_ROOT, ".res");
	make_blob("main.blob", res_paths);
	for (U32 i= 0; res_paths[i]; ++i)
		free(res_paths[i]);
	free(res_paths);
}

internal
void spawn_entity(World *world, PhysWorld *phys_world, Renderer *rend, ResBlob *blob, V2d pos)
{
	local_persist U32 i= 0;
	++i;
	const char *name= "wbarrel";
	if (i % 3 == 1)
		name= "rollbot";
	else if (i % 3 == 2)
		name= "wbox";

	Model *model= (Model*)res_by_name(blob, ResType_Model, name);
	U32 b_node_h= alloc_node(world, NodeType_RigidBody);
	U32 m_node_h= alloc_node(world, NodeType_ModelEntity);

	U32 modelentity_h= node_impl_handle(world, m_node_h);
	set_modelentity(rend, modelentity_h, model);

	U32 body_h= node_impl_handle(world, b_node_h);
	set_rigidbody(phys_world, body_h,
			pos, 0.0,
			(RigidBodyDef*)res_by_name(blob, ResType_RigidBodyDef, name));

	add_routing(world,
			m_node_h, offsetof(ModelEntity, pos),
			b_node_h, offsetof(RigidBody, pos),
			sizeof(V3d));
	add_routing(world,
			m_node_h, offsetof(ModelEntity, rot),
			b_node_h, offsetof(RigidBody, rot),
			sizeof(Qd));
}

#define SAVEFILE_PATH "save.bin"
#define DEFAULT_BLOB_PATH "main.blob"

int main(int argc, const char **argv)
{
	Device *d= plat_init("Revolc engine", 800, 600);

	if (!file_exists(DEFAULT_BLOB_PATH))
		make_main_blob();

	ResBlob *blob= g_env.res_blob= load_blob(DEFAULT_BLOB_PATH);
	print_blob(blob);

	Renderer *rend= create_renderer();

	PhysWorld *phys_world= create_physworld();
	World *world= create_world();

	if (file_exists(SAVEFILE_PATH)) {
		load_world(world, SAVEFILE_PATH);
	} else {
		for (int i= 0; i < 50; ++i) {
			spawn_entity(
					world, phys_world, rend, blob,
					(V2d) {sin(i), 1 + cos(i)});
		}
	}

	while (!d->quit_requested) {
		plat_update(d);
		{ // User input
			V2d cursor= {
				2.0*d->cursor_pos[0]/d->win_size[0] - 1.0,
				-2.0*d->cursor_pos[1]/d->win_size[1] + 1.0
			};

			V2d cursor_on_world= screen_to_world_point(rend, cursor);

			F32 dt= d->dt;
			F32 spd= 25.0;
			if (d->keyDown['w'])
				rend->cam_pos.y += spd*dt;
			if (d->keyDown['a'])
				rend->cam_pos.x -= spd*dt;
			if (d->keyDown['s'])
				rend->cam_pos.y -= spd*dt;
			if (d->keyDown['d'])
				rend->cam_pos.x += spd*dt;

			if (d->keyDown['y'])
				rend->cam_pos.z -= spd*dt;
			if (d->keyDown['h'])
				rend->cam_pos.z += spd*dt;

			if (d->keyDown['e']) {
				spawn_entity(world, phys_world, rend, blob, cursor_on_world);
			}
			
			if (d->keyPressed['q'])
				phys_world->debug_draw= !phys_world->debug_draw;

			if (d->keyPressed[KEY_F12]) {
				make_main_blob();
				blob= g_env.res_blob= reload_blob(blob, DEFAULT_BLOB_PATH);
			}

			if (d->keyPressed[KEY_F5])
				save_world(world, SAVEFILE_PATH);
			if (d->keyPressed[KEY_F9]) {
				destroy_world(world);
				world= create_world();
				load_world(world, SAVEFILE_PATH);
			}

			local_persist cpBody *body= NULL;
			if (d->lmbDown) {
				cpVect p= {cursor_on_world.x, cursor_on_world.y};
				cpShape *shape=
					cpSpacePointQueryNearest(
							phys_world->space,
							p, 0.1,
							CP_SHAPE_FILTER_ALL, NULL);

				if (!body && shape) {
					body= cpShapeGetBody(shape);
				}

				if (body) {
					cpBodySetPosition(body, p);
					cpBodySetVelocity(body, cpv(0, 0));
				}
			} else if (body) {
				body= NULL;
			}
		}

		upd_physworld(phys_world, d->dt);
		upd_world(world, d->dt);
		render_frame(rend);

		gl_check_errors("loop");
		plat_sleep(1);
	}

	destroy_world(world);
	destroy_physworld(phys_world);
	destroy_renderer(rend);

	unload_blob(blob);
	g_env.res_blob= NULL;

	plat_quit(d);

	return 0;
}
