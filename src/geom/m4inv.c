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

/* Routines for computing inverses of 4 x 4 matrices, fine-tuned for
 * the special case where the last columns is the same as that of the
 * identity.
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

#define SMALL 1e-20         /* Small enough to be considered zero */
#define DSWAP(a,b,temp) (temp=(a), (a)=(b), (b)=(temp))
#define MAT3_IS_ZERO(n) ((n) < 1e-12 && (n) > -1e-12)

/* DESCR: Shuffles rows any 3x3 matrix. The matrix @inv@ is
 *   converted by placing the row of the source whose index is the
 *   integer @row0@ into its zeroth row, and similaryl for the
 *   other two. Thus, for example, if @row0@ = 0, @row1@ = 2 and
 *   @row2@ = 1, then the first and second rows are swapped.
 * RETURNS: void
 */
static void MAT3_inv3_swap(double inv[3][3],/* Shuffle rows in this matrix */
						   int row0, /* 1st entry of permutation list */
						   int row1, /* 2nd entry of permutation list */
						   int row2) /* 3rd entry of permutation list */
      
{
    register int i, tempi;
    double temp;

#define SWAP_ROWS(a, b) \
   for (i = 0; i < 3; i++) DSWAP(inv[a][i], inv[b][i], temp); \
   DSWAP(a, b, tempi)

    if (row0 != 0) {
        if (row1 == 0) {
            SWAP_ROWS(row0, row1);
        } else {
            SWAP_ROWS(row0, row2);
        }
    }
    if (row1 != 1) {
        SWAP_ROWS(row1, row2);
    }
}


/* DESCR: Applies gaussian elimination to the matrix @src@ to compute a
 *   matrix @inv@ which is the inverse of @src@.
 * RETURNS: TRUE if the gaussian elimination was successful, FALSE if no
 *   pivot element could be found at some level.
 * DETAILS: Essentially, Gaussian elimination is used. This is the
 *   technique where one writes down the matrix M on the left side
 *   of a page and the identity on the right. One then performs
 *   row operations on the matrix to simplify it, and performs
 *   exactly the same row operations on the identity (which makes
 *   it more complicated). When the original matrix has been
 *   simplified down to an identity matrix, the original identity
 *   matrix has been complicated all the way to being the inverse
 *   of the original matrix M. The particular order of operations
 *   is that the rows of the original are swapped so that the
 *   largest element in the zeroth column is moved to the (0,0)
 *   position. Then one divides the zeroth row by this element to
 *   make the (0,0) entry be 1. Then, one subtracts the (1,0)
 *   entry times the zeroth row from the first row, and the (2,0)
 *   entry times the zeroth row from the second row. The zeroth
 *   column is now that of the identity. On then moves on to the
 *   second and third columns and proceeds similarly. The routine
 *   differs from this standard technique only in the order in
 *   which it chooses the columns to operate on. The @row0@
 *   argument biases the choice, and the variables @row1@ and
 *   @row2@ are chosen so as to produce the least roundoff error
 *   in the process.
 */
static int MAT3_inv3_second_col(register double source[3][3],
									/* Matrix to be inverted */
								register double inv[3][3],
									/* Inverse of 3 x 3 matrix */
								int row0)/* The first row to use as a pivot */
      
