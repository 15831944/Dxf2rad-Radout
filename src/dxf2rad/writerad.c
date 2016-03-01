/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.


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
#ifdef __cplusplus
    extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "geomtypes.h"
#include "geomdefs.h"
#include "dlltypes.h"
#include "dllproto.h"
#include "geomproto.h"


extern void 
WriteSimpleText(FILE *fp, SimpleText *texts)
{
	return; /* dummy */
}


extern int WriteCyl(FILE *fp, char *matName,
					int id, Cyl3 *cyls)
{
    int cylCnt = 0;
    Cyl3 *cyl;
    Vector3 dir;
	char *material = matName;

#ifdef DEBUG
    fprintf(stderr, "WriteCyl(%p, %p)\n", contblks, cyls);
#endif
    if (cyls == NULL)
        return 1;
    for (cyl = cyls; cyl; cyl = cyl->next) {
		if(matName == NULL) material = cyl->material;
		if(cyl->erad == 0.0) { /* it's a point/sphere */
			int sign;
			sign = (cyl->srad > 0 ? 1 : -1);
			if (sign > 0) {
				fprintf(fp, "\n%s sphere %s.%d.%d\n",
					material, material, id, cylCnt++);
			} else {
				fprintf(fp, "\n%s bubble %s.%d.%d\n",
					material, material, id, cylCnt++);
			}
			fprintf(fp, "0\n0\n4");
			fprintf(fp, "\t%.8g\t%.8g\t%.8g\t%.8g\n",
				cyl->svert.x, cyl->svert.y, cyl->svert.z,
				fabs(cyl->srad));
		} else if (cyl->length != 0.0) { /* it's a cylinder or tube */
			if(cyl->length >= 0.0) {
				fprintf(fp, "\n%s cylinder %s.%d.%d\n",
					material, material, id, cylCnt++);
			} else {
				fprintf(fp, "\n%s tube %s.%d.%d\n",
					material, material, id, cylCnt++);
			}
            fprintf(fp, "0\n0\n7");
            fprintf(fp, "\t%.8g %.8g %.8g\n", cyl->svert.x,
                    cyl->svert.y, cyl->svert.z);
            fprintf(fp, "\t%.8g %.8g %.8g\n", cyl->evert.x,
                    cyl->evert.y, cyl->evert.z);
            fprintf(fp, "\t%.8g\n", cyl->srad);
            /* bottom cap */
            fprintf(fp, "\n%s ring %s.%d.%d\n",
				material, material, id, cylCnt++);
            fprintf(fp, "0\n0\n8");
            fprintf(fp, "\t%.8g %.8g %.8g\n",
				cyl->svert.x, cyl->svert.y, cyl->svert.z);
			if(cyl->length >= 0.0) {
	            (void)V3Normalize(V3Sub(&cyl->svert, &cyl->evert, &dir));
			} else {
	            (void)V3Normalize(V3Sub(&cyl->evert, &cyl->svert, &dir));
			}
            fprintf(fp, "\t%.8g %.8g %.8g\n", dir.x, dir.y, dir.z);
            fprintf(fp, "\t0 %.8g\n", cyl->srad);
			/* top cap */
            fprintf(fp, "\n%s ring %s.%d.%d\n",
				material, material, id, cylCnt++);
            fprintf(fp, "0\n0\n8");
            fprintf(fp, "\t%.8g %.8g %.8g\n",
				cyl->evert.x, cyl->evert.y, cyl->evert.z);
			if(cyl->length >= 0.0) {
	            (void)V3Normalize(V3Sub(&cyl->evert, &cyl->svert, &dir));
			} else {
	            (void)V3Normalize(V3Sub(&cyl->svert, &cyl->evert, &dir));
			}
            fprintf(fp, "\t%.8g %.8g %.8g\n", dir.x, dir.y, dir.z);
            fprintf(fp, "\t0 %.8g\n", cyl->srad);
        } else {            /* it's a ring */
            fprintf(fp, "\n%s ring %s.%d.%d\n",
				material, material, id, cylCnt++);
            fprintf(fp, "0\n0\n8");
            fprintf(fp, "\t%.8g %.8g %.8g\n",
				cyl->svert.x, cyl->svert.y, cyl->svert.z);
            (void)V3Normalize(&cyl->normal);
            fprintf(fp, "\t%.8g %.8g %.8g\n",
					cyl->normal.x, cyl->normal.y, cyl->normal.z);

            fprintf(fp, "\t0 %.8g\n", cyl->srad);
        }
    }
	Cyl3FreeList(cyls);
    return 1;
}


/* ARGSUSED */
extern int WritePoint(FILE *fp, char *matName,
					  int id, Cyl3 *point)
{
    int pntCnt = 0, sign;
	char *material = matName;

	if(matName == NULL) material = point->material;
    if (point == NULL)
		return 1;
    if (point->srad == 0.0)
        return 0;
	sign = (point->srad > 0 ? 1 : -1);
    if (sign > 0)
        fprintf(fp, "\n%s sphere %s.%d.%d\n", material, material, id, pntCnt);
    else
        fprintf(fp, "\n%s bubble %s.%d.%d\n", material, material, id, pntCnt);
    fprintf(fp, "0\n0\n4");
    fprintf(fp, "\t%.8g\t%.8g\t%.8g\t%.8g\n",
		point->svert.x, point->svert.y, point->svert.z,
		fabs(point->srad));
    return 1;
}


extern int WritePoly(FILE *fp, char *matName, int id, Poly3 *polys)
{
    int i, polyCnt = 0;
    Poly3 *poly;
	char *material = matName;

    if (polys == NULL)
        return 1;
#ifdef SMOOTHING
    if (options.smooth) {
        SmoothSetPolyList(polys, options.smooth);
        SmoothMakeVertexNormals(options.smooth);
        /* options.smooth->polygonTable = NULL; */
        for (poly = options.smooth->polygonTable; poly; poly = poly->next)
			if(matName == NULL) material = cyl->material;
            WriteBaryTriangle(fp, material, id, polyCnt++, poly);
        SmoothFreeData(options.smooth);
        /* Poly3FreeList(polys); */
        return 1;
    }
#endif
    for (poly = polys; poly; poly = poly->next) {
		if(matName == NULL) material = poly->material;
        if (poly->nverts < 3)
            continue;
        fprintf(fp, "\n%s polygon %s.%d.%d\n", material, material, id,
                ++polyCnt);
        fprintf(fp, "0\n0\n%d", poly->nverts * 3);
        for (i = 0; i < (int)poly->nverts; i++)
            fprintf(fp, "\t%.8g\t%.8g\t%.8g\n", poly->verts[i].x,
                    poly->verts[i].y, poly->verts[i].z);
    }
    Poly3FreeList(polys);
    return 1;
}
#ifdef __cplusplus
    }
#endif
