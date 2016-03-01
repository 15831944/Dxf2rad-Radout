/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


The MIT License (MIT)

Copyright (c) 1999-2016 Georg Mischler
(Originally acquired from Philip Thompson)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* This file contains routines that perform geometry-related operations
 * on matrices.
 */

#include <stdio.h>
#include <math.h>

#ifdef UNIX
	#include <unistd.h>
#endif
#ifdef _WIN32
	#include <stdlib.h>
#endif

#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"

#define MAT3_EPSILON    1e-12           /* Close enough to zero   */

/* DESCR: Take a matrix that transforms points and return a matrix
 *   appropriate for transforming directions. This is just
 *   the upper 3 x 3 block of the source matrix.
 */
void M4DirectionMatrix(Matrix4 mat, /* Source matrix */
					   Matrix4 res) /* Matrix for transforming directions */

{
	M4Copy(mat, res);
	res[0][3] = res[1][3] = res[2][3] = 0.0;
	res[3][3] = 1.0;
}


/* DESCR: This takes a matrix used to transform points, and returns a
 *   corresponding matrix that can be used to transform vectors
 *   that must remain perpendicular to planes defined by the
 *   points.  It is useful when you are transforming some object
 *   that has both points and normals in its definition, and you
 *   only have the transformation matrix for the points. This
 *   routine returns FALSE if the normal matrix is uncomputable.
 *   Otherwise, it returns TRUE. The computed matrix is the
 *   adjoint for the non-homogeneous part of the transformation.
 * RETURNS: FALSE if the normal matrix is uncomputable, true otherwise.
 * DETAILS: The normal matrix is uncomputable if the determinant of the
 *   source matrix is too small in absolute value (i.e.,
 *   approximately zero). 
 */
int M4NormalMatrix(Matrix4 mat, /* The matrix from which it is computed */
				   Matrix4 res) /* The compute normal matrix */

{
	register int ret;
	Matrix4 tmp_mat;

	M4DirectionMatrix(mat, tmp_mat);
	if ((ret = M4Invert(tmp_mat, tmp_mat)) != 0)
		M4Transpose(tmp_mat, res);
	return ret;
}


/* DESCR: Takes a 3-vector of scale values, and creates a matrix with
 *   these as its diagonal entries and zeroes off the diagonal,
 *   i.e., a scale matrix with scale factors given by the entries
 *   of the given vector.
 * RETURNS: void
 */
void M4VecScale(Matrix4 mat,   /* A diagonal matrix (a scaling matrix) */
				Vector3 *scale) /* The entries for the diagonal */

{
	M4SetIdentity(mat);
	mat[0][0] = scale->x;
	mat[1][1] = scale->y;
	mat[2][2] = scale->z;
}


/* DESCR: Sets up a matrix for a rotation about an axis given by the
 *   line from (0,0,0) to @axis@, through an angle given by
 *   @angle_in_radians@.  Looking along the axis toward the
 *   origin, the rotation is counter-clockwise. The matrix is
 *   returned in @resMat@.
 * RETURNS: void
 */
void M4Rotate(Matrix4 resMat,    /* Rotation matrix produced by the routine */
			  Vector3 *axis,     /* The axis of the rotation matrix */
			  double angle_in_radians) /* The angle of rotation */

