#include "global/env.h"
#include "rigidbody.h"

void apply_torque(RigidBody *b, F64 torque)
{
	cpBodySetTorque(b->cp_body, cpBodyGetTorque(b->cp_body) + torque);
}

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

void remove_constraint(Constraint *c)
{
	cpSpaceRemoveConstraint(g_env.physworld->cp_space, c);
	cpConstraintFree(c);
}

