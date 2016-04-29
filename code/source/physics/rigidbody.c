#include "core/archive.h"
#include "chipmunk_util.h"
#include "global/env.h"
#include "rigidbody.h"

void apply_force(RigidBody *b, V2d force)
{ cpBodySetForce(b->cp_body, cpvadd(cpBodyGetForce(b->cp_body), to_cpv(force))); }

void apply_force_at(RigidBody *b, V2d force, V2d world_point)
{ cpBodyApplyForceAtWorldPoint(b->cp_body, to_cpv(force), to_cpv(world_point)); }

void apply_torque(RigidBody *b, F64 torque)
{ cpBodySetTorque(b->cp_body, cpBodyGetTorque(b->cp_body) + torque); }

void apply_impulse_local(RigidBody *b, V2d i, V2d p)
{ cpBodyApplyImpulseAtLocalPoint(b->cp_body, to_cpv(i), to_cpv(p)); }

void apply_impulse_at(RigidBody *b, V2d i, V2d p)
{ cpBodyApplyImpulseAtWorldPoint(b->cp_body, to_cpv(i), to_cpv(p)); }

void rigidbody_set_velocity(RigidBody *b, V2d v)
{ cpBodySetVelocity(b->cp_body, to_cpv(v)); }

F64 rigidbody_mass(const RigidBody *b)
{ return cpBodyGetMass(b->cp_body); }

V2d rigidbody_velocity_at(const RigidBody *b, V2d world_point)
{
	return from_cpv(cpBodyGetVelocityAtWorldPoint(
				b->cp_body, to_cpv(world_point)));
}

Constraint *add_simplemotor(RigidBody *b)
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

void apply_slide_joint(RigidBody *body, V2d body_p, V2d ground_p, F64 min, F64 max)
{
	JointInfo info = {
		.type = JointType_slide,
		.body_a = body->cp_body,
		.body_b = g_env.physworld->cp_ground_body,
		.anchor_a_1 = body_p,
		.anchor_b = ground_p,
		.min = min,
		.max = max,
	};
	push_array(JointInfo)(&g_env.physworld->used_joints, info);
}

void apply_groove_joint(RigidBody *body, V2d ground_p_1, V2d ground_p_2)
{
	JointInfo info = {
		.type = JointType_groove,
		.body_a = g_env.physworld->cp_ground_body,
		.body_b = body->cp_body,
		.anchor_a_1 = ground_p_1,
		.anchor_a_2 = ground_p_2,
	};
	push_array(JointInfo)(&g_env.physworld->used_joints, info);
}

internal V2d world_to_local_rigidbody(RigidBody *body, V2d world_point)
{
	return from_cpv(cpBodyWorldToLocal(body->cp_body, to_cpv(world_point)));
}

void apply_spring_joint(	RigidBody *a, RigidBody *b, V2d a_p, V2d b_p,
							F64 length, F64 stiffness, F64 damping)
{
	JointInfo info = {
		.type = JointType_spring,
		.body_a = a->cp_body,
		.body_b = b->cp_body,
		.anchor_a_1 = world_to_local_rigidbody(a, a_p),
		.anchor_b = world_to_local_rigidbody(b, b_p),
		.length = length,
		.stiffness = stiffness,
		.damping = damping,
	};
	push_array(JointInfo)(&g_env.physworld->used_joints, info);
}

V2d apply_velocity_target(RigidBody *b, V2d velocity, F64 max_force)
{
	V2d dif = sub_v2d(velocity, b->velocity);
	// @todo Could be optimal if dt was taken into account
	V2d force = scaled_v2d(50.0*rigidbody_mass(b), dif);

	if (length_sqr_v2d(force) > max_force*max_force)
		force = scaled_v2d(max_force, normalized_v2d(force));
	apply_force(b, force);

	return force;
}


void pack_rigidbody(	struct WArchive *ar,
						const RigidBody *begin,
						const RigidBody *end)
{
	for (const RigidBody *it = begin; it != end; ++it) {
		pack_strbuf(ar, it->def_name, sizeof(it->def_name));
		lossy_pack_t3d(ar, &it->tf);
		lossy_pack_v2d(ar, &it->velocity);
	}
}

void unpack_rigidbody(	struct RArchive *ar,
						RigidBody *begin,
						RigidBody *end)
{
	for (RigidBody *it = begin; it != end; ++it) {
		*it = (RigidBody) {};
		unpack_strbuf(ar, it->def_name, sizeof(it->def_name));
		lossy_unpack_t3d(ar, &it->tf);
		lossy_unpack_v2d(ar, &it->velocity);
	}
}
