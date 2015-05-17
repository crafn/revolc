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

void apply_velocity_target(RigidBody *b, V2d velocity, F64 max_force)
{
	V2d dif= sub_v2d(velocity, b->velocity);
	// @todo Could be optimal if dt was taken into account
	V2d force= scaled_v2d(50.0*rigidbody_mass(b), dif);

	if (length_sqr_v2d(force) > max_force*max_force)
		force= scaled_v2d(max_force, normalized_v2d(force));
	apply_force(b, force);
}

F64 rigidbody_mass(const RigidBody *b)
{
	return cpBodyGetMass(b->cp_body);
}
