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

/*  Poly.c
 *  Poly3 C Library
 *  By Philip Thompson
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef UNIX
	#include <unistd.h>
#endif

#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"


extern Poly3*
Poly3Alloc(int nverts, int closed, Poly3 *next)
{
    Poly3 *poly;

    if (nverts < 0) {
        fprintf(stderr,"Poly3Alloc: nverts negative");
        return NULL;
    }
    if ((poly = (Poly3 *)malloc(sizeof(Poly3))) == NULL) { /* local heap */
        fprintf(stderr, "Poly3Alloc: can't alloc poly");
        return NULL;
    }
    if (nverts > 0) {
        if (!(poly->verts = (Point3 *)calloc(nverts, sizeof(Point3)))) { /* local heap */
            fprintf(stderr, "Poly3Alloc: can't alloc poly vertices");
            free(poly); /* local heap */
            return NULL;
        }
    } else
        poly->verts = NULL;
    poly->nverts = (unsigned short)nverts;
    poly->closed = (unsigned short)closed;
    poly->next = next;
	poly->material = NULL; /* this will point to an external reference, don't free! */
    return poly;
}


extern void
Poly3Free(Poly3 **poly) 
{
    if (*poly == NULL)
        return;
    if ((*poly)->verts)
        free((*poly)->verts); /* local heap */
    free(*poly); /* local heap */
    *poly = NULL;
}


extern void Poly3FreeList(Poly3 *polys) 
{
    if (polys == NULL)
        return;
    if (polys->verts)
        free(polys->verts); /* local heap */
    if (polys->next)
        Poly3FreeList(polys->next);
    free(polys); /* local heap */
}


extern Poly3 *Poly3GetLast(Poly3 *polys)
{
    while (polys->next)
        polys = polys->next;
    return polys;
}


extern void Poly3Print(FILE *fp, Poly3 *poly, int pntFlag)
{
    int i;

    if (poly == NULL)
        return;
    fprintf(fp, "Poly  nverts %d  closed %d  next %p\n",
            poly->nverts, poly->closed, (void*)poly->next);
    if (pntFlag)
        for (i = 0; i < poly->nverts; i++)
            fprintf(fp, " (%.3f,%.3f,%.3f)", poly->verts[i].x,
                    poly->verts[i].y, poly->verts[i].z);
    fprintf(fp, "\n");
}


extern void Poly3PrintList(FILE *fp, Poly3 *polys)
{
    Poly3 *ptr;
    int i, cnt = 0;

    for (ptr = polys; ptr != NULL; ptr = ptr->next, cnt++) {
        fprintf(fp, "Poly[%d]   nverts %d  closed %d\n", cnt,
                ptr->nverts, ptr->closed);
        for (i = 0; i < ptr->nverts; i++)
            fprintf(fp, " (%.3f,%.3f,%.3f)", ptr->verts[i].x,
                    ptr->verts[i].y, ptr->verts[i].z);
        fprintf(fp, "\n");
    }
}

/* Divide a face along the shortest diagonal. Return two new polygons.
*/
extern Poly3 *FaceSubDivide(Poly3 *poly)

{
    double dist1, dist2;
    Poly3 *np1, *np2;

    if (poly->nverts < 4)
        return NULL;
    dist1 = V3DistanceBetween2Points(poly->verts, poly->verts+2);
    dist2 = V3DistanceBetween2Points(poly->verts+1, poly->verts+3);
    if ((dist1 == 0.0) || (dist2 == 0.0))
        return NULL;
    if (((np2 = Poly3Alloc(3, 1, NULL)) == NULL) ||
            ((np1 = Poly3Alloc(3, 1, np2)) == NULL))
        return NULL;
	np1->material = poly->material; /* points into table */
    np1->verts[0] = poly->verts[0];
    np1->verts[1] = poly->verts[1];
	np2->material = poly->material; /* points into table */
    np2->verts[0] = poly->verts[2];
    np2->verts[1] = poly->verts[3];
    if (dist1 > dist2) {
        np1->verts[2] = poly->verts[3];
        np2->verts[2] = poly->verts[1];
    } else {
        np1->verts[2] = poly->verts[2];
        np2->verts[2] = poly->verts[0];
    }
    return np1;
}