{
	Vector3 base1, /* 1st unit basis vec, normalized axis */
	base2,        /* 2nd unit basis vec, perp to axis */
	base3;        /* 3rd unit basis vec, perp to axis & base2 */
	Matrix4 base_mat,   /* Change-of-basis matrix */
	base_mat_trans;    /* Inverse of c-o-b matrix */
	double dot = 0.0;

	/* Step 1: extend { axis } to a basis for 3-space: { axis, base2,
	 * base3 } which is orthonormal (all three have unit length, and
	 * all three are mutually orthogonal). Also should be oriented,
	 * i.e. axis cross base2 = base3, rather than -base3. Method:
	 * Find a vector linearly independent from axis. For this we
	 * either use the y-axis, or, if that is too close to axis, the
	 * z-axis. 'Too close' means that the dot product is too near to 1.
	 */
	base1 = *axis;
	dot = V3Normalize(&base1);
	/* If the axis was too short to normalize (i.e., it is almost a 
	 * zero length vector), V3Normalize will set dot to 0.  In
	 * this case, just return the identity matrix (no rotation).
	 */
	if (dot == 0.0) {
		M4SetIdentity(resMat);
		return;
	}
	V3PerpVec(&base1, TRUE, &base2);
	(void)V3Cross(&base1, &base2, &base3);

	/* Set up the change-of-basis matrix, and its inverse */
	M4SetIdentity(base_mat);
	M4SetIdentity(base_mat_trans);
	M4SetIdentity(resMat);
	base_mat[0][0] = base_mat_trans[0][0] = base1.x;
	base_mat[0][1] = base_mat_trans[1][0] = base2.x;
	base_mat[0][2] = base_mat_trans[2][0] = base3.x;
	base_mat[1][0] = base_mat_trans[0][1] = base1.y;
	base_mat[1][1] = base_mat_trans[1][1] = base2.y;
	base_mat[1][2] = base_mat_trans[2][1] = base3.y;
	base_mat[2][0] = base_mat_trans[0][2] = base1.z;
	base_mat[2][1] = base_mat_trans[1][2] = base2.z;
	base_mat[2][2] = base_mat_trans[2][2] = base3.z;

	/* If T(u) = Ru, where R is base_mat, then T(x-axis) = axis,
	 * T(y-axis) = base2, and T(z-axis) = base3. The inverse of
	 * base_mat is its transpose.  OK?
	 */
	resMat[1][1] = resMat[2][2] = cos(angle_in_radians);
	resMat[1][2] = -(resMat[2][1] = sin(angle_in_radians));
	M4MatMult(resMat, base_mat_trans, resMat);
	M4MatMult(base_mat, resMat, resMat);
}


/* Get the rotation matrix of a vector about a an arbitrary axis
 * through the origin. "Computer Graphics, p. 227" has a typo, beware!
 */
void M4RotateAboutAxis(Vector3 *dir, double rotang, Matrix4 mat)

{
	double xx = dir->x * dir->x;
	double yy = dir->y * dir->y;
	double zz = dir->z * dir->z;
	double xy = dir->x * dir->y;
	double yz = dir->y * dir->z;
	double zx = dir->z * dir->x;
	double cos1 = 1.0 - cos(rotang);
	double cosa = cos(rotang);
	double sina = sin(rotang);

	mat[0][0] = xx + cosa * (1.0 - xx);
	mat[1][0] = xy * cos1 + dir->z * sina;
	mat[2][0] = zx * cos1 - dir->y * sina;
	mat[0][1] = xy * cos1 - dir->z * sina;
	mat[1][1] = yy + cosa * (1.0 - yy);
	mat[2][1] = yz * cos1 + dir->x * sina;
	mat[0][2] = zx * cos1 + dir->y * sina;
	mat[1][2] = yz * cos1 - dir->x * sina;
	mat[2][2] = zz + cosa * (1.0 - zz);
	/* fill in the rest */
	mat[0][3] = mat[1][3] = mat[2][3] = 0.0;
	mat[3][0] = mat[3][1] = mat[3][2] = 0.0;
	mat[3][3] = 1.0;
}


/* DESCR: Given the unit-length normal to a plane (@normal@) through
 *   the origin, this sets @resMat@ to one that reflects
 *   points through that plane.
 * RETURNS: void
 */
void M4Mirror(Matrix4 resMat,  /* A matrix that reflects through given plane */
			  Vector3 *normal) /* Plane normal to plane through origin  */

{
	Matrix4 ab, c;
	Vector3 basis2, basis3;

	/* Find an orthonormal basis with the normal as one vector */
	V3PerpVec(normal, TRUE, &basis2);
	(void)V3Cross(normal, &basis2, &basis3);

	/* Mirroring is A (change of basis) x B (mirror) x C (change basis
	 * back). The A matrix uses the basis vectors as rows, and C uses 
	 * them as columns.  B is the identity matrix with the first
	 * entry a -1 instead of a 1.  A and B are implicitly multiplied 
	 * here by negating the first row of A. */
	M4SetIdentity(ab);
	ab[0][0] = -normal->x;  /* Negate first row */
	ab[1][0] = basis2.x;
	ab[2][0] = basis3.x;
	ab[0][1] = -normal->y;  /* Negate first row */
	ab[1][1] = basis2.y;
	ab[2][1] = basis3.y;
	ab[0][2] = -normal->z;  /* Negate first row */
	ab[1][2] = basis2.z;
	ab[2][2] = basis3.z;
	/* Create the C matrix with the basis as column vectors */
	M4SetIdentity(c);
	c[0][0] = normal->x;
	c[0][1] = basis2.x;
	c[0][2] = basis3.x;
	c[1][0] = normal->y;
	c[1][1] = basis2.y;
	c[1][2] = basis3.y;
	c[2][0] = normal->z;
	c[2][1] = basis2.z;
	c[2][2] = basis3.z;
	/* Multiply them */
	M4MatMult(c, ab, resMat);
}


