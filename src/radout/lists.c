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

/*  lists.c - code for sorting and exploding entities into the various
 *  lists we use.
 */


#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
  #include <stdlib.h>
#endif /* _WIN32 */

#include "cadcompat.h"


#include "adtools.h"
#include "geomtypes.h"
#include "geomdefs.h"
#include "resbuf.h"
#include "radout.h"
#include "dlltypes.h"
#include "dllproto.h"
#include "geomproto.h"
#include "cproto.h"

extern Options options;

DLL_LIST exportBlockList, exportColorLists[MAXCOLORS], exportLayerLists;
DLL_SETUP entListsSetup = NULL, layerListsSetup = NULL;

/*  fill layer list with all visible layers (not frozen or off) */
void VisLayList ()
{
    DLL_LIST list;
    char *lay;
	ADSResBuf *rb;
	int i62, i70;

	for (rb = ads_tblnext("LAYER", TRUE); rb; rb = ads_tblnext("LAYER", FALSE)) {
		(void)RBGetInt(&i62, rb, 62);
		(void)RBGetInt(&i70, rb, 70);
		if ((0 < i62) && (0 == (1 & i70))) {

			lay = RBGetNewStr(rb, 2);
			if ((list = DllNewList(entListsSetup)) == NULL) {
				WarnMsg("VisLayList: Can't make entity list for layer %s", lay);
				free(lay);
				continue;
			}
			DllInsert(exportLayerLists, lay, list);
		}
		ads_relrb(rb);
	}
}


int MakeExportLists(ExportMode mode)

{
    int i;

	if (!(entListsSetup = DllSetupList(NULL, FreeListProc, NULL, NULL)))
		return 0;
    if ((exportBlockList = DllNewList(entListsSetup)) == NULL) {
        WarnMsg("MakeExportLists: can't make export block list");
        return 0;
    }
    if (mode == EXP_BYCOLOR) {      /* init array of color lists */
        for (i = 1; i < MAXCOLORS; i++) {
            if (!(exportColorLists[i] = DllNewList(entListsSetup))) {
                WarnMsg("MakeExportLists: can't make export colors list");
                return 0;
            }
        }
        return 1;
    } else {    /* init list of layer lists */
        layerListsSetup = DllSetupList(CompStrKey, FreeStrProc,
                FreeListProc, NULL);
        if ((exportLayerLists = DllNewList(layerListsSetup)) == NULL) {
            WarnMsg("MakeExportLists: can't make export layers list");
            return 0;
        }
		VisLayList();
    }
    return 1;
}


/*  Add a hierarchical block list to the export blocks list
 */
static int AddToBlockList(DLL_LIST blist)

{
    if (!DllPushR(exportBlockList, blist, NULL)) {
        ads_fail("AddToBlockList: failed");
        return 0;
    }
    return 1;
}


/* Add an entity to the appropriate color list for final export */
static int AddEntToColorLists(DLL_LIST blist, int color)

{
    if ((color < 1) || (color >= MAXCOLORS)) {
        WarnMsg("AddEntToColorLists: color out of range: %d\n", color);
        return 0;
    }
    if (!DllPushR(exportColorLists[color], blist, NULL)) {
        WarnMsg("AddEntToColorLists: error %p   color %d\n",
                blist, color);
        return 0;
    }
    return 1;
}


/* Add an entity to the appropriate layer list for final export */
int AddEntToLayerLists(DLL_LIST blist, char *layer)

{
    DLL_LIST list;
	/*  do not add the entity, if layer is not in list           */
    if (DllSearch(exportLayerLists, layer, &list) != NULL) {
		(void)DllPushR(list, blist, NULL);
    }
    return 1;
}

/* Checks to see if the type of entity is to be output
 */
static int IsValidEntType(char *type)

{
    if (BinarySearch(type, 0, options.entTypesCount - 1,
                    options.entTypes) >= 0)
        return TRUE;
    return False;
}


/* Get our own subtypes for the polyline type */
char *GetEntType(ADSResBuf *rb, char *type)

