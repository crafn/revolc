#include "chipmunk_util.h"
#include "core/color.h"
#include "core/memory.h"
#include "global/env.h"
#include "physworld.h"
#include "visual/renderer.h" // Debug draw

typedef struct PolyCell {
	U8 fill;
} PolyCell;
#define EPSILOND 0.000000000001
#define SWAP(type, x, y) do { type temp= x; x= y; y= temp; } while(0)

internal
PolyCell* rasterized_poly(V2i *rect_ll, V2i *rect_size, const Poly *poly)
{
	U32 v_count= poly->v_count;
	if (v_count < 3)
		fail("Poly should have more than 2 verts");

	// Bounding box
	V2i ll= {S32_MAX, S32_MAX};
	V2i tr= {S32_MIN, S32_MIN};
	for (U32 i= 0; i < v_count; ++i) {
		V2d p= poly->v[i];
		V2i cell_p= round_v2d_to_v2i(p);

		if (cell_p.x < ll.x)
			ll.x= cell_p.x;
		if (cell_p.y < ll.y)
			ll.y= cell_p.y;

		if (cell_p.x + 1 > tr.x)
			tr.x= cell_p.x + 1;
		if (cell_p.y + 1 > tr.y)
			tr.y= cell_p.y + 1;
	}

	*rect_ll= ll;
	*rect_size= sub_v2i(tr, ll);

	struct RowBounding {
		S32 min, max;
		bool set;
	} *bounds;

	const U32 row_count= rect_size->y;
	bounds= frame_alloc(sizeof(*bounds)*row_count);
	for (U32 i= 0; i < row_count; ++i) {
		bounds[i].min= 0;
		bounds[i].max= rect_size->x;
		bounds[i].set= false;
	}

	// Adjust bounds
	for (U32 i= 0; i < v_count; ++i) {
		V2d cur_p= poly->v[i];
		V2d next_p= poly->v[(i + 1) % v_count];
		F64 inv_slope= (next_p.x - cur_p.x)/(next_p.y - cur_p.y);

		// At which side are we
		bool is_left_side= cur_p.y > next_p.y;
		bool is_horizontal_segment= ABS(cur_p.y - next_p.y) < EPSILOND;

		S32 low_y= floor(cur_p.y + 0.5);
		S32 high_y= floor(next_p.y + 0.5);
		if (high_y < low_y)
			SWAP(S32, low_y, high_y);

		for (S32 y= low_y; y < high_y; ++y) {
			/// @todo Fix problems with horizontal segments
			// x = (y - y0)/k + x0
			F64 x_at_middle_y= (y - (cur_p.y + 0.5))*inv_slope + (cur_p.x + 0.5);
			S32 x= floor(x_at_middle_y + 0.5);
			S32 column_x= x - ll.x;

			U32 row_i= y - ll.y;
			ensure(row_i < row_count);
			struct RowBounding *row= &bounds[row_i];
			row->set= true;
			if (is_horizontal_segment) {
				S32 left_column= floor(cur_p.x + 0.5) - ll.x;
				S32 right_column= floor(next_p.x + 0.5) - ll.x;
				if (left_column > right_column)
					SWAP(S32, left_column, right_column);
				row->min= MAX(row->min, left_column);
				row->max= MIN(row->max, right_column);
			} else if (is_left_side) {
				row->min= MAX(row->min, column_x);
			} else if (!is_left_side) {
				row->max= MIN(row->max, column_x);
			}
		}
	}

	const U32 cell_count= rect_size->x*rect_size->y;
	PolyCell *cells= frame_alloc(sizeof(*cells)*cell_count);
	memset(cells, 0, sizeof(*cells)*cell_count);

	// Draw according to bounds
	for (U32 y= 0; y < row_count; ++y) {
		struct RowBounding *row= &bounds[y];
		if (!row->set)
			continue;
		for (S32 x= row->min; x < row->max; ++x) {
			const U32 cell_i= x + y*rect_size->x;
			ensure(cell_i < cell_count);
			cells[cell_i].fill= 64;
		}
	}

	return cells;
}

internal
PolyCell* rasterized_circle(V2i *rect_ll, V2i *rect_size, const Circle *circle)
{
	V2i ll= round_v2d_to_v2i(
			sub_v2d(circle->pos, (V2d) {circle->rad, circle->rad}));
	V2i tr= round_v2d_to_v2i(
			add_v2d(circle->pos, (V2d) {circle->rad, circle->rad}));
	tr= add_v2i(tr, (V2i) {1, 1});
	V2i size= sub_v2i(tr, ll);

	*rect_ll= ll;
	*rect_size= size;

	const U32 cell_count= rect_size->x*rect_size->y;
	PolyCell *cells= frame_alloc(sizeof(*cells)*cell_count);
	for (S32 y= 0; y < size.y; ++y) {
		for (S32 x= 0; x < size.x; ++x) {
			V2d test_p= {x + ll.x, y + ll.y};
			U32 i= x + y*size.x;
			ensure(i < cell_count);

			if (dist_sqr_v2d(test_p, circle->pos) >= circle->rad*circle->rad) {
				cells[i].fill= 0;
			} else {
				cells[i].fill= 64;
			}
		}
	}
	return cells;
}

typedef enum {
	ShapeType_poly,
	ShapeType_circle
} ShapeType;

