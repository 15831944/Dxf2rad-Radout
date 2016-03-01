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

/* radout.c - Output routines for entities into Radiance format
 */


#include <sys/types.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#ifdef UNIX
	#include <unistd.h>
	#include <pwd.h>
	#include <sys/time.h>
#endif


#include "cadcompat.h"

#include "adtools.h"
#include "resbuf.h"
#include "geomtypes.h"
#include "geomdefs.h"
#include "radout.h"
#include "writerad.h"
#include "dlltypes.h"
#include "dllproto.h"
#include "geomproto.h"
#include "cproto.h"


extern void acadrgb(int, RGB*);

extern Options options;
extern DLL_LIST exportColorLists[MAXCOLORS], exportLayerLists;

/*  Evaluate the transformation out of a hierarchy of blocks.
 *  Warn if a multiple insert is encountered and skip it.
 */
static int GetMcsToWcs(DLL_LIST contblks, Matrix4 matrix)

{
    ads_namep namep;
    int nrow = 0, ncol = 0;
    Matrix4 nxform;
    ADSResBuf *edata = NULL;

    if (contblks == NULL)
        return 0;
    if ((namep = DllPopF(contblks, NULL)) == NULL)
        return 0;
#ifdef DEBUG
    fprintf(stderr, "GetMcsToWcs: namep: %lx\n", *namep);
    PrintEntityHeader(stderr, namep);
#endif
    edata = ads_entget(namep);
    GetEntXForm(edata, matrix);
    ads_relrb(edata);

    free(namep);
    while (DllGetListSize(contblks) > 0) {
        namep = DllPopF(contblks, NULL);
        edata = ads_entget(namep);
        free(namep);
        GetEntXForm(edata, nxform);
        M4MatMult(nxform, matrix, matrix);
        if ((RBGetInt(&nrow, edata, 71) && (nrow > 0)) ||
                (RBGetInt(&ncol, edata, 71) && (ncol > 0))) {
            ads_printf("minsert of block %s ignored\n",
                    RBSearch(edata, 2)->resval.rstring);
        }
        ads_relrb(edata);
    }
#ifdef DEBUG
    M4Print(stderr, matrix);
#endif
    return 1;
}


/*  Transform a list of polys from model-cs to wcs.
 *  Contele is a hierarchical list of container block names
 *  or the name of a single wcs entity.
 */
Poly3 *TransformPolyToWCS(Poly3 *polys, DLL_LIST contblks)
{
    Matrix4 matrix;

#ifdef DEBUG
    fprintf(stderr, "TransformPolyToWCS(%p, %p)\n", polys, contblks);
#endif
    if (contblks && (DllGetListSize(contblks) > 0))
        GetMcsToWcs(contblks, matrix);
    else
        M4SetIdentity(matrix);
    if ((options.scaleFactor != 0.0) && (options.scaleFactor != 1.0))
        M4Scale(matrix, options.scaleFactor, options.scaleFactor,
                options.scaleFactor);
    /* now transform to wcs */
	M4TransformPolys(polys, matrix);
	return polys;
}


/*  Transform a list of cylinders/points from model-cs to wcs.
 */
Cyl3 *TransformCylToWCS(Cyl3 *cyls, DLL_LIST contblks)
{
	Matrix4 matrix;

    if (contblks && (DllGetListSize(contblks) > 0))
        GetMcsToWcs(contblks, matrix);
    else
        M4SetIdentity(matrix);
    if ((options.scaleFactor != 0.0) && (options.scaleFactor != 1.0))
        M4Scale(matrix, options.scaleFactor, options.scaleFactor,
                options.scaleFactor);
	M4TransformCyls(cyls, matrix);
	return cyls;
}



Cyl3 *PointToCyl(ADSResBuf *edata)

{
    Cyl3 *cyl;
	double pdsize = 0.0;

    (void)RBGetDouble(&pdsize, edata, 39);
	if(pdsize == 0.0) {
		GetDoubleVar(&pdsize, "PDSIZE");
	}
	if(pdsize == 0.0) {
		return NULL;
	}
	cyl = Cyl3Alloc(NULL);
    if (cyl == NULL) return NULL;
    if (RBGetPoint3(&cyl->svert, edata, 10) == NULL) {
        WarnMsg("PointToCyl: can't get center point");
        free(cyl);
        return NULL;
    }
	cyl->srad = pdsize / (2.0 * (options.scaleFactor ? options.scaleFactor : 1.0));
    return cyl;
}