{
    register int row1, row2, i1, i2, i;
    double temp, a, b;

    /* Find which row to use */
    if (row0 == 0)
        i1 = 1, i2 = 2;
    else if (row0 == 1)
        i1 = 0, i2 = 2;
    else
        i1 = 0, i2 = 1;
    /* Find which is larger in abs. val.:the entry in [i1][1] or
     * [i2][1] and use that value for pivoting.
     */
    a = source[i1][1];
    if (a < 0)
        a = -a;
    b = source[i2][1];
    if (b < 0)
        b = -b;
    if (a > b)
        row1 = i1;
    else
        row1 = i2;
    row2 = (row1 == i1 ? i2 : i1);
    /* Scale row1 in source */
    if ((source[row1][1] < SMALL) && (source[row1][1] > -SMALL))
        return 0;
    temp = 1.0 / source[row1][1];
    source[row1][1] = 1.0;
    source[row1][2] *= temp; /* source[row1][0] is zero already */
    /* Scale row1 in inv */
    inv[row1][row1] = temp;  /* it used to be a 1.0 */
    inv[row1][row0] *= temp;
    /* Clear column one, source, and make corresponding changes in inv
    */
    for (i = 0; i < 3; i++)
        if (i != row1) {     /* for i = all rows but row1 */
            temp = -source[i][1];
            source[i][1] = 0.0;
            source[i][2] += temp * source[row1][2];
            inv[i][row1] = temp * inv[row1][row1];
            inv[i][row0] += temp * inv[row1][row0];
        }
    /* Scale row2 in source */
    if ((source[row2][2] < SMALL) && (source[row2][2] > -SMALL))
        return (FALSE);
    temp = 1.0 / source[row2][2];
    source[row2][2] = 1.0;   /* source[row2][*] is zero already */
    /* Scale row2 in inv */
    inv[row2][row2] = temp;  /* it used to be a 1.0 */
    inv[row2][row0] *= temp;
    inv[row2][row1] *= temp;
    /* Clear column two, source, and make corresponding changes in inv
     */
    for (i = 0; i < 3; i++)
        if (i != row2) {     /* for i = all rows but row2 */
            temp = -source[i][2];
            source[i][2] = 0.0;
            inv[i][row0] += temp * inv[row2][row0];
            inv[i][row1] += temp * inv[row2][row1];
            inv[i][row2] += temp * inv[row2][row2];
        }
    /* Now all is done except that the inverse needs to have its rows
     * shuffled. row0 needs to be moved to inv[0][*], row1 to inv[1][*],
     * etc.
     * We didn't do the swapping before the elimination so that we
     * could more easily keep track of what ops are needed to be done
     * in the inverse.
     */
    MAT3_inv3_swap(inv, row0, row1, row2);
    return TRUE;
}


/* DESCR: Fast inversion routine for 3 x 3 matrices. Takes a matrix
 *   @source@ and computes its inverse in @inv@.
 * RETURNS: TRUE if inverse could be computed, FALSE if determinant is
 *   too close to 0.
 * DETAILS: This takes 30 multiplies/divides, as opposed to 39 for
 *   Cramer's Rule. The algorithm consists of performing fast
 *   gaussian elimination, by never doing any operations where the
 *   result is guaranteed to be zero, or where one operand is
 *   guaranteed to be zero. This is done at the cost of clarity,
 *   alas.
 */
static int MAT3_invert3(double source[3][3],/* A 3 x 3 matrix to be inverted */
						double inv[3][3])/* The inverse of the source matrix */

{
    register int i, row0;
    double temp, a, b, c;

    inv[0][0] = inv[1][1] = inv[2][2] = 1.0;
    inv[0][1] = inv[0][2] = inv[1][0] = inv[1][2] = inv[2][0] = inv[2][1] = 0.0;

    /* attempt to find the largest entry in first column to use as
     * pivot */
    a = source[0][0];
    if (a < 0)
        a = -a;
    b = source[1][0];
    if (b < 0)
        b = -b;
    c = source[2][0];
    if (c < 0)
        c = -c;
    if (a > b) {
        if (a > c)
            row0 = 0;
        else
            row0 = 2;
    } else {
        if (b > c)
            row0 = 1;
        else
            row0 = 2;
    }
    /* Scale row0 of source */
    if ((source[row0][0] < SMALL) && (source[row0][0] > -SMALL))
        return (FALSE);
    temp = 1.0 / source[row0][0];
    source[row0][0] = 1.0;
    source[row0][1] *= temp;
    source[row0][2] *= temp;
    /* Scale row0 of inverse */
    inv[row0][row0] = temp;  /* other entries are zero -- no effort  */

    /* Clear column zero of source, and make corresponding changes in
     * inverse */
    for (i = 0; i < 3; i++)
        if (i != row0) {     /* for i = all rows but row0 */
            temp = -source[i][0];
            source[i][0] = 0.0;
            source[i][1] += temp * source[row0][1];
            source[i][2] += temp * source[row0][2];
            inv[i][row0] = temp * inv[row0][row0];
        }
        /* We've now done gaussian elimination so that the source and
         * inverse look like the same.
         * We now proceed to do elimination on the second column.
         */
    if (!MAT3_inv3_second_col(source, inv, row0))
        return (FALSE);
    return TRUE;
}


