/*
This file is part of

* Radout  - Export geometry from Autocad to Radiance scene files.


The MIT License (MIT)

Copyright (c) 1999-2016 Georg Mischler
(Originally acquired from Philip Tompson)

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

/*  geom.c - Parse geometry into polygons
 */


#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
  #include <stdlib.h>
#endif /* _WIN32 */

#include "cadcompat.h"
#include "adtools.h"
#include "resbuf.h"

#include "geomtypes.h"
#include "geomdefs.h"
#include "radout.h"
#include "dlltypes.h"
#include "dllproto.h"
#include "geomproto.h"
#include "cproto.h"

extern Options options;


/*  Convert a polyface mesh to a polygon list
 */
Poly3 *PFaceToPoly(ADSResBuf *edata)

{
    int i, num = 1;
	unsigned short j;
    ADSResBuf *ri, *rb;
    ads_name ename;
    Poly3 *poly = NULL, *firstp = NULL, *polys = NULL;
    Point3 nodes[MAXVERTS];

#ifdef DEBUG
    ads_printf( "PFaceToPoly(%p)\n", edata);
#endif
    ads_name_set(RBSearch(edata, -1)->resval.rlname, ename);
    ads_entnext(ename, ename);
    rb = ads_entget(ename);
    while ((ri = RBSearch(rb, 0)) &&
            (strcmp(ri->resval.rstring, "SEQEND") != 0)) {
        /* test for a location vertex and place in array */
        if ((RBSearch(rb, 70)->resval.rint & 192) == 192) {
            (void)RBGetPoint3(&nodes[num], rb, 10);
            if (++num == MAXVERTS) {
                WarnMsg("PFaceToPoly: too many vertices (%d)", num);
                break;
            }
        } else if (RBSearch(rb, 70)->resval.rint == 128) {
            /* we have a face definition vertex */
            if ((poly = Poly3Alloc(4, 1, firstp)) == NULL) {
				ads_relrb(rb);
                return NULL;
			}
            for (i = 71, j = 0; i < 75; i++) {
                if (((ri = RBSearch(rb, i)) == NULL) ||
                        ((ri->resval.rint == 0)))
                    break;
                poly->verts[j++] = nodes[abs(ri->resval.rint)];
            }
            poly->nverts = j;
            if ((poly->nverts < 3) || !PolyCheckColinear(poly) ||
                    (PolyGetArea(poly) <= 0.0)) {
                Poly3Free(&poly);
            } else if ((poly->nverts == 4) && (options.smooth ||
                    !FaceCheckCoplanar(poly))) {
                if ((polys = FaceSubDivide(poly)) == NULL)
                   polys = poly;
                else
                    Poly3Free(&poly);
                Poly3GetLast(polys)->next = firstp;
                firstp = polys;
            } else
                firstp = poly;
        } else
            WarnMsg("PFaceToPoly: unknown vertex flag found (%d)",
                    RBSearch(rb, 70)->resval.rint);
        ads_relrb(rb);
        if ( (ads_entnext(ename, ename) != RTNORM) ||
			 (!(rb = ads_entget(ename)) ) ) {
            ErrorMsg("PFaceToPoly: something bad happend");
            return NULL;
        }
    }
    if (rb) ads_relrb(rb);
    return firstp;
}



Poly3 *TraceToPoly(ADSResBuf *edata)

{
    double thick = 0.0;
    Poly3 *poly = NULL;

#ifdef DEBUG
    ads_printf( "TraceToPoly(%p)\n", edata);
#endif
    poly = Poly3Alloc(4, 1, NULL);
    (void)RBGetPoint3(&poly->verts[0], edata, 10);
    (void)RBGetPoint3(&poly->verts[1], edata, 11);
    (void)RBGetPoint3(&poly->verts[3], edata, 12);
    (void)RBGetPoint3(&poly->verts[2], edata, 13);
    if (!PolyCheckColinear(poly) || (PolyGetArea(poly) <= 0.0)) {
        free(poly->verts);
        free(poly);
        return NULL;
    }
    (void)RBGetDouble(&thick, edata, 39);
    poly->next = CopyPolyUp(poly, thick);
    poly->next->next = CreateSideWalls(poly, thick);
    (void)TransformEnt(poly, edata);
    return poly;
}



