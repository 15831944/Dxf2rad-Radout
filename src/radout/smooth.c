/*
This file is part of

* Radout  - Export geometry from Autocad to Radiance scene files.


For any changes from the Graphics-Gems original (see below):

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


/* smooth.c - Compute vertex normals for polygons. Lifted from
 * "Building Vertex Normals from an Unstructured Polygon List"
 * by Andrew Glassner, glassner@parc.xerox.com
 * in "Graphics Gems IV", Academic Press, 1994
 *
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
 * 
 * The general idea is to 1) initialize the tables, 2) add polygons
 * one by one, 3) optionally enable edge preservation, 4)
 * optionally set the fuzz factor, 5) compute the normals, 6) do
 * something with the new normals, then 7) free the new storage.
 * The calls to do this are: 
 * 
 * 1) smooth = InitSmooth();
 * 2) IncludePolygon(int nverts, Point3 *verts, Smooth smooth);
 * 3) (optional) enableEdgePreservation(Smooth smooth, double minDot);
 * 4) (optional) setFuzzFraction(smooth Smooth, double fuzzFraction);
 * 5) SmoothMakeVertexNormals(smooth);
 * 6) YOUR CODE
 * 7) FreeSmooth();
 * 
 * Edge preservation is used to retain sharp creases in the model.
 * If it is enabled, then the dot product of each pair of polygons
 * sharing a vertex is computed. If this value is below the value
 * of 'minDot' (that is, the two polygons are a certain distance
 * away from flatness), then the polygons are not averaged together
 * for the vertex normal. 
 * 
 * If you want to re-compute the results without edge preservation, call
 * disableEdgePreservation(smooth);
 * 
 * The general flow of the algorithm is:
 * 1. currentHash = scan hashTable
 * 2. while (any unmarked) {
 * 3. firstVertex = first unmarked vertex.       set to MARKWORKING
 * 4. normal = firstVertex->polygon->normal
 * 5. scan currentHash.  If vertex = firstVertex
 * 6. normal += vertex->polygon->normal
 * 7. set vertex to MARKWORKING
 * (end of scan)
 * 8. set normal to unit length
 * 9. scan currentHash.  If vertex set to MARKWORKING
 * 10. set vertex->normal = normal
 * 11. set to MARKDONE
 * (end of scan)
 * (end while)
 * 
 * The HASH macro must always return a non-negative value, even for
 * negative inputs.
 * The size of the hash table can be adjusted to taste.
 * The fuzz for comparison needs to be matched to the resolution of the
 * model.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef UNIX
	#include <unistd.h>
#endif

#include "cadcompat.h"

#include "adtools.h"
#include "geomtypes.h"
#include "geomdefs.h"
#include "radout.h"
#include "dlltypes.h"
#include "geomproto.h"
#include "cproto.h"

#define MARKWAITING     0
#define MARKWORKING     1
#define MARKDONE        2

/* new array creator */
#define MALLOC(x, num) (x *)malloc((unsigned)((num) * sizeof(x)))
/* fuzzy comparison macro */
#define FUZZEQ(x,y)             (fabs((x)-(y))<(smooth->fuzz))
/* hash table size; related to HASH */
#define HASH_TABLE_SIZE        1000
/* quantization increment */
#define QSIZE    1000.0
#define QUANT(x)     (((int)((x)*QSIZE))/QSIZE)
#define ABSQUANT(x)  (((int)((fabs(x))*QSIZE))/QSIZE)
#define HASH(pt)     ((int)(((3*ABSQUANT(pt->x)) + \
                      (5*ABSQUANT(pt->y)) + (7*ABSQUANT(pt->z))) * \
                      HASH_TABLE_SIZE)) % HASH_TABLE_SIZE

typedef double BARYCCM[3][4];


/*  hash this vertex and add it into the linked list
 */
static void AddVertexToTable(Point3 *pt, Poly3 *poly, int vNum, Smooth smooth)

