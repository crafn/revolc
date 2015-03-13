#include "chipmunk_util.h"
#include "core/color.h"
#include "core/malloc.h"
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

	typedef struct RowBounding {
		S32 min, max;
		bool set;
	} RowBounding;

	const U32 row_count= rect_size->y;
	RowBounding *bounds= frame_alloc(sizeof(*bounds)*row_count);
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
		bool is_horizontal_segment= abs(cur_p.y - next_p.y) < EPSILOND;

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
			RowBounding *row= &bounds[row_i];
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
		RowBounding *row= &bounds[y];
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
void modify_grid(int add_mul, const void *shp, ShapeType shp_type, V2d pos, Qd rot)
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
		grid[grid_i].dynamic_portion += poly_cell->fill*add_mul;
	}
	}
}


internal
void modify_grid_with_shapes(
		int add_mul, // +1 or -1
		const Poly *polys, U32 poly_count,
		const Circle *circles, U32 circle_count,
		V2d pos, Qd rot)
{
	for (U32 i= 0; i < poly_count; ++i) {
		modify_grid(add_mul, &polys[i], ShapeType_poly, pos, rot);
	}
	for (U32 i= 0; i < circle_count; ++i) {
		modify_grid(add_mul, &circles[i], ShapeType_circle, pos, rot);
	}
}

void create_physworld()
{
	PhysWorld *w= zero_malloc(sizeof(*w));
	ensure(!g_env.physworld);
	g_env.physworld= w;

	w->cp_space= cpSpaceNew();
	cpSpaceSetIterations(w->cp_space, 10);
	cpSpaceSetGravity(w->cp_space, cpv(0, -20));
}

void destroy_physworld()
{
	PhysWorld *w= g_env.physworld;
	ensure(w);
	g_env.physworld= NULL;
	cpSpaceFree(w->cp_space);

	free(w);
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

	/// @todo Mass & moment could be precalculated to ResBlob
	F64 total_mass= 0;
	F64 total_moment= 0;
	{
		for (U32 i= 0; i < def->circle_count; ++i) {
			F32 mass= 1.0; /// @todo Mass from density	
			total_mass += mass;
			total_moment +=
				cpMomentForCircle(mass, 0,
					def->circles[i].rad, to_cpv(def->circles[i].pos));
		}

		for (U32 i= 0; i < def->poly_count; ++i) {
			Poly *poly= &def->polys[i];
			F32 mass= 1.0;

			cpVect cp_verts[poly->v_count];
			for (U32 v_i= 0; v_i < poly->v_count; ++v_i)
				cp_verts[v_i]= to_cpv(poly->v[v_i]);

			total_mass += mass;
			total_moment +=
				cpMomentForPoly(mass, poly->v_count, cp_verts, cpvzero, 0.0);
		}
	}

	if (!b->is_static)
		b->cp_body= cpBodyNew(total_mass, total_moment);
	else
		b->cp_body= cpBodyNewStatic();
	cpSpaceAddBody(w->cp_space, b->cp_body);
	cpBodySetPosition(b->cp_body, to_cpv((V2d) {b->tf.pos.x, b->tf.pos.y}));

	U32 circle_count= def->circle_count;
	U32 poly_count= def->poly_count;
	Circle *circles= def->circles;
	Poly *polys= def->polys;
	if (b->has_own_shape) { // Override def with own shape (e.g. ground)
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
			cpShapeSetFriction(shape, 2.5);
			cpShapeSetElasticity(shape, 0.5);
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

internal
void cp_remove_constraint(cpBody *b, cpConstraint *c, void *data)
{ remove_constraint(c); }

void free_rigidbody(RigidBody *b)
{
	PhysWorld *w= g_env.physworld;

	const U32 h= b - w->bodies;
	ensure(h < MAX_RIGIDBODY_COUNT);

	if (b->is_in_grid) {
		modify_grid_with_shapes(-1,
				b->polys, b->poly_count,
				b->circles, b->circle_count,
				v3d_to_v2d(b->prev_tf.pos), b->prev_tf.rot);
	}

	for (U32 i= 0; i < b->cp_shape_count; ++i) {
		cpSpaceRemoveShape(w->cp_space, b->cp_shapes[i]);
		cpShapeFree(b->cp_shapes[i]);
	}

	cpBodyEachConstraint(b->cp_body, cp_remove_constraint, NULL);

	cpSpaceRemoveBody(w->cp_space, b->cp_body);
	cpBodyFree(b->cp_body);

	*b= (RigidBody) { .allocated= false };
	--w->body_count;
}

void * storage_rigidbody()
{ return g_env.physworld->bodies; }

internal
void phys_draw_circle(
		cpVect pos, cpFloat angle, cpFloat radius,
		cpSpaceDebugColor outlineColor,
		cpSpaceDebugColor fillColor,
		cpDataPointer data)
{
	const U32 v_count= 30;
	V2d v[v_count];
	for (U32 i= 0; i < v_count; ++i) {
		F64 a= angle + i*3.141*2.0/v_count;
		v[i].x= pos.x + cos(a)*radius;
		v[i].y= pos.y + sin(a)*radius;
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
	V2d v[count];
	for (U32 i= 0; i < count; ++i) {
		v[i].x= verts[i].x;
		v[i].y= verts[i].y;
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

	dt= 1.0/60.0;

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


		b->tf_changed= !equals_v3d(b->prev_tf.pos, b->tf.pos) ||
						!equals_qd(b->prev_tf.rot, b->tf.rot);
	}

	if (w->debug_draw) {
		cpSpaceDebugDrawOptions options= {
			.drawCircle= phys_draw_circle,
			.drawPolygon= phys_draw_poly,
			.flags= CP_SPACE_DEBUG_DRAW_SHAPES,
		};
		cpSpaceDebugDraw(w->cp_space, &options);

		Texel *grid= g_env.renderer->grid_ddraw_data;
		for (U32 i= 0; i < GRID_CELL_COUNT; ++i) {
			grid[i].r= MIN(w->grid[i].dynamic_portion*3, 255);
			grid[i].g= 0;
			grid[i].b= MIN(w->grid[i].static_portion*3, 255);
			grid[i].a= MIN(w->grid[i].static_portion*3 + w->grid[i].dynamic_portion*3, 255);
		}
	}
}
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
				modify_grid_with_shapes(-1,
						b->polys, b->poly_count,
						b->circles, b->circle_count,
						v3d_to_v2d(b->prev_tf.pos), b->prev_tf.rot);
			}
			modify_grid_with_shapes(1,
					b->polys, b->poly_count,
					b->circles, b->circle_count,
					v3d_to_v2d(b->tf.pos), b->tf.rot);
		}

		b->is_in_grid= true;
		b->shape_changed= false;
		b->tf_changed= false;
		b->prev_tf= b->tf;
	}
	
}
