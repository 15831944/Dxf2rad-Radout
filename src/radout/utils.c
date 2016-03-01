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

/* utils.c - miscellaneous stuff for converting AutoCAD into Radiance
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef UNIX
	#include <unistd.h>
#endif  /* UNIX */

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


Options options;
DLL_LIST exportBlockList, exportColorLists[MAXCOLORS], exportLayerLists;

/* These are not identical with the autocad entities. Especially, we
   subdivide polylines into several subtypes, which then again are 
   (partly) common between normal and light weight polylines.

   WARNING: TYPECOUNT in radout.h must reflect the number of
            entries here, including the trailing NULL */
char *validEntTypeList[] = {   /* sorted types to be converted */
		"3DFACE",
		"3DSOLID",
		"ARC",
		"BODY",
		"CIRCLE",
		"LINE",
		"PFACE",
		"PLINE",
		"PMESH",
		"POINT",
		"POLYGON",
		"REGION",
		"SOLID",
		"TRACE",
		"WPLINE",
		NULL
};

/* alloc space and copy name
 */
ads_namep NewADSName(ads_name name)

{
    ads_namep namep;

    if ((namep = malloc(sizeof(ads_name))) == NULL)
        ads_fail("NewADSName: can't alloc name.\n");
    ads_name_set(name, namep);
    return namep;
}

#pragma warning( disable : 4100 ) /* unused argument  */

/* ARGSUSED */
void *CopyNameProc(void *data, void *info)

{
    return (void*)NewADSName((ads_namep)data);
}
#pragma warning( default : 4100 ) /* reset unused argument  */


/*  another unused function ... */
int CompIntKey(void *key1, void *key2)

{
    return *((int*)key1) - *((int*)key2);
}


int CompStrKey(void *key1, void *key2)

{
    return strcmp((char*)key1, (char*)key2);
}

#pragma warning( disable : 4100 ) /* unused argument  */

/* ARGSUSED */
void FreeStrProc(void *ptr, void *info)
{
    acad_free(ptr); /* strings live on the autocad heap */
}

/* ARGSUSED */
void FreePtrProc(void *ptr, void *info)
{
    free(ptr);
}

/* ARGSUSED */
void FreeListProc(void *data, void *info)
{
    DLL_LIST list = (DLL_LIST)data;

    DllDestroyList(list, FreePtrProc, NULL, NULL);
}
#pragma warning( default : 4100 ) /* reset unused argument  */



extern void RadoutFreeAll()
{
    int i;
#ifdef DEBUG
    fprintf(stderr, "RadoutFreeAll() entered\n");
#endif
    strFREE(options.basename);
#ifdef SMOOTHING
    if (options.smooth != NULL) {
        SmoothFreeAll(options.smooth);
        options.smooth = NULL;
    }
#endif
    for (i = 0; i < TYPECOUNT; i++)
         strFREE(options.entTypes[i]);
    if (exportBlockList)
        DllDestroyList(exportBlockList, FreeListProc, NULL, NULL);
    if (exportLayerLists)
        DllDestroyList(exportLayerLists, FreePtrProc, FreeListProc, NULL);
    exportBlockList = exportLayerLists = NULL;
    for (i = 1; i < MAXCOLORS; i++) {
         DllDestroyList(exportColorLists[i], FreeListProc, NULL, NULL);
         exportColorLists[i] = NULL;
    }
#ifdef DEBUG
    fprintf(stderr, "RadoutFreeAll() done.\n");
#endif
}


extern int RadoutSetup()
{
	char *dotpos;
    RadoutFreeAll();
    /* memset(&options, 0, sizeof(Options)); */
    options.makeGeom = options.makeMats = 1;
    options.exportMode = EXP_BYCOLOR;
    (void)GetStrVar("DWGNAME", options.dwgName);
    if ((int)strlen(options.dwgName) <= 0)
        return 0;
    options.basename = StripPrefix(options.dwgName);
	dotpos = strchr(options.basename, '.'); /* clip extension */
	if(dotpos) *dotpos = '\0';
    return 1;
}



/* Search an ordered array of strings in a recursive binary fashion.
 * Returns the index of the string found. Less than 0 if false.
 */
int BinarySearch(char *name,int l, int h, char *array[])

{
    register int m, rc;

    if (l > h)
        return (-1);
    m = (l + h) / 2;
    if ((rc = strcmp(name, array[m])) == 0)
        return (m);
    else if (rc < 0)
        return (BinarySearch(name, l, m - 1, array));
    else
        return (BinarySearch(name, m + 1, h, array));
}


/* retreive displayed color */
int GetColorVal(ADSResBuf *data, DLL_LIST contblks, int level)

{
    int color = 0;
    char layer[64];
    ads_namep bname;
    ADSResBuf *l_rb = NULL, *c_rb = NULL;

#ifdef DEBUG
    ads_printf(" GetColorVal(%p, %d) reached.\n", data, level);
#endif
    /* bylayer: get color of entities layer. */
    if ((RBGetInt(&color, data, 62) == NULL) || (color == BYLAYER)) {
        if (!GetLayerVal(layer, data, contblks, ++level)) {
            WarnMsg("GetColorVal: can't get layer");
            return RTERROR;
        }
        if ((l_rb = ads_tblsearch("LAYER", layer, FALSE)) != NULL) {
            if (RBGetInt(&color, l_rb, 62) == NULL)
                color = WHITE;
            ads_relrb(l_rb);
        } else
            color = WHITE;
    }
    if (color == BYBLOCK) {
        /* byblock: get color of encapsulating block(s) */
        if (contblks && (bname = DllRank(contblks, level, NULL))) {
            if ((c_rb = ads_entget(bname)) != NULL) {
                color = GetColorVal(c_rb, contblks, ++level);
                ads_relrb(c_rb);
            } else
                color = WHITE;
        } else
            color = WHITE;
    }
#ifdef DEBUG
    ads_printf("GetColorVal = %d\n", color);
#endif
    return color;
}                            /* GetColorVal */


/*  retrieve floating layer
 */
int GetLayerVal(char layer[], ADSResBuf *data, DLL_LIST contblks, int level)

{
    ads_namep bname;
    ADSResBuf *c_rb = NULL;

    if (RBGetStr(layer, data, 8) == NULL) {
        WarnMsg("GetLayerVal: no layer found");
        return 0;
    }
    if ((strcmp(layer, "0") == 0) && (contblks &&
            (bname = DllRank(contblks, level, NULL)))) {
        c_rb = ads_entget(bname);
        GetLayerVal(layer, c_rb, contblks, ++level);    /* next level */
        ads_relrb(c_rb);
    }
    return RTNORM;
}


/*  Strip the filename of its prefix and make a new copy.
 */
#ifdef _WIN32
#define PATHSEPCHAR '\\'
#else
#define PATHSEPCHAR '/'
#endif /* _WIN32 */
char *StripPrefix(char *name)

{
    char *ptr, *nptr;

    if ((ptr = strrchr(name, PATHSEPCHAR)) == NULL)
        return StrDup(name);
	if (NULL == (nptr = StrDup(ptr+1))) {
		nptr = acad_malloc(sizeof(""));
		nptr[0] = '\0';
	}
	return nptr;
}


/*  Transpose certain characters to create an acceptable filename.
 */
void RegulateName(char *name)

{
    char *ptr;

    for (ptr = name; *ptr != '\0';  ptr++) {
        if (*ptr == '!' || *ptr == '$' || *ptr == '|')
            *ptr = '_';
        else
            *ptr = (char)tolower(*ptr);
    }   
}

/*** end utils.c ***/