internal
void modify_grid(bool dynamic, int add_mul, const void *shp, ShapeType shp_type, V2d pos, Qd rot)
{
	GridCell *grid= g_env.physworld->grid;

	// Rasterize
	V2i rect_ll, rect_size;
	PolyCell *poly_cells;
	if (shp_type == ShapeType_poly) {
		Poly poly= *(Poly*)shp;
		// Scale poly so that 1 unit == 1 cell
		for (U32 i= 0; i < poly.v_count; ++i) {
			V2d p= poly.v[i];
			p= rot_v2d(rotation_z_qd(rot), p);
			p= add_v2d(p, pos);
			p= scaled_v2d(GRID_RESO_PER_UNIT, p);
			p= sub_v2d(p, (V2d) {0.5, 0.5});
			poly.v[i]= p;
		}
		poly_cells= rasterized_poly(&rect_ll, &rect_size, &poly);
	} else if (shp_type == ShapeType_circle) {
		Circle circle= *(Circle*)shp;
		// Scale circle so that 1 unit == 1 cell
		V2d p= circle.pos;
		p= rot_v2d(rotation_z_qd(rot), p);
		p= add_v2d(p, pos);
		p= scaled_v2d(GRID_RESO_PER_UNIT, p);
		p= sub_v2d(p, (V2d) {0.5, 0.5});
		circle.pos= p;
		circle.rad *= GRID_RESO_PER_UNIT;
		poly_cells= rasterized_circle(&rect_ll, &rect_size, &circle);
	} else { 
		fail("Unknown shape");
	}

	// Blit to grid
	const V2i grid_ll= {-GRID_WIDTH_IN_CELLS/2, -GRID_WIDTH_IN_CELLS/2};
	for (S32 y= 0; y < rect_size.y; ++y) {
	for (S32 x= 0; x < rect_size.x; ++x) {
		// Grid cell pos from scaled world coordinates
		const V2i grid_p= {
			x + rect_ll.x - grid_ll.x,
			y + rect_ll.y - grid_ll.y
		};
		if (	grid_p.x < 0 || grid_p.y < 0 ||
				grid_p.x >= GRID_WIDTH_IN_CELLS || grid_p.y >= GRID_WIDTH_IN_CELLS)
			continue; // Don't blit outside grid

		const U32 grid_i= grid_p.x + grid_p.y*GRID_WIDTH_IN_CELLS;
		PolyCell *poly_cell= &poly_cells[x + y*rect_size.x];
		grid[grid_i].body_portion += poly_cell->fill*add_mul;
	}
	}
}


internal
void modify_grid_with_shapes(
		bool dynamic,
		int add_mul, // +1 or -1
		const Poly *polys, U32 poly_count,
		const Circle *circles, U32 circle_count,
		V2d pos, Qd rot)
{
	for (U32 i= 0; i < poly_count; ++i) {
		modify_grid(dynamic, add_mul, &polys[i], ShapeType_poly, pos, rot);
	}
	for (U32 i= 0; i < circle_count; ++i) {
		modify_grid(dynamic, add_mul, &circles[i], ShapeType_circle, pos, rot);
	}
}

internal
void cp_remove_constraint(cpBody *b, cpConstraint *c, void *data)
{ remove_constraint(c); }

internal
void cp_destroy_body_shape(cpBody *b, cpShape *s, void *data)
{
	cpSpace *space= data;
	cpSpaceRemoveShape(space, s);
	cpShapeFree(s);
}

internal
void cp_destroy_body_shapes(cpSpace *space, cpBody *body)
{
	cpBodyEachShape(body, cp_destroy_body_shape, space);
}

internal
cpBody *cp_create_body(cpSpace *space, float mass, float moment, bool is_static)
{
	cpBody *body;
	if (!is_static)
		body= cpBodyNew(mass, moment);
	else
		body= cpBodyNewStatic();
	cpSpaceAddBody(space, body);
	return body;
}

internal
void cp_destroy_body(cpSpace *space, cpBody *body)
{
	cp_destroy_body_shapes(space, body);
	cpBodyEachConstraint(body, cp_remove_constraint, NULL);

	cpSpaceRemoveBody(space, body);
	cpBodyFree(body);
}

void create_physworld()
{
	PhysWorld *w= ZERO_ALLOC(gen_ator(), sizeof(*w), "physworld");
	ensure(!g_env.physworld);
	g_env.physworld= w;

	w->griddef= (GridDef) {
		.offset= (V2i) {-GRID_WIDTH_IN_CELLS/2, -GRID_WIDTH_IN_CELLS/2},
		.reso= (V2i) {GRID_WIDTH_IN_CELLS, GRID_WIDTH_IN_CELLS},
		.reso_per_unit= GRID_RESO_PER_UNIT,
		.cell_count= GRID_CELL_COUNT,
		.sizeof_cell= sizeof(*w->grid),
		.sizeof_grid= sizeof(w->grid),
	};

	w->cp_space= cpSpaceNew();
	cpSpaceSetIterations(w->cp_space, 10);
	cpSpaceSetGravity(w->cp_space, cpv(0, -25));

	{ // Create static "ground" body
		w->cp_ground_body= cp_create_body(w->cp_space, 0, 0, true);

		w->ground_body.cp_body= w->cp_ground_body;
		w->ground_body.cp_data.body= &w->ground_body;
		w->ground_body.is_static= true;
		w->ground_body.allocated= true;

		cpBodySetUserData(w->cp_ground_body, &w->ground_body.cp_data);
	}

#ifdef USE_FLUID
	{ // Fluid proto
		const int half= GRID_WIDTH_IN_CELLS/2;
		for (U32 y_= GRID_WIDTH_IN_CELLS; y_ > 1; --y_) {
			const U32 y= y_ - 1;
			for (U32 x= 0; x < GRID_WIDTH_IN_CELLS; ++x) {
				GridCell *cur= &w->grid[x + y*GRID_WIDTH_IN_CELLS];

				if ((x - half)*(x - half) + (y - half - 90)*(y - half - 90) < 80)
					cur->water= 1;
			}
		}
	}
#endif
}

