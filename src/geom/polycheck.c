
/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


The MIT License (MIT)

Copyright (c) 1999-2016 Georg Mischler
(originally acquired from Philip Thompson)

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
#include  <stdio.h>
#include  <math.h>
#include  <stdlib.h>

#include  "geomtypes.h"
#include  "geomdefs.h"
#include  "geomproto.h"


int PolyCheckCoincident(Poly3 *poly)

{
    register int i = 0, j = 0;
    Point3 *verts;

#ifdef DEBUG1
    fprintf(stderr, "PolyCheckCoincident: checking poly %d\n", poly->nverts);
#endif
    if (poly->nverts < 3) {         /* must have at least 3 points */
#ifdef DEBUG1
		/*  degenerate polys are a valid state, so avoid message */
        fprintf(stderr, "PolyCheckCoincident: too few points: %d", poly->nverts);
#endif
        return 0;
    }
    /* do a fast test first */
    for (j=0, i=1; i < (int)poly->nverts; j++, i++) {
        if ((poly->verts[i].x == poly->verts[j].x) &&
                (poly->verts[i].y == poly->verts[j].y) &&
                (poly->verts[i].z == poly->verts[j].z))
            break;
    }
    if (i == (int)poly->nverts)
        return 1;

    /* found some, make an array and copy verts only as needed */
	verts = (Point3*)malloc(poly->nverts * sizeof(Point3));
    if (verts == NULL) { /* local heap */
        fprintf(stderr, "PolyCheckCoincident: can't malloc");
        return 0;
    }
    j = 0;
    verts[j] = poly->verts[0];
    for (i=1; i < (int)poly->nverts; i++) {
        if (V3ISEQUAL(&verts[j], &poly->verts[i]))
            continue;
        verts[j++] = poly->verts[i];
    }
    if ((verts[0].x == verts[j-1].x) && (verts[0].y == verts[j-1].y) &&
            (verts[0].z == verts[j-1].z))
        j--;
    free((char*)poly->verts); /* local heap */
    if ((j < 3) && poly->closed) {
        poly->verts = NULL;
        poly->nverts = 0;
        free(verts); /* local heap */
        return 0;
    }
	poly->verts = (Point3*)realloc(verts, j * sizeof(Point3));
    if (poly->verts == NULL) { /* local heap */
        fprintf(stderr, "PolyCheckCoincident: can't realloc");
        poly->nverts = 0;
        return 0;
    }
    poly->nverts = (unsigned short)j;
    return 1;
}


int PolyCheckColinear(Poly3 *poly)

{
    register int i, j, k, nverts = 0;
    Vector3 v1, v2, *verts;

#ifdef DEBUG1
    fprintf(stderr, "PolyCheckColinear: checking poly %d\n", poly->nverts);
#endif
    if (poly->nverts < 3)        /* must have at least 3 points */
        return 0;
    /* check for coincidence and colinearity */
    /* found some, make an array and copy verts only as needed */
	verts = (Point3*)malloc(poly->nverts * sizeof(Point3));
    if (verts == NULL) {
        fprintf(stderr, "PolyCheckColinear: can't alloc points: %d\n",
                poly->nverts);
        return 0;
    }
    for (i = 0, j = 1, k = 2; k < (int)poly->nverts; ) {
        /* check coincidence first */
        if ((poly->verts[i].x == poly->verts[j].x) &&
                (poly->verts[i].y == poly->verts[j].y) &&
                (poly->verts[i].z == poly->verts[j].z)) {
            j = k;
            k++;
            continue;
        }
        if ((poly->verts[j].x == poly->verts[k].x) &&
                (poly->verts[j].y == poly->verts[k].y) &&
                (poly->verts[j].z == poly->verts[k].z)) {
            k++;
            continue;
        }
        /* make sure there is no doubling back */
        if ((poly->verts[i].x == poly->verts[k].x) &&
                (poly->verts[i].y == poly->verts[k].y) &&
                (poly->verts[i].z == poly->verts[k].z)) {
            i+=2; j+=2; k+=2;
            continue;
        }
        /* now nothing is coincident, check directions */
        if (V3Normalize(V3Sub(&poly->verts[i], &poly->verts[j], &v1))
                == 0.0) {
            fprintf(stderr, "PolyCheckColinear: error calculating direction.");
            return 0;
        }
        if (V3Normalize(V3Sub(&poly->verts[j], &poly->verts[k], &v2))
                == 0.0) {
            fprintf(stderr, "PolyCheckColinear: error calculating direction.");
            return 0;
        }
        if ((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z)) {
#ifdef DEBUG2
            fprintf(stderr, "PolyCheckColinear: found point %d\n", j);
#endif
            j = k;
            k++;
            continue;
        }
        verts[nverts] =  poly->verts[i];
        nverts++;
        i = j;
        j = k;
        k++;
    }
    /* no, we don't check the closing condition. */
    if (i < (int)poly->nverts)
        verts[nverts++] = poly->verts[i];
    if (j < (int)poly->nverts) {
        if ((poly->verts[i].x != poly->verts[j].x) ||
                (poly->verts[i].y != poly->verts[j].y) ||
                (poly->verts[i].z != poly->verts[j].z))
            verts[nverts++] = poly->verts[j];
    } 
    /* now check if first and last are coincident */
    if ((verts[0].x == verts[nverts-1].x) &&
            (verts[0].y == verts[nverts-1].y) &&
            (verts[0].z == verts[nverts-1].z)) {
        nverts--;
        poly->closed = 1;
    }

    free((char*)poly->verts); /* local heap */
    if ((nverts < 3) && poly->closed){
        poly->verts = NULL;
        poly->nverts = 0;
        free(verts); /* local heap */
#ifdef DEBUG1
        fprintf(stderr, "PolyCheckColinear: too few points remain: %d", nverts);
#endif
        return 0;         /* all must be different */
    }
	poly->verts = (Point3*)realloc(verts, nverts*sizeof(Point3));
    if (poly->verts == NULL) {
        fprintf(stderr, "PolyCheckColinear: can't realloc");
        poly->nverts = 0;
        return 0;
    }
    poly->nverts = (unsigned short)nverts;
    return 1;
}


