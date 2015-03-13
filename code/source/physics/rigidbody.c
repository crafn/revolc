#include "rigidbody.h"

void apply_torque(RigidBody *b, F64 torque)
{
	cpBodySetTorque(b->cp_body, cpBodyGetTorque(b->cp_body) + torque);
}