Poly3 *ArcToPoly(ADSResBuf *edata)

{
    int i, dir = CW;
    double thick = 0.0, radius, a1, a2;
    Point3 center;
    Poly3 *poly = NULL, *arc = NULL, *polys = NULL;

#ifdef DEBUG
    ads_printf( "ArcToPoly(%p, %d)\n", edata, dir);
#endif
    if ((RBGetDouble(&thick, edata, 39) == NULL) || (thick == 0.0))
        return NULL;
    (void)RBGetPoint3(&center, edata, 10);
    (void)RBGetDouble(&radius, edata, 40);
    (void)RBGetDouble(&a1, edata, 50);
    (void)RBGetDouble(&a2, edata, 51);
    if ((poly = SegmentArc(&center, dir,
		options.distTolerance, options.angleTolerance,
		radius, a1, a2)) == NULL)
        return NULL;
    if ((arc = Poly3Alloc(poly->nverts + 2, 0, poly)) == NULL) {
		Poly3FreeList(poly);
        return NULL;
	}
    for (i = 0; i < (int)poly->nverts; i++)
        arc->verts[i + 1] = poly->verts[i];
    (void)V3AddPolar2D(&center, a1, radius, &arc->verts[poly->nverts + 1]);
    (void)V3AddPolar2D(&center, a2, radius, &arc->verts[0]);
    if ((polys = CreateSideWalls(arc, thick)) == NULL)
        return NULL;
    Poly3FreeList(arc);
    (void)TransformEnt(polys, edata);
    return polys;
}


/* Convert a polygon mesh (grid) to our own polygon list
 */
Poly3 *MeshToPoly(ADSResBuf *edata)

{
    int i, j, m = 0, n = 0, mclosed = 0, nclosed = 0;
    ADSResBuf *rb;
    ads_name ename;
    Poly3 *poly = NULL, *firstp = NULL, *polys = NULL;
    Point3 **nodes;

#ifdef DEBUG
    ads_printf( "MeshToPoly(%p) entered\n", edata);
#endif
    if (RBSearch(edata, 70)->resval.rint & (2 | 4 | 8 | 64))
        return NULL;
    (void)RBGetInt(&m, edata, 71);
    (void)RBGetInt(&n, edata, 72);
    if (RBSearch(edata, 70)->resval.rint & 1)
        mclosed = 1;
    if (RBSearch(edata, 70)->resval.rint & 32)
        nclosed = 1;

    if ((nodes = (Point3 **)malloc(n * sizeof(Point3 *))) == NULL) {
        WarnSys("MeshToPoly: can't alloc nodes");
        return NULL;
    }
    for (i = 0; i < n; i++) {
        if ((nodes[i] = (Point3 *)malloc(m * sizeof(Point3))) == NULL) {
            WarnSys("MeshToPoly: can't alloc nodes");
            return NULL;
        }
    }
    ads_name_set(RBSearch(edata, -1)->resval.rlname, ename);
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            ads_entnext(ename, ename);
            rb = ads_entget(ename);
            (void)RBGetPoint3(&nodes[j][i], rb, 10);
            ads_relrb(rb);
        }
    }
    m--;
    n--;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            poly = Poly3Alloc(4, 1, firstp);
            poly->verts[3] = nodes[i][j];
            poly->verts[2] = nodes[i+1][j];
            poly->verts[1] = nodes[i+1][j+1];
            poly->verts[0] = nodes[i][j+1];
            if (!PolyCheckColinear(poly) || (PolyGetArea(poly) <= 0.0)) {
                Poly3Free(&poly);
                continue;
            }
            if ((poly->nverts == 4) && (options.smooth ||
                    !FaceCheckCoplanar(poly))) {
                if ((polys = FaceSubDivide(poly)) == NULL)
                    polys = poly;
                else
                    Poly3Free(&poly);
                Poly3GetLast(polys)->next = firstp;
                firstp = polys;
            } else
                firstp = poly;
        }
        if (mclosed) {
            if ((poly = Poly3Alloc(4, 1, firstp)) == NULL)
                goto done;
            poly->verts[3] = nodes[i][m];
            poly->verts[2] = nodes[i+1][m];
            poly->verts[1] = nodes[i+1][0];
            poly->verts[0] = nodes[i][0];
            if (!PolyCheckColinear(poly) || (PolyGetArea(poly) <= 0.0)) {
                Poly3Free(&poly);
                continue;
            }
            if ((poly->nverts == 4) && (options.smooth ||
                    !FaceCheckCoplanar(poly))) {
                if ((polys = FaceSubDivide(poly)) == NULL)
                    polys = poly;
                else
                    Poly3Free(&poly);
                Poly3GetLast(polys)->next = firstp;
                firstp = polys;
            } else
                firstp = poly;
        }
    }
    if (nclosed) {
        for (j = 0; j < m; j++) {
            if ((poly = Poly3Alloc(4, 1, firstp)) == NULL)
                goto done;
            poly->verts[3] = nodes[n][j];
            poly->verts[2] = nodes[0][j];
            poly->verts[1] = nodes[0][j+1];
            poly->verts[0] = nodes[n][j+1];
            if (!PolyCheckColinear(poly) || (PolyGetArea(poly) <= 0.0)) {
                Poly3Free(&poly);
                continue;
            }
            if ((poly->nverts == 4) && (options.smooth ||
                    !FaceCheckCoplanar(poly))) {
                if ((polys = FaceSubDivide(poly)) == NULL)
                    polys = poly;
                else
                    Poly3Free(&poly);
                Poly3GetLast(polys)->next = firstp;
                firstp = polys;
            } else
                firstp = poly;
        }
    }