int PolyCheckCoplanar(Poly3 *poly)

{
    register int i;
    register double a, b, c, d;
    double x1, y1, z1, x2, y2, z2, x3, y3, z3;

#ifdef DEBUG1
    fprintf(stderr, "PolyCheckCoplanar: checking poly %d\n", poly->nverts);
#endif
    if (poly->nverts < 4) {         /* must have at least 3 points */
#ifdef DEBUG1
        fprintf(stderr, "PolyCheckCoplanar: too few points: %d\n", poly->nverts);
#endif
        return 1;
    }
    /* Calculate the normal vector */
    x1 = poly->verts[0].x - poly->verts[1].x;
    y1 = poly->verts[0].y - poly->verts[1].y;
    z1 = poly->verts[0].z - poly->verts[1].z;
    x2 = poly->verts[1].x;
    y2 = poly->verts[1].y;
    z2 = poly->verts[1].z;
    x3 = poly->verts[2].x - poly->verts[1].x;
    y3 = poly->verts[2].y - poly->verts[1].y;
    z3 = poly->verts[2].z - poly->verts[1].z;
    a = (y1 * z3 - y3 * z1);
    b = (x3 * z1 - x1 * z3);
    c = (x1 * y3 - x3 * y1);
    d = -(a * x2 + b * y2 + c * z2);

    /* make sure all points are coplanar */
    for (i = 3; i < (int)poly->nverts; i++) {
        if (fabs((a * poly->verts[i].x) + (b * poly->verts[i].y) +
                (c * poly->verts[i].z) + d) > EPSILON)
            return 0;
    }
    return 1;
}


int FaceCheckCoplanar(Poly3 *face)

{
    register double a, b, c, d;
    double x1, y1, z1, x2, y2, z2, x3, y3, z3;

#ifdef DEBUG1
    fprintf(stderr, "FaceCheckCoplanar: checking face %d\n", face->nverts);
#endif
    if (face->nverts == 3)        /* 3 points define a plane */
        return 1;
    else if (face->nverts < 4) {     /* must have at least 3 points */
#ifdef DEBUG1
        fprintf(stderr, "FaceCheckCoplanar: too few points: %d", face->nverts);
#endif
        return 0;
    }
    /* Calculate the normal vector */
    x1 = face->verts[0].x - face->verts[1].x;
    y1 = face->verts[0].y - face->verts[1].y;
    z1 = face->verts[0].z - face->verts[1].z;
    x2 = face->verts[1].x;
    y2 = face->verts[1].y;
    z2 = face->verts[1].z;
    x3 = face->verts[2].x - face->verts[1].x;
    y3 = face->verts[2].y - face->verts[1].y;
    z3 = face->verts[2].z - face->verts[1].z;
    a = (y1 * z3 - y3 * z1);
    b = (x3 * z1 - x1 * z3);
    c = (x1 * y3 - x3 * y1);
    d = -(a * x2 + b * y2 + c * z2);

    /* make sure all points are coplanar */
    if (fabs((a * face->verts[3].x) + (b * face->verts[3].y) +
            (c * face->verts[3].z) + d) > EPSILON)
        return 0;
    return 1;
}


double PolyGetArea(Poly3 *poly)
{
    register int i;
    Point3 norm, v1, v2, v3;
    double area;

    if (poly->nverts < 3) {
#ifdef DEBUG1
        fprintf(stderr, "PolyGetArea: too few points: %d", poly->nverts);
#endif
        return 0.0;
    }
    norm.x = norm.y = norm.z = 0.0;
    v1.x = v1.y = v1.z = 0.0;
    for (i = 1; i < (int)poly->nverts; i++) {
        v2.x = poly->verts[i].x - poly->verts[0].x;
        v2.y = poly->verts[i].y - poly->verts[0].y;
        v2.z = poly->verts[i].z - poly->verts[0].z;
        (void)V3Cross(&v1, &v2, &v3);
        norm.x += v3.x;
        norm.y += v3.y;
        norm.z += v3.z;
		v1 = v2;
    }
    if ((area = V3Normalize(&norm)) == 0.0)
        return 0.0;
    return(0.5 * area);
}


/* return the area of triangle formed by three vertices p0, p1 and p2
 */
double TriGetArea(Point3 *p0, Point3 *p1, Point3 *p2)

{
    Point3 area;

    /* area of the polygon's projection onto the YZ plane */
    area.x = ((p1->y - p0->y) * (p1->z + p0->z) +
            (p2->y - p1->y) * (p2->z + p1->z) +
            (p0->y - p2->y) * (p0->z + p2->z));
    /* area of the polygon's projection onto the ZX plan */
    area.y = ((p1->z - p0->z) * (p1->x + p0->x) +
            (p2->z - p1->z) * (p2->x + p1->x) +
            (p0->z - p2->z) * (p0->x + p2->x));
    /* area of the polygon's projection onto the XY plane */
    area.z = ((p1->x - p0->x) * (p1->y + p0->y) +
            (p2->x - p1->x) * (p2->y + p1->y) +
            (p0->x - p2->x) * (p0->y + p2->y));
    /* length (magnitude) of the area vector */
    return (0.5 * sqrt(area.x * area.x + area.y * area.y + area.z * area.z));
}

/*** end polycheck.c ***/