{
    int hash = HASH(pt);
    HashNode newNode = NEWTYPE(HashNodeStruct);

    newNode->next = smooth->hashTable[hash];
    smooth->hashTable[hash] = newNode;
    newNode->polygon = poly;;
    newNode->vertexNum = vNum;
    newNode->marked = MARKWAITING;
}


/* compute the normal for this polygon using Newell's method
 * (see Tampieri, Gems III, pg 517)
 */
static void MakePolyNormal(Poly3 *poly)
{
    int i;
    Point3 *vp, *p0, *p1;

    poly->normal.x = poly->normal.y = poly->normal.z = 0.0;
    vp = poly->verts;
    for (i = 0; i < (int)poly->nverts; i++) {
        p0 = vp++;
        p1 = vp;
        if (i == poly->nverts - 1)
            p1 = poly->verts;
        poly->normal.x += (p1->y - p0->y) * (p1->z + p0->z);
        poly->normal.y += (p1->z - p0->z) * (p1->x + p0->x);
        poly->normal.z += (p1->x - p0->x) * (p1->y + p0->y);
    }
    (void)V3Negate(&poly->normal);
    (void)V3Normalize(&poly->normal);
}


/* add a polygon to the tables
 */
void IncludePolygon(int nverts, Point3 *verts, Smooth smooth)

{
    int i;
    Point3 *vp, *ovp;
    Poly3 *poly = NEWTYPE(Poly3);

#ifdef DEBUG
    fprintf(stderr, "IncludePolygon(%d, %p, %p) entered\n", nverts,
            verts, smooth);
#endif
    poly->next = NULL;
    if (smooth->polyTail != NULL)
        smooth->polyTail->next = poly;
    else
        smooth->polygonTable = poly;
    smooth->polyTail = poly;
    poly->verts = MALLOC(Point3, nverts);
    poly->normals = MALLOC(Point3, nverts);
    vp = poly->verts;
    ovp = verts;
    poly->nverts = (unsigned short)nverts;
    for (i = 0; i < nverts; i++) {
        vp->x = ovp->x;
        vp->y = ovp->y;
        vp->z = ovp->z;
        AddVertexToTable(vp, poly, i, smooth);
        vp++;
        ovp++;
    }
    MakePolyNormal(poly);
#ifdef DEBUG
    fprintf(stderr, "IncludePolygon(%p) done.\n", poly);
#endif
}


void SmoothSetPolyList(Poly3 *polys, Smooth smooth)

{
    int i;
    Point3 *vp;
    Poly3 *poly;

#ifdef DEBUG
    fprintf(stderr, "SmoothSetPolyList(%p, %p) entered\n",
            polys, smooth);
#endif
    smooth->polygonTable = polys;
    smooth->polyTail = Poly3GetLast(polys);
    for (poly = polys; poly != NULL; poly = poly->next) {
        if (poly->nverts != 3) {
            fprintf(stderr, "SmoothSetPolyList: not a triangle!");
            Poly3Print(stderr, poly, FALSE);
            continue;
        }
        poly->normals = MALLOC(Point3, poly->nverts);
        for (vp = poly->verts, i = 0; i < (int)poly->nverts; i++, vp++)
            AddVertexToTable(vp, poly, i, smooth);
        MakePolyNormal(poly);
    }
#ifdef DEBUG
    fprintf(stderr, "SmoothSetPolyList() done.\n");
#endif
}


void EnableEdgePreservation(Smooth smooth, double minDot)

{
    smooth->edgeTest = TRUE;
    smooth->minDot = minDot;
}


void DisableEdgePreservation(Smooth smooth)

{
    smooth->edgeTest = FALSE;
}


void SetFuzzFraction(Smooth smooth, double fuzzFraction)

{
    smooth->fuzzFraction = fuzzFraction;
}