/* DESCR: Sets @resMat@ to be a shear matrix that does the
 *   following:Given a plane normal (@normal@) and a shear vector
 *   (@shear@), the transformation leaves points in the plane
 *   (through the origin, perp to the normal) fixed, while moving
 *   points at a distance d from the plane by d times the shear
 *   vector.  (The shear vector is first made to be perpendicular
 *   to the normal.) The normal vector is assumed to be unit
 *   length, and results are incorrect if it is not.
 * RETURNS: void
 */
void M4Shear(Vector3 *normal, /* The normal to the shear plane */
			 Vector3 *shear,  /* The shearing vector [perp to normal] */
			 Matrix4 result)  /* A shear matrix generated by this routine  */

{
	double dot;
	Vector3 shear_used;

	shear_used = *shear;
	M4SetIdentity(result);
	/* Make sure shear vector is perpendicular to the normal */
	dot = V3Dot(normal, shear);
	if (!IS_ZERO(dot, MAT3_EPSILON)) {
		/* shear_used = shear2; */
		shear_used = *normal;
		(void)V3Scale(&shear_used, dot);
		(void)V3Sub(&shear_used, shear, &shear_used);
	}
	/* Set columns to coordinate axes transformed by shear */
	result[0][0] += normal->x * shear_used.x;
	result[0][1] += normal->y * shear_used.x;
	result[0][2] += normal->z * shear_used.x;
	result[1][0] += normal->x * shear_used.y;
	result[1][1] += normal->y * shear_used.y;
	result[1][2] += normal->z * shear_used.y;
	result[2][0] += normal->x * shear_used.z;
	result[2][1] += normal->y * shear_used.z;
	result[2][2] += normal->z * shear_used.z;
}


/* DESCR: Given @from@, @at@, and @up@ points, this routine computes a
 *   matrix (@resMat@) that implements an orientation
 *   transformation.  The transformation brings the coordinate
 *   axes to a new position. The origin is moved to @from@, the
 *   negative Z-axis is transformed so that it points in the
 *   direction from @from@ to @at@, and the positive Y-axis is
 *   transformed so that it points in the direction P(@up-from@),
 *   where P is a function that takes a vector and makles it
 *   perpendicular to the @from-at@ vector.
 * RETURNS: TRUE if the three given points are non-colinear, FALSE
 *   otherwise, in which case the returned matrix contains garbage.
 * DETAILS: To be more precise, one could say that the translation
 *   component of the returned matrix is just translation from the
 *   origin to the @from@ point, and that the rest of the matrix
 *   is computed by considering the vectors @at@ - @from@ and @up@
 *   - @from@. These two are run through the gram-schmidt process,
 *   rendering them orthonormal, and then extended to an oriented
 *   basis of 3-space. The standard basis is transformed to this
 *   basis b1, b2, b3, by sending the negative Z axis to b1, the
 *   positive Y axis to b2, and the positive X axis to b3.
 */
int M4Orient(Vector3 *from,  /* Point to which origin will be translated */
			 Vector3 *at,    /* Z-axis is transformed to from-at */
			 Vector3 *up,    /* Y-axis is transformed to from-up */
			 Matrix4 resMat) /* Orientation matrix produced by routine */

{
	double t;
	Vector3 basis1, basis2, basis3;

	/* Find a unit vector between 'from' and 'at' */
	(void)V3Sub(from, at, &basis3);
	(void)V3Normalize(&basis3);

	/* Extend to an orthonormal basis of 3-space */
	(void)V3Sub(up, from, &basis2);

	t = V3Dot(&basis3, &basis2);
	(void)V3Combine(&basis2, &basis3, &basis2, 1.0, -t);
	if (V3Normalize(&basis2) == 0.0) {
		fprintf(stderr, "Dependent from, at, and up points in M4Orient");
		return FALSE;
	}
	(void)V3Cross(&basis2, &basis3, &basis1);

	/* Create the matrix with the basis as row vectors, and "from" as
	 * translation */
	M4SetIdentity(resMat);
	resMat[0][0] = basis1.x;
	resMat[0][1] = basis2.x;
	resMat[0][2] = basis3.x;
	resMat[1][0] = basis1.y;
	resMat[1][1] = basis2.y;
	resMat[1][2] = basis3.y;
	resMat[2][0] = basis1.z;
	resMat[2][1] = basis2.z;
	resMat[2][2] = basis3.z;
	resMat[0][3] = from->x;
	resMat[1][3] = from->y;
	resMat[2][3] = from->z;
	return TRUE;
}


