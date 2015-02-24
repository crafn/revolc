/* Copyright (c) 2013 Scott Lembcke and Howling Moon Software
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/// @defgroup cpArbiter cpArbiter
/// The cpArbiter struct tracks pairs of colliding shapes.
/// They are also used in conjuction with collision handler callbacks
/// allowing you to retrieve information on the collision or change it.
/// A unique arbiter value is used for each pair of colliding objects. It persists until the shapes separate.
/// @{

#define CP_MAX_CONTACTS_PER_ARBITER 2

/// Get the restitution (elasticity) that will be applied to the pair of colliding objects.
cpFloat cpArbiterGetRestitution(const cpArbiter *arb);
/// Override the restitution (elasticity) that will be applied to the pair of colliding objects.
void cpArbiterSetRestitution(cpArbiter *arb, cpFloat restitution);
/// Get the friction coefficient that will be applied to the pair of colliding objects.
cpFloat cpArbiterGetFriction(const cpArbiter *arb);
/// Override the friction coefficient that will be applied to the pair of colliding objects.
void cpArbiterSetFriction(cpArbiter *arb, cpFloat friction);

// Get the relative surface velocity of the two shapes in contact.
cpVect cpArbiterGetSurfaceVelocity(cpArbiter *arb);

// Override the relative surface velocity of the two shapes in contact.
// By default this is calculated to be the difference of the two surface velocities clamped to the tangent plane.
void cpArbiterSetSurfaceVelocity(cpArbiter *arb, cpVect vr);

/// Get the user data pointer associated with this pair of colliding objects.
cpDataPointer cpArbiterGetUserData(const cpArbiter *arb);
/// Set a user data point associated with this pair of colliding objects.
/// If you need to perform any cleanup for this pointer, you must do it yourself, in the separate callback for instance.
void cpArbiterSetUserData(cpArbiter *arb, cpDataPointer userData);

/// Calculate the total impulse including the friction that was applied by this arbiter.
/// This function should only be called from a post-solve, post-step or cpBodyEachArbiter callback.
cpVect cpArbiterTotalImpulse(const cpArbiter *arb);
/// Calculate the amount of energy lost in a collision including static, but not dynamic friction.
/// This function should only be called from a post-solve, post-step or cpBodyEachArbiter callback.
cpFloat cpArbiterTotalKE(const cpArbiter *arb);

/// Mark a collision pair to be ignored until the two objects separate.
/// Pre-solve and post-solve callbacks will not be called, but the separate callback will be called.
cpBool cpArbiterIgnore(cpArbiter *arb);

/// Return the colliding shapes involved for this arbiter.
/// The order of their cpSpace.collision_type values will match
/// the order set when the collision handler was registered.
void cpArbiterGetShapes(const cpArbiter *arb, cpShape **a, cpShape **b);

/// A macro shortcut for defining and retrieving the shapes from an arbiter.
#define CP_ARBITER_GET_SHAPES(__arb__, __a__, __b__) cpShape *__a__, *__b__; cpArbiterGetShapes(__arb__, &__a__, &__b__);

/// Return the colliding bodies involved for this arbiter.
/// The order of the cpSpace.collision_type the bodies are associated with values will match
/// the order set when the collision handler was registered.
void cpArbiterGetBodies(const cpArbiter *arb, cpBody **a, cpBody **b);

/// A macro shortcut for defining and retrieving the bodies from an arbiter.
#define CP_ARBITER_GET_BODIES(__arb__, __a__, __b__) cpBody *__a__, *__b__; cpArbiterGetBodies(__arb__, &__a__, &__b__);

/// A struct that wraps up the important collision data for an arbiter.
struct cpContactPointSet {
	/// The number of contact points in the set.
	int count;
	
	/// The normal of the collision.
	cpVect normal;
	
	/// The array of contact points.
	struct {
		/// The position of the contact on the surface of each shape.
		cpVect pointA, pointB;
		/// Penetration distance of the two shapes. Overlapping means it will be negative.
		/// This value is calculated as cpvdot(cpvsub(point2, point1), normal) and is ignored by cpArbiterSetContactPointSet().
		cpFloat distance;
	} points[CP_MAX_CONTACTS_PER_ARBITER];
};

/// Return a contact set from an arbiter.
cpContactPointSet cpArbiterGetContactPointSet(const cpArbiter *arb);

/// Replace the contact point set for an arbiter.
/// This can be a very powerful feature, but use it with caution!
void cpArbiterSetContactPointSet(cpArbiter *arb, cpContactPointSet *set);

/// Returns true if this is the first step a pair of objects started colliding.
cpBool cpArbiterIsFirstContact(const cpArbiter *arb);
/// Returns true if the separate callback is due to a shape being removed from the space.
cpBool cpArbiterIsRemoval(const cpArbiter *arb);

/// Get the number of contact points for this arbiter.
int cpArbiterGetCount(const cpArbiter *arb);
/// Get the normal of the collision.
cpVect cpArbiterGetNormal(const cpArbiter *arb);
/// Get the position of the @c ith contact point on the surface of the first shape.
cpVect cpArbiterGetPointA(const cpArbiter *arb, int i);
/// Get the position of the @c ith contact point on the surface of the second shape.
cpVect cpArbiterGetPointB(const cpArbiter *arb, int i);
/// Get the depth of the @c ith contact point.
cpFloat cpArbiterGetDepth(const cpArbiter *arb, int i);

/// If you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly.
/// You must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own.
cpBool cpArbiterCallWildcardBeginA(cpArbiter *arb, cpSpace *space);
/// If you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly.
/// You must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own.
cpBool cpArbiterCallWildcardBeginB(cpArbiter *arb, cpSpace *space);

/// If you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly.
/// You must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own.
cpBool cpArbiterCallWildcardPreSolveA(cpArbiter *arb, cpSpace *space);
/// If you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly.
/// You must decide how to handle the wildcard's return value since it may disagree with the other wildcard handler's return value or your own.
cpBool cpArbiterCallWildcardPreSolveB(cpArbiter *arb, cpSpace *space);

/// If you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly.
void cpArbiterCallWildcardPostSolveA(cpArbiter *arb, cpSpace *space);
/// If you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly.
void cpArbiterCallWildcardPostSolveB(cpArbiter *arb, cpSpace *space);

/// If you want a custom callback to invoke the wildcard callback for the first collision type, you must call this function explicitly.
void cpArbiterCallWildcardSeparateA(cpArbiter *arb, cpSpace *space);
/// If you want a custom callback to invoke the wildcard callback for the second collision type, you must call this function explicitly.
void cpArbiterCallWildcardSeparateB(cpArbiter *arb, cpSpace *space);

/// @}
