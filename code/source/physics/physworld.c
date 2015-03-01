#include "global/env.h"
#include "chipmunk_util.h"
#include "core/malloc.h"
#include "physworld.h"
#include "visual/renderer.h" // Debug draw


void create_physworld()
{
	PhysWorld *w= zero_malloc(sizeof(*w));
	ensure(!g_env.phys_world);
	g_env.phys_world= w;

	w->space= cpSpaceNew();
	cpSpaceSetIterations(w->space, 10);
	cpSpaceSetGravity(w->space, cpv(0, -20));
}

void destroy_physworld()
{
	PhysWorld *w= g_env.phys_world;
	ensure(w);
	g_env.phys_world= NULL;
	cpSpaceFree(w->space);

	free(w);
}

internal
U32 alloc_rigidbody_noinit()
{
	PhysWorld *w= g_env.phys_world;

	if (w->body_count >= MAX_RIGIDBODY_COUNT)
		fail("Too many rigid bodies");

	while (w->bodies[w->next_body].allocated)
		w->next_body= (w->next_body + 1) % MAX_RIGIDBODY_COUNT;

	++w->body_count;
	return w->next_body;
}

internal
void set_rigidbody(U32 h, RigidBodyDef *def)
{
	PhysWorld *w= g_env.phys_world;

	ensure(h < MAX_RIGIDBODY_COUNT && w->bodies[h].cp_body == NULL);
	RigidBody *b= &w->bodies[h];
	strncpy(b->def_name, def->res.name, sizeof(b->def_name));
	b->shape_changed= true;

	/// @todo Mass & moment could be precalculated to ResBlob
	F64 total_mass= 0;
	F64 total_moment= 0;
	for (U32 i= 0; i < def->circle_count; ++i) {
		F32 mass= 1.0; /// @todo Mass from density	
		total_mass += mass;
		total_moment +=
			cpMomentForCircle(mass, 0,
				def->circles[i].rad, to_cpv(def->circles[i].pos));
	}

	for (U32 i= 0; i < def->poly_count; ++i) {
		Poly *poly= &def->polys[i];
		F32 mass= 1.0;

		cpVect cp_verts[poly->v_count];
		for (U32 v_i= 0; v_i < poly->v_count; ++v_i)
			cp_verts[v_i]= to_cpv(poly->v[v_i]);

		total_mass += mass;
		total_moment +=
			cpMomentForPoly(mass, poly->v_count, cp_verts, cpvzero, 0.0);
	}

	if (!b->is_static)
		b->cp_body= cpBodyNew(total_mass, total_moment);
	else
		b->cp_body= cpBodyNewStatic();
	cpSpaceAddBody(w->space, b->cp_body);
	cpBodySetPosition(b->cp_body, to_cpv((V2d) {b->pos.x, b->pos.y}));

	U32 circle_count= def->circle_count;
	U32 poly_count= def->poly_count;
	Circle *circles= def->circles;
	Poly *polys= def->polys;
	if (b->has_own_shape) {
		circle_count= b->circle_count;
		poly_count= b->poly_count;
		circles= b->circles;
		polys= b->polys;
	}

	for (U32 i= 0; i < circle_count; ++i) {
		b->cp_shapes[b->cp_shape_count++]=
			cpSpaceAddShape(
					w->space,
					cpCircleShapeNew(	b->cp_body,
										circles[i].rad,
										to_cpv(circles[i].pos)));
	}

	for (U32 i= 0; i < poly_count; ++i) {
		Poly *poly= &polys[i];
		F32 mass= 1.0;

		cpVect cp_verts[poly->v_count];
		for (U32 v_i= 0; v_i < poly->v_count; ++v_i)
			cp_verts[v_i]= to_cpv(poly->v[v_i]);

		total_mass += mass;
		total_moment +=
			cpMomentForPoly(mass, poly->v_count, cp_verts, cpvzero, 0.0);
		b->cp_shapes[b->cp_shape_count++]=
			cpSpaceAddShape(w->space,
					cpPolyShapeNew(	b->cp_body,
									poly->v_count,
									cp_verts,
									cpTransformIdentity,
									0.0));
	}

	for (U32 i= 0; i < b->cp_shape_count; ++i) {
		cpShape *shape= b->cp_shapes[i];
		cpShapeSetFriction(shape, 0.7);
		cpShapeSetElasticity(shape, 0.7);
	}
}

U32 resurrect_rigidbody(const RigidBody *dead)
{
	PhysWorld *w= g_env.phys_world;

	U32 h= alloc_rigidbody_noinit();
	w->bodies[h]= *dead;
	w->bodies[h].allocated= true;
	w->bodies[h].cp_body= NULL;
	w->bodies[h].cp_shape_count= 0;

	set_rigidbody(
			h,
			(RigidBodyDef*)res_by_name(
				g_env.res_blob,
				ResType_RigidBodyDef,
				dead->def_name));
	return h;
}