void destroy_physworld()
{
	PhysWorld *w= g_env.physworld;
	ensure(w);
	g_env.physworld= NULL;

	cp_destroy_body(w->cp_space, w->cp_ground_body);

	cpSpaceFree(w->cp_space);

	FREE(gen_ator(), w);
}

internal
U32 alloc_rigidbody_noinit()
{
	PhysWorld *w= g_env.physworld;

	if (w->body_count >= MAX_RIGIDBODY_COUNT)
		fail("Too many rigid bodies");

	while (w->bodies[w->next_body].allocated)
		w->next_body= (w->next_body + 1) % MAX_RIGIDBODY_COUNT;

	++w->body_count;
	return w->next_body;
}

internal
void set_rigidbody(U32 h, RigidBodyDef *def)
{
	PhysWorld *w= g_env.physworld;

	ensure(h < MAX_RIGIDBODY_COUNT && w->bodies[h].cp_body == NULL);
	RigidBody *b= &w->bodies[h];
	strncpy(b->def_name, def->res.name, sizeof(b->def_name));
	b->shape_changed= true;

	const PhysMat *mat=
		(PhysMat*)res_by_name(	g_env.resblob,
								ResType_PhysMat,
								def->mat_name);

	/// @todo Mass & moment could be precalculated to ResBlob
	F64 total_mass= 0;
	F64 total_moment= 0;
	{
		for (U32 i= 0; i < def->circle_count; ++i) {
			const Circle *circle= &def->circles[i];
			F32 mass= mat->density*cpAreaForCircle(0.0, circle->rad);
			total_mass += mass;
			total_moment +=
				cpMomentForCircle(mass, 0, circle->rad, to_cpv(circle->pos));
		}

		for (U32 i= 0; i < def->poly_count; ++i) {
			Poly *poly= &def->polys[i];

			cpVect cp_verts[poly->v_count];
			for (U32 v_i= 0; v_i < poly->v_count; ++v_i)
				cp_verts[v_i]= to_cpv(poly->v[v_i]);

			F32 mass= mat->density*cpAreaForPoly(poly->v_count, cp_verts, 0.0);
			total_mass += mass;
			total_moment +=
				cpMomentForPoly(mass, poly->v_count, cp_verts, cpvzero, 0.0);
		}
	}

	if (def->disable_rot)
		total_moment= INFINITY;

	b->cp_body= cp_create_body(w->cp_space, total_mass, total_moment, def->is_static);
	b->cp_data.body= b;
	cpBodySetUserData(b->cp_body, &b->cp_data);
	b->is_static= def->is_static;

	cpBodySetPosition(b->cp_body, to_cpv((V2d) {b->tf.pos.x, b->tf.pos.y}));
	cpBodySetAngle(b->cp_body, rotation_z_qd(b->tf.rot));

	U32 circle_count= def->circle_count;
	U32 poly_count= def->poly_count;
	Circle *circles= def->circles;
	Poly *polys= def->polys;
	if (b->has_own_shape) { // Override def with own shape
		circle_count= b->circle_count;
		poly_count= b->poly_count;
		circles= b->circles;
		polys= b->polys;
	} else { // Cache shapes to RigidBody (for fast grid calcs)
		ensure(poly_count < MAX_SHAPES_PER_BODY);
		ensure(circle_count < MAX_SHAPES_PER_BODY);
		b->circle_count= circle_count;
		b->poly_count= poly_count;
		memcpy(b->circles, circles, sizeof(*circles)*circle_count);
		memcpy(b->polys, polys, sizeof(*polys)*poly_count);
	}

	{ // Create physics shapes
		for (U32 i= 0; i < circle_count; ++i) {
			b->cp_shapes[b->cp_shape_count++]=
				cpSpaceAddShape(
						w->cp_space,
						cpCircleShapeNew(	b->cp_body,
											circles[i].rad,
											to_cpv(circles[i].pos)));
		}

		for (U32 i= 0; i < poly_count; ++i) {
			Poly *poly= &polys[i];
			F32 mass= 1.0;

			cpVect cp_verts[poly->v_count];
			for (U32 v_i= 0; v_i < poly->v_count; ++v_i)
				cp_verts[v_i]= to_cpv(poly->v[v_i]);

			total_mass += mass;
			total_moment +=
				cpMomentForPoly(mass, poly->v_count, cp_verts, cpvzero, 0.0);
			b->cp_shapes[b->cp_shape_count++]=
				cpSpaceAddShape(w->cp_space,
						cpPolyShapeNew(	b->cp_body,
										poly->v_count,
										cp_verts,
										cpTransformIdentity,
										0.0));
		}

		for (U32 i= 0; i < b->cp_shape_count; ++i) {
			cpShape *shape= b->cp_shapes[i];
			cpShapeSetFriction(shape, mat->friction);
			cpShapeSetElasticity(shape, mat->restitution);
		}
	}
}

U32 resurrect_rigidbody(const RigidBody *dead)
{
	PhysWorld *w= g_env.physworld;

	U32 h= alloc_rigidbody_noinit();
	w->bodies[h]= *dead;
	w->bodies[h].allocated= true;
	w->bodies[h].cp_body= NULL;
	w->bodies[h].cp_shape_count= 0;
	w->bodies[h].is_in_grid= false;

	set_rigidbody(
			h,
			(RigidBodyDef*)res_by_name(
				g_env.resblob,
				ResType_RigidBodyDef,
				dead->def_name));
	return h;
}

void free_rigidbody(RigidBody *b)
{
	PhysWorld *w= g_env.physworld;

	const U32 h= b - w->bodies;
	ensure(h < MAX_RIGIDBODY_COUNT);

	if (b->is_in_grid) {
		modify_grid_with_shapes(
				cpBodyGetType(b->cp_body) == CP_BODY_TYPE_DYNAMIC,
				-1,
				b->polys, b->poly_count,
				b->circles, b->circle_count,
				v3d_to_v2d(b->prev_tf.pos), b->prev_tf.rot);
	}

	cp_destroy_body(w->cp_space, b->cp_body);

	*b= (RigidBody) { .allocated= false };
	--w->body_count;
}

