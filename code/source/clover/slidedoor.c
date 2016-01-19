#include "slidedoor.h"

void init_slidedoor(SlideDoor *d)
{
	*d = (SlideDoor) {
		.delta_open = (V2d) {0, 5},
		.open_force = 100,
		.close_force = 100,
	};
}

void upd_slidedoor(SlideDoor *door, RigidBody *body)
{
	V2d anchor_open = add_v2d(door->delta_open, door->pos);
	apply_groove_joint(body, anchor_open, door->pos);

	V2d dir = normalized_v2d(door->delta_open);
	if (door->open)
		apply_force(body, scaled_v2d(door->open_force, dir));
	else
		apply_force(body, scaled_v2d(-door->close_force, dir));

	V2d dif = sub_v2d(door->pos, v3d_to_v2d(body->tf.pos));
	door->fraction = length_v2d(dif)/length_v2d(door->delta_open);
	door->half_open_state = door->fraction > 0.5;
}