/* DESCR: Given @from@, @to@, and @up@ points, this computes a matrix
 *   implementing a span transformation; a span transformation
 *   takes a bi-unit object (e.g., the cube extending from -1 to 1
 *   in each axis) and stretches, rotates, and translates it so
 *   that what was the Z-axis of the objects extends from @from@
 *   to @to@, and the former Y-axis of the object is aligned with
 *   the vector @up@ (which is made perpendicular to @from@ - @to@).
 *   If the flag @vol_preserve@ is TRUE, the
 *   transformation also scales in the (original) Y- and X-
 *   directions to keep the determinant of the upper 3x3 part of
 *   the transformation being 1.  
 * RETURNS: TRUE if computation succeeded, FALSE if @from@, @to@, and
 *   @up@ are colinear, in which case the computed matrix contains
 *   garbage. 
 */
int M4Span(Vector3 *from,    /* Where the -1 point on Z-axis goes   */
		   Vector3 *at,      /* Where the +1 point in the Z-axis goes */
		   Vector3 *up,      /* Where the Y-axis goes */
		   int vol_preserve, /* Should we preserve volume?  */
		   Matrix4 resMat)   /* Computed span transformation matrix */

{
	Matrix4 scale_mat;
	Vector3 middle, scale_vec;
	double length, x_scale;

	(void)V3Sub(at, from, &scale_vec);
	length = V3Length(&scale_vec) / 2.0;  /* am assuming object of */
	if (vol_preserve) {      /* bi-unit length to be spanned */
		x_scale = 1.0 / sqrt(length);
		V3SET(x_scale, x_scale, length, &scale_vec);
	} else
		V3SET(1.0, 1.0, length, &scale_vec);
	M4VecScale(scale_mat, &scale_vec);

	(void)V3Add(at, from, &middle);
	(void)V3Scale(&middle, 0.5);
	if (!M4Orient(&middle, at, up, resMat)) {
		fprintf(stderr, "M4Orient failed");
		return FALSE;
	}
	M4MatMult(resMat, scale_mat, resMat);
	return TRUE;
}


#define EPSILON_JUMP    (1e3)

/* DESCR: Given a rotation matrix @mat@, find the axis about which it
 *   rotates and how many radians it rotates about that axis.  It
 *   returns the axis vector in @axis@ and the angle in @angle@.
 * RETURNS: TRUE.
 * DETAILS: Method: the axis of rotation (v) is an eigenvector of the
 *   rotation matrix (M).  Its associated eigenvalue is 1. Mv =
 *   (lambda)v ==> Mv = v ==> Mv = Iv ==> (M - I)v = 0.  So v is
 *   in the kernel of M - I. Then grab a vector perpendicular to v
 *   (arm), extend it to a basis (arm2). Apply M to arm and
 *   project that onto arm and arm2.  These are the sin and cos of
 *   the angle, so use atan2 to find the angle. This returns TRUE
 *   if all goes well, FALSE otherwise. Note: Rodriguez's formula
 *   could be used instead, and would be far faster.
 */
int M4GetAxisAndAngle(Matrix4 mat,   /* A rotation matrix to be studied */
					  Vector3 *axis, /* The axis of rotation */
					  double *angle) /* The amount of the rotation */