/* set all the hash-table linked lists to NULL
*/
Smooth InitSmooth()
{
    int i;
    Smooth smooth = NEWTYPE(SmoothStruct);

    for (i = 0; i < HASH_TABLE_SIZE; i++)
        smooth->hashTable[i] = NULL;
    smooth->polygonTable = smooth->polyTail = NULL;
    smooth->fuzzFraction = smooth->fuzz = SMOOTH_TOLERANCE;
    smooth->edgeTest = FALSE;
    smooth->minDot = 0.2;
    return smooth;
}


static void SmoothFreePoly(Poly3 *poly)

{
    if (poly->verts != NULL)
        free(poly->verts);
    if (poly->normals != NULL)
        free(poly->normals);
    poly->next = NULL;
    free(poly);
}


/* free up all the memory */
void SmoothFreeData(Smooth smooth)

{
    int i;
    HashNode headNode, nextNode;
    Poly3 *poly, *nextPoly;

#ifdef DEBUG
    fprintf(stderr, "SmoothFreeData(%p) entered\n", smooth);
#endif
    if (smooth == NULL)
        return;
    for (i = 0; i < HASH_TABLE_SIZE; i++) {
        headNode = smooth->hashTable[i];
        while (headNode != NULL) {
            nextNode = headNode->next;
            free(headNode);
            headNode = nextNode;
        }
        smooth->hashTable[i] = NULL;
    }
    for (poly = smooth->polygonTable; poly != NULL; poly = nextPoly) {
        nextPoly = poly->next;
        SmoothFreePoly(poly);
    }
    smooth->polygonTable = smooth->polyTail = NULL;
    smooth->fuzzFraction = smooth->fuzz = SMOOTH_TOLERANCE;
    smooth->edgeTest = FALSE;
#ifdef DEBUG
    fprintf(stderr, "SmoothFreeData() done\n");
#endif
}


/* free up all the memory */
void SmoothFreeAll(Smooth smooth)

{
    if (smooth == NULL)
        return;
    SmoothFreeData(smooth);
    free(smooth);
}


static void ComputeFuzz(Smooth smooth)
{
    int i;
    double od, d;
    Point3 min, max,*v;
    Poly3 *poly = smooth->polygonTable;

    min.x = max.x = poly->verts->x;
    min.y = max.y = poly->verts->y;
    min.z = max.z = poly->verts->z;
    while (poly != NULL) {
        v = poly->verts;
        for (i = 0; i < (int)poly->nverts; i++) {
            if (v->x < min.x)
                min.x = v->x;
            if (v->y < min.y)
                min.y = v->y;
            if (v->z < min.z)
                min.z = v->z;
            if (v->x > max.x)
                max.x = v->x;
            if (v->y > max.y)
                max.y = v->y;
            if (v->z > max.z)
                max.z = v->z;
            v++;
        }
        poly = poly->next;
    }
    d = fabs(max.x - min.x);
    if ((od = fabs(max.y - min.y)) > d)
        d = od;
    if ((od = fabs(max.z - min.z)) > d)
        d = od;
    smooth->fuzz = od * smooth->fuzzFraction;
}


/* get first node in this list that isn't marked as done
 */
static HashNode GetFirstWaitingNode(HashNode node)
{
    while (node != NULL) {
        if (node->marked != MARKDONE)
            return node;
        node = node->next;
    }
    return NULL;
}


/* are these two verts the same to with the tolerance? */
static int CompareVerts(Point3 * v0, Point3 * v1, Smooth smooth)
{
    double q0, q1;

    q0 = QUANT(v0->x);
    q1 = QUANT(v1->x);
    if (!FUZZEQ(q0, q1))
        return FALSE;
    q0 = QUANT(v0->y);
    q1 = QUANT(v1->y);
    if (!FUZZEQ(q0, q1))
        return FALSE;
    q0 = QUANT(v0->z);
    q1 = QUANT(v1->z);
    if (!FUZZEQ(q0, q1))
        return FALSE;
    return TRUE;
}