void * storage_rigidbody()
{ return g_env.physworld->bodies; }

RigidBody * get_rigidbody(U32 h)
{ return g_env.physworld->bodies + h; }

internal
void phys_draw_circle(
		cpVect pos, cpFloat angle, cpFloat radius,
		cpSpaceDebugColor outlineColor,
		cpSpaceDebugColor fillColor,
		cpDataPointer data)
{
	const U32 v_count= 30;
	V3d v[v_count];
	for (U32 i= 0; i < v_count; ++i) {
		F64 a= angle + i*3.141*2.0/v_count;
		v[i].x= pos.x + cos(a)*radius;
		v[i].y= pos.y + sin(a)*radius;
		v[i].z= 0.0;
	}
	v[0].x= v[0].x*0.9 + pos.x*0.1;
	v[0].y= v[0].y*0.9 + pos.y*0.1;

	Color c= {0.4, 0.7, 1.0, 0.5};
	ddraw_poly(
			c,
			v,
			v_count);
}

internal
void phys_draw_poly(
		int count, const cpVect *verts, cpFloat radius,
		cpSpaceDebugColor outlineColor, cpSpaceDebugColor fillColor,
		cpDataPointer data)
{
	V3d v[count];
	for (int i= 0; i < count; ++i) {
		v[i].x= verts[i].x;
		v[i].y= verts[i].y;
		v[i].z= 0.0;
	}

	Color c= {0.4, 0.7, 1.0, 0.5};
	ddraw_poly(
			c,
			v,
			count);
}

void upd_physworld(F64 dt)
{
	PhysWorld *w= g_env.physworld;

	//dt= 1.0/60.0;
	dt= MIN(dt, 1.0/30.0);

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		if (b->input_force.x != 0.0 || b->input_force.y != 0.0) {
			cpBodyApplyForceAtWorldPoint(
					b->cp_body,
					to_cpv(b->input_force),
					cpv(b->tf.pos.x, b->tf.pos.y));
			b->input_force= (V2d) {};
		}

		if (b->max_target_force > 0.0)
			apply_velocity_target(b, b->target_velocity, b->max_target_force);
		b->max_target_force= 0.0;
	}
	/// @todo Accumulation
	/// @todo Reset torques etc. only after all timesteps are done
	cpSpaceStep(w->cp_space, dt*0.5);
	cpSpaceStep(w->cp_space, dt*0.5);

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		cpVect p= cpBodyGetPosition(b->cp_body);
		cpVect r= cpBodyGetRotation(b->cp_body);
		b->tf.pos.x= p.x;
		b->tf.pos.y= p.y;
		b->tf.rot= qd_by_xy_rot_matrix(r.x, r.y);
		b->velocity= from_cpv(cpBodyGetVelocity(b->cp_body));



		b->tf_changed= !equals_v3d(b->prev_tf.pos, b->tf.pos) ||
						!equals_qd(b->prev_tf.rot, b->tf.rot);
	}
}


#ifdef USE_FLUID_PROTO

#define LEFT_SIDE_MASK (1<<0)
#define UP_SIDE_MASK (1<<1)
#define RIGHT_SIDE_MASK (1<<2)
#define DOWN_SIDE_MASK (1<<3)
const U8 side_masks[4]= {
	LEFT_SIDE_MASK,
	RIGHT_SIDE_MASK,
	DOWN_SIDE_MASK,
	UP_SIDE_MASK,
};
#define SIDE_LEFT 0
#define SIDE_UP 1
#define SIDE_RIGHT 2
#define SIDE_DOWN 3

#define MAX_SINKNESS (U16_MAX - 1)
#define STATIC_SINKNESS U16_MAX // Can't move
typedef struct FluidEdge {
	U32 cell;
	U16 sinkness;
	U8 side; // SIDE_LEFT or SIDE_UP or ...
	bool side_water;
	bool side_occupied;
	bool flow_established;
} FluidEdge;

internal
int sinks_first_cmp(const void *e1, const void *e2)
{
#define E1 ((FluidEdge*)e1)
#define E2 ((FluidEdge*)e2)
	return	(E1->sinkness < E2->sinkness) - (E1->sinkness > E2->sinkness);
#undef E1
#undef E2
}

