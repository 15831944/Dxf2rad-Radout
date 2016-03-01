/*
This version of this file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.
*/

/*  v3vec.c
 *   3D Vector C Library
 *   Originally from "Graphics GemsIII", Academic Press, 1990
 *   Modified by Philip Thompson
 *
 * As of: http://www.realtimerendering.com/resources/GraphicsGems/
 *
 * EULA: The Graphics Gems code is copyright-protected. In other words, you
 * cannot claim the text of the code as your own and resell it. Using the code
 * is permitted in any program, product, or library, non-commercial or
 * commercial. Giving credit is not required, though is a nice gesture. The
 * code comes as-is, and if there are any flaws or problems with any Gems code,
 * nobody involved with Gems - authors, editors, publishers, or webmasters -
 * are to be held responsible. Basically, don't be a jerk, and remember that
 * anything free comes with no guarantee.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef UNIX
	#include <unistd.h>
#endif

#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"



/* returns squared length of input vector */
double V3SquaredLength(Vector3 *a)

{
	return ((a->x * a->x) + (a->y * a->y) + (a->z * a->z));
}


/* returns length of input vector */
double V3Length(Vector3 *v)

{
	return sqrt((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
}


/* negates the input vector and returns it */
Vector3 *V3Negate(Vector3 *v)

{
	v->x = -v->x;
	v->y = -v->y;
	v->z = -v->z;
	return v;
}


Vector3 *V3Scale(Vector3 *v, double s)

{
	v->x *= s;
	v->y *= s;
	v->z *= s;
	return v;
}


/* normalizes the input vector and returns it */
double V3Normalize(Vector3 *v)

{
	register double len;

	if ((len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z)) != 0.0) {
		v->x /= len;
		v->y /= len;
		v->z /= len;
	}
	return len;
}


/* scales the input vector to the new length and returns it */
Vector3 *V3Normalize2(Vector3 *v, double newlen)

{
	register double len;

	if ((len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z)) != 0.0) {
		v->x *= newlen / len;
		v->y *= newlen / len;
		v->z *= newlen / len;
	}
	return v;
}


/* return vector sum c = a+b */
Vector3 *V3Add(Vector3 *a, Vector3 *b, Vector3 *c)

{
	c->x = a->x + b->x;
	c->y = a->y + b->y;
	c->z = a->z + b->z;
	return c;
}


/* return vector difference c = a-b */
Vector3 *V3Sub(Vector3 *a, Vector3 *b, Vector3 *c)

{
	c->x = a->x - b->x;
	c->y = a->y - b->y;
	c->z = a->z - b->z;
	return c;
}

/* return the dot product of vectors a and b */
double V3Dot(Vector3 *a, Vector3 *b)

{
	return ((a->x * b->x) + (a->y * b->y) + (a->z * b->z));
}


/* returns the anle between 2D vectors, like acad's ads_angle() */
double V3GetAngle2D(Point3 *pt1, Point3 *pt2)

{
	double dx, dy;

	dx = pt2->x - pt1->x;
	dy = pt2->y - pt1->y;
	if ((dx*dx + dy*dy) == 0.0)
		return 0.0;
	return atan2(dy,dx);
}


/* returns the cosine of the angle from a to b to c */
double V3CosineAngle(Point3 *a, Point3 *b, Point3 *c)

{
	Point3 ab, cb;
	double dot, norm1, norm2;

	(void)V3Sub(a, b, &ab);
	(void)V3Sub(c, b, &cb);
	norm1 = V3Dot(&ab, &ab);
	norm2 = V3Dot(&cb, &cb);
	if ((norm1 < EPSILON) || (norm2 < EPSILON)) {
		fprintf(stderr, "V3CosineAngle: cannot compute cosine\n");
		return 0.0;
	}
	dot = V3Dot(&ab, &cb);
	return(dot / sqrt(norm1 * norm2));
}


/* replaces a by a + b*c; */
Point3 *V3AddScalarMult(Point3 *a, double b, Point3 *c)

{
	a->x += b * c->x;
	a->y += b * c->y;
	a->z += b * c->z;
	return a;
}


/* add a 2D polar value to a a point, like autocad's ads_polar() */
Point3 *V3AddPolar2D(Point3 *pt, double angle, double dist, Point3 *rslt)

{
	rslt->x = pt->x + (dist * cos(angle));
	rslt->y = pt->y + (dist * sin(angle));
	rslt->z = pt->z;
	return rslt;
}


/* linearly interpolate between vectors by an amount alpha */
/* and return the resulting vector. */
/* When alpha=0, result=lo.  When alpha=1, result=hi. */
Vector3 *V3Lerp(Vector3 *lo, Vector3 *hi, double alpha,
				Vector3 *result)

