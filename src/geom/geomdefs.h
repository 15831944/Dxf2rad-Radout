/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


The MIT License (MIT)

Copyright (c) 1999-2016 Georg Mischler

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

/*
* Geomdef.h
* geometry macro definitions
* originally from "Graphics Gems", Academic Press, 1990
*/

#ifndef GDEFS_H
#define GDEFS_H 1

#include <math.h>

#ifdef __cplusplus
    extern "C" {
#endif


#ifndef TRUE
  #define TRUE 1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

#define RTOD      57.2957795131 /* Radians to degrees */
#define DEG_TO_RAD      0.0174532925199

#define MAXVERTS    10001

#define NEWTYPE(x)      (x*)(malloc((unsigned)sizeof(x)))

/* absolute value of a */
#define ABS(a)          (((a)<0) ? -(a) : (a))

/* round a to nearest integer towards 0 */
#define FLOOR(a)                ((a)>0 ? (int)(a) : -(int)(-a))

/* round a to nearest integer away from 0 */
#define CEILING(a) \
((a)==(int)(a) ? (a) : (a)>0 ? 1+(int)(a) : -(1+(int)(-a)))

/* round a to nearest int */
#define ROUND(a)        ((a)>0 ? (int)(a+0.5) : -(int)(0.5-a))

/* take sign of a, either -1, 0, or 1 */
#define ZSGN(a)         (((a)<0) ? -1 : (a)>0 ? 1 : 0)

/* take binary sign of a, either -1, or 1 if >= 0 */
#define SGN(a)          (((a)<0) ? -1 : 0)

/* square a */
#define SQR(a)          ((a)*(a))

/* find minimum of a and b */
#define MIN(a,b)        (((a)<(b))?(a):(b))

/* find maximum of a and b */
#define MAX(a,b)        (((a)>(b))?(a):(b))

/* swap a and b (see Gem by Wyvill) */
#define SWAP(a,b)       { a^=b; b^=a; a^=b; }

/* linear interpolation from l (when a=0) to h (when a=1)*/
/* (equal to (a*h)+((1-a)*l) */
#define LERP(a,l,h)     ((l)+(((h)-(l))*(a)))

/* clamp the input to the specified range */
#define CLAMP(v,l,h)    ((v)<(l) ? (l) : (v) > (h) ? (h) : v)

#define IS_ZERO(n,eps)        (((n) < eps) && ((n) > -eps))

#define BLACK   0
#define RED     1
#define YELLOW  2
#define GREEN   3
#define CYAN    4
#define BLUE    5
#define MAGENTA 6
#define WHITE   7

#define EPSILON  1E-8

/* M_PI         - pi
 * M_PI_2       - pi/2
 * M_PI_4       - pi/4
 * M_1_PI       - 1/pi
 * M_2_PI       - 2/pi
 * M_2_SQRTPI   - 2/(sqrt(pi)
 */
#ifndef M_PI
#define M_PI       3.1415926535897931160E0  /*Hex  2^ 1 * 1.921FB54442D18 */
#endif
#ifndef M_PI_2
#define M_PI_2     1.5707963267948965580E0  /*Hex  2^ 0 * 1.921FB54442D18 */
#endif
#ifndef M_PI_4
#define M_PI_4     7.8539816339744827900E-1 /*Hex  2^-1 * 1.921FB54442D18 */
#endif
#ifndef M_1_PI
#define M_1_PI     3.1830988618379067154E-1 /*Hex  2^-2 * 1.45f306dc9c883 */
#endif
#ifndef M_2_PI
#define M_2_PI     6.3661977236758134308E-1 /*Hex  2^-1 * 1.45f306dc9c883 */
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.1283791670955125739E0  /*Hex  2^ 0 * 1.20dd750429b6d */
#endif 
#ifndef M_2PI
#define M_2PI      6.2831853071795862320E0  /*Hex  2^ 2 * 1.921FB54442D18 */
#endif

/* arc/bulge angle directions */
#define CW  1
#define CCW -1


#define FUZZ 0.0000001
#define V_EQUAL(v1,v2,f) (((v1)<((v2)+f))&&((v1)>((v2)-f)))
#define V3EQUAL(p1,p2,f) (V_EQUAL((p1).x,(p2).x,f) \
	&& V_EQUAL((p1).y,(p2).y,f) && V_EQUAL((p1).z,(p2).z,f))
#define V2EQUAL(p1,p2,f) (V_EQUAL((p1).x,(p2).x,f) \
	&& V_EQUAL((p1).y,(p2).y,f))

#ifdef __cplusplus
    }
#endif
#endif  /* GDEFS_H */