/* compute the normal for an unmarked vertex */
static void ProcessHashNode(HashNode firstNode, Smooth smooth)
{
    HashNode scanNode = firstNode->next;
    Point3 *firstVert = &firstNode->polygon->verts[firstNode->vertexNum];
    Point3 *headNorm = &firstNode->polygon->normal;
    Point3 *testVert, *testNorm, normal;

    firstNode->marked = MARKWORKING;
    normal.x = firstNode->polygon->normal.x;
    normal.y = firstNode->polygon->normal.y;
    normal.z = firstNode->polygon->normal.z;
    while (scanNode != NULL) {
        testVert = &scanNode->polygon->verts[scanNode->vertexNum];
        if (CompareVerts(testVert, firstVert, smooth)) {
            testNorm = &scanNode->polygon->normal;
            if ((smooth->edgeTest == FALSE) ||
                    (V3Dot(testNorm, headNorm) > smooth->minDot)) {
                (void)V3Add(&normal, testNorm, &normal);
                scanNode->marked = MARKWORKING;
            }
        }
        scanNode = scanNode->next;
    }
    (void)V3Normalize(&normal);
    scanNode = firstNode;
    while (scanNode != NULL) {
        if (scanNode->marked == MARKWORKING) {
            testNorm = &scanNode->polygon->normals[scanNode->vertexNum];
            testNorm->x = normal.x;
            testNorm->y = normal.y;
            testNorm->z = normal.z;
            scanNode->marked = MARKDONE;
            testVert = &scanNode->polygon->verts[scanNode->vertexNum];
        }
        scanNode = scanNode->next;
    }
}


/*  scan each list at each hash table entry until all nodes are marked
 */
void SmoothMakeVertexNormals(Smooth smooth)

{
    int i;
    HashNode currentHashNode, firstNode;

#ifdef DEBUG
    fprintf(stderr, "SmoothMakeVertexNormals(%p) entered\n", smooth);
#endif
    ComputeFuzz(smooth);
    for (i = 0; i < HASH_TABLE_SIZE; i++) {
        currentHashNode = smooth->hashTable[i];
        do {
            if ((firstNode = GetFirstWaitingNode(currentHashNode)) != NULL)
                ProcessHashNode(firstNode, smooth);
        } while (firstNode != NULL);
    }
#ifdef DEBUG
    fprintf(stderr, "SmoothMakeVertexNormals() done.\n");
#endif
}


#define CALNAME  "tmesh.cal" /* the name of our auxiliary file */
#define TEXNAME  "T-nor"     /* triangle texture name (reused) */
#define VOIDID   "void"      /* this is defined in object.h */
#define FTINY    1e-6


/* compute barycentric vectors
 */
static int CompBaryc(register BARYCCM bcm,
					 Point3 *v1, Point3 *v2, Point3 *v3)

{
    int i;
    double d;
    Point3 va, vab, vcb, *vt;

    for (i = 0; i < 3; i++) {
        vab.x = v1->x - v2->x;
        vcb.x = v3->x - v2->x;
        vab.y = v1->y - v2->y;
        vcb.y = v3->y - v2->y;
        vab.z = v1->z - v2->z;
        vcb.z = v3->z - v2->z;
        d = V3Dot(&vcb, &vcb);
        if (d <= FTINY)
            return 0;
        d = V3Dot(&vcb, &vab) / d;
        va.x = vab.x - vcb.x * d;
        va.y = vab.y - vcb.y * d;
        va.z = vab.z - vcb.z * d;
        d = V3Dot(&va, &va);
        if (d <= FTINY)
            return(-1);
        va.x /= d;
        bcm[i][0] = va.x;
        va.y /= d;
        bcm[i][1] = va.y;
        va.z /= d;
        bcm[i][2] = va.z;
        bcm[i][3] = -V3Dot(v2, &va);
        /* rotate vertices */
        vt = v1;
        v1 = v2;
        v2 = v3;
        v3 = vt;
    }
    return 1;
}


/*  Write out a barycentric triangle.
 */
void WriteBaryTriangle(FILE *fp, char *matName, int num, int cnt, Poly3 *poly)

