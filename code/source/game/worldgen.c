#include "worldgen.h"
#include "physics/shapes.h"
#include "core/random.h"

F64 ground_surf_y(F64 x)
{
	return	sin(x*0.02 + 2.0)*(sin(x*0.075 + 0.3)*0.8 +
			sin(x*0.07)*2.0) + sin(x*0.0037 + 2.0)*(sin(x*0.02 + 0.1) + 1.0)*10.0 +
			0.3*sin(x*0.5)*sin(x*0.011 + 0.4) + sin(x*0.0001)*50 - 7;
}

internal
void try_spawn_ground(World *world, V2d pos)
{
	F64 l_g_y= ground_surf_y(pos.x - 0.0) - 0.5;
	F64 r_g_y= ground_surf_y(pos.x + 1.0) - 0.5;
	if (pos.y - 0.5 > l_g_y &&
		pos.y - 0.5 > r_g_y)
		return; // Both lower corners over ground

	U8 poly_count= 1;
	Poly poly= {};
	poly.v[0]= (V2d) {-0.5, -0.5};
	poly.v[1]= (V2d) {+0.5, -0.5};
	poly.v[2]= (V2d) {+0.5, +0.5};
	poly.v[3]= (V2d) {-0.5, +0.5};
	poly.v_count= 4;
	
	F64 k= r_g_y - l_g_y;

	if (pos.y + 0.5 <= l_g_y &&
		pos.y + 0.5 <= r_g_y) {
		// Both upper corners under ground
		// Nop
	} else if (	pos.y + 0.5 > l_g_y &&
				pos.y + 0.5 > r_g_y) {
		// Neither upper corner under ground
		poly.v[2].y= r_g_y - pos.y;
		poly.v[3].y= l_g_y - pos.y;

		if (poly.v[2].y < -0.5) {
			// Right lower corner over ground
			poly.v[2].x= poly.v[1].x= 0.5 - (r_g_y - pos.y + 0.5)/k;

			poly.v[2].y= -0.5;
			poly.v[1].y= -0.5;
		}

		if (poly.v[3].y < -0.5) {
			// Left lower ground over ground
			poly.v[3].x= poly.v[0].x= -0.5 - (l_g_y - pos.y + 0.5)/k;

			poly.v[3].y= -0.5;
			poly.v[0].y= -0.5;
		}
	} else {
		// Other upper corner under ground
		poly.v[2].y= MIN(r_g_y - pos.y, 0.5);
		poly.v[3].y= MIN(l_g_y - pos.y, 0.5);

		// New vertex
		poly.v_count += 1;
		poly.v[4]= poly.v[3];

		// New vertex to intersection point
		poly.v[3]= (V2d) { -0.5 - (l_g_y - pos.y - 0.5)/k, 0.5 };
	}

	T3d tf= {{1, 1, 1}, identity_qd(), (V3d) {pos.x + 0.5, pos.y + 0.5, 0.0}};
	bool true_var= true;
	SlotVal init_vals[]= {
		{"body",	"tf",				WITH_DEREF_SIZEOF(&tf)},
		{"body",	"is_static",		WITH_DEREF_SIZEOF(&true_var)},
		{"body",	"has_own_shape",	WITH_DEREF_SIZEOF(&true_var)},
		{"body",	"def_name",			WITH_STR_SIZE("block_dirt")},
		{"body",	"polys",			WITH_DEREF_SIZEOF(&poly)},
		{"body",	"poly_count",		WITH_DEREF_SIZEOF(&poly_count)},
		{"visual",	"model_name",		WITH_STR_SIZE("block_dirt")},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "block");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), world->next_entity_id++);
}

void spawn_visual_prop(World *world, V3d pos, F64 rot, V3d scale, const char *name)
{
	T3d tf= {scale, qd_by_axis((V3d){0, 0, 1}, rot), pos};
	SlotVal init_vals[]= {
		{"visual",	"tf",			WITH_DEREF_SIZEOF(&tf)},
		{"visual",	"model_name",	WITH_STR_SIZE(name)},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "visual_prop");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), world->next_entity_id++);
}

void spawn_phys_prop(World *world, V2d pos, const char *name, bool is_static)
{
	T3d tf= {{1, 1, 1}, identity_qd(), (V3d) {pos.x, pos.y, 0}};
	SlotVal init_vals[]= {
		{"body",	"tf",			WITH_DEREF_SIZEOF(&tf)},
		{"body",	"is_static",	WITH_DEREF_SIZEOF(&is_static)},
		{"body",	"def_name",		WITH_STR_SIZE(name)},
		{"visual",	"model_name",	WITH_STR_SIZE(name)},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "phys_prop");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), world->next_entity_id++);
}

void generate_test_world(World *w)
{
	for (int y= 0; y < GRID_WIDTH_IN_CELLS; ++y) {
		for (int x= 0; x < GRID_WIDTH_IN_CELLS; ++x) {
			V2d wpos= {
				(x + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2,
				(y + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2,
			};
			if (ground_surf_y(wpos.x) < wpos.y)
				continue;
			if ((wpos.x + 14)*(wpos.x + 14) + wpos.y*wpos.y < 6*6)
				continue;
			g_env.physworld->grid[GRID_INDEX(x, y)].material= GRIDCELL_MATERIAL_GROUND;
		}
	}
	g_env.physworld->grid_modified= true;
}