done:
    for (i = 0; i < n; i++)
        free(nodes[i]);
    free(nodes);
#ifdef DEBUG
    ads_printf( "MeshToPoly(%p) done\n", firstp);
#endif
    return firstp;
}


/*  Make a polygon list describing a line depending on entity data,
 *  if thickness > 0
 */
Poly3 *LineToPoly(ADSResBuf *edata)

{
    double thick = 0.0;
    Point3 dir;
    Poly3 *poly;

#ifdef DEBUG
    ads_printf( "LineToPoly(%p)\n", edata);
#endif
    if ((RBGetDouble(&thick, edata, 39) == NULL) || (thick == 0.0))
        return NULL;
    if ((poly = Poly3Alloc(4, 1, NULL)) == NULL) {
        WarnMsg("LineToPoly: can't alloc poly");
        return NULL;
    }
    (void)RBGetPoint3(&dir, edata, 210);
    (void)RBGetPoint3(&poly->verts[0], edata, 10);
    (void)RBGetPoint3(&poly->verts[1], edata, 11);
    (void)V3Translate(&poly->verts[0], &dir, thick, &poly->verts[3]);
    (void)V3Translate(&poly->verts[1], &dir, thick, &poly->verts[2]);
    return poly;
}


/* make a polygon list describing a 2D polyline depending on edata,
 * the number of segments for arcs and the type: 1 - pline w/ thickness,
 * 2 - closed pline as polygon, 3 - pline w/ constant width.
 */
Poly3 *PlineToPoly(ADSResBuf *edata, int typ)

