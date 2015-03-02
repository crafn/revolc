#include "global/env.h"
#include "chipmunk_util.h"
#include "core/malloc.h"
#include "physworld.h"
#include "visual/renderer.h" // Debug draw


void create_physworld()
{
	PhysWorld *w= zero_malloc(sizeof(*w));
	ensure(!g_env.phys_world);
	g_env.phys_world= w;

	w->space= cpSpaceNew();
	cpSpaceSetIterations(w->space, 10);
	cpSpaceSetGravity(w->space, cpv(0, -20));
}

void destroy_physworld()
{
	PhysWorld *w= g_env.phys_world;
	ensure(w);
	g_env.phys_world= NULL;
	cpSpaceFree(w->space);

	free(w);
}

internal
U32 alloc_rigidbody_noinit()
{
	PhysWorld *w= g_env.phys_world;

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
	PhysWorld *w= g_env.phys_world;

	ensure(h < MAX_RIGIDBODY_COUNT && w->bodies[h].cp_body == NULL);
	RigidBody *b= &w->bodies[h];
	strncpy(b->def_name, def->res.name, sizeof(b->def_name));
	b->shape_changed= true;

	/// @todo Mass & moment could be precalculated to ResBlob
	F64 total_mass= 0;
	F64 total_moment= 0;
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

	if (!b->is_static)
		b->cp_body= cpBodyNew(total_mass, total_moment);
	else
		b->cp_body= cpBodyNewStatic();
	cpSpaceAddBody(w->space, b->cp_body);
	cpBodySetPosition(b->cp_body, to_cpv((V2d) {b->pos.x, b->pos.y}));

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

	for (U32 i= 0; i < circle_count; ++i) {
		b->cp_shapes[b->cp_shape_count++]=
			cpSpaceAddShape(
					w->space,
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
			cpSpaceAddShape(w->space,
					cpPolyShapeNew(	b->cp_body,
									poly->v_count,
									cp_verts,
									cpTransformIdentity,
									0.0));
	}

	for (U32 i= 0; i < b->cp_shape_count; ++i) {
		cpShape *shape= b->cp_shapes[i];
		cpShapeSetFriction(shape, 0.7);
		cpShapeSetElasticity(shape, 0.7);
	}
}

U32 resurrect_rigidbody(const RigidBody *dead)
{
	PhysWorld *w= g_env.phys_world;

	U32 h= alloc_rigidbody_noinit();
	w->bodies[h]= *dead;
	w->bodies[h].allocated= true;
	w->bodies[h].cp_body= NULL;
	w->bodies[h].cp_shape_count= 0;

	set_rigidbody(
			h,
			(RigidBodyDef*)res_by_name(
				g_env.res_blob,
				ResType_RigidBodyDef,
				dead->def_name));
	return h;
}


void free_rigidbody(U32 h)
{
	PhysWorld *w= g_env.phys_world;

	ensure(h < MAX_RIGIDBODY_COUNT);

	RigidBody *b= &w->bodies[h];
	for (U32 i= 0; i < b->cp_shape_count; ++i) {
		cpSpaceRemoveShape(w->space, b->cp_shapes[i]);
		cpShapeFree(b->cp_shapes[i]);
	}
	cpSpaceRemoveBody(w->space, b->cp_body);
	cpBodyFree(b->cp_body);

	*b= (RigidBody) { .allocated= false };
	--w->body_count;
}

void * storage_rigidbody()
{ return g_env.phys_world->bodies; }

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
	PhysWorld *w= g_env.phys_world;

	dt= 1.0/60.0;

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		if (b->input_force.x != 0.0 || b->input_force.y != 0.0) {
			cpBodyApplyForceAtWorldPoint(
					b->cp_body,
					to_cpv(b->input_force),
					cpv(b->pos.x, b->pos.y));
			b->input_force= (V2d) {};
		}
	}
	/// @todo Accumulation
	cpSpaceStep(w->space, dt*0.5);
	cpSpaceStep(w->space, dt*0.5);

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		cpVect p= cpBodyGetPosition(b->cp_body);
		cpVect r= cpBodyGetRotation(b->cp_body);
		b->pos.x= p.x;
		b->pos.y= p.y;
		b->rot.cs= r.x;
		b->rot.sn= r.y;
	}

	if (w->debug_draw) {
		cpSpaceDebugDrawOptions options= {
			.drawCircle= phys_draw_circle,
			.drawPolygon= phys_draw_poly,
			.flags= CP_SPACE_DEBUG_DRAW_SHAPES,
		};
		cpSpaceDebugDraw(w->space, &options);

		Texel *grid= g_env.renderer->grid_ddraw_data;
		for (U32 i= 0; i < GRID_CELL_COUNT; ++i) {
			grid[i].r= MIN(w->grid[i].dynamic_portion*3, 255);
			grid[i].g= 0;
			grid[i].b= MIN(w->grid[i].static_portion*3, 255);
			grid[i].a= MIN(w->grid[i].static_portion*3 + w->grid[i].dynamic_portion*3, 255);
		}
	}
}

/// Cheap and inaccurate line rasterization
U32 rasterized_line(
		V2i *cells,
		U32 max_cells,
		V2d a,
		V2d b)
{
	V2d dif= sub_v2d(b, a);
	F64 length= length_v2d(dif);
	F64 step_length= 1.0/GRID_RESO_PER_UNIT*0.5; // Arbitrary
	V2d step= scaled_v2d(normalized_v2d(dif), step_length);
	U32 steps= length/step_length;
	U32 added= 0;
	for (U32 i= 0; i < steps; ++i) {
		V2i cell_p= round_v2d_to_v2i(a);
		if (i == 0 || !equals_v2i(cells[added - 1], cell_p)) {
			ensure(added < max_cells);
			cells[added++]= cell_p;
		}
		a= add_v2d(a, step);
	}
	return added;
}

