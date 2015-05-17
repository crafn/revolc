#include "chipmunk_util.h"
#include "global/env.h"
#include "rigidbody.h"

void apply_force(RigidBody *b, V2d force)
{ cpBodySetForce(b->cp_body, cpvadd(cpBodyGetForce(b->cp_body), to_cpv(force))); }

void apply_torque(RigidBody *b, F64 torque)
{ cpBodySetTorque(b->cp_body, cpBodyGetTorque(b->cp_body) + torque); }

void apply_impulse_local(RigidBody *b, V2d i, V2d p)
{ cpBodyApplyImpulseAtLocalPoint(b->cp_body, to_cpv(i), to_cpv(p)); }

void apply_impulse_world(RigidBody *b, V2d i, V2d p)
{ cpBodyApplyImpulseAtWorldPoint(b->cp_body, to_cpv(i), to_cpv(p)); }

Constraint * add_simplemotor(RigidBody *b)
{
	return cpSpaceAddConstraint(
			g_env.physworld->cp_space,
			cpSimpleMotorNew(	b->cp_body,
								cpSpaceGetStaticBody(g_env.physworld->cp_space),
								0.0)
			);
}

void set_simplemotor_rate(Constraint *c, F64 rate)
{ cpSimpleMotorSetRate(c, rate); }

void set_constraint_max_force(Constraint *c, F64 force)
{ cpConstraintSetMaxForce(c, force); }

void remove_constraint(Constraint *c)
{
	cpSpaceRemoveConstraint(g_env.physworld->cp_space, c);
	cpConstraintFree(c);
}

