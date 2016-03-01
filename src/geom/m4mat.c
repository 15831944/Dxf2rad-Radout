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

/* Contains functions that operate on matrices, but not on matrix-vector
 * pairs, etc. 
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

/* determinant of 3x3 matrix with given entries */
#define DET3(a,b,c,d,e,f,g,h,i) \
        ((a*e*i + b*f*g + c*d*h) - (a*f*h + b*d*i + c*e*g))


/* DESCR: Compute a determinant of the lower 3 x 3 matrix of a 4 x 4 matrix 
 *    @mat@, multiplied by the sign of the permutation (@col1@, @col2@, 
 *    @col3@). If (@col1@, @col2@, @col3@ ) = (0, 1, 2), one gets the ordinary
 *    determinant.
 * RETURNS: The determinant.
 */
static double M4DeterminantLower3(Matrix4 mat,
								 /* Matrix whose determinant we will compute  */
								  int col1,/* First entry in permutation */
								  int col2,/* 2nd entry in permutation */
								  int col3)/* 3rd entry in permutation  */
       
{
    double det;

    det = DET3(mat[1][col1], mat[1][col2], mat[1][col3],
            mat[2][col1], mat[2][col2], mat[2][col3],
            mat[3][col1], mat[3][col2], mat[3][col3]);
    return det;
}


/*  Set the orthogonal matrix from points (or vectors)
 */
extern void M4SetFromPoints(Matrix4 m, Point3 *p1, Point3 *p2, Point3 *p3)

{
    m[0][0] = p1->x; m[1][0] = p1->y; m[2][0] = p1->z;
    m[0][1] = p2->x; m[1][1] = p2->y; m[2][1] = p2->z;
    m[0][2] = p3->x; m[1][2] = p3->y; m[2][2] = p3->z;
    m[0][3] = m[1][3] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.0;
    m[3][3] = 1.0;
}


/* DESCR  : Sets @m@ to be the identity matrix.
 * This version is almost 8 times faster than nested for loops.
 * RETURNS: void
 */
extern void M4SetIdentity(Matrix4 m) /* Matrix to be set to identity */

{
    double *mat = (double *)m;

    *mat++ = 1.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 1.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 1.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat = 1.0;
}


/* set a matrix to the identity */
extern void M3SetIdentity(Matrix3 mat)

{
    register int i, j;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            if (i == j)
                mat[i][i] = 1.0;
            else
                mat[i][j] = 0.0;
}


/* DESCR  : Sets @m@ to be the zero matrix.
 * RETURNS: void
 */
extern void M4Zero(Matrix4 m)  /* Matrix to be set to zero */
{
    double *mat = (double *)m;

    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat++ = 0.0;
    *mat = 0.0;
}


/* DESCR  : Compute the determinant of @mat@.
 * RETURNS: The determinant, a double.
 */
extern double M4Determinant(Matrix4 m)  /* Matrix whose determinant we compute */
{
    double det;

    det = (m[0][0] * M4DeterminantLower3(m, 1, 2, 3)
            - m[0][1] * M4DeterminantLower3(m, 0, 2, 3)
            + m[0][2] * M4DeterminantLower3(m, 0, 1, 3)
            - m[0][3] * M4DeterminantLower3(m, 0, 1, 2));
    return det;
}


/* DESCR  : Copy the matrix @f@ into the matrix @t@.
 * RETURNS: void
 */
extern void M4Copy(Matrix4 source, /* Source matrix [input] */
			Matrix4 target) /* Target matrix [output] */
 
{
    double *to = (double *)target;
    double *from = (double *)source;

    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to = *from;
}


/* DESCR: Computes the matrix product @mat1@ * @mat2@ and places the
 *    result in @prod@. The matrices being multiplied may be the
 *    same as the result, i.e., a call of the form M4MatMult(a,a,b) is
 *    safe.
 * RETURNS: void
 */
extern void M4MatMult(Matrix4 mat1, Matrix4 mat2, Matrix4 prod)

{
    register int i, j;
    Matrix4 tmp_mat;
    register double *tmp;

    if (((tmp = (double *)prod) == (double *)mat1) ||
            (tmp == (double *)mat2))
        tmp = (double *)tmp_mat;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            *(tmp++) = (mat1[i][0] * mat2[0][j] +
                    mat1[i][1] * mat2[1][j] + mat1[i][2] * mat2[2][j] +
                    mat1[i][3] * mat2[3][j]);
    if (((double *)prod == (double *)mat1) ||
            ((double *)prod == (double *)mat2))
        M4Copy(tmp_mat, prod);
    return;
}


/* DESCR: Compute the transpose of @mat@ and place it in @resMat@.
 *      The routine may be called with two two arguments equal (i.e.,
 *      MAT3transpose(a, a)) and will still work properly.
 * RETURNS: void
 */
extern void M4Transpose(Matrix4 mat,  /* The matrix whose transpose we compute */
				 Matrix4 trans)/* The computed transpose */
    
