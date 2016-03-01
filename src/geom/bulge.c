/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


For all parts not taken from Graphic-Gems:

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

/*  bulge.c - calculate tesselation for arc segments
 */


#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
  #include <stdlib.h>
#endif /* _WIN32 */

#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"



double StartTangent(double ang)

{
    double tangent = ang + M_PI_2;

    if (ang == 0.0) {
        while ((cos(tangent) > 0.0) || (sin(tangent) >= 0.0))
            tangent += M_PI_2;
    } else if ((ang > 0.0) && (ang <= M_PI_2)) {
        while ((cos(tangent) < 0.0) || (sin(tangent) > 0.0))
            tangent += M_PI_2;
    } else if ((ang > M_PI_2) && (ang <= M_PI)) {
        while ((cos(tangent) < 0.0) || (sin(tangent) < 0.0))
            tangent += M_PI_2;
    } else if ((ang > M_PI) && (ang <= 3.0 * M_PI_2)) {
        while ((cos(tangent) >= 0.0) || (sin(tangent) < 0.0))
            tangent += M_PI_2;
    } else if ((ang > 3.0 * M_PI_2) && (ang <= 2.0 * M_PI)) {
        while ((cos(tangent) > 0.0) || (sin(tangent) >= 0.0))
            tangent += M_PI_2;
    }
    return tangent;
}


/* Computes the parameters for a polyline which approximates a circular
 *  arc, maintaining the arc length and start and end positions.
 * From Graphic Gems 2 by Christopher J. Musial
 *
 *
 * * As of: http://www.realtimerendering.com/resources/GraphicsGems/
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
 *
 * Entry:
 *   radius - the radius of the arc
 *   a1 - the angle at which the arc starts
 *   a2 - the angle at which the arc ends
 *   curveTolerance - Maximum allowable distance between the curve and
 *          any part of the polyline
 *   polylineTolerance - Maximum allowable difference between the span
 *          of the polyline and the chord length of the arc, measured
 *          along the axis of the chord (not used as input value)
 *   angleTolerance - Maximum angle between segments.
 * Exit:
 *   segementLength - the length of each segemnt in the polyline
 *   initialAngle - Angle between the tangent vector of the arc at its
 *          initial point and the first segment of the polyline.
 *          Subtract this value from the arc's tangent vector to
 *          get the direction vector of the initial polyline segment
 *   incrAngle - Difference in the direction vectors of 2
 *          adjacent polyline segments. Substract this value from the
 *          direction vector of 1 segment to get he direction vector of
 *          1 segment to get the direction vector of the following
 *          segment.
 * Return value: The number of segments in the polyline
 */
int ArcApprox(double radius, double a1, double a2,
					 double curveTolerance, double angleTolerance,
					 double *segLen, double *initialAngle, double *incrAngle)

{
    int i, numSegments = 0, numTries = 0;
	double plineTolerance;
    double curveToChord, arcLen, chordLen, spanLen = 0.0;
    double startTangent, angle;
    double trialAngles[2] = {0.0,0.0}, trialErrors[2] = {0.0,0.0};
    double tempAngle;

    if ((angle = a2 - a1) < 0.0)
        angle += M_2PI;

    /* set the tolerances according to the size of the arc */
    curveTolerance = plineTolerance = (angle / M_2PI) * radius *
            curveTolerance;
    tempAngle = angle / 2.0;

    /* Calculate the number of segments needed for the polyline */
    curveToChord = radius * (1.0 - cos(tempAngle / 2.0));
    for (numSegments = 2; (curveToChord >= curveTolerance
							|| tempAngle >= angleTolerance);) {
        numSegments *= 2;
        tempAngle /= 2.0;
        curveToChord = radius * (1.0 - cos(tempAngle / 2.));
    }
    /* Calculate the various parameters */
    arcLen = radius * angle;
    chordLen = 2.0 * radius * sin(angle / 2.0);
    startTangent = angle / 2.0;
    *segLen = arcLen / numSegments;
    do {
        /* find the next trial value for the incremental angle */
        switch (numTries) {
        case 0:
            /* First time - use the low end of the range */
            *incrAngle = 0.0;
            break;
        case 1:
            /* Second time - use the high end of the range. Save the
             * trial  values from the previous calculation for the
             * later use.
             */
            trialAngles[0] = *incrAngle;
            trialErrors[0] = spanLen - chordLen;
            *incrAngle = angle / (numSegments - 1);
            break;
        default:
            /* Calculate the next trial value by linear interpolation
             * of the previous 2 trials */
            trialAngles[1] = *incrAngle;
            trialErrors[1] = spanLen - chordLen;
            *incrAngle =
                    (trialAngles[1] - trialAngles[0]) * -trialErrors[1] /
                    (trialErrors[1] - trialErrors[0]) + trialAngles[1];
            trialAngles[0] = trialAngles[1];
            trialErrors[0] = trialErrors[1];
        }
        /* Calculate the polyline span for the new incremental angle */
        for (spanLen = 0, i = 0; i < numSegments; i++) {
            *initialAngle = (angle - (numSegments - 1) * *incrAngle) / 2;
            spanLen += cos(startTangent - *initialAngle - i *
                    *incrAngle);
        }
        spanLen *= *segLen;
        numTries++;
    } while (fabs(spanLen - chordLen) > plineTolerance);
    return numSegments;
}