{
	int i;
	Matrix4 minus_ident, basis;         /* basis for kernel     */
	Vector3 arm, arm2, rotated_arm;
	double epsilon = EPSILON_JUMP;

	/* Subtract identity from rotation matrix (upper 3x3) */
	M4Copy(mat, minus_ident);
	for (i = 0; i < 3; i++) {
		minus_ident[i][i] -= 1.0;
		minus_ident[3][i] = minus_ident[i][3] = 0.0;
	}
	minus_ident[3][3] = 1.0;

	/* A very-near rotation matrix might be rejected due to epsilon
	 * error.  This occurs in the MAT4KernelBasis.  So, if finding
	 * the kernel fails, try a larger epsilon.  larger epsilon = old
	 * epsilon times EPSILON_JUMP */
	for (epsilon = MAT3_EPSILON; !M4KernelBasis(basis, minus_ident, epsilon);
			epsilon *= EPSILON_JUMP) ;
	axis->x = basis[0][0];
	axis->y = basis[0][1];
	axis->z = basis[0][2];
	(void)V3Normalize(axis);
	V3PerpVec(axis, TRUE, &arm);
	(void)V3Cross(axis, &arm2, &arm);
	(void)M4MultVector3(&arm, mat, &rotated_arm);
	*angle = atan2(V3Dot(&arm2, &rotated_arm), V3Dot(&arm, &rotated_arm));
	return TRUE;
}


/* DESCR: Compute the transformation matrix needed to align @vec1@ with
 *   @vec2@, storing the result in @resMat@.
 * RETURNS: void
 * DETAILS: The final tranformation matrix is computed as the product of
 *   two intermediate transformation matrices.  The first of these
 *   aligns @vec1@ with the X-axis.  To compute this matrix we
 *   simply take the inverse of the matrix to align the X-axis
 *   with @vec1@ (easily computed by extending @vec1@ to an
 *   orthonormal basis in 3-space).  The second intermediate
 *   matrix aligns the X-axis with @vec2@.  This is generated by
 *   extending @vec2@ to an orthonormal basis in 3-space.
 */
#define UNIT 1
void M4Align(Vector3 *vec1,  /* Vector to be transformed */
			 Vector3 *vec2,  /* move vec1 so it points in same dir'n as vec2 */
			 Matrix4 resMat) /* Computed matrix aligning vec1 with vec2 */

{
	Vector3 v, vp, vpp;
	Vector3 w, wp, wpp;
	Matrix4 a_t, b;

	/* Compute the three basis vectors for the transformation matrix to 
	 * align the X-axis with "vec1".  Basis 1 ("v") is the normalized
	 * "vec1".  Basis 2 ("vp") is a vector perpendicular to "v".
	 * Basis  3 ("vpp") is the cross product of "v" and "vp". */
	v = *vec1;
	(void)V3Normalize(&v);
	V3PerpVec(&v, UNIT, &vp);
	(void)V3Cross(&v, &vp, &vpp);

	/* Set up the transformation matrix to align "vec1" with the *
	 * X-axis.  This is the inverse of the matrix that would be formed
	 * from the basis vectors just computed.  Since the basis vectors
	 * are orthonormal, the inverse is simply the transpose. */
	a_t[0][0] = v.x;
	a_t[0][1] = vp.x;
	a_t[0][2] = vpp.x;
	a_t[0][3] = 0.0;
	a_t[1][0] = v.y;
	a_t[1][1] = vp.y;
	a_t[1][2] = vpp.y;
	a_t[1][3] = 0.0;
	a_t[2][0] = v.z;
	a_t[2][1] = vp.z;
	a_t[2][2] = vpp.z;
	a_t[2][3] = 0.0;
	a_t[3][0] = 0.0;
	a_t[3][1] = 0.0;
	a_t[3][2] = 0.0;
	a_t[3][3] = 1.0;

	/* Compute the three basis vectors for the transformation matrix to 
	 * align the X-axis with "vec2".  The process is the same as that
	 * used for the previous set of basis vectors. */
	w = *vec2;
	(void)V3Normalize(&w);
	V3PerpVec(&w, UNIT, &wp);
	(void)V3Cross(&w, &wp, &wpp);

	/* Set up the transformation matrix to align the X-axis with
	 * "vec2".  This is done directly using the basis vectors just
	 * computed.  */
	b[0][0] = w.x;
	b[0][1] = w.y;
	b[0][2] = w.z;
	b[0][3] = 0.0;
	b[1][0] = wp.x;
	b[1][1] = wp.y;
	b[1][2] = wp.z;
	b[1][3] = 0.0;
	b[2][0] = wpp.x;
	b[2][1] = wpp.y;
	b[2][2] = wpp.z;
	b[2][3] = 0.0;
	b[3][0] = 0.0;
	b[3][1] = 0.0;
	b[3][2] = 0.0;
	b[3][3] = 1.0;
	/* Compute the final transformation matrix (to align "vec1" with
	 * "vec2") as the product of the two transformation matrices just 
	 * computed.  */
	M4MatMult(a_t,b, resMat);
}