/* DESCR:  Perform gaussian elimination on the last row of the 4 x 4
 *   matrix @src@. If the (3,3) entry of src is zero, some other
 *   nonzero entry in last row is discovered, and its column is
 *   swapped with the last column (the index of that column is
 *   stored in @swap@).  Then, whether such a swap took place or
 *   not, the (3,3) entry is scaled to 1 (and its former values
 *   stored in @s@), and then an appropriate multiple of the final
 *   column is subtracted    from each earlier column to make the
 *   final row be (0,0,0,1). The multiples used are stored in the
 *   vector @r@.
 * RETURNS: TRUE if the operation succeeded, FALSE is all entries in the
 *   third row are zero (or near zero).
 */
static int MAT3_inv4_pivot(register Matrix4 src,
								/* A 4 x 4 matrix we're trying to invert */
						   Vector3 *r,
								/* Vector of eliminants of the last row */
						   double *s,/* The pivoting factor */
						   int *swap)
								/* column that got swapped with the last column */
        
{
    register int i, j;
    double temp, max;

    *swap = -1;
    if (MAT3_IS_ZERO(src[3][3])) {
        /* Look for a different pivot element: one with largest abs
         * value */
        max = 0.0;
        for (i = 0; i < 4; i++) {
            if (src[3][i] > max)
                max = src[3][*swap = i];
            else if (src[3][i] < -max)
                max = -src[3][*swap = i];
        }
        /* No pivot element available ! */
        if (*swap < 0)
            return (FALSE);
        else
            for (j = 0; j < 4; j++)
                DSWAP(src[j][*swap], src[j][3], temp);
    }
    V3SET(-src[3][0], -src[3][1], -src[3][2], r);
    *s = 1.0 / src[3][3];
    src[3][0] = src[3][1] = src[3][2] = 0.0;
    src[3][3] = 1.0;
    for (i = 0; i < 3; i++)
        src[i][3] *= *s;
    for (i = 0; i < 3; i++) {
        src[i][0] += r->x * src[i][3];
        src[i][1] += r->y * src[i][3];
        src[i][2] += r->z * src[i][3];
    }
    return TRUE;
}


/* DESCR: This computes, in @resMat@, in the inverse of the 4 x 4
 *    matrix @mat@. The matrices @resMat@ and @mat@ may be
 *    identical, i.e., once may safely call M4Invert(m, m).
 * RETURNS: TRUE is matrix could be inverted, FALSE if determinant is
 *    too close to zero or some unusual event occurred during gaussian
 *    elimination. 
 * DETAILS: Fast inversion routine for 4 x 4 matrices. This routine has
 *    been specially tweaked to notice the following: If the matrix
 *    has (0,0,0,1) as its final column (as do many matrices in
 *    graphics), then we compute the inverse of the upper left 3x3
 *    matrix and use this to find the general inverse. In the event
 *    that the right column is not 0-0-0-1, we do gaussian
 *    elimination to make it so, then use the 3x3 inverse, and then
 *    undo our gaussian elimination.
 */
int M4Invert(Matrix4 mat, Matrix4 resMat)

