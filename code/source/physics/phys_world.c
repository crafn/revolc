#include "global/env.h"
#include "core/malloc.h"
#include "phys_world.h"
#include "visual/renderer.h" // Debug draw

PhysWorld *create_physworld()
{
	PhysWorld *w= zero_malloc(sizeof(*w));
	if (g_env.phys_world == NULL)
		g_env.phys_world= w;

	w->space= cpSpaceNew();
	 cpSpaceSetIterations(w->space, 20);
	cpSpaceSetGravity(w->space, cpv(0, -20));

	w->ground=
		cpSegmentShapeNew(
				cpSpaceGetStaticBody(w->space),
				cpv(-20, 0), cpv(20, -1), 0);
	cpShapeSetFriction(w->ground, 1);
	cpSpaceAddShape(w->space, w->ground);

	return w;
}

void destroy_physworld(PhysWorld *w)
{
	cpSpaceRemoveShape(w->space, w->ground);
	cpShapeFree(w->ground);
	cpSpaceFree(w->space);

	if (g_env.phys_world == w)
		g_env.phys_world= NULL;
	free(w);
}

U32 alloc_rigidbody(PhysWorld *w)
{
	if (w->body_count >= MAX_RIGIDBODY_COUNT)
		fail("Too many rigid bodies");

	while (w->bodies[w->next_body].allocated)
		w->next_body= (w->next_body + 1) % MAX_RIGIDBODY_COUNT;

	RigidBody *b= &w->bodies[w->next_body];
	*b= (RigidBody) { .allocated= true };

	// Test init
	{
		cpFloat radius = 0.85;
		cpFloat mass = 1;
		cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);

		cpBody *body = cpSpaceAddBody(w->space, cpBodyNew(mass, moment));
		cpBodySetPosition(body, cpv(sin(w->next_body), 1+cos(w->next_body)));
	
		cpShape *shape= NULL;
		if (w->next_body % 2 == 0) {
			shape= cpSpaceAddShape(w->space, cpCircleShapeNew(body, radius, cpvzero));
		} else {
			cpVect rect[]= {
				cpv(-1, -1), cpv(-1, 1), cpv(1, 1), cpv(1, -1)
			};
			shape= cpSpaceAddShape(w->space, cpPolyShapeNew(body, 4, rect, cpTransformIdentity, 0.0));
		}
		cpShapeSetFriction(shape, 0.7);
		cpShapeSetElasticity(shape, 0.7);

		w->bodies[w->next_body].body= body;
		w->bodies[w->next_body].shape= shape;
	}

	++w->body_count;
	return w->next_body;
}

void free_rigidbody(PhysWorld *w, U32 h)
{
	ensure(h < MAX_RIGIDBODY_COUNT);

	RigidBody *b= &w->bodies[h];
	cpSpaceRemoveShape(w->space, b->shape);
	cpShapeFree(b->shape);
	cpSpaceRemoveBody(w->space, b->body);
	cpBodyFree(b->body);

	*b= (RigidBody) { .allocated= false };
	--w->body_count;
}

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
			g_env.renderer,
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
			g_env.renderer,
			c,
			v,
			count);
}

void upd_physworld(PhysWorld *w, F32 dt)
{
	if (dt > 1.0/30.0)
		dt= 1.0/30.0;

	/// @todo Accumulation
	cpSpaceStep(w->space, dt);

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		b->pos.x= cpBodyGetPosition(b->body).x;
		b->pos.y= cpBodyGetPosition(b->body).y;
		b->rot.cs= cpBodyGetRotation(b->body).x;
		b->rot.sn= cpBodyGetRotation(b->body).y;
	}
	
	if (w->debug_draw) {
		cpSpaceDebugDrawOptions options= {
			.drawCircle= phys_draw_circle,
			.drawPolygon= phys_draw_poly,
			.flags= CP_SPACE_DEBUG_DRAW_SHAPES,
		};
		cpSpaceDebugDraw(w->space, &options);
	}

}