/* DESCR:   Computes a unit vector perpendicular to @vec@, and stores it
 *      in @resVec@. If @is_unit@ is TRUE, @vec@ is assumed to be
 *      of unit length (possibly allowing a time savings by avoiding
 *      normalization of @resVec@).
 * RETURNS: void
 * DETAILS: The method used takes advantage of this technique: As it is
 *      easier to compute a perpendicular vector in R2 than R3, we
 *      essentially do that.  We first determine the component of
 *      @vec@ with the mimimum absolute value, setting corresponding
 *      component of @resVec@ to 0. We next set the other two
 *      components of @resVec@ by swapping the remaining
 *      (non-mimimum) components of @vec@ and negating one of these.
 *      Then we normalize as necessary.
 */
void V3PerpVec(Vector3 *vec,   /* A vector to which a perp is needed (input) */
			   int is_unit,    /* TRUE is input vector is a unit vector */
			   Vector3 *resVec)/* The vecotr perp to vec (output) */

{
	double min = 0.0;
	Vector3 abs_vec;

	/* Compute the absolute value of the components of the input vector.
	 */
	abs_vec.x = fabs(vec->x);
	abs_vec.y = fabs(vec->y);
	abs_vec.z = fabs(vec->z);
	/* Generate the perpendicular vector by setting the "min" compenent
	 * to 0, and swapping the "other1" and "other2' components from
	 * "vec", negating one of these.
	 */
	if (abs_vec.x <= abs_vec.y)
		if (abs_vec.x <= abs_vec.z) {
			min = resVec->x = 0.0;
			resVec->y = vec->z;
			resVec->z = -vec->y;
		} else {
			min = resVec->z = 0.0;
			resVec->x = vec->y;
			resVec->y = -vec->x;
		}
		else
			if (abs_vec.y <= abs_vec.z) {
				min = resVec->y = 0.0;
				resVec->x = vec->z;
				resVec->z = -vec->x;
			} else {
				min = resVec->z = 0.0;
				resVec->x = vec->y;
				resVec->y = -vec->x;
			}
	/* If the input vector has unit length and the component with the
	 * mimimum absolute value is 0, the computed perpendicular vector
	 * already has unit length.  Otherwise, we must normalize it.
	 */
	if (!is_unit || min != 0.0)
		(void)V3Normalize(resVec);
}


/*  Create a matrix that transforms from an Entity CS
 *  to the parent CS, as used for Autocad entities.
 */
#define ARBBOUND   0.015625  /* 1/64th  */

void M4GetAcadXForm(Matrix4 mx, Vector3 *zaxis,
			  int isinsert, Vector3 *inpt, double rotang,
			  double xscl, double yscl, double zscl, Vector3 *bpt)

{
	Vector3 xtilt, ytilt, ztilt;
	Point3 pt;
	Matrix4 mxrot;

	if (zaxis == NULL) {
		M4SetIdentity(mx);
		V3SET(0.0, 0.0, 1.0, zaxis);
	} else {
		/* find the axis of the entity coordinate system (ECS)
		 * specified by the entity's X,Y and Z DXF field. */
		if ((fabs(zaxis->x) < ARBBOUND) && (fabs(zaxis->y) < ARBBOUND)) {
			V3SET(0.0, 1.0, 0.0, &ytilt);
			(void)V3Cross(&ytilt, zaxis, &xtilt);
		} else {
			V3SET(0.0, 0.0, 1.0, &ztilt);
			(void)V3Cross(&ztilt, zaxis, &xtilt);
		}
		(void)V3Normalize(&xtilt);
		(void)V3Cross(zaxis, &xtilt, &ytilt);
		(void)V3Normalize(&ytilt);
		ztilt = *zaxis;
		/* othogonal matrix stuff, see Computer Graphics - sec 5.7 */
		M4SetFromPoints(mx, &xtilt, &ytilt, &ztilt);
	}
	if (!isinsert)
		return;

	if (inpt != NULL)
		(void)M4MultVector3(inpt, mx, &pt);
	if (rotang != 0.0) {
		if ((ztilt.x == 0.0) && (ztilt.y == 0.0) && (ztilt.z == 1.0))
			M4RotateZ(mx, rotang);
		else {
			M4RotateAboutAxis(&ztilt, rotang, mxrot);
			M4MatMult(mxrot, mx, mx);
		}
	}
	M4Scale(mx, xscl, yscl, zscl);
	M4Translate(mx, pt.x, pt.y, pt.z);

	/* the block base point */
	if (bpt != NULL) {
		(void)M4MultVector3(bpt, mx, &pt);
		M4Translate(mx, -pt.x, -pt.y, -pt.z);
	}
}