Cyl3 *CircleToCyl(ADSResBuf *edata)
{
	Matrix4 mx;
    Cyl3 *cyl;
    double thick = 0.0;

#ifdef DEBUG
    fprintf(stderr, "CircleToCyl(%p)\n", edata);
#endif
	cyl = Cyl3Alloc(NULL);
    if (cyl == NULL) return NULL;
    if (RBGetPoint3(&cyl->svert, edata, 10) == NULL) {
        WarnMsg("CircleToCyl: can't get center point");
        free(cyl);
        return NULL;
    }
    if (RBGetDouble(&cyl->srad, edata, 40) == NULL) {
        WarnMsg("CircleToCyl: can't get radius");
        free(cyl);
        return NULL;
    }
    (void)RBGetDouble(&thick, edata, 39);
    (void)RBGetPoint3(&(cyl->normal), edata, 210);

	/* a disk or cylinder always has an ending radius (a sphere doesn't) */
    cyl->erad = cyl->srad;
    cyl->evert.x = cyl->svert.x;
    cyl->evert.y = cyl->svert.y;
    cyl->evert.z = cyl->svert.z + thick;
	cyl->length = thick;

	/* transform points from ECS to next higher level CS */
	M4GetAcadXForm(mx, &(cyl->normal), 0, NULL, 0.0, 1.0, 1.0, 1.0, NULL);
	M4TransformCyls(cyl, mx);

    return cyl;
}