{
	result->x = LERP(alpha, lo->x, hi->x);
	result->y = LERP(alpha, lo->y, hi->y);
	result->z = LERP(alpha, lo->z, hi->z);
	return result;
}

/* make a linear combination of two vectors and return the result. */
/* result = (a * ascl) + (b * bscl) */
Vector3 *V3Combine(Vector3 *a, Vector3 *b, Vector3 *result,
				   double ascl, double bscl)

{
	result->x = (ascl * a->x) + (bscl * b->x);
	result->y = (ascl * a->y) + (bscl * b->y);
	result->y = (ascl * a->z) + (bscl * b->z);
	return result;
}


/* multiply two vectors together component-wise and return the result */
Vector3 *V3Mult(Vector3 *a, Vector3 *b, Vector3 *result)

{
	result->x = a->x * b->x;
	result->y = a->y * b->y;
	result->z = a->z * b->z;
	return result;
}

/* return the distance between two points */
double V3DistanceBetween2Points(Point3 *a, Point3 *b)

{
	double dx = a->x - b->x;
	double dy = a->y - b->y;
	double dz = a->z - b->z;
	return (sqrt((dx * dx) + (dy * dy) + (dz * dz)));
}


double V3DistancePoint2Plane(Point3 *pnt, Plane3 *pleq)

{
	return (V3Dot((Point3*)pleq, pnt) + pleq->d);
}


/* return the cross product c = a cross b */
Vector3 *V3Cross(Vector3 *a, Vector3 *b, Vector3 *c)

{
	c->x = (a->y * b->z) - (a->z * b->y);
	c->y = (a->z * b->x) - (a->x * b->z);
	c->z = (a->x * b->y) - (a->y * b->x);
	return c;
}


/* create, initialize, and return a new vector */
Vector3 *V3New(double x, double y, double z)

{
	Vector3 *v = NEWTYPE(Vector3);

	v->x = x;
	v->y = y;
	v->z = z;
	return v;
}


/* create, initialize, and return a duplicate vector */
Vector3 *V3Duplicate(Vector3 *a)

{
	Vector3 *v = NEWTYPE(Vector3);

	v->x = a->x;
	v->y = a->y;
	v->z = a->z;
	return v;
}


/* multiply a vector by a matrix and return the transformed point */
Vector3 *M4MultVector3(Vector3 *in, Matrix4 m, Vector3 *out)

{
	out->x = (in->x * m[0][0]) + (in->y * m[0][1]) + (in->z * m[0][2]);
	out->y = (in->x * m[1][0]) + (in->y * m[1][1]) + (in->z * m[1][2]);
	out->z = (in->x * m[2][0]) + (in->y * m[2][1]) + (in->z * m[2][2]);
	return out;
}


/* multiply a point by a matrix and return the transformed point */
Point3 *M4MultPoint3(Point3 *in, Matrix4 m, Point3 *out)

{
	out->x = (in->x*m[0][0]) + (in->y*m[0][1]) + (in->z*m[0][2]) + m[0][3];
	out->y = (in->x*m[1][0]) + (in->y*m[1][1]) + (in->z*m[1][2]) + m[1][3];
	out->z = (in->x*m[2][0]) + (in->y*m[2][1]) + (in->z*m[2][2]) + m[2][3];
	return out;
}


/* multiply a point by a projective matrix and return the transformed point */
Point3 *V3MulPointByProjMatrix(Point3 *in, Matrix4 m, Point3 *out)

{
	double w;

	out->x = (in->x*m[0][0]) + (in->y*m[1][0]) + (in->z*m[2][0]) + m[3][0];
	out->y = (in->x*m[0][1]) + (in->y*m[1][1]) + (in->z*m[2][1]) + m[3][1];
	out->z = (in->x*m[0][2]) + (in->y*m[1][2]) + (in->z*m[2][2]) + m[3][2];
	w      = (in->x*m[0][3]) + (in->y*m[1][3]) + (in->z*m[2][3]) + m[3][3];
	if (w != 0.0) {
		out->x /= w;
		out->y /= w;
		out->z /= w;
	}
	return out;
}


/*  Translates a point in the direction "dir" by the distance "dist". */
Point3 *V3Translate(Point3 *point, Point3 *dir, double dist, Point3 *result)

{
	result->x = point->x + (dir->x * dist);
	result->y = point->y + (dir->y * dist);
	result->z = point->z + (dir->z * dist);
	return result;
}

/*** end v3Vec.c ***/