/*  Transform a list of Poly3s. */
extern void
M4TransformPolys(Poly3 *polys, Matrix4 matrix)
{
	int i;
	Point3 pt;
	Poly3 *poly;

	for (poly = polys; poly; poly = poly->next) {
		for (i = 0; i < (int)poly->nverts; i++) {
			(void)M4MultPoint3(&poly->verts[i], matrix, &pt);
			poly->verts[i] = pt;
		}
	}
}

/*  Transform a list of Cyl3s. */
extern void
M4TransformCyls(Cyl3 *cyls, Matrix4 matrix)
{
	double oldrad;
	Point3 pt0, pt;
	Cyl3 *cyl;

	for (cyl = cyls; cyl; cyl = cyl->next) {
		oldrad = cyl->srad;
		pt0 = cyl->svert;
		pt0.x += cyl->srad;
		/* start point */
		(void)M4MultPoint3(&cyl->svert, matrix, &pt);
		cyl->svert = pt;
		/* end point */
		if(cyl->erad != 0.0) {
			(void)M4MultPoint3(&cyl->evert, matrix, &pt);
			cyl->evert = pt;
		}
		/* normal */
		(void)M4MultVector3(&cyl->normal, matrix, &pt);
		cyl->normal = pt;
		/* side point */
		(void)M4MultPoint3(&pt0, matrix, &pt);
		/* calculate new radius */
		cyl->srad = V3DistanceBetween2Points(&cyl->svert, &pt);
		if(cyl->erad != 0.0) cyl->erad = cyl->srad;
		/* calculate new length */
		if(cyl->length != 0.0) cyl->length = cyl->length*cyl->srad/oldrad;
	}
}


extern Poly3 *
M4TransformPolysCopy(Poly3 *polys, Matrix4 matrix)
{
	int i;
	Poly3 *poly, *newpoly, *outpoly = NULL;

	for (poly = polys; poly; poly = poly->next) {
		newpoly = Poly3Alloc(poly->nverts, poly->closed, poly->next);
		newpoly->material = poly->material;
		for (i = 0; i < (int)poly->nverts; i++) {
			(void)M4MultPoint3(&poly->verts[i], matrix, &newpoly->verts[i]);
		}
		newpoly->next = outpoly;
		outpoly = newpoly;
	}
	return outpoly;
}


extern Cyl3 *
M4TransformCylsCopy(Cyl3 *cyls, Matrix4 matrix)
{
	Cyl3 *cyl, *newcyl, *outcyls = NULL;

	for (cyl = cyls; cyl; cyl = cyl->next) {
		newcyl = Cyl3Alloc(outcyls);
		if(newcyl == NULL)return NULL;
		newcyl->material = cyl->material;
		newcyl->svert = cyl->svert;
		newcyl->evert = cyl->evert;
		newcyl->normal = cyl->normal;
		newcyl->srad = cyl->srad;
		newcyl->erad = cyl->erad;
		newcyl->length = cyl->length;
		outcyls = newcyl;
	}
	(void)M4TransformCyls(outcyls, matrix);
	return outcyls;
}



/*  Transform a list of SimpleTexts. */
extern void
M4TransformSimpleTexts(SimpleText *texts, Matrix4 matrix)
{
	Point3 pt;
	SimpleText *text;

	for (text = texts; text; text = text->next) {
		(void)M4MultPoint3(&text->position, matrix, &pt);
		text->position = pt;
	}
}

/* refcopy dosen't copy the actual text data! */
extern SimpleText *
M4TransformSimpleTextRefcopy(SimpleText *texts, Matrix4 matrix)
{
	SimpleText *text, *newtext, *outtexts = NULL;

	for (text = texts; text; text = text->next) {
		newtext = SimpleTextAlloc(NULL, outtexts);
		if(newtext == NULL)return NULL;
		newtext->position = text->position;
		if(text->s != NULL) {
			newtext->ref = text;
		} else if(text->ref != NULL && text->ref->s != NULL) {
			newtext->ref = text->ref;
		}
		outtexts = newtext;
	}
	(void)M4TransformSimpleTexts(outtexts, matrix);
	return outtexts;
}


/*** end m4geom.c ***/
