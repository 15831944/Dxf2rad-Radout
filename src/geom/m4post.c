/*
This file is part of
*
* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.
*
*/

/* Efficient Post-Concatenation of Transformation Matrices
 * by Joseph M. Cychosz
 * from "Graphics Gems", Academic Press, 1990
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
 */


/*
 * Modifications by Philip Thompson to conform to acad and
 * "Computer Graphics" matrix conventions.
 *
 *  A collection of basic routines used to perform the following
 *  direct post-concatenated transformation operations:
 *  M4RotateX      Post-concatenate a x-axis rotation.
 *  M4RotateY      Post-concatenate a y-axis rotation.
 *  M4RotateZ      Post-concatenate a z-axis rotation.
 *  M4Scale        Post-concatenate a scaling.
 *  M4Translate    Post-concatenate a translation.
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


/*  M4RotateX - Post-concatenate a x-axis rotation matrix.
 */
void M4RotateX(Matrix4 m, /* Current transformation matrix */
			   double a)   /* Rotation angle */
  
{
    register int i;
    double t, c = cos(a), s = -sin(a);

    for (i = 0; i < 4; i++) {
        t = m[i][1];
        m[i][1] = t * c - m[i][2] * s;
        m[i][2] = t * s + m[i][2] * c;
    }
}


/*  M4RotateY - Post-concatenate a y-axis rotation matrix.
 */
void M4RotateY(Matrix4 m, /* Current transformation matrix */
			   double a)  /* Rotation angle */
 
{
    register int i;
    double t, c = cos(a), s = -sin(a);

    for (i = 0; i < 4; i++) {
        t = m[i][0];
        m[i][0] = t * c + m[i][2] * s;
        m[i][2] = m[i][2] * c - t * s;
    }
}


/*  M4RotateZ - Post-concatenate a z-axis rotation matrix.
 */
void M4RotateZ(Matrix4 m,  /* Current transformation matrix */
			   double a)  /* Rotation angle */
     
{
    register int i;
    double t, c = cos(a), s = -sin(a);

    for (i = 0; i < 4; i++) {
        t = m[i][0];
        m[i][0] = t * c - m[i][1] * s;
        m[i][1] = t * s + m[i][1] * c;
    }
}


/*  M4Scale - Post-concatenate a scaling.
 */
void M4Scale(Matrix4 m, /* Current transformation matrix */
			 double sx, double sy, double sz)/* Scale factors about x,y,z */

{
    register int i;

    for (i = 0; i < 3; i++) {
        m[i][0] *= sx;
        m[i][1] *= sy;
        m[i][2] *= sz;
    }
    /* translation vector */
    m[0][3] *= sx;
    m[1][3] *= sy;
    m[2][3] *= sz;
}


/*  M4Translate - Post-concatenate a translation.
 */
void M4Translate(Matrix4 m,/* Current transformation matrix */
				 double tx, double ty, double tz)/* Translation distance */

{
    register int i;

    for (i = 0; i < 4; i++) {
        m[0][i] += m[3][i] * tx;
        m[1][i] += m[3][i] * ty;
        m[2][i] += m[3][i] * tz;
    }
}

/*** end m4post.c ***/