{
    int i, dir = CW;
	unsigned short len = 0;
    double thick = 0.0, bulge = 0.0;
    ADSResBuf *ri, *rb, *pltype;
    int flag = 0;
    ads_name ename;
    Point3 p0, p1, p2, *plist;
    Poly3 *polys, *poly, *tpoly, *wpoly = NULL;

#ifdef DEBUG
    ads_printf( "PlineToPoly(%p, %d)\n", edata, typ);
#endif

	/* this is a hack to redirect lwpolylines  */
	pltype = RBSearch(edata, 0);
	if (strcmp(pltype->resval.rstring, "LWPOLYLINE") == 0) {
		return LWPolyToPoly(edata, typ);
	}

    (void)RBGetInt(&flag, edata, 70);
    (void)RBGetDouble(&thick, edata, 39);
    if ((flag & (8 | 16 | 32 | 64)) /* no 3D stuff allowed */
		)
        return NULL;
    ads_entnext((RBSearch(edata, -1))->resval.rlname, ename);
    rb = ads_entget(ename);
    (void)RBGetPoint3(&p0, rb, 10);
    if ((poly = Poly3Alloc(MAXVERTS, 0, NULL)) == NULL) {
        WarnMsg("PlineToPoly: can't alloc poly");
        return NULL;
    }
    plist = poly->verts;
    while ((ri = RBSearch(rb, 0)) &&
            (strcmp(ri->resval.rstring, "SEQEND") != 0)) {
        (void)RBGetPoint3(&p1, rb, 10);
        plist[len++] = p1;
        (void)RBGetDouble(&bulge, rb, 42);
        ads_entnext((RBSearch(rb, -1))->resval.rlname, ename);
        ads_relrb(rb);
        rb = ads_entget(ename);
        if ((bulge != 0.0) && ((flag & 1) ||
                strcmp(RBSearch(rb, 0)->resval.rstring, "SEQEND"))) {
            Point3 center;
            Poly3 *tp;
            double radius, a1, a2;

            if (RBGetPoint3(&p2, rb, 10) == NULL)
                p2 = p0;
            radius = BulgeToArc(&p1, &p2, bulge, &dir, &center, &a1, &a2);
            tp = SegmentArc(&center, dir,
				options.distTolerance, options.angleTolerance,
				radius, a1, a2);
            for (i = 0; i < (int)tp->nverts; i++)
                plist[len++] = tp->verts[i];
            Poly3FreeList(tp);
        }
    }
    if (len <= 0) {
        ads_relrb(rb);
        free(poly->verts);
        free(poly);
        return NULL;
    }
    poly->nverts = len;
    poly->closed = (unsigned short)(flag & 1);
    PolyCheckColinear(poly);
#ifdef DEBUG
    Poly3Print(stderr, poly, 1);
#endif
    /* wide line */
    if ((typ == 3) && ((ri = RBSearch(edata, 40)) != NULL)) {
		wpoly = WidePlist(poly, ri->resval.rreal / 2.0);
		Poly3FreeList(poly);
		poly = wpoly;
		if (thick != 0.0)
			poly->next = CopyPolyUp(poly, thick);
    }
    /* thick line */
    if (thick != 0.0) {
        polys = CreateSideWalls(poly, thick);
        if (typ == 1) {
            Poly3FreeList(poly);
            poly = polys;
		} else if (typ == 2) {
	/*  thick polygons are killed elsewhere if polygons are off ...*/
            tpoly = Poly3GetLast(poly);
			tpoly->next = CopyPolyUp(poly, thick);
			tpoly->next->next = polys;
        } else {
            tpoly = Poly3GetLast(poly);
			tpoly->next = polys;
        }
    } else { /*  remove flat unwide open plines */
        if (typ == 1) {
            Poly3FreeList(poly);
            return NULL;
		}
	}
    /* transform from ecs to mcs/wcs */
    (void)TransformEnt(poly, edata);
#ifdef DEBUG
    Poly3PrintList(stderr, poly);
#endif
    return poly;
}


/*  This is the new LWPOLY data type in R14.
 * make a polygon list describing a 2D polyline depending on edata,
 * the number of segments for arcs and the type: 1 - pline w/ thickness,
 * 2 - closed pline as polygon, 3 - pline w/ constant width.
 */
Poly3 *LWPolyToPoly(ADSResBuf *edata, int typ)

