#include "worldgen.h"
#include "physics/shapes.h"

// Thanks R.. http://stackoverflow.com/questions/19083566/what-are-the-better-pseudo-random-number-generator-than-the-lcg-for-lottery-sc
internal
U32 temper(U32 x)
{
    x ^= x>>11;
    x ^= x<<7 & 0x9D2C5680;
    x ^= x<<15 & 0xEFC60000;
    x ^= x>>18;
    return x;
}
U32 random_u32_base(U64 *seed)
{
    *seed = 6364136223846793005ULL * *seed + 1;
    return temper(*seed >> 32);
}

// Not production quality
U32 random_u32(U32 min, U32 max, U64 *seed)
{
	ensure(max > min);
	return random_u32_base(seed)%(max - min) + min;
}

// Not production quality
S32 random_s32(S32 min, S32 max, U64 *seed)
{
	ensure(max > min);
	return random_u32_base(seed)%(max - min) + min;
}

// Not production quality
internal
F64 random_f64(F64 min, F64 max, U64 *seed)
{
	ensure(max > min);
	F64 f= (F64)random_u32_base(seed)/U32_MAX;
	return f*(max - min) + min;
}

internal
F64 ground_surf_y(F64 x)
{
	return	sin(x*0.02 + 2.0)*(sin(x*0.075 + 0.3)*0.8 +
			sin(x*0.07)*2.0) + sin(x*0.0037 + 2.0)*(sin(x*0.02 + 0.1) + 1.0)*10.0 +
			0.3*sin(x*0.5)*sin(x*0.011 + 0.4) + sin(x*0.0001)*50;
}

internal
void try_spawn_ground(World *world, V2d pos)
{
	F64 l_g_y= ground_surf_y(pos.x - 0.5);
	F64 r_g_y= ground_surf_y(pos.x + 0.5);
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

	pos= add_v2d(pos, (V2d) {0.5, 0.5});
	bool true_var= true;
	SlotVal init_vals[]= {
		{"body",	"pos",				WITH_DEREF_SIZEOF(&pos)},
		{"body",	"is_static",		WITH_DEREF_SIZEOF(&true_var)},
		{"body",	"has_own_shape",	WITH_DEREF_SIZEOF(&true_var)},
		{"body",	"def_name",			WITH_STR_SIZE("block_dirt")},
		{"body",	"polys",			WITH_DEREF_SIZEOF(&poly)},
		{"body",	"poly_count",		WITH_DEREF_SIZEOF(&poly_count)},
		{"visual",	"model_name",		WITH_STR_SIZE("block_dirt")},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.res_blob, ResType_NodeGroupDef, "block");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), 0);
}

internal
void spawn_visual_prop(World *world, V3d pos, V3d scale, const char *name)
{
	SlotVal init_vals[]= {
		{"visual",	"pos",			WITH_DEREF_SIZEOF(&pos)},
		{"visual",	"scale",		WITH_DEREF_SIZEOF(&scale)},
		{"visual",	"model_name",	WITH_STR_SIZE(name)},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.res_blob, ResType_NodeGroupDef, "visual_prop");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), 0);
}

internal
void spawn_phys_prop(World *world, V2d pos, const char *name, bool is_static)
{
	local_persist U64 group_i= 0;
	group_i= (group_i + 1) % 3;

	SlotVal init_vals[]= {
		{"body",	"pos",			WITH_DEREF_SIZEOF(&pos)},
		{"body",	"is_static",	WITH_DEREF_SIZEOF(&is_static)},
		{"body",	"def_name",		WITH_STR_SIZE(name)},
		{"visual",	"model_name",	WITH_STR_SIZE(name)},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.res_blob, ResType_NodeGroupDef, "phys_prop");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), group_i);
}

void generate_world(World *w, U64 seed)
{
	spawn_visual_prop(w, (V3d) {0, 0, -500}, (V3d) {600, 1200, 1}, "sky_day");
	spawn_visual_prop(w, (V3d) {-100, -50, -490}, (V3d) {700, 250, 1}, "bg_mountain");

	spawn_visual_prop(w, (V3d) {20, -120, -100}, (V3d) {220, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {-70, -60, -200}, (V3d) {220, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {60, -80, -160}, (V3d) {200, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {150, -50, -300}, (V3d) {200, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {400, -70, -350}, (V3d) {200, 110, 1}, "bg_meadow");

	for (int y= -50; y <= 50; ++y) {
		for (int x= -50; x < 50; ++x) {
			V2d pos= {x, y};
			try_spawn_ground(w, pos);
		}
	}
	for (int i= 0; i < 20; ++i) {
			V2d pos= {
				random_f64(-30.0, 30.0, &seed),
				random_f64(0.0, 40.0, &seed),
			};
			if (pos.y < ground_surf_y(pos.x))
				continue;
			spawn_phys_prop(w, pos, "wbarrel", false);
			spawn_phys_prop(w, pos, "rollbot", false);
			spawn_phys_prop(w, pos, "wbox", false);
	}
}
