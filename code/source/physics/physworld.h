#ifndef REVOLC_PHYSICS_PHYS_WORLD_H
#define REVOLC_PHYSICS_PHYS_WORLD_H

#include "build.h"
#include "core/grid.h"
#include "physgrid.h"
#include "rigidbody.h"
#include "rigidbodydef.h"

#include <chipmunk/chipmunk.h>

typedef enum JointType {
	JointType_slide,
	JointType_groove,
	JointType_last,
} JointType;

typedef struct JointInfo {
	JointType type;
	cpBody *body_a;
	cpBody *body_b;
	V2d anchor_a_1;
	V2d anchor_a_2;
	V2d anchor_b;
	F64 min;
	F64 max;

	cpConstraint *cp_joint;
} JointInfo;

DECLARE_ARRAY(JointInfo)

typedef struct PhysWorld {
	bool debug_draw;

	RigidBody bodies[MAX_RIGIDBODY_COUNT];
	U32 next_body, body_count;

	PhysGrid grid;

	cpSpace *cp_space;
	cpBody *cp_ground_body;
	RigidBody ground_body; // So that every cpShape has a RigidBody

	Array(JointInfo) used_joints; // Joints used this frame
	Array(JointInfo) existing_joints;
} PhysWorld;

/// @note Sets g_env.physworld
REVOLC_API void create_physworld();
REVOLC_API void destroy_physworld();

// @todo overwrite_rigidbody
REVOLC_API U32 resurrect_rigidbody(const RigidBody *dead);
REVOLC_API void free_rigidbody(Handle h);
REVOLC_API void * storage_rigidbody();
REVOLC_API RigidBody * get_rigidbody(U32 h);
REVOLC_API Handle rigidbody_handle(RigidBody *b);

REVOLC_API U32 resurrect_physgrid(const PhysGrid *dead);
REVOLC_API void *storage_physgrid();

REVOLC_API void upd_physworld(F64 dt);
REVOLC_API void post_upd_physworld();
REVOLC_API void upd_phys_rendering();

typedef struct QueryInfo {
	RigidBody *body;
	F64 distance;
} QueryInfo;

REVOLC_API QueryInfo *query_bodies(U32 *count, V2d pos, F64 max_dist);

// 0 = empty, 1 = partial, 2 = full
REVOLC_API U32 grid_material_fullness_in_circle(V2d center, F64 rad, U8 material);
// Returns number of changed cells
REVOLC_API U32 set_grid_material_in_circle(V2d center, F64 rad, U8 material);
REVOLC_API GridCell grid_cell(V2i vec);

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