{
    int i, dir = CW;
	unsigned short len = 0;
    double thick = 0.0, bulge = 0.0, height = 0.0, width = 0.0;
    ADSResBuf *ri, *currb, *curw1rb, * curw2rb, *curbrb;
    int flag = 0;
    Point3 p0, p1, p2, *plist;
    Poly3 *polys, *poly, *tpoly, *wpoly = NULL;

#ifdef DEBUG
    ads_printf( "LWPolyToPoly(%p, %d)\n", edata, typ);
#endif
    (void)RBGetInt(&flag, edata, 70);
    (void)RBGetDouble(&thick, edata, 39);
    (void)RBGetDouble(&height, edata, 38);

	if ((ri = RBSearch(edata, 43)) != NULL) {
		width = ri->resval.rreal;
	} else if ((ri = RBSearch(edata, 40)) != NULL) {
		width = ri->resval.rreal;
	} else width = 0.0;
	

	if ((poly = Poly3Alloc(MAXVERTS, 0, NULL)) == NULL) {
        WarnMsg("PlineToPoly: can't alloc poly");
        return NULL;
    }
    plist = poly->verts;

    currb = RBGetPoint3(&p1, edata->rbnext, 10);
	p0 = p1;
    while (currb) {
		p1.z = height;
        plist[len++] = p1;
		
		if (currb->rbnext && currb->rbnext->restype == 40) {
			curw1rb = currb->rbnext;
		} else curw1rb = currb;

		if (curw1rb->rbnext && curw1rb->rbnext->restype == 41) {
			curw2rb = curw1rb->rbnext;
		} else curw2rb = curw1rb;

		if (curw2rb->rbnext && curw2rb->rbnext->restype == 42) {
			curbrb  = curw2rb->rbnext;
			bulge = curbrb->resval.rreal;
		} else bulge = 0.0;

        if ((bulge != 0.0) && (flag & 1)) {
            Point3 center;
            Poly3 *tp;
            double radius, a1, a2;

            if (curw2rb->rbnext && curw2rb->rbnext->restype == 10) {
				p2.x = curw2rb->resval.rpoint[0];
				p2.y = curw2rb->resval.rpoint[1];
				p2.z = height;
			}
			else p2 = p0;

            radius = BulgeToArc(&p1, &p2, bulge, &dir, &center, &a1, &a2);
            tp = SegmentArc(&center, dir,
				options.distTolerance, options.angleTolerance,
				radius, a1, a2);
            for (i = 0; i < (int)tp->nverts; i++)
                plist[len++] = tp->verts[i];
            Poly3FreeList(tp);
        }
		currb = RBGetPoint3(&p1, currb->rbnext, 10);
    }
    if (len <= 0) {
        free(poly->verts);
        free(poly);
        return NULL;
    }
    poly->nverts = len;
    poly->closed = (unsigned short)(flag & 1);
    PolyCheckColinear(poly);
#ifdef DEBUG
    Poly3Print(stderr, poly, 1);
#endif
    /* wide line */
    if ((typ == 3) && (width > 0)) {
		wpoly = WidePlist(poly, width / 2.0);
		Poly3FreeList(poly);
		poly = wpoly;
		if (thick != 0.0)
			poly->next = CopyPolyUp(poly, thick);
    }
    /* thick line */
    if (thick != 0.0) {
        polys = CreateSideWalls(poly, thick);
        if (typ == 1) {
            Poly3FreeList(poly);
            poly = polys;
		} else if (typ == 2) {
	    /*  thick polygons are killed elsewhere if polygons are off ...*/
            tpoly = Poly3GetLast(poly);
			tpoly->next = CopyPolyUp(poly, thick);
			tpoly->next->next = polys;
        } else {
            tpoly = Poly3GetLast(poly);
			tpoly->next = polys;
        }
    } else { /*  remove flat unwide open plines */
        if (typ == 1) {
            Poly3FreeList(poly);
            return NULL;
		}
	}
    /* transform from ecs to mcs/wcs */
    (void)TransformEnt(poly, edata);
#ifdef DEBUG
    Poly3PrintList(stderr, poly);
#endif
    return poly;
}




Poly3 *FaceToPoly(ADSResBuf *edata)

{
    Poly3 *poly, *plist;

    if ((poly = Poly3Alloc(4, 1, NULL)) == NULL)
        return NULL;
    (void)RBGetPoint3(&poly->verts[0], edata, 10);
    (void)RBGetPoint3(&poly->verts[1], edata, 11);
    (void)RBGetPoint3(&poly->verts[2], edata, 12);
    (void)RBGetPoint3(&poly->verts[3], edata, 13);
    while ((poly->nverts > 1) &&
            (V3ISEQUAL(&poly->verts[0], &poly->verts[poly->nverts-1])))
        poly->nverts--;
    if ((poly->nverts < 3) || (!PolyCheckColinear(poly)) ||
            (PolyGetArea(poly) <= 0.0)) {
        Poly3Free(&poly);
        return NULL;
    }
    if ((poly->nverts == 4) && (options.smooth || !FaceCheckCoplanar(poly))) {
        if ((plist = FaceSubDivide(poly)) != NULL) {
            Poly3Free(&poly);
            return plist;
        }
    }
    return poly;
}

/*** end geom.c ***/
