#include "core/malloc.h"
#include "phys_world.h"

PhysWorld *create_physworld()
{
	PhysWorld *w= zero_malloc(sizeof(*w));
	if (g_env.phys_world == NULL)
		g_env.phys_world= w;

	w->space= cpSpaceNew();
	cpSpaceSetGravity(w->space, cpv(0, -100));

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
		cpFloat radius = 1;
		cpFloat mass = 1;
		cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);
		cpBody *ballBody = cpSpaceAddBody(w->space, cpBodyNew(mass, moment));
		cpBodySetPosition(ballBody, cpv(sin(w->next_body), 1+cos(w->next_body)));
		cpShape *ballShape = cpSpaceAddShape(w->space, cpCircleShapeNew(ballBody, radius, cpvzero));
		cpShapeSetFriction(ballShape, 0.7);

		w->bodies[w->next_body].body= ballBody;
		w->bodies[w->next_body].shape= ballShape;
	}

	++w->body_count;
	return w->next_body;
}

void free_rigidbody(PhysWorld *w, U32 h)
{
	ensure(h < MAX_RIGIDBODY_COUNT);

	RigidBody *b= &w->bodies[h];
	cpShapeFree(b->shape);
	cpBodyFree(b->body);

	*b= (RigidBody) { .allocated= false };
	--w->body_count;
}

void upd_physworld(PhysWorld *w, F32 dt)
{
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
}