Poly3 *SegmentArc(Point3 *center, int dir,
				  double distTolerance, double angleTolerance,
				  double radius, double a1, double a2)

{
    int num, i;
    double startTangent, direction, segLen, initialAngle;
    double incrAngle;
    Poly3 *pline;
    Point3 p2;

    (void)V3AddPolar2D(center, a2, radius, &p2);
    /* number of segments in the polyline */
    num = ArcApprox(radius, a1, a2,
			distTolerance, angleTolerance,
            &segLen, &initialAngle, &incrAngle);

    /* Allocate memory for the polyline */
    pline = Poly3Alloc(num - 1, 0, NULL);
    /* find the start tangent for the arc */
    startTangent = StartTangent(a2);

    /* Calculate the coordinates of each vertex in the polyline. */
    if (dir == CW) {
        startTangent = StartTangent(a2);
        (void)V3AddPolar2D(&p2, startTangent - initialAngle, segLen,
                &pline->verts[0]);
        for (i = 1; i < num - 1; i++) {
            direction = startTangent - initialAngle - i * incrAngle;
            (void)V3AddPolar2D(&pline->verts[i - 1], direction, segLen,
                    &pline->verts[i]);
        }
    } else {
        (void)V3AddPolar2D(&p2, startTangent - initialAngle, segLen,
                &pline->verts[num - 2]);
        for (i = 1; i < num - 1; i++) {
            direction = startTangent - initialAngle - i * incrAngle;
            (void)V3AddPolar2D(&pline->verts[num - 1 - i], direction, segLen,
                    &pline->verts[num - 2 - i]);
        }
    }
    return pline;
}

double BulgeToArc(Point3 *p1, Point3 *p2, double bulge, int *dir,
						 Point3 *center, double *a1, double *a2)

{
    double cotbce, radius;

    cotbce = ((1.0 / bulge) - bulge) / 2.0;
    center->x = ((p1->x + p2->x) - ((p2->y - p1->y) * cotbce)) / 2.0;
    center->y = ((p1->y + p2->y) + ((p2->x - p1->x) * cotbce)) / 2.0;
    center->z = p1->z;
    radius = V3DistanceBetween2Points(p1, center);
    *a1 = atan2((p1->y - center->y), (p1->x - center->x));
    *a2 = atan2((p2->y - center->y), (p2->x - center->x));
    if (*a1 < 0.0)
        *a1 += M_2PI;
    if (*a2 < 0.0)
        *a2 += M_2PI;

    /* if the bulge is negative then the interior angle is clockwise
     */
    if (bulge < 0.0) {
        *dir = CW;
        cotbce = *a1;
        *a1 = *a2;
        *a2 = cotbce;
    } else
        *dir = CCW;
#ifdef DEBUG
    ads_printf( "BulgeToArc(p1 %f, %f, %f)\n", p1->x, p1->y, p1->z);
    ads_printf( "BulgeToArc(p2 %f, %f, %f)\n", p2->x, p2->y, p2->z);
    ads_printf( "BulgeToArc(%f, %d)\n", bulge, *dir);
    ads_printf( "BulgeToArc: center %f, %f, %f\n", center->x,
            center->y, center->z);
    ads_printf( "BulgeToArc: %f, %f, %f, %d done\n", *a1, *a2,
            radius, *dir);
#endif
    return radius;
}




/*** end bulge.c ***/