void free_rigidbody(U32 h)
{
	PhysWorld *w= g_env.phys_world;

	ensure(h < MAX_RIGIDBODY_COUNT);

	RigidBody *b= &w->bodies[h];
	for (U32 i= 0; i < b->cp_shape_count; ++i) {
		cpSpaceRemoveShape(w->space, b->cp_shapes[i]);
		cpShapeFree(b->cp_shapes[i]);
	}
	cpSpaceRemoveBody(w->space, b->cp_body);
	cpBodyFree(b->cp_body);

	*b= (RigidBody) { .allocated= false };
	--w->body_count;
}

void * storage_rigidbody()
{ return g_env.phys_world->bodies; }

internal
void phys_draw_circle(
		cpVect pos, cpFloat angle, cpFloat radius,
		cpSpaceDebugColor outlineColor,
		cpSpaceDebugColor fillColor,
		cpDataPointer data)
{
	const U32 v_count= 30;
	V2d v[v_count];
	for (U32 i= 0; i < v_count; ++i) {
		F64 a= angle + i*3.141*2.0/v_count;
		v[i].x= pos.x + cos(a)*radius;
		v[i].y= pos.y + sin(a)*radius;
	}
	v[0].x= v[0].x*0.9 + pos.x*0.1;
	v[0].y= v[0].y*0.9 + pos.y*0.1;

	Color c= {0.4, 0.7, 1.0, 0.5};
	ddraw_poly(
			c,
			v,
			v_count);
}

internal
void phys_draw_poly(
		int count, const cpVect *verts, cpFloat radius,
		cpSpaceDebugColor outlineColor, cpSpaceDebugColor fillColor,
		cpDataPointer data)
{
	V2d v[count];
	for (U32 i= 0; i < count; ++i) {
		v[i].x= verts[i].x;
		v[i].y= verts[i].y;
	}

	Color c= {0.4, 0.7, 1.0, 0.5};
	ddraw_poly(
			c,
			v,
			count);
}

void upd_physworld(F64 dt)
{
	PhysWorld *w= g_env.phys_world;

	dt= 1.0/60.0;

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		if (b->input_force.x != 0.0 || b->input_force.y != 0.0) {
			cpBodyApplyForceAtWorldPoint(
					b->cp_body,
					to_cpv(b->input_force),
					cpv(b->pos.x, b->pos.y));
			b->input_force= (V2d) {};
		}
	}
	/// @todo Accumulation
	cpSpaceStep(w->space, dt*0.5);
	cpSpaceStep(w->space, dt*0.5);

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		cpVect p= cpBodyGetPosition(b->cp_body);
		cpVect r= cpBodyGetRotation(b->cp_body);
		b->pos.x= p.x;
		b->pos.y= p.y;
		b->rot.cs= r.x;
		b->rot.sn= r.y;
	}

	if (w->debug_draw) {
		cpSpaceDebugDrawOptions options= {
			.drawCircle= phys_draw_circle,
			.drawPolygon= phys_draw_poly,
			.flags= CP_SPACE_DEBUG_DRAW_SHAPES,
		};
		cpSpaceDebugDraw(w->space, &options);

		Texel *grid= g_env.renderer->grid_ddraw_data;
		for (U32 i= 0; i < GRID_CELL_COUNT; ++i) {
			grid[i].r= MIN(w->grid[i].dynamic_portion*2, 255);
			grid[i].g= 0;
			grid[i].b= MIN(w->grid[i].static_portion*2, 255);
			grid[i].a= MIN(w->grid[i].static_portion*2 + w->grid[i].dynamic_portion*2, 255);
		}
	}
}

void post_upd_physworld()
{
	PhysWorld *w= g_env.phys_world;

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		if (	b->prev_pos.x != b->pos.x ||
				b->prev_pos.y != b->pos.y) {

			const U32 prev_i= GRID_INDEX(b->prev_pos.x, b->prev_pos.y);
			if (prev_i < GRID_CELL_COUNT) {
				GridCell *prev_c= &w->grid[prev_i];

				if (b->is_static)
					prev_c->static_portion -= 100;
				else
					prev_c->dynamic_portion -= 100;
			}

			const U32 cur_i= GRID_INDEX(b->pos.x, b->pos.y);
			if (cur_i < GRID_CELL_COUNT) {
				GridCell *cur_c= &w->grid[cur_i];

				if (b->is_static)
					cur_c->static_portion += 100;
				else
					cur_c->dynamic_portion += 100;
			}
		}

		b->shape_changed= false;
		b->prev_pos= b->pos;
	}
	
}