/* Move a list of points along z-axis (for objects w/ elevations) */
extern Poly3 *CopyPolyUp(Poly3 *poly, double dist)

{
    unsigned short i;
    Poly3 *np;

    if ((np = Poly3Alloc((int)poly->nverts, (int)poly->closed, NULL)) == NULL) {
        fprintf(stderr, "CopyPolyUp: can't alloc poly");
        return NULL;
    }
	np->material = poly->material; /* points into table */
    for (i = 0; i < poly->nverts; i++) {
        np->verts[i].x = poly->verts[i].x;
        np->verts[i].y = poly->verts[i].y;
        np->verts[i].z = poly->verts[i].z + dist;
    }
    return np;
}




/* Generates a polygon contouring the line of points offset
 * at both sides by the distance hwdth (halfwidth)
 */
extern Poly3 *WidePlist(Poly3 *poly, double hwdth)

{
    int i, j, k, n;
    double a1, a2, ang, adif, doff;
    Poly3 *np;

#ifdef DEBUG
    ads_printf("WidePlist(%p, %f)\n", poly, hwdth);
#endif
    if (hwdth <= 0.0) {
        fprintf(stderr, "WidePlist: bad line width: %f", hwdth);
        return NULL;
    }
    if (poly->closed)
        n = 2 * (poly->nverts + 1);
    else
        n = 2 * poly->nverts;
    if ((np = Poly3Alloc(n, 1, NULL)) == NULL) {
        fprintf(stderr, "WidePlist: can't alloc poly");
        return NULL;
    }
	np->material = poly->material; /* points into table */
    /* do interior points */
    for (i = 0, j = 1, k = 2; k < (int)poly->nverts; i++, j++, k++) {
        a1 = V3GetAngle2D(&poly->verts[i], &poly->verts[j]);
        a2 = V3GetAngle2D(&poly->verts[j], &poly->verts[k]);
        ang = (a1 + a2 - M_PI) / 2.0;
        adif = (a2 - a1) / 2.0;
        doff = hwdth / cos(adif);
        (void)V3AddPolar2D(&poly->verts[j], ang, doff, &np->verts[j]);
        (void)V3AddPolar2D(&poly->verts[j], ang, -doff, &np->verts[(n - 1) - j]);
    }
    /* now do the end points */
    if (poly->closed) {
        a1 = V3GetAngle2D(&poly->verts[poly->nverts - 1], &poly->verts[0]);
        a2 = V3GetAngle2D(&poly->verts[0], &poly->verts[1]);
        ang = (a1 + a2 - M_PI) / 2.0;
        adif = (a2 - a1) / 2.0;
        doff = hwdth / cos(adif);
        (void)V3AddPolar2D(&poly->verts[0], ang, doff, &np->verts[0]);
        (void)V3AddPolar2D(&poly->verts[0], ang, -doff, &np->verts[n - 1]);

        a1 = V3GetAngle2D(&poly->verts[i], &poly->verts[j]);
        a2 = V3GetAngle2D(&poly->verts[j], &poly->verts[0]);
        ang = (a1 + a2 - M_PI) / 2.0;
        adif = (a2 - a1) / 2.0;
        doff = hwdth / cos(adif);
        (void)V3AddPolar2D(&poly->verts[j], ang, doff, &np->verts[j]);
        (void)V3AddPolar2D(&poly->verts[j], ang, -doff, &np->verts[j + 3]);

        np->verts[poly->nverts] = np->verts[0];
        np->verts[poly->nverts + 1] = np->verts[n - 1];
    } else {
        ang = V3GetAngle2D(&poly->verts[0], &poly->verts[1]) - M_PI_2;
        (void)V3AddPolar2D(&poly->verts[0], ang, hwdth, &np->verts[0]);
        (void)V3AddPolar2D(&poly->verts[0], ang, -hwdth, &np->verts[n - 1]);

        ang = V3GetAngle2D(&poly->verts[poly->nverts - 1],
                &poly->verts[poly->nverts - 2]) - M_PI_2;
        (void)V3AddPolar2D(&poly->verts[poly->nverts - 1], ang, -hwdth,
                &np->verts[poly->nverts - 1]);
        (void)V3AddPolar2D(&poly->verts[poly->nverts - 1], ang, hwdth,
                &np->verts[poly->nverts]);
    }
    return np;
}

