#include "slidedoor.h"

void upd_slidedoor(SlideDoor *door, SlideDoor *door_end, RigidBody *body, RigidBody *body_end)
{
	for (; door != door_end; ++door, ++body) {
		apply_groove_joint(body, door->anchor_open, door->anchor_closed);

		V2d dir = normalized_v2d(sub_v2d(door->anchor_open, door->anchor_closed));
		if (door->open)
			apply_force(body, scaled_v2d(door->open_force, dir));
		else
			apply_force(body, scaled_v2d(-door->close_force, dir));
	}
}