int WriteMatsAndRif(Options *opts)
{
    int i;
    time_t now; 
	char * str[100], *layer;
    char *basename, lay[64], matFName[80], rifName[80];
    FILE *fp;
    RGB color;
    ADSResBuf *l_rb;

    now = time(NULL);
    basename = StripPrefix(opts->basename);
    if (opts->makeMats) {
        strcat(strcpy(matFName, opts->basename), "_mat.rad");
        if ((fp = fopen(matFName, "w")) == NULL) {
            WarnSys("WriteMatsAndRif: can't open file: %s\n", matFName);
            return 0;
        }
        ads_printf("Writing file: %s\n", matFName);
        fprintf(fp, "## Radiance default Autocad materials for model: %s\n",
                basename);
        fprintf(fp, "## Created: %s", ctime(&now));

        if (opts->exportMode == EXP_BYCOLOR) {
            for (i = 1; i < MAXCOLORS; i++) {
                if (DllGetListSize(exportColorLists[i]) <= 0)
                    continue;
                sprintf(lay, "%s%d", options.colorprefix, i);
                RegulateName(lay);
                acadrgb(i, &color);    /*  original colext used */
                fprintf(fp, "\nvoid plastic %s%s\n", basename, lay);
                fprintf(fp, "0\n0\n");
                fprintf(fp, "5 %.8g %.8g %.8g 0 0\n", color.red,
                        color.grn, color.blu);
            }
        } else {
            for (layer = DllFirst(exportLayerLists, NULL); layer!= NULL;
                    layer = DllNext(exportLayerLists, NULL)) {
                sprintf(lay, "%s%s", options.layerprefix, layer);
                RegulateName(lay);
                if ((l_rb = ads_tblsearch("LAYER", layer, FALSE)) != NULL) {
                    if (RBGetInt(&i, l_rb, 62) == NULL)
                        i = WHITE;
                    ads_relrb(l_rb);
                } else
                    i = WHITE;
                acadrgb(i, &color);    /*  original colext used */
                fprintf(fp, "\nvoid plastic %s_%s\n", basename, lay);
                fprintf(fp, "0\n0\n");
                fprintf(fp, "5 %.8g %.8g %.8g 0 0\n", color.red,
                        color.grn, color.blu);
            }
        }
        (void)fclose(fp);
    }
    if (opts->makeRif) {
        strcat(strcpy(rifName, opts->basename), ".rif");
        if ((fp = fopen(rifName, "w")) == NULL) {
            WarnSys("WriteMatsAndRif: can't open file: %s\n", rifName);
            return 0;
        }
        ads_printf("Writing file: %s\n", rifName);
        fprintf(fp, "## The Radiance Input File (rif) for model: %s\n",
                basename);
        fprintf(fp, "## Created: %s", ctime(&now));
        strcat(strcpy(matFName, basename), "_mat.rad");
        if (opts->makeMats)
            fprintf(fp, "materials= %s\n", matFName);
        sprintf((char*)str, "scene=");
        if (opts->makeSun)
            sprintf((char*)str+strlen((char*)str), " %s_sun.rad", basename);
        if (opts->exportMode == EXP_BYCOLOR) {
            for (i = 1; i < MAXCOLORS; i++) {
                if (DllGetListSize(exportColorLists[i]) <= 0)
                    continue;
                sprintf(lay, "%s%d", options.colorprefix, i);
                RegulateName(lay);
                sprintf((char*)str+strlen((char*)str), " %s%s.rad", basename, lay);
                if ((int)strlen((char*)str) > 60) {
                    fputs((char*)str, fp);
                    fprintf(fp, "\n");
                    sprintf((char*)str, "scene=");
                }
            }
        } else {
            for (layer = DllFirst(exportLayerLists, NULL); layer!= NULL;
                    layer = DllNext(exportLayerLists, NULL)) {
                sprintf(lay, "%s%s", options.layerprefix, layer);
                RegulateName(lay);
                sprintf((char*)str+strlen((char*)str), " %s%s.rad", basename, lay);
                if ((int)strlen((char*)str) > 60) {
                    fputs((char*)str, fp);
                    fprintf(fp, "\n");
                    sprintf((char*)str, "scene=");
                }
            }
        }
        if ((int)strlen((char*)str) > (int)strlen("scene=")) {
            fputs((char*)str, fp);
            fprintf(fp, "\n");
        }
        fprintf(fp, "OCTREE= %s.oct\n", basename);
        fprintf(fp, "# ZONE= E low_x hi_x low_y hi_y low_z hi_z\n");
        fprintf(fp, "QUALITY= M\n");
        fprintf(fp, "UP= Z\n");
        fprintf(fp, "INDIRECT= 0\n");
        fprintf(fp, "# AMBFILE= %s.amb\n", basename);
        fprintf(fp, "# VARIABILITY= M\n");
        fprintf(fp, "REPORT= 10 %s.err\n", basename);
        (void)fclose(fp);
    }
    strFREE(basename);
    return 1;
}


int WriteDaylight(Options *opts)

{
    FILE *fp;
    char *basename, extName[80];

    if (!opts->makeSun)
        return 0;
    basename = StripPrefix(opts->basename);
    strcat(strcpy(extName, opts->basename), "_sun.rad");
    if ((fp = fopen(extName, "w")) == NULL) {
        WarnSys("WriteExterior: can't open file: %s\n", extName);
		strFREE(basename);
        return 0;
    }
    ads_printf("Writing file: %s\n", extName);

    fprintf(fp, "## The Sun, sky and ground light for model: %s\n", basename);
    fprintf(fp, "## Created by Radout.\n");
    fprintf(fp, "\n!gensky %d %d %.8g +i -a %.8g -o %.8g -m %d\n",
            (int)opts->sunlight[MONTH], (int)opts->sunlight[DAY],
            opts->sunlight[HOUR], opts->sunlight[LAT],
            opts->sunlight[LONG], (int)opts->sunlight[TZ]*15);
    fprintf(fp, "\nskyfunc glow sky_glow\n0\n0\n4 .8 .8 1.2 0\n");
    fprintf(fp, "\nsky_glow source sky\n0\n0\n4 0 0 1 180\n");
    fprintf(fp, "\nskyfunc glow ground_glow\n0\n0\n4 1 .7 .25 0\n");
    fprintf(fp, "\nground_glow source ground\n0\n0\n4 0 0 -1 180\n");
    (void)fclose(fp);
    strFREE(basename);
    return 1;
}


/*** end radout.c ***/