{
    int flag;
    ADSResBuf *ri;
	double thick = 0.0, width = 0.0;

    ri = RBSearch(rb, 0);
	if (    (strcmp(ri->resval.rstring, "LINE") == 0)
		 || (strcmp(ri->resval.rstring, "ARC")  == 0) ) {
		/*  another fix: Lines and arcs without thickness will
			be left out. This should avoid most empty output
		    files now. What we don't catch will be layers
		    or colors with only degenerate entities on them
		    like 3dfaces with zero area. If users create
		    such stuff, let them live with the empty files ...
		*/
		(void)RBGetDouble(&thick, rb, 39);
		if(thick != 0.0) strcpy(type, ri->resval.rstring);
		else strcpy(type, "");
	}
    else if (strcmp(ri->resval.rstring, "POLYLINE") == 0) {
        if ((RBGetInt(&flag, rb, 70)) == NULL) {
            ads_printf("GetEntType: search for flag failed.\n");
            return NULL;
        }
        if (flag & 64)
            strcpy(type, "PFACE");
        else if (flag & 16)
            strcpy(type, "PMESH");
        else if (flag & 8)
            strcpy(type, "3DPOLY");
        else if ((ri = RBSearch(rb, 40)) && (ri->resval.rreal > 0.0))
            strcpy(type, "WPLINE");
        else if ((flag & 1) && ((flag & 8) == 0)) {
			/*  if we don't sample polygons and it has thickness, pass it as
				pline, else as "".
				We duplicate IsValidEntType()s job here, but we just hope that
				WriteEntity() will never see the result...
			    This operation will help us to avoid empty output files.
			*/
			if (IsValidEntType("POLYGON"))
				strcpy(type, "POLYGON");
			else {
				(void)RBGetDouble(&thick, rb, 39);
				if(thick != 0.0) strcpy(type, "PLINE");
				else strcpy(type, "");
			}
		}
        else
            strcpy(type, "PLINE");
    }
	/*  lwpolyline is similar, but simpler (as the name suggests) */
    else if (strcmp(ri->resval.rstring, "LWPOLYLINE") == 0) {

		/* width is either overall width or just the first 
		opening width we find */
		if ((ri = RBSearch(rb, 43)) != NULL) {
			width = ri->resval.rreal;
		} else if ((ri = RBSearch(rb, 40)) != NULL) {
			width = ri->resval.rreal;
		} else width = 0.0;

        if ((RBGetInt(&flag, rb, 70)) == NULL) {
            ads_printf("GetEntType: search for flag failed.\n");
            return NULL;
        } else if (width > 0) strcpy(type, "WPLINE");
        else if (flag & 1) {
			if (IsValidEntType("POLYGON"))
				strcpy(type, "POLYGON");
			else {
					(void)RBGetDouble(&thick, rb, 39);
					if(thick != 0.0) strcpy(type, "PLINE");
					else strcpy(type, "");
			}
		}
        else
            strcpy(type, "PLINE");
	}
	else {
		strcpy(type, ri->resval.rstring);
	}
    return type;
}



/* Extract entities out of a selection set and add them to the
 * appropriate list.
 */
int SampleEnts(ads_name selset, ExportMode  expMode)

{
    char layer[64], type[16];
    long int num, numtot;
    ads_name ename;
    ADSResBuf *edata;
    DLL_LIST list;

    if (ads_sslength(selset, &numtot) != RTNORM)
        return 0;
    for (num = 0; num < numtot; num++) {
        if (num % 20 == 0)
            ads_printf("Sampling entities: %ld/%ld\r", num, numtot);
        if (ads_ssname(selset, num, ename) != RTNORM)
            return 0;
        if ((edata = ads_entget(ename)) == NULL) {
            WarnMsg("SampleEnts: can't get ent: %lx", ename);
            return 0;
        }
        if (strcmp(RBSearch(edata, 0)->resval.rstring, "INSERT") == 0) {
            if ((list = DllNewList(entListsSetup)) == NULL) {
                WarnMsg("SampleEnts: can't make block entity list");
                return 0;
            }
            (void)DllPushF(list, NewADSName(ename), NULL);
            AddToBlockList(list);
            ads_relrb(edata);
            continue;
        }
        if (!IsValidEntType(GetEntType(edata, type))) {
            ads_relrb(edata);
            continue;
        }
        if ((list = DllNewList(entListsSetup)) == NULL) {
            WarnMsg("SampleEnts: can't make entity list");
            ads_relrb(edata);
            return 0;
        }
        (void)DllPushF(list, NewADSName(ename), NULL);
        switch (expMode) {
			case EXP_BYCOLOR:
				if (!AddEntToColorLists(list, GetColorVal(edata, NULL, 0)))
					return 0;
				break;
			case EXP_BYLAYER:
				if (!GetLayerVal(layer, edata, list, 0))
					return 0;
				if (!AddEntToLayerLists(list, layer))
					return 0;
				break;
			default:
				ads_fail("SampleEnts: unknown export mode.");
        }
        ads_relrb(edata);
    }
	ads_printf("Sampling entities: %ld/%ld\n", num, numtot);
    return RTNORM;
}


/* Get the entities out of a block and add them to the appropriate
 * lists: if a block, back to blockList - if an entity onto the
 * the entity list according to samplemode.
 */
static int SampleBlock(DLL_LIST blist)