{
    int i;
    BARYCCM bvecs;

    /* compute barycentric coordinates */
    if (!CompBaryc(bvecs, &poly->verts[0], &poly->verts[1],
             &poly->verts[2]))
        return;
    /* put out texture (if any) */
    fprintf(fp, "\n%s texfunc %s\n", matName, TEXNAME);
    fprintf(fp, "4 dx dy dz %s\n", CALNAME);
    fprintf(fp, "0\n21\n");
    for (i = 0; i < 3; i++)
        fprintf(fp, "%14.8f %14.8f %14.8f %14.8f\n",
                bvecs[i][0], bvecs[i][1], bvecs[i][2], bvecs[i][3]);
    fprintf(fp, "\t%14.12g %14.12g %14.12g\n", poly->normals[0].x,
            poly->normals[1].x, poly->normals[2].x);
    fprintf(fp, "\t%14.12g %14.12g %14.12g\n", poly->normals[0].y,
            poly->normals[1].y, poly->normals[2].y);
    fprintf(fp, "\t%14.12g %14.12g %14.12g\n", poly->normals[0].z,
            poly->normals[1].z, poly->normals[2].z);
    /* put out triangle */
    fprintf(fp, "\n%s polygon %s.%d.%d\n", TEXNAME, matName, num, cnt);
    fprintf(fp, "0\n0\n9\n");
    fprintf(fp, "%18.12g %18.12g %18.12g\n", poly->verts[0].x,
            poly->verts[0].y, poly->verts[0].z);
    fprintf(fp, "%18.12g %18.12g %18.12g\n", poly->verts[1].x,
            poly->verts[1].y, poly->verts[1].z);
    fprintf(fp, "%18.12g %18.12g %18.12g\n", poly->verts[2].x,
            poly->verts[2].y, poly->verts[2].z);
}


#if 0
/* put out a quadrilateral
*/
PutQuad(poly);
{
    Matrix4 norm;
    Vector3 v1, v2, vc1, vc2;
    int axis, ok1, ok2;

    /* compute exact normals */
    fvsum(v1, poly->verts[1], poly->verts[0], -1.0);
    fvsum(v2, poly->verts[2], poly->verts[0], -1.0);
    fcross(vc1, v1, v2);
    ok1 = normalize(vc1) != 0.0;
    fvsum(v1, poly->verts[2], poly->verts[3], -1.0);
    fvsum(v2, poly->verts[1], poly->verts[3], -1.0);
    fcross(vc2, v1, v2);
    ok2 = normalize(vc2) != 0.0;
    if (!(ok1 | ok2))
        return 0;

    /* compute normal interpolation */
    axis = NormInterp(norm, p0i, p1i, p2i, p3i);

    /* put out quadrilateral? */
    if (ok1 & ok2 && fabs(fdot(vc1, vc2)) >= 1.0 - FTINY) {
        printf("\n%s ", mod);
        if (axis != -1) {
            printf("texfunc %s\n", TEXNAME);
            printf("4 surf_dx surf_dy surf_dz %s\n", QCALNAME);
            printf("0\n13\t%d\n", axis);
            pvect(norm[0]);
            pvect(norm[1]);
            pvect(norm[2]);
            fvsum(v1, norm[3], vc1, -0.5);
            fvsum(v1, v1, vc2, -0.5);
            pvect(v1);
            printf("\n%s ", TEXNAME);
        }
        printf("polygon %s.%d\n", name, faceno);
        printf("0\n0\n12\n");
        pvect(vlist[p0i[0]]);
        pvect(vlist[p1i[0]]);
        pvect(vlist[p3i[0]]);
        pvect(vlist[p2i[0]]);
        return (1);
    }
    /* put out triangles? */
    if (ok1) {
        printf("\n%s ", mod);
        if (axis != -1) {
            printf("texfunc %s\n", TEXNAME);
            printf("4 surf_dx surf_dy surf_dz %s\n", QCALNAME);
            printf("0\n13\t%d\n", axis);
            pvect(norm[0]);
            pvect(norm[1]);
            pvect(norm[2]);
            fvsum(v1, norm[3], vc1, -1.0);
            pvect(v1);
            printf("\n%s ", TEXNAME);
        }
        printf("polygon %s.%da\n", name, faceno);
        printf("0\n0\n9\n");
        pvect(vlist[p0i[0]]);
        pvect(vlist[p1i[0]]);
        pvect(vlist[p2i[0]]);
    }
    if (ok2) {
        printf("\n%s ", mod);
        if (axis != -1) {
            printf("texfunc %s\n", TEXNAME);
            printf("4 surf_dx surf_dy surf_dz %s\n", QCALNAME);
            printf("0\n13\t%d\n", axis);
            pvect(norm[0]);
            pvect(norm[1]);
            pvect(norm[2]);
            fvsum(v2, norm[3], vc2, -1.0);
            pvect(v2);
            printf("\n%s ", TEXNAME);
        }
        printf("polygon %s.%db\n", name, faceno);
        printf("0\n0\n9\n");
        pvect(vlist[p2i[0]]);
        pvect(vlist[p1i[0]]);
        pvect(vlist[p3i[0]]);
    }
    return (1);
}