extern Poly3 *CreateSideWalls(Poly3 *poly, double thick)
/* extrude a polygon */
{
    int i;
	unsigned short j;
    Poly3 *np = NULL, *firstp = NULL;

#ifdef DEBUG
    ads_printf( "CreateSideWalls(%p, %f)\n", poly, thick);
#endif
    for (i = 0, j = 1; j < poly->nverts; i++, j++) {
        if ((np = Poly3Alloc(4, 1, firstp)) == NULL) {
            fprintf(stderr, "CreateSideWalls: can't alloc poly");
            return NULL;
        }
		np->material = poly->material; /* points into table */
        np->verts[0] = poly->verts[i];
        np->verts[1] = poly->verts[i];
        np->verts[1].z += thick;
        np->verts[2] = poly->verts[j];
        np->verts[2].z += thick;
        np->verts[3] = poly->verts[j];
        firstp = np;
    }
    if (poly->closed) {
        if ((np = Poly3Alloc(4, 1, firstp)) == NULL) {
            fprintf(stderr, "CreateSideWalls: can't alloc poly");
            return NULL;
        }
		np->material = poly->material; /* points into table */
        np->verts[0] = poly->verts[i];
        np->verts[1] = poly->verts[i];
        np->verts[1].z += thick;
        np->verts[2] = poly->verts[0];
        np->verts[2].z += thick;
        np->verts[3] = poly->verts[0];
        firstp = np;
    }
    return firstp;
}


extern Cyl3 *
Cyl3Alloc(Cyl3 *next)
{
	Cyl3 *cyl;

	cyl = (Cyl3*)malloc(sizeof(Cyl3));
    if (cyl == NULL) {
        fprintf(stderr, "Cyl3Alloc: can't alloc cylinder\n");
        return NULL;
    }

	cyl->next = next;
	cyl->material = NULL;
	cyl->length = cyl->srad = cyl->erad = 0.0;
	return cyl;
}

extern void
Cyl3Free(Cyl3 **cyl)
{
	if(*cyl == NULL) return;
	free(*cyl);
	*cyl = NULL;
}

extern void
Cyl3FreeList(Cyl3 *cyls)
{
	if(cyls == NULL) return;
	if(cyls->next) Cyl3FreeList(cyls->next);
	free(cyls);
}


extern SimpleText*
SimpleTextAlloc(char *s, SimpleText *next)
{
	SimpleText *text;
	size_t tlen = 0;


	text = (SimpleText*)malloc(sizeof(SimpleText));
    if (text == NULL) {
        fprintf(stderr, "SimpleTextAlloc: can't alloc simple text\n");
        return NULL;
	}
	if(s != NULL) {
		tlen = strlen(s);
	}
	if(tlen > 0) {
		text->s = (char *)malloc(tlen+1);
		if (text->s == NULL) {
			fprintf(stderr,"SimpleTextAlloc: can't alloc string for simple text\n");
			free(text);
			return NULL;
		}
		strncpy(text->s, s, tlen+1);
	} else {
		text->s = NULL;
	}
	text->next = next;
	return text;
}

extern void
SimpleTextFree(SimpleText *text)
{
	if(text == NULL) return;
	if(text->next != NULL) SimpleTextFree(text->next);
	if(text->s != NULL) free(text->s);
	free(text);
}

/*** end poly.c ***/