{
    Matrix4 src, inv;
    register int i, j, simple;
    double m[3][3], inv3[3][3], s=0.0, temp;
    Vector3 r, t;
    int swap = -1;

    M4Copy(mat, src);
    M4SetIdentity(inv);
    V3ZERO(&r);
    V3ZERO(&t);
    /* If last column is not (0,0,0,1), use special code */
    simple = (mat[0][3] == 0.0 && mat[1][3] == 0.0 &&
            mat[2][3] == 0.0 && mat[3][3] == 1.0);
    if (!simple && !MAT3_inv4_pivot(src, &r, &s, &swap))
        return (FALSE);
    t.x = src[0][3];        /* Translation vector */
    t.y = src[1][3];
    t.z = src[2][3];
    /* Copy upper-left 3x3 matrix */
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            m[i][j] = src[i][j];
    if (!MAT3_invert3(m, inv3))
        return (FALSE);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            inv[i][j] = inv3[i][j];
    for (i = 0; i < 3; i++) {
        inv[i][3] -= t.x * inv3[i][0];
        inv[i][3] -= t.y * inv3[i][1];
        inv[i][3] -= t.z * inv3[i][2];
    }
    if (!simple) {
        /* We still have to undo our gaussian elimination from earlier
         * on */
        /* add r0 * first row to last row */
        /* add r1 * 2nd   row to last row */
        /* add r2 * 3rd   row to last row */
        for (i = 0; i < 4; i++) {
            inv[3][i] += r.x * inv[0][i] + r.y * inv[1][i] + r.z * inv[2][i];
            inv[3][i] *= s;
        }
        if (swap >= 0)
            for (i = 0; i < 4; i++)
                DSWAP(inv[swap][i], inv[3][i], temp);
    }
    M4Copy(inv, resMat);
    return TRUE;
}


/* Computes the inverse of a 3D affine matrix; i.e. a matrix with a
 * dimensionality of 4 where the last row has the entries
 * (0, 0, 0, 1).
 * This procedure treats the 4 by 4 matrix as a block matrix and
 * calculates the inverse of the upper 3x3 submatrix (assuming it is
 * non-singular) for a significant performance improvement over a 
 * general procedure that can invert any nonsingular matrix:
 *          |          | -1       |   -1     -1   -1|
 *          | A      C |          |  A     -A  C B  |
 *    -1    |          |          |                 |
 *   M   =  |          |     =    |              -1 |
 *          | 0      1 |          |  0         B    |
 *  where     M is a 4 by 4 matrix,
 *            A is the 3 by 3 upper left submatrix of M,
 *            C is the 3 by 1 rigth column submatrix of M.
 * Input:
 *   in   - 3D affine matrix
 * Output:
 *   out  - inverse of 3D affine matrix
 * Returned value:
 *   TRUE   if input matrix is nonsingular
 *   FALSE  otherwise
 */
#define ACCUMULATE  if (temp >= 0.0)  \
  pos += temp; \
 else \
  neg += temp;
#define PRECISION_LIMIT (1.0e-15)

int AffineMatrix4Inverse(register Matrix4 in, register Matrix4 out)

{
    register double det_1;
    double pos=0.0, neg=0.0, temp;

    /* Calculate the determinant of submatrix A and determine if the
     * the matrix is singular as limited by the double precision
     * floating-point data representation.
     */
    temp = in[0][0] * in[1][1] * in[2][2];
    ACCUMULATE
    temp = in[0][1] * in[1][2] * in[2][0];
    ACCUMULATE
    temp = in[0][2] * in[1][0] * in[2][1];
    ACCUMULATE
    temp = -in[0][2] * in[1][1] * in[2][0];
    ACCUMULATE
    temp = -in[0][1] * in[1][0] * in[2][2];
    ACCUMULATE
    temp = -in[0][0] * in[1][2] * in[2][1];
    ACCUMULATE
    det_1 = pos + neg;
    /* Is the submatrix A singular? */
    if ((det_1 == 0.0) || (ABS(det_1 / (pos - neg)) < PRECISION_LIMIT)) {
        /* Matrix M has no inverse */
        fprintf(stderr, "affine_matrix4_inverse: singular matrix");
        return 0;
    } else {
        /* Calculate inverse(A) = adj(A) / det(A) */
        det_1 = 1.0 / det_1;
        out[0][0] = (in[1][1] * in[2][2] - in[1][2] * in[2][1]) * det_1;
        out[1][0] = -(in[1][0] * in[2][2] - in[1][2] * in[2][0]) * det_1;
        out[2][0] = (in[1][0] * in[2][1] - in[1][1] * in[2][0]) * det_1;
        out[0][1] = -(in[0][1] * in[2][2] - in[0][2] * in[2][1]) * det_1;
        out[1][1] = (in[0][0] * in[2][2] - in[0][2] * in[2][0]) * det_1;
        out[2][1] = -(in[0][0] * in[2][1] - in[0][1] * in[2][0]) * det_1;
        out[0][2] = (in[0][1] * in[1][2] - in[0][2] * in[1][1]) * det_1;
        out[1][2] = -(in[0][0] * in[1][2] - in[0][2] * in[1][0]) * det_1;
        out[2][2] = (in[0][0] * in[1][1] - in[0][1] * in[1][0])*det_1;
        /* Calculate -inverse(A) * C */
        out[3][0] = -(out[0][0] * in[0][3] + out[0][1] * in[1][3] +
            out[0][2] * in[2][3]);
        out[3][1] = -(out[1][0] * in[0][3] + out[1][1] * in[1][3] +
               out[1][2] * in[2][3]);
        out[3][2] = -(out[2][0] * in[0][3] + out[2][1] * in[1][3] +
               out[2][2] * in[2][3]); 
        /* Fill in last row*/
        out[3][0] = out[3][1] = out[3][2] = 0.0;
        out[3][3] = 1.0;
        return 1;
    }
}
#undef ACCUMULATE
#undef PRECISION_LIMIT