// Part of fluid update
internal
void process_fluid_area(GridCell *grid, U32 cell_i, U16 area_id)
{
#	define MAX_HEADS (1024*20) // @todo Remove
#	define MAX_CELLS (1024*40) // @todo Remove
#	define MAX_EDGES (1024*4) // @todo Remove
	U32 area_cells[MAX_CELLS]= {};
	U32 area_cell_count= 0;

	FluidEdge edges[MAX_EDGES]= {};
	U32 edge_count= 0;

	// Search through fluid area while
	// @todo Efficient flood-fill search
	//  - establishing correct pressure gradient
	//  - find fluid edges (single cell can have multiple edges)
	// Ground level pressure is adjusted afterwards
	U32 heads[MAX_HEADS]= {cell_i};
	U32 head_count= 1;
	grid[cell_i].pressure= U16_MAX/2;
	U16 min_pressure= U16_MAX;
	while (head_count > 0) {
		// @todo bounds checks
		U32 cur= heads[--head_count];
		area_cells[area_cell_count++]= cur;
		ensure(area_cell_count < MAX_CELLS);
		min_pressure= MIN(min_pressure, grid[cur].pressure);

		const U32 sides[4]= {
			cur - 1,
			cur + GRID_WIDTH_IN_CELLS,
			cur + 1,
			cur - GRID_WIDTH_IN_CELLS,
		};
		const int pressure_add[4]= {
			0, -1, 0, 1
		};
		int water_side_count= 0;
		U8 side_water= 0;
		U8 side_occupied= 0;
		for (U32 i= 0; i < 4; ++i) {
			const U32 side= sides[i];
			if (grid[side].water) {
				++water_side_count;
				side_water |= side_masks[i];
			}

			if (grid[side].water || grid[side].static_portion) {
				side_occupied |= side_masks[i];
			}

			if (grid[side].water && grid[side].pressure == 0) {
				// Enlarge to all sides with water & unprocessed area
				U16 side_pressure= grid[cur].pressure + pressure_add[i];
				grid[side].pressure= side_pressure; 
#ifndef SIMPLE_FLUID_SWAP_DYNAMICS
				grid[side].fluid_area_id= area_id;
#endif

				heads[head_count++]= side;
				ensure(head_count < MAX_HEADS);
			}
		}

		// Collect edges
		for (U8 i= 0; i < 4; ++i) {
			if ((side_water & side_masks[i]))
				continue;
			edges[edge_count++]= (FluidEdge) {
				.cell= cur,
				.side= i,
				.side_water= side_water & side_masks[i],
				.side_occupied= side_occupied & side_masks[i],
			};
			ensure(edge_count < MAX_EDGES);
		}
	}

	// TEMPTEST
	/*
	V3d prev= {};
	for (U32 i= 0; i < edge_count;) {
		const U32 i_cell= edges[i].cell;
		U32 next= i;
		while (next < edge_count && edges[next].cell == i_cell)
			++next;
		next %= edge_count;
		U32 nnext= next;
		while (nnext < edge_count && edges[nnext].cell == edges[next].cell)
			++nnext;
		nnext %= edge_count;
		V3d p1= {	edges[i].cell % GRID_WIDTH_IN_CELLS,
					edges[i].cell/GRID_WIDTH_IN_CELLS};
		V3d p2= {	edges[next].cell % GRID_WIDTH_IN_CELLS,
					edges[next].cell/GRID_WIDTH_IN_CELLS};
		V3d p3= {	edges[nnext].cell % GRID_WIDTH_IN_CELLS,
					edges[nnext].cell/GRID_WIDTH_IN_CELLS};
		p1.x /= GRID_RESO_PER_UNIT;
		p1.y /= GRID_RESO_PER_UNIT;
		p2.x /= GRID_RESO_PER_UNIT;
		p2.y /= GRID_RESO_PER_UNIT;
		p3.x /= GRID_RESO_PER_UNIT;
		p3.y /= GRID_RESO_PER_UNIT;
		p1.x -= 50;
		p1.y -= 50;
		p2.x -= 50;
		p2.y -= 50;
		p3.x -= 50;
		p3.y -= 50;

		V3d mid1= {(p1.x + p2.x)/2.0, (p1.y + p2.y)/2.0};
		V3d mid2= {(p2.x + p3.x)/2.0, (p2.y + p3.y)/2.0};

		p2.x= (mid1.x + p2.x + mid2.x)/3.0;
		p2.y= (mid1.y + p2.y + mid2.y)/3.0;

		ddraw_line((Color) {1.0, 0.0, 0.0, 1.0}, prev, p2);
		prev= p2;
		while (i < edge_count && edges[i].cell == i_cell)
			++i;
	}
	*/

	// Normalize pressure values so that smallest is 1
	for (U32 i= 0; i < area_cell_count; ++i) {
		const U32 cell_i= area_cells[i];
		grid[cell_i].pressure -= min_pressure - 1;
	}

	// Calculate edge sinkness (based on e.g. pressure)
	for (U32 i= 0; i < edge_count; ++i) {
		FluidEdge *cur= &edges[i];

		if (!cur->side_occupied) {
			if (cur->side == SIDE_DOWN) { // Empty down
				cur->sinkness= MAX_SINKNESS;
			} else if (	cur->side == SIDE_LEFT ||
						cur->side == SIDE_RIGHT) { // Empty side
				cur->sinkness= grid[cur->cell].pressure;
			} else if (cur->side == SIDE_UP) { // Empty up
				cur->sinkness= grid[cur->cell].pressure;
			}
		} else {
			cur->sinkness= STATIC_SINKNESS; // Don't flow through this side
		}
	}

	// Sort edges so that most sinking are first
	// STATIC_SINKNESS are first and should be skipped in later processing
	qsort(edges, edge_count, sizeof(*edges), sinks_first_cmp);

	{
#		ifdef SIMPLE_FLUID_SWAP_DYNAMICS
		// Dynamics by swapping source cells with sinks
		for (U32 i= 0; i < edge_count; ++i) {
			edges[i].flow_established= true;
		}
#		else
		// Mark edges to grid (required at flow analysis)
		for (U32 i= 0; i < edge_count;) {
			const U32 cell= edges[i].cell;
			grid[cell].edge_begin_index= i;
			while (++i < edge_count && edges[i].cell == cell)
				;
		}

		// Dynamics by flow analysis

		U32 sinks_begin= 0;
		// Filter out edges with static sinkness
		while (sinks_begin < edge_count && edges[sinks_begin].sinkness == STATIC_SINKNESS)
			++sinks_begin;
		const U32 sinks_count= (edge_count - sinks_begin)/2;
		const U32 sinks_end= sinks_begin + sinks_count;

		{ // BFS calculating fluid potential starting from sinks
			U32 heads[MAX_HEADS][2];
			U32 head_count= 0;
			U32 head_pool= 0;
			U32 cur_pot= 1;
			for (U32 i= sinks_begin; i < sinks_end; ++i) {
				heads[head_count++][head_pool]= edges[i].cell;
				grid[edges[i].cell].potential= cur_pot;
				ensure(head_count < MAX_HEADS);
			}
			while (head_count > 0) {
				U32 next_head_pool= (head_pool + 1) % 2;
				U32 next_head_count= 0;
				for (U32 i= 0; i < head_count; ++i) {
					const U32 cur= heads[i][head_pool];
					const U32 sides[4]= {
						cur - 1,
						cur + GRID_WIDTH_IN_CELLS,
						cur + 1,
						cur - GRID_WIDTH_IN_CELLS,
					};
					for (U32 s= 0; s < 4; ++s) {
						const U32 side= sides[s];
						if (grid[side].water && grid[side].potential == 0) {
							// Enlarge to all sides with water & uninit potential
							grid[side].potential= cur_pot + 1;
							heads[next_head_count++][next_head_pool]= side;
							ensure(next_head_count < MAX_HEADS);
						}
					}
				}

				head_pool= next_head_pool;
				head_count= next_head_count;
				++cur_pot;
			}
		}

		const FluidEdge *sources_begin= edges + sinks_end;
		const FluidEdge *sources_end= edges + edge_count;
		{ // Find fluid paths and swap endpoints
			// (not sure if direction matters)

			struct Head {
				U32 src_edge;
				U32 cell;
				U8 possible_sides[4];
				U8 possible_side_count; // Could use Head[4] pools for perf
				bool already_reached; // true if source == sink cell
			};
			typedef struct Head Head;

			Head heads[MAX_HEADS][2];
			U32 head_count= 0;
			U32 head_pool= 0;
			const int max_overlapping_paths= 0;
			// Initial heads (in decresing sourceness)
			const FluidEdge *e= sources_end - 1;
			while (e >= sources_begin) {
				Head h= {
					.src_edge= e - edges,
					.cell= e->cell,
				};
				// !!! Relying on the fact that edges for a single cell are
				// adjacent in `edges`
				while (--e >= sources_begin && e->cell == h.cell)
					;
				// @todo Not sure if should adjust sides when head is created/advanced
				//       or in the place where it's now
				++grid[h.cell].fluid_path_count;
				heads[head_count++][head_pool]= h;
				ensure(head_count < MAX_HEADS);
			}

			// Crawl from sources to sinks according to fluid potential
			while (head_count > 0) {
				U32 next_head_pool= (head_pool + 1) % 2;
				U32 next_head_count= 0;

				// Find possible sides to be advanced through
				for (U32 i= 0; i < head_count; ++i) {
					const U32 cur= heads[i][head_pool].cell;
					grid[cur].draw_something= 1;
					if (grid[cur].potential == 1) {
						// We've reached a sink!
						// Now can move water from source to sink
						//debug_print("sink reached!");
						const U32 source= heads[i][head_pool].src_edge;
	
						// @todo Proper edge
						const U32 sink= grid[cur].edge_begin_index;
						ensure(edges[sink].cell == cur);

						edges[source].flow_established= true;
						edges[sink].flow_established= true;
						continue;
					}

					const U32 sides[4]= {
						cur - 1,
						cur + GRID_WIDTH_IN_CELLS,
						cur + 1,
						cur - GRID_WIDTH_IN_CELLS,
					};
					for (U32 k= 0; k < 4; ++k) {
						if (	grid[sides[k]].potential > 0 &&
								grid[sides[k]].fluid_path_count <= max_overlapping_paths &&
								grid[sides[k]].fluid_area_id == area_id) {
							const U32 side_i= heads[i][head_pool].possible_side_count++;
							heads[i][head_pool].possible_sides[side_i]= k;
						}
					}

				}

				// Advance heads
				// Heads with zero sides are implicitly killed.
				// Conflicting next positions are solved by a heuristic, in which
				// heads with fewer possible positions are advanced first.
				for (U8 side_count= 1; side_count <= 4; ++side_count) { 
					for (U32 i= 0; i < head_count; ++i) {
						if (heads[i][head_pool].possible_side_count != side_count)
							continue;

						const U32 cur= heads[i][head_pool].cell;
						const U32 sides[4]= {
							cur - 1,
							cur + GRID_WIDTH_IN_CELLS,
							cur + 1,
							cur - GRID_WIDTH_IN_CELLS,
						};

						U16 min_pot= U16_MAX;
						const U8 invalid_side= 5;
						U8 min_pot_side= invalid_side;
						// Pick the best side to advance
						// @todo fluid_path_count should be taken into account
						for (U8 s= 0; s < side_count; ++s) {
							const U8 side= heads[i][head_pool].possible_sides[s];
							const U16 pot= grid[sides[side]].potential;
							if (	grid[sides[side]].fluid_path_count <=
										max_overlapping_paths &&
									pot < min_pot) {
								ensure(grid[sides[side]].water);
								min_pot= pot; 
								min_pot_side= side;
							}
						}

						if (min_pot_side == invalid_side) {
							// Dead end due to the advances of other heads in this step
							continue;
						}

						// Advanced head
						Head h= {
							.src_edge= heads[i][head_pool].src_edge,
							.cell= sides[min_pot_side],
						};
						++grid[sides[min_pot_side]].fluid_path_count;
						heads[next_head_count++][next_head_pool]= h;
					}
				}

				head_pool= next_head_pool;
				head_count= next_head_count;
			}
		}
#		endif

		// Move fluid from sources to sinks
		if (edge_count > 0) {
			U32 sink= 0;
			// Filter out edges with static sinkness
			while (sink < edge_count && edges[sink].sinkness == STATIC_SINKNESS)
				++sink;

			U32 source= edge_count - 1;
			// If there's odd number of edges, then one source/sink ignored, which
			// shouldn't matter too much
			for (; sink < source; ++sink, --source) {
				while (!edges[sink].flow_established) {
					++sink;
					if (sink >= source)
						goto out_of_edges;
				}
				while (!edges[source].flow_established) {
					--source;
					if (sink >= source)
						goto out_of_edges;
				}
				ensure(sink < edge_count);
				ensure(source < edge_count);

				if (ABS(edges[sink].sinkness - edges[source].sinkness) <= 1)
					continue; // Remove jitter at surfaces

				if (grid[edges[source].cell].already_swapped)
					continue;

				const U32 sink_sides[4]= {
					edges[sink].cell - 1,
					edges[sink].cell + GRID_WIDTH_IN_CELLS,
					edges[sink].cell + 1,
					edges[sink].cell - GRID_WIDTH_IN_CELLS,
				};
				bool flow= false;
				U32 side= sink_sides[edges[sink].side];
				if (	!grid[side].water &&
						!grid[side].static_portion &&
						!grid[side].already_swapped) {
					grid[side].water= 1;
					grid[side].already_swapped= true;
					flow= true;
				}
				if (!flow)
					continue; // No room, no swap

				grid[edges[source].cell].water= 0;
				grid[edges[source].cell].already_swapped= true;
			}
			out_of_edges:;
		}
	}

#	undef MAX_EDGES
#	undef MAX_HEADS
#	undef MAX_CELLS
}

