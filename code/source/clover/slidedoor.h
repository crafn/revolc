#ifndef CLOVER_SLIDEDOOR_H
#define CLOVER_SLIDEDOOR_H

#include "build.h"

typedef struct SlideDoor {
	bool open; // Wanted state
	V2d pos; // Closed position
	V2d delta_open;

	F64 open_force;
	F64 close_force;

	F64 fraction; // 0 when closed, 1 when open
	bool half_open_state; // Changes midway
} PACKED SlideDoor;

MOD_API void init_slidedoor(SlideDoor *d);
MOD_API void upd_slidedoor(SlideDoor *door, SlideDoor *door_end, RigidBody *body, RigidBody *body_end);

#endif