#define u  ((ax+1)%3)
#define v  ((ax+2)%3)

/* compute normal interpolation
*/
static int NormInterp(Poly3 poly, Matrix4 resmat);

{
    int i, j, int ax;
    unsigned u, v;
    Matrix4 eqnmat;
    Vector3 v1;

    /* find dominant axis */
    V3Copy(&poly->normals[0], &v1);
    V3Add(&v1, poly->normals[1], &v1);
    V3Add(&v1, poly->normals[2], &v1);
    V3Add(&v1, poly->normals[3], &v1);
    ax = ABS(v1.x) > ABS(v1.y) ? &v1.x - &v1 : &v1.y - &v1;
    ax = ABS(*((double*)(&v1+ ax))) > ABS(v1.z) ? ax : &v1.z - &v1;
    if (ax == 1)
        u = &v1.y - &v1;
    if ((ax == 0) || (ax == 2))
        u = &v1.x - &v1;
    /* assign equation matrix */
    eqnmat[0][0] = vlist[p0i[0]][u] * vlist[p0i[0]][v];
    eqnmat[0][1] = vlist[p0i[0]][u];
    eqnmat[0][2] = vlist[p0i[0]][v];
    eqnmat[1][0] = vlist[p1i[0]][u] * vlist[p1i[0]][v];
    eqnmat[1][1] = vlist[p1i[0]][u];
    eqnmat[1][2] = vlist[p1i[0]][v];
    eqnmat[2][0] = vlist[p2i[0]][u] * vlist[p2i[0]][v];
    eqnmat[2][1] = vlist[p2i[0]][u];
    eqnmat[2][2] = vlist[p2i[0]][v];
    eqnmat[3][0] = vlist[p3i[0]][u] * vlist[p3i[0]][v];
    eqnmat[3][1] = vlist[p3i[0]][u];
    eqnmat[3][2] = vlist[p3i[0]][v];
    eqnmat[0][3] = eqnmat[1][3] = eqnmat[2][3] = eqnmat[3][3] = 1.0;
    /* invert matrix (solve system) */
    if (!invmat4(eqnmat, eqnmat))
        return 0;         /* no solution */
    /* compute result matrix */
    for (j = 0; j < 4; j++)
        for (i = 0; i < 3; i++)
            resmat[j][i] = eqnmat[j][0] * vnlist[p0i[2]][i] +
                    eqnmat[j][1] * vnlist[p1i[2]][i] +
                    eqnmat[j][2] * vnlist[p2i[2]][i] +
                    eqnmat[j][3] * vnlist[p3i[2]][i];
    return (ax);
}
#undef u
#undef v

#endif /* 0 */
/*** smooth.c ***/