#endif // USE_FLUID_PROTO

void post_upd_physworld()
{
	PhysWorld *w= g_env.physworld;

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		// Update changes to grid
		if (b->tf_changed || !b->is_in_grid) {
			if (cpBodyGetType(b->cp_body) == CP_BODY_TYPE_STATIC) {
				// Notify physics about static body repositioning
				cpSpaceReindexShapesForBody(w->cp_space, b->cp_body);
			}

			if (b->is_in_grid) {
				modify_grid_with_shapes(
						cpBodyGetType(b->cp_body) == CP_BODY_TYPE_DYNAMIC,
						-1,
						b->polys, b->poly_count,
						b->circles, b->circle_count,
						v3d_to_v2d(b->prev_tf.pos), b->prev_tf.rot);
			}
			modify_grid_with_shapes(
					cpBodyGetType(b->cp_body) == CP_BODY_TYPE_DYNAMIC,
					1,
					b->polys, b->poly_count,
					b->circles, b->circle_count,
					v3d_to_v2d(b->tf.pos), b->tf.rot);
		}

		b->is_in_grid= true;
		b->shape_changed= false;
		b->tf_changed= false;
		b->prev_tf= b->tf;
	}

	if (w->grid_modified) {
		w->grid_modified= false;

		cp_destroy_body_shapes(w->cp_space, w->cp_ground_body);

		// Recreate static ground shapes
		// @todo This is just a temp solution. Must smooth more.
		for (int y= 0; y < GRID_WIDTH_IN_CELLS; ++y) {
			cpVect poly[6];
			const F64 width= 1.0/GRID_RESO_PER_UNIT;
			bool left_reached= false;
			for (int x= 0; x < GRID_WIDTH_IN_CELLS + 1; ++x) {
				V2d wp= {x*width - GRID_WIDTH/2, y*width - GRID_WIDTH/2};

				// Calculate cell status at top and bottom for simple smoothing
#define CELL_ON(x, y) (w->grid[GRID_INDEX((x), (y))].material != GRIDCELL_MATERIAL_AIR)

				if (	x == GRID_WIDTH_IN_CELLS ||
						!CELL_ON(x, y)) {
					if (left_reached) {
						left_reached= false;

						// Right side of the layer
						// @note x is one off
						int top_cell_dif= 0;
						int bottom_cell_dif= 0;
						if (x > 0 && x + 1 < GRID_WIDTH_IN_CELLS) {
							if (y + 1 < GRID_WIDTH_IN_CELLS) {
								top_cell_dif += CELL_ON(x, y + 1);
								top_cell_dif -= !CELL_ON(x - 1, y + 1);
							}
							if (y > 0) {
								bottom_cell_dif += CELL_ON(x, y - 1);
								bottom_cell_dif -= !CELL_ON(x - 1, y - 1);
							}
						}
						poly[3].x= wp.x + width*bottom_cell_dif*0.5;
						poly[3].y= wp.y;
						poly[4].x= wp.x;
						poly[4].y= wp.y + width*0.5;
						poly[5].x= wp.x + width*top_cell_dif*0.5;
						poly[5].y= wp.y + width;
						cpShape *shape= cpSpaceAddShape(w->cp_space,
								cpPolyShapeNew(	w->cp_ground_body,
												6,
												poly,
												cpTransformIdentity,
												0.0));
						// @todo Set in data
						cpShapeSetFriction(shape, 1);
						cpShapeSetElasticity(shape, 0.1);
					}
				} else {
					if (!left_reached) {
						left_reached= true;

						// Left side of the layer
						int top_cell_dif= 0;
						int bottom_cell_dif= 0;
						if (x > 0 && x + 1 < GRID_WIDTH_IN_CELLS) {
							if (y + 1 < GRID_WIDTH_IN_CELLS) {
								top_cell_dif -= CELL_ON(x - 1, y + 1);
								top_cell_dif += !CELL_ON(x, y + 1);
							}
							if (y > 0) {
								bottom_cell_dif -= CELL_ON(x - 1, y - 1);
								bottom_cell_dif += !CELL_ON(x , y - 1);
							}
						}

						poly[0].x= wp.x + width*top_cell_dif*0.5;
						poly[0].y= wp.y + width;
						poly[1].x= wp.x;
						poly[1].y= wp.y + width*0.5;
						poly[2].x= wp.x + width*bottom_cell_dif*0.5;
						poly[2].y= wp.y;
					}
				}
#undef CELL_ON
			}
		}
	}