typedef struct PolyCell {
	U8 fill; // true/false
	U8 is_edge;
} PolyCell;

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

	// Get all edge cells
	const U32 max_edge_cell_count= rect_size->x*rect_size->y*2 + 2; // *2+2 just in case
	V2i *edge_cells= frame_alloc(sizeof(*edge_cells)*max_edge_cell_count);
	U32 edge_cell_count= 0;
	for (U32 i= 0; i < v_count; ++i) {
		edge_cell_count += rasterized_line(
				edge_cells + edge_cell_count,
				max_edge_cell_count - edge_cell_count,
				poly->v[i], poly->v[ (i + 1)%v_count ]);
	}
	ensure(edge_cell_count <= max_edge_cell_count);

	for (U32 i= 0; i < edge_cell_count; ++i) {
		ensure(edge_cells[i].x >= ll.x && edge_cells[i].y >= ll.y);
		ensure(edge_cells[i].x < tr.x && edge_cells[i].y < tr.y);
	}

	// Write all edge cells
	const U32 cell_count= rect_size->x*rect_size->y;
	PolyCell *cells= frame_alloc(sizeof(*cells)*cell_count);

	for (U32 edge_i= 0; edge_i < edge_cell_count; ++edge_i) {
		V2i edge_p= edge_cells[edge_i];
		V2i c_p= sub_v2i(edge_p, *rect_ll);
		U32 c_i= c_p.x + c_p.y*rect_size->x;
		ensure(c_i < cell_count);
		cells[c_i].fill= 64;
		cells[c_i].is_edge= true;
	}

	// Fill the polygon
	for (U32 y= 0; y < rect_size->y; ++y) {
		bool edge_countered= false;

		// Sweep left-to-right
		for (U32 x= 0; x < rect_size->x; ++x) {
			const U32 i= x + y*rect_size->x;
			if (cells[i].is_edge)
				edge_countered= true;

			if (!cells[i].is_edge && edge_countered)
				cells[i].fill= 64;
		}

		// Right-to-left
		for (U32 x_= rect_size->x; x_ > 0; --x_) {
			const U32 x= x_ - 1;
			const U32 i= x + y*rect_size->x;
			if (cells[i].is_edge)
				break;

			cells[i].fill= 0;
		}

	}

	return cells;
}

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

			if (distance_sqr_v2d(test_p, circle->pos) >= circle->rad*circle->rad)
				continue;

			U32 i= x + y*size.x;
			ensure(i < cell_count);
			cells[i].fill= 64;
			/// @todo Mark edges
		}
	}
	return cells;
}

typedef enum {
	ShapeType_poly,
	ShapeType_circle
} ShapeType;

void modify_grid(int add_mul, void *shp, ShapeType shp_type, V2d pos, Qd rot)
{
	GridCell *grid= g_env.phys_world->grid;

	// Rasterize
	V2i rect_ll, rect_size;
	PolyCell *poly_cells;
	if (shp_type == ShapeType_poly) {
		Poly poly= *(Poly*)shp;
		// Scale poly so that 1 unit == 1 cell
		for (U32 i= 0; i < poly.v_count; ++i) {
			V2d p= poly.v[i];
			p= rot_v2d_qd(p, rot);
			p= add_v2d(p, pos);
			p= scaled_v2d(p, GRID_RESO_PER_UNIT);
			p= sub_v2d(p, (V2d) {0.5, 0.5});
			poly.v[i]= p;
		}
		poly_cells= rasterized_poly(&rect_ll, &rect_size, &poly);
	} else if (shp_type == ShapeType_circle) {
		Circle circle= *(Circle*)shp;

		// Scale circle so that 1 unit == 1 cell
		V2d p= circle.pos;
		p= rot_v2d_qd(p, rot);
		p= add_v2d(p, pos);
		p= scaled_v2d(p, GRID_RESO_PER_UNIT);
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

void post_upd_physworld()
{
	PhysWorld *w= g_env.phys_world;

	for (U32 i= 0; i < MAX_RIGIDBODY_COUNT; ++i) {
		RigidBody *b= &w->bodies[i];
		if (!b->allocated)
			continue;

		bool t_changed= !equals_v3d(b->prev_pos, b->pos) ||
						!equals_qd(b->prev_rot, b->rot);
		if (t_changed) {
			for (U32 poly_i= 0; poly_i < b->poly_count; ++poly_i) {
				modify_grid(-1, &b->polys[0], ShapeType_poly,
							v3d_to_v2d(b->prev_pos), b->prev_rot);
				modify_grid(1, &b->polys[0], ShapeType_poly,
							v3d_to_v2d(b->pos), b->rot);
			}
			for (U32 circle_i= 0; circle_i < b->circle_count; ++circle_i) {
				modify_grid(-1, &b->circles[0], ShapeType_circle,
							v3d_to_v2d(b->prev_pos), b->prev_rot);
				modify_grid(1, &b->circles[0], ShapeType_circle,
							v3d_to_v2d(b->pos), b->rot);
			}
		}

		b->shape_changed= false;
		b->prev_pos= b->pos;
		b->prev_rot= b->rot;
	}
	
}
