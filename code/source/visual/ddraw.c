#include "ddraw.h"
#include "global/env.h"
#include "renderer.h"

void ddraw_poly(Color c, V3d *poly, U32 count, S32 layer)
{
	Renderer *r = g_env.renderer;

	if (r->ddraw_v_count + count > MAX_DEBUG_DRAW_VERTICES)
		critical_print("ddraw_poly: Too many vertices");
	if (r->ddraw_i_count + count > MAX_DEBUG_DRAW_INDICES)
		critical_print("ddraw_poly: Too many indices");

	Texture *white =
		(Texture*)res_by_name(
			g_env.resblob,
			ResType_Texture,
			"white");
	V3f atlas_uv = white->atlas_uv.uv;
	atlas_uv.x += 0.5*white->reso.x/TEXTURE_ATLAS_WIDTH;
	atlas_uv.y += 0.5*white->reso.y/TEXTURE_ATLAS_WIDTH;

	U32 start_index = r->ddraw_v_count;
	for (U32 i = 0; i < count; ++i) {
		TriMeshVertex v = default_vertex();
		v.pos = v3d_to_v3f(poly[i]);
		v.uv = atlas_uv;
		v.outline_uv = (V2f) {0.5, 0.5};
		v.color = c;
		v.outline_color = c;
		r->ddraw_v[r->ddraw_v_count++] = v;
	}

	U32 start_i_index = r->ddraw_i_count;
	for (U32 i = 0; i < count - 2; ++i) {
		r->ddraw_i[r->ddraw_i_count++] = 0;
		r->ddraw_i[r->ddraw_i_count++] = i + 1;
		r->ddraw_i[r->ddraw_i_count++] = i + 2;
	}

	drawcmd(	identity_t3d(),
				&r->ddraw_v[start_index], count,
				&r->ddraw_i[start_i_index], (count - 2)*3,
				((Texture*)res_by_name(g_env.resblob, ResType_Texture, "white"))->atlas_uv,
				c,
				c,
				layer,
				0.0,
				true);
}

void ddraw_circle(Color c, V3d p, F64 rad, S32 layer)
{
	const U32 v_count = 15;
	V3d v[v_count];
	for (U32 i = 0; i < v_count; ++i) {
		F64 a = i*3.141*2.0/v_count;
		v[i].x = p.x + cos(a)*rad;
		v[i].y = p.y + sin(a)*rad;
		v[i].z = 0.0;
	}
	ddraw_poly(c, v, v_count, layer);
}

void ddraw_line(Color c, V3d a, V3d b, F64 w, S32 layer)
{
	F64 scale = screen_to_world_size((V2i) {1, 0}).x*w;
	F64 rot = atan2(a.y - b.y, a.x - b.x);
	F64 c1 = cos(rot - PI/2)*scale;
	F64 s1 = sin(rot - PI/2)*scale;
	F64 c2 = cos(rot + PI/2)*scale;
	F64 s2 = sin(rot + PI/2)*scale;
	V3d quad[4] = {
		{a.x + c1, a.y + s1, a.z},
		{b.x + c1, b.y + s1, b.z},
		{b.x + c2, b.y + s2, b.z},
		{a.x + c2, a.y + s2, a.z},
	};
	ddraw_poly(c, quad, 4, layer);
}

void ddraw_dot(Color c, V3d p, F64 w, S32 layer)
{
	F64 rad = 0.5*screen_to_world_size((V2i) {1, 0}).x*w;
	V3d quad[4] = {
		{p.x - rad, p.y - rad},
		{p.x + rad, p.y - rad},
		{p.x + rad, p.y + rad},
		{p.x - rad, p.y + rad},
	};
	ddraw_poly(c, quad, 4, layer);
}