#ifdef USE_FLUID_PROTO
	{ // Update fluids
		// @todo To thread -- latency of 1 frame is acceptable

		{ // Calculate pressure
			// Reset
			for (U32 x= 0; x < GRID_WIDTH_IN_CELLS; ++x) {
				for (U32 y= 0; y < GRID_WIDTH_IN_CELLS; ++y) {
					GridCell *c= &w->grid[GRID_INDEX(x, y)];
					c->pressure= 0;
					c->already_swapped= false;
					c->draw_something= 0;
#					ifndef SIMPLE_FLUID_SWAP_DYNAMICS
					c->potential= 0;
					c->fuid_path_count= 0;
					c->edge_begin_index= 0;
					c->fluid_area_id= 0;
#endif
				}
			}

			U16 area_id= 1;
			for (U32 i= 0; i < GRID_CELL_COUNT; ++i) {
				if (w->grid[i].water && w->grid[i].pressure == 0) {
					process_fluid_area(w->grid, i, area_id++);
				}
			}
		}
	}
#endif
}

void upd_phys_rendering()
{
	PhysWorld *w= g_env.physworld;
#ifdef USE_FLUID_PROTO
	{ // Update fluids to renderer
		Texel *grid= g_env.renderer->fluid_grid;
		for (U32 i= 0; i < GRID_CELL_COUNT; ++i) {
			F32 p= w->grid[i].pressure + 1;
			//F32 p= w->grid[i].potential*3 + 1;
			//F32 p= w->grid[i].draw_something*50 + 1;
			grid[i].r= CLAMP(10, 0, 255);
			grid[i].g= CLAMP(100 - p*5, 0, 255);
			grid[i].b= CLAMP(255 - p*5, 0, 255);
			grid[i].a= w->grid[i].water*240;

			if (w->grid[i].draw_something) {
				grid[i].r= 255;
				grid[i].a= 255;
			}
		}
	}
#endif

	{ // Update occlusion grid for graphics
		U8 *grid= g_env.renderer->occlusion_grid;
		for (U32 i= 0; i < GRID_CELL_COUNT; ++i) {
			grid[i]= MIN(	w->grid[i].material*255 +
							w->grid[i].body_portion*8,
							255);
		}
	}

	g_env.renderer->draw_grid= w->debug_draw;
	if (!w->debug_draw)
		return;
	cpSpaceDebugDrawOptions options= {
		.drawCircle= phys_draw_circle,
		.drawPolygon= phys_draw_poly,
		.flags= CP_SPACE_DEBUG_DRAW_SHAPES,
	};
	cpSpaceDebugDraw(w->cp_space, &options);

	Texel *grid= g_env.renderer->grid_ddraw_data;
	for (U32 i= 0; i < GRID_CELL_COUNT; ++i) {
		U8 ground_portion= w->grid[i].material == GRIDCELL_MATERIAL_GROUND ? 126 : 0;
		grid[i].r= MIN(w->grid[i].body_portion*3, 255);
		grid[i].g= w->grid[i].draw_something*255;
		grid[i].b= ground_portion;
		grid[i].a= MIN(ground_portion + w->grid[i].body_portion*3 + w->grid[i].draw_something*255, 255);

		w->grid[i].draw_something= 0;
	}
}