{
    int i, j;
    Matrix4 tmp_mat;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            tmp_mat[i][j] = mat[j][i];
    M4Copy(trans, tmp_mat);
}


/* DESCR: This prints the matrix @mat@ to the file pointer @fp@, using
 *    the format string @format@ to pass to fprintf.  The strings
 *    @head@ and @tail@ are printed at the beginning and end of
 *    each line, and the string @title@ is printed at the top.
 * RETURNS: void
 */
extern void M4PrintFormatted(FILE *fp,     /* File stream to which it is printed */
					  Matrix4 mat,  /* Matrix to be printed out */
					  char *title,  /* Title of printout */
					  char *head,   /* Printed b efore each row of matrix */
					  char *format, /* Format for entries of matrix */
					  char *tail)   /* Printed after each row of matrix */

{
    int i, j;

    /* This is to allow this to be called easily from a debugger */
    if (fp == NULL)
        fp = stderr;
    if (title == NULL)
        title = "\n";
    if (head == NULL)
        head = "  ";
    if (format == NULL)
        format = "%#8.4lf  ";
    if (tail == NULL)
        tail = "\n";
    (void)fprintf(fp, title);
    for (i = 0; i < 4; i++) {
        (void)fprintf(fp, head);
        for (j = 0; j < 4; j++)
            (void)fprintf(fp, format, mat[i][j]);
        (void)fprintf(fp, tail);
    }
}


/* DESCR: Print the matrix @mat@ on the stream described by the file
 *        point @fp@.
 * RETURNS: void
 */
extern void M4Print(FILE *fp,   /* The stream on which to print it */
			 Matrix4 mat)/* The matrix to be printed */
  
{
    M4PrintFormatted(fp, mat, NULL, NULL, NULL, NULL);
}


/* DESCR: This compares two matrices @m1@ and @m2@ for equality
 *     within @epsilon@.  If all corresponding elements in the
 *     two matrices are equal within @epsilon@, then TRUE is
 *     returned.  Otherwise, FALSE is returned. Epsilon should be
 *     zero or positive.
 * RETURNS: TRUE if all entries differ pairwise by less than epsilon,
 *     FALSE otherwise.
 */
extern int M4IsEqual(Matrix4 m1,     /* The first matrix to be compared */
			  Matrix4 m2,     /* The second matrix to be compared */
			  double epsilon) /* The value within which entries must match */

{
    int i, j;
    double diff;

    /* Special-case for epsilon of 0 */
    if (epsilon == 0.0) {
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                if (m1[i][j] != m2[i][j])
                    return FALSE;
    } else {                            /* Any other epsilon */
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++) {
                diff = m1[i][j] - m2[i][j];
                if (!IS_ZERO(diff, epsilon))
                    return FALSE;
            }
    }
    return TRUE;
}


/* DESCR: Compute the trace of @mat@ (i.e., sum of diagonal entries).
 * RETURNS: The trace of @mat@. 
 */
extern double M4Trace(Matrix4 mat)  /* Matrix whose trace we are computing */
{
    return (mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3]);
}


/* DESCR:  Raises @mat@ to the power @power@, placing the result in
 *    @resMat@.
 * RETURNS: TRUE if the power is non-negative, FALSE otherwise
 * DETAILS: The computation is performed by starting with the identity
 *    matrix as the result. Then look at the bits of the binary
 *    representation of the power, from left to right. Every time a
 *    1 is encountered, we square the matrix and then multiply the
 *    result by the original matrix. Every time that a 0 is
 *    encountered, just square the matrix.  When done with all the
 *    bits in the power, the matrix will be the correct result
 */
extern int M4Power(Matrix4 resMat, /* The source matrix raised to the power */
			Matrix4 mat,    /* The source matrix */
			int power)      /* The power to which to raise the source */
 
{
    int count, rev_power;
    Matrix4 temp_mat;

    if (power < 0) {
        fprintf(stderr, "Negative power given to M4Power:");
        return FALSE;
    }
    /* Reverse the order of the bits in power to make the looping
     * construct * effiecient (and word-size-independent). */
    for (rev_power = count = 0; power > 0; count++, power = power >> 1)
        rev_power = (rev_power << 1) | (power & 1);
    power = rev_power;
    M4SetIdentity(temp_mat);
    /* For each bit in power */
    for (; count > 0 || rev_power != 0; count--, rev_power = rev_power >> 1) {
        /* Square working matrix */
        M4MatMult(temp_mat, temp_mat, temp_mat);
        /* If bit is 1, multiply by original matrix */
        if (power & 1)
            M4MatMult(temp_mat, mat, temp_mat);
    }
    M4Copy(temp_mat, resMat);
    return TRUE;
}