/* AnglePreservingMatrix4Inverse
 * Computes the inverse of a 3-D angle-preserving matrix.
 * This procedure treats the 4 by 4 angle-preserving matrix as a block
 * matrix and calculates the inverse of one submatrix for a significant
 * performance improvement over a general procedure that can invert any
 * nonsingular matrix:
 *          |          | -1       |   -2 T      -2  T   |
 *          | A      C |          |  s  A     - s  A  C |
 *    -1    |          |          |                     |
 *   M   =  |          |     =    |                     |
 *          | 0      1 |          |    0           1    |
 * where      M is a 4 by 4 angle-preserving matrix,
 *            A is the 3 by 3 upper-left submatrix of M,
 *            C is the 3 by 1 upper-right submatrix of M.
 * Input:
 *   in   - 3-D angle-preserving matrix
 * Output:
 *   out  - inverse of 3-D angle-preserving matrix
 * Returned value:
 *   TRUE   if input matrix is nonsingular
 *   FALSE  otherwise
 */
int AnglePreservingMatrix4Inverse(Matrix4 in, Matrix4 out)

{
    double scale;

    /* Calculate the square of the isotropic scale factor */
    scale = in[0][0] * in[0][0] + in[0][1] * in[0][1] + in[0][2] * in[0][2];
    /* Is the submatrix A singular? */
    if (scale == 0.0) {
        /* Matrix M has no inverse */
        fprintf(stderr,  "angle_preserving_matrix4_inverse: singular matrix");
        return 0;
    }
    /* Calculate the inverse of the square of the isotropic scale
     * factor */
    scale = 1.0 / scale;
    /* Transpose and scale the 3 by 3 upper-left submatrix */
    out[0][0] = scale * in[0][0];
    out[1][0] = scale * in[0][1];
    out[2][0] = scale * in[0][2];
    out[0][1] = scale * in[1][0];
    out[1][1] = scale * in[1][1];
    out[2][1] = scale * in[1][2];
    out[0][2] = scale * in[2][0];
    out[1][2] = scale * in[2][1];
    out[2][2] = scale * in[2][2];
    /* Calculate -(transpose(A) / s*s) C */
    out[0][3] = -(out[0][0] * in[0][3] + out[0][1] * in[1][3] + 
            out[0][2] * in[2][3]);
    out[1][3] = -(out[1][0] * in[0][3] + out[1][1] * in[1][3] +
            out[1][2] * in[2][3]);
    out[2][3] = -(out[2][0] * in[0][3] + out[2][1] * in[1][3] +
            out[2][2] * in[2][3]);

    /* Fill in last row */
    out[3][0] = out[3][1] = out[3][2] = 0.0;
    out[3][3] = 1.0;
    return 1;
}

/*** m4invert.c ***/
