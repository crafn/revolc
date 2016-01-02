#ifndef CLOVER_SLIDEDOOR_H
#define CLOVER_SLIDEDOOR_H

#include "build.h"

typedef struct SlideDoor {
	bool open; // Wanted state
	F64 open_force;
	F64 close_force;
	V2d anchor_open;
	V2d anchor_closed;
} PACKED SlideDoor;

REVOLC_API void upd_slidedoor(SlideDoor *door, SlideDoor *door_end, RigidBody *body, RigidBody *body_end);

#endif