/* DESCR: Computes the column-reduced form of @mat@ in @resMat@.
 *   After column reduction, each column's first non-zero entry is
 *   a one. The leading ones are ordered highest to lowest from
 *   the first column to the last. 
 * RETURNS: The rank of the matrix, i.e., the number of linearly
 *   independent columns of the original matrix.
 * DETAILS: Performs the reduction through simple column operations:
 *   multiply a column by a scalar to make its leading value one,
 *   then add a multiple of this column to each of the others to
 *   put zeroes in all other entries of that row. The number
 *   @epsilon@ is used to test near equality with zero.  It should
 *   be non-negative.
 */
extern int M4ColumnReduce(Matrix4 resMat,  /* The column-reduced matrix [output] */
				   Matrix4 mat,     /* The original matrix [input] */
				   double epsilon)  /* The tolerance for testing against 0 */

{
    int row, col, next_col, r;
    double t, factor;

    M4Copy(resMat, mat);

    /* next_col is the next column in which a leading one will be
     * placed */
    next_col = 0;
    for (row = 0; row < 4; row++) {
        for (col = next_col; col < 4; col++)
            if (!IS_ZERO(resMat[row][col], epsilon))
                break;
        if (col == 4)
            continue;        /* all 0s, try next row */
        if (col != next_col) /* swap cols */
            for (r = row; r < 4; r++) {
                t = resMat[r][col];
                resMat[r][col] = resMat[r][next_col];
                resMat[r][next_col] = t;
            }
        /* Make a leading 1 */
        factor = 1.0 / resMat[row][next_col];     /* make a leading 1 */
        resMat[row][next_col] = 1.0;
        for (r = row + 1; r < 4; r++)
            resMat[r][next_col] *= factor;

        for (col = 0; col < 4; col++)   /* zero out cols */
            if (col != next_col) {      /* could skip if already zero */
                factor = -resMat[row][col];
                resMat[row][col] = 0.0;
                for (r = row + 1; r < 4; r++)
                    resMat[r][col] += factor * resMat[r][next_col];
            }
        if (++next_col == 4)
            break;
    }
    return (next_col);
}


/* DESCR: This computes the kernel of the transformation represented by
 *    @mat@ from its column-reduced form, and places a basis for
 *    the kernel in the rows of @resMat@. The number "epsilon"
 *    is used to test near-equality with zero. It should be
 *    non-negative.  If there is no basis (i.e., the matrix is
 *    non-singular), this returns FALSE.
 * RETURNS: TRUE if the nullity is nonzero, FALSE otherwise. 
 */
extern int M4KernelBasis(Matrix4 resMat, /* Stores basis of kernel of mat [output] */
				  Matrix4 mat,    /* Matrix whose kernel we compute [input] */
				  double epsilon) /* Tolerance for comparison with zero [input] */
        
{
    Matrix4 reduced;
    int leading[4],     /* Indices of rows with leading ones    */
     other[4],     /* Indices of other rows                */
     rank,         /* Rank of column-reduced matrix        */
     dim;          /* Dimension of kernel (4 - rank)       */
    int lead_count, other_count, r, c;

    /* Column-reduce the matrix */
    rank = M4ColumnReduce(reduced, mat, epsilon);
    /* Get dimension of kernel. If zero, there is no basis */
    if ((dim = 4 - rank) == 0)
        return (FALSE);
    /* If matrix transforms all points to origin, return identity */
    if (rank == 0)
        M4SetIdentity(resMat);
    else {
        /* Find rows with leading ones, and other rows */
        lead_count = other_count = 0;
        for (r = 0; r < 4; r++) {
            if (reduced[r][lead_count] == 1.0)
                leading[lead_count++] = r;
            else
                other[other_count++] = r;
        }
        /* Put identity into rows with no leading ones */
        for (r = 0; r < dim; r++)
            for (c = 0; c < other_count; c++)
                resMat[r][other[c]] = (r == c ? -1.0 : 0.0);
        /* Copy other values */
        for (r = 0; r < dim; r++)
            for (c = 0; c < rank; c++)
                resMat[r][leading[c]] = reduced[other[r]][c];
    }
    return TRUE;
}


/* DESCR: Multiply matrix @m@ by the real number @scalar@ and put the
 *    result in @resMat@.  
 * RETURNS: void
 */
extern void M4ScalarMult(Matrix4 m,      /* Matrix to be scaled [input] */
				  double scalar,  /* Scaling factor [input] */
				  Matrix4 resMat) /* The result of scaling the matrix [output] */
   
{
    int i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            resMat[i][j] = scalar * m[i][j];
}


/* DESCR: Compute the sum of matrices @A@ and @B@ and place the result
 *    in @resMat@, which may be one of @A@ and @B@ without any
 *    fear of errors.
 * RETURNS:    void
 */
extern void M4Add(Matrix4 A, Matrix4 B,
		   Matrix4 resMat)/* The sum of the matrices [output] */
{
  
    int i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            resMat[i][j] = A[i][j] + B[i][j];
}
/*** end m4mat.c ***/