{
    ADSResBuf *ri, *rb1, *rb2, *edata;
    char layer[64], type[16];
    ads_name ename;
    ads_namep blent;
    DLL_LIST newList = NULL;

#ifdef DEBUG
    fprintf(stderr, "SampleBlock(%p)\n", blist);
#endif
    blent = DllFirst(blist, NULL);
    rb1 = ads_entget(blent);
    rb2 = ads_tblsearch("BLOCK", RBSearch(rb1, 2)->resval.rstring, 1);
    ads_relrb(rb1);
    ads_name_set(RBSearch(rb2, -2)->resval.rlname, ename);
    ads_relrb(rb2);
    do {
        edata = ads_entget(ename);
        ri = RBSearch(edata, 0);
        if (strcmp(ri->resval.rstring, "INSERT") == 0) {
            if (!(newList = DllCopyList(blist, entListsSetup,
                    CopyNameProc, NULL, NULL)))
                return 0;
            (void)DllPushF(newList, NewADSName(ename), NULL);
            AddToBlockList(newList);
            ads_relrb(edata);
            continue;
        }
        if (strcmp(ri->resval.rstring, "ENDBLK") == 0) {
            ads_relrb(edata);
            break;
        }
        if (!IsValidEntType(GetEntType(edata, type))) {
            ads_relrb(edata);
            continue;
        }
        if (!(newList = DllCopyList(blist, entListsSetup, CopyNameProc,
                NULL, NULL)))
            return 0;
        if (!DllPushF(newList, NewADSName(ename), NULL))
            return 0;
        switch (options.exportMode) {
			case EXP_BYCOLOR:
				AddEntToColorLists(newList, GetColorVal(edata, newList, 0));
				break;
			case EXP_BYLAYER:
				GetLayerVal(layer, edata, newList, 0);
				AddEntToLayerLists(newList, layer);
				break;
			default:
				ads_fail("SampleBlock: bad export mode\n");
        }
        ads_relrb(edata);
        if (ads_usrbrk())
            return 0;
    } while (ads_entnext(ename, ename) == RTNORM);
    return 1;
}


/*  Extract the entities of a block stored previously on the block
 *  list.  Pass the result on for proper storage.
 */
int SampleBlocksLists(DLL_LIST exportlists)

{
    long num = 0, numTot = 0;
    DLL_LIST blist;

    numTot = DllGetListSize(exportlists);
    while (!ads_usrbrk() && (blist = DllPopF(exportlists, NULL)) != NULL) {
        if (num % 20 == 0)
            ads_printf("Sampling blocks: %ld/%ld\r", num, numTot);
        if (!SampleBlock(blist))
            return 0;
        DllDestroyList(blist, FreePtrProc, NULL, NULL);
        num++;
    }
	ads_printf("Sampling blocks: %ld/%ld\n", num, numTot);
    return 1;
}



void GetEntXForm(ADSResBuf *edata, Matrix4 mx)
{
    double xscl = 1.0, yscl = 1.0, zscl = 1.0, rotang = 0.0;
    Vector3 zaxis, *zaxis_p = NULL;
    Point3 inpt, *inpt_p = NULL;
	Point3 bpt, *bpt_p = NULL;
    ADSResBuf *bdata, *rb;
	int isinsert = 0;

    rb = RBGetPoint3(&zaxis, edata, 210);
	if(rb != NULL && V3Normalize(&zaxis) != 0.0) zaxis_p = &zaxis;

    if (strcmp(RBSearch(edata, 0)->resval.rstring, "INSERT") == 0) {
		isinsert = 1;
	    if(RBGetPoint3(&inpt, edata, 10) != NULL) inpt_p = &inpt;
		(void)RBGetDouble(&rotang, edata, 50);
		(void)RBGetDouble(&xscl, edata, 41);
		(void)RBGetDouble(&yscl, edata, 42);
		(void)RBGetDouble(&zscl, edata, 43);

		/* the block base point */
		bdata = ads_tblsearch("BLOCK", RBSearch(edata, 2)->resval.rstring, 1);
		if(bdata) {
			rb = RBGetPoint3(&bpt, bdata, 10);
			if (rb != NULL) bpt_p = &bpt;
		    ads_relrb(bdata);
		}
	}
	M4GetAcadXForm(mx, zaxis_p, isinsert, inpt_p, rotang, xscl, yscl, zscl, bpt_p);

#ifdef DEBUG
    fprintf(stderr, "GetEntXForm: bpt: %f %f %f\n", bpt.x,
            bpt.y, bpt.z);
    M4Print(stderr, mx);
    fprintf(stderr, "GetEntXForm: inpt: %f  %f  %f\n", inpt.x, inpt.y,
            inpt.z);
    fprintf(stderr, "GetEntXForm: pt: %f  %f  %f\n", pt.x, pt.y, pt.z);
#endif
}



/*  Transform an entity (decomposed into polys) from ecs to mcs/wcs.
 */
Poly3 *TransformEnt(Poly3 *polys, ADSResBuf *edata)

{
    int i;
    Poly3 *poly;
    Point3 pnt;
    Matrix4 matrix;

#ifdef DEBUG
    fprintf(stderr, "TransformEnt(%p, %p)\n", polys, edata);
#endif
    if (edata == NULL) {
        WarnMsg("TransformEnt: no entity passed");
        return NULL;
    }
    GetEntXForm(edata, matrix);
    /* now apply the transform to the points to get mcs/wcs */
    for (poly = polys; poly; poly = poly->next) {
        for (i = 0; i < (int)poly->nverts; i++) {
            (void)M4MultPoint3(&poly->verts[i], matrix, &pnt);
            poly->verts[i] = pnt;
        }
    }
    return polys;
}

/*** end lists.c ***/
