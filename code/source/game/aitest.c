#include "aitest.h"
#include "global/env.h"
#include "resources/resblob.h"
#include "visual/renderer.h"
#include "world.h"

internal
AiTest temptest_aitest_storage[MAX_NODE_COUNT];
internal
U32 next_aitest= 0;

U32 resurrect_aitest(const AiTest *dead)
{
	while (temptest_aitest_storage[next_aitest].allocated)
		next_aitest= (next_aitest + 1) % MAX_NODE_COUNT;
	temptest_aitest_storage[next_aitest].allocated= true;

	temptest_aitest_storage[next_aitest]= *dead;
	return next_aitest;
}

void free_aitest(U32 handle)
{
	temptest_aitest_storage[handle].allocated= false;
}

void * storage_aitest()
{ return temptest_aitest_storage; }

void upd_aitest(	AiTest *t,
					AiTest *e)
{
	V2d target= {0.0, 30.0};
	for (;t != e; ++t) {
		V2d p= {t->input_pos.x, t->input_pos.y};
		V2d dif= sub_v2d(target, p);
		F64 r2= length_sqr_v2d(dif);
		t->force= scaled_v2d(1000.0/(r2 + 10.0), dif);
	}
}

void rotate_modelentity(ModelEntity *e, U32 count)
{
	for (U32 i= 0; i < count; ++i, e++) {
		//e->rot.cs= cos(acos(e->rot.cs) + 0.01);
		//e->rot.sn= sin(asin(e->rot.sn) + 0.01);
	}
}


void poly_to_modelentity(	ModelEntity *e, ModelEntity *e_end,
							RigidBody *b, RigidBody *b_end)
{
	//debug_print("POLY_TO_MODELENTITY, %i", b->shape_changed);
	ensure(e_end - e == b_end - b);
	ensure(b->poly_count == 1 && b->circle_count == 0 && "@todo");

	for (; e != e_end; ++e, ++b) {
		V2d *poly= b->polys[0].v;

		U32 v_count= b->polys[0].v_count;
		U32 i_count= (v_count - 2)*3;

		ensure(v_count > 2);
		/// @todo Don't malloc in game logic!!!
		TriMeshVertex *vert= malloc(sizeof(*vert)*v_count);
		MeshIndexType *ind= malloc(sizeof(*ind)*i_count);

		{ // Calc mesh
			for (U32 i= 0; i < v_count; ++i) {
				TriMeshVertex v= {
					.pos= { .x= poly[i].x, .y= poly[i].y },
					.uv= {poly[i].x*0.99 + 0.5, poly[i].y*0.99 + 0.5},
					.color= { 1.0, 0.0, 0.0, 0.5 },
				};
				vert[i]= v;
			}

			U32 index= 0;
			for (U32 i= 0; i < v_count - 2; ++i) {
				ind[index++]= 0;
				ind[index++]= i + 1;
				ind[index++]= i + 2;
			}
			ensure(index == i_count);
		}

		{ // Switch mesh
			// Free old mesh
			if (e->has_own_mesh) {
				free(e->vertices);
				free(e->indices);
			}

			e->has_own_mesh= true;
			// Ownership transfer
			e->vertices= vert;
			e->indices= ind;
			e->mesh_v_count= v_count;
			e->mesh_i_count= i_count;
		}
	}
}