U32 set_grid_material_in_circle(V2d center, F64 rad, U8 material)
{
	U32 changed_count= 0;
	const V2i cell= GRID_VEC_W(center.x, center.y);
	const int rad_in_cells= rad*GRID_RESO_PER_UNIT;
	for (int y= cell.y - rad_in_cells; y < cell.y + rad_in_cells; ++y) {
	for (int x= cell.x - rad_in_cells; x < cell.x + rad_in_cells; ++x) {
		if (	x < 0 || x >= GRID_WIDTH_IN_CELLS ||
				y < 0 || y >= GRID_WIDTH_IN_CELLS)
			continue;

		if (	(x - cell.x)*(x - cell.x) +
				(y - cell.y)*(y - cell.y) > rad_in_cells*rad_in_cells)
			continue;

		if (g_env.physworld->grid[GRID_INDEX(x, y)].material != material)
			++changed_count;
		g_env.physworld->grid[GRID_INDEX(x, y)].material= material;
		g_env.physworld->grid_modified= true;
	}
	}
	return changed_count;
}

U32 grid_material_fullness_in_circle(V2d center, F64 rad, U8 material)
{
	bool empty= true;
	bool full= true;
	const V2i cell= GRID_VEC_W(center.x, center.y);
	const int rad_in_cells= rad*GRID_RESO_PER_UNIT;
	for (int y= cell.y - rad_in_cells; y < cell.y + rad_in_cells; ++y) {
	for (int x= cell.x - rad_in_cells; x < cell.x + rad_in_cells; ++x) {
		if (	x < 0 || x >= GRID_WIDTH_IN_CELLS ||
				y < 0 || y >= GRID_WIDTH_IN_CELLS)
			continue;

		if (	(x - cell.x)*(x - cell.x) +
				(y - cell.y)*(y - cell.y) > rad_in_cells*rad_in_cells)
			continue;

		if (g_env.physworld->grid[GRID_INDEX(x, y)].material == material)
			empty= false;
		else
			full= false;
	}
	}
	return !empty + full;
}

GridCell grid_cell(V2i v)
{
	if (	v.x < 0 || v.x >= GRID_WIDTH_IN_CELLS ||
			v.y < 0 || v.y >= GRID_WIDTH_IN_CELLS)
		return (GridCell) {};
	return g_env.physworld->grid[GRID_INDEX(v.x, v.y)];
}

