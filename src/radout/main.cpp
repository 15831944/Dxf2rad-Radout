/*
This file is part of

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

/* main.cpp */

// posix
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef UNIX
  #include <unistd.h>
# endif

#include "cadcompat.h"

// our ADS wrapper interface
#include "adtools.h"
#include "adcolor.h"

// geometry manipulation
#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"

// our linked list library
#include "dlltypes.h"
#include "dllproto.h"

// general local definitions
#include "radout.h"
#include "cproto.h"
#include "lispapi.h"
#include "writerad.h"


/* system commands */
extern "C" int rf_osexec (struct resbuf *rb);

/* acad symbolname (no spaces etc ...) */
#ifdef ACIS
  #ifdef R14
    #define APPNAME "RADOUT_4_0_1_rxADS_SimpleACIS"
  #endif // R14
  #ifdef R15
    #define APPNAME "RADOUT_4_0_1_rxADS_FullACIS"
  #endif // R15
#else // ACIS
  #ifdef IC2000
    #define APPNAME "RADOUT_4_0_1_SDS"
  #elif defined(IC2004)
    #define APPNAME "RADOUT_4_0_1_SDS4"
  #else // IC2000
    #define APPNAME "RADOUT_4_0_1_rxADS"
  #endif // IC2000
#endif // ACIS


extern "C" DLL_LIST exportBlockList;
extern "C" DLL_LIST exportColorLists[MAXCOLORS], exportLayerLists;
extern "C" char *validEntTypeList[];
extern "C" Options options;

static int Radout(resbuf*);


static FuncEntry funcTable[] = {
    {"c:radout", Radout},               /* the "native" program with it's own dcl */
	{"sample_n_export",sample_n_export},/* lisp callable "api" for external control */

	/* External colour system to AutoCAD colour functions */
	/* Those functions are taken from the colext.c demo program by Autodesk */
	/* The slightly modified colext.c is included as AdsLib/adcolor.c */
	/* Those might as well be called externally if preferred */
	{"CMY",       cmyac},
	{"CNS",       cnsac},
	{"CTEMP",     ctempac},
	{"HLS",       hlsac},
	{"HSV",       hsvac},
	{"RGB",       rgbac},
	{"YIQ",       yiqac},
	/* External colour system to RGB functions */
	{"CMY-RGB",   cmyrgb},
	{"CNS-RGB",   cnsrgb},
	{"CTEMP-RGB", ctemprgb},
	{"HLS-RGB",   hlsrgb},
	{"HSV-RGB",   hsvrgb},
	{"YIQ-RGB",   yiqrgb},
	/* AutoCAD colour index to external colour system functions */
	{"TO-RGB",    torgb},
	{"TO-CMY",    tocmy},
	{"TO-YIQ",    toyiq},
	{"TO-HSV",    tohsv},
	{"TO-HLS",    tohls},
	{"TO-CNS",    tocns},
	/* Control and utility functions */
	{"CNSERR",    cnser},
	{"COLSET",    colset},

	/* system command execution */
	{"rf_osexec",     rf_osexec},
};



/*-----------------------------------------------------------------------*/

/* LOADFUNCS  --  Define external functions with AutoLISP.
 *   Normally expanded to call ads_defun() once for each external
 *   function to be defined, assigning each one a different ADS
 *   function code.  ads_regfunc() is then called to specify the
 *   handler function for each ADS function code.
 */
extern int Radout_LoadFuncs()
{
    int i;
    ADSResBuf *rb;
    static char *localname = APPNAME;

    for (i = 0; i < ELEMENTS(funcTable); i++) {
        if (ads_defun(funcTable[i].funcName, (short)i) != RTNORM)
            return RTERROR;
    }
    if ((rb = ads_tblsearch("APPID", localname, 0)) == NULL) {
        if (ads_regapp(APPNAME) != RTNORM) {
            WarnMsg("can't register XDATA for %s.", localname);
            return RTERROR;
        }
    } else
        ads_relrb(rb);
    (void)memset(&options, 0, sizeof(Options));
    options.scaleFactor = 1.0;
    options.distTolerance = DIST_TOLERANCE;
    options.angleTolerance = ANGLE_TOLERANCE;
    ads_printf("%s  ", APPNAME);
    return RTNORM;
}


/* DoFunc -- call command handlers */
extern int Radout_DoFuncs()
{
    int val;
    ADSResBuf *rb = NULL;

    /* Get the function code and check that it's within range. (It
     * can't fail to be, but paranoia doesn't hurt.)
     */
    if ((val = ads_getfuncode()) < 0 || val >= ELEMENTS(funcTable)) {
        ads_fail("Received nonexistent function code.");
        return RTERROR;
    }
    /* fetch the arguments, if any. */
    rb = ads_getargs();
    /* call the handler and return its status. */
    val = (*funcTable[val].func)(rb);
    if (rb) ads_relrb(rb);
    return val;
}


extern int Radout_CleanUp()
{
    RadoutFreeAll();
    return RTNORM;
}


#ifdef ACIS
extern int SolidToPoly(ADSResBuf *edata, Poly3 **outpolys);
#endif // ACIS

static int WriteEntity(FILE *fp, ads_namep ename, DLL_LIST contblks,
					   char *matName, int id)
{
    char type[16];
    ADSResBuf *edata;
	Poly3 *polys;
	Cyl3 *cyls;

    edata = ads_entget(ename);
    (void)GetEntType(edata, type);
#ifdef DEBUG
    fprintf(stderr, "WriteEntity: type %s\n", type);
#endif
    if (strcmp(type, "LINE") == 0) {
		polys = LineToPoly(edata);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
	} else if (strcmp(type, "3DFACE") == 0) {
		polys = FaceToPoly(edata);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "PLINE") == 0) {
		polys = PlineToPoly(edata, 1);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "POLYGON") == 0) {
		polys = PlineToPoly(edata, 2);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "WPLINE") == 0) {
		polys = PlineToPoly(edata, 3);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "PFACE") == 0) {
		polys = PFaceToPoly(edata);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "PMESH") == 0) {
		polys = MeshToPoly(edata);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "CIRCLE") == 0) {
		cyls = CircleToCyl(edata);
		(void)TransformCylToWCS(cyls, contblks);
        WriteCyl(fp, matName, id, cyls);
    } else if (strcmp(type, "SOLID") == 0) {
		polys = TraceToPoly(edata);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "TRACE") == 0) {
		polys = TraceToPoly(edata);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "ARC") == 0) {
		polys = ArcToPoly(edata);
		(void)TransformPolyToWCS(polys, contblks);
        WritePoly(fp, matName, id, polys);
    } else if (strcmp(type, "POINT") == 0) {
		cyls = PointToCyl(edata);
		(void)TransformCylToWCS(cyls, contblks);
        WritePoint(fp, matName, id, cyls);
    } else if (strcmp(type, "3DSOLID") == 0 ||
             strcmp(type, "REGION") == 0 ||
             strcmp(type, "BODY") == 0) {
#ifdef ACIS
			Poly3 *polys;
			if(SolidToPoly(edata, &polys) < 0) return -1;
			(void)TransformPolyToWCS(polys, contblks);
			WritePoly(fp, matName, id, polys);
#endif /* ACIS */
    } else {
        WarnMsg("WriteEntity: unknown entity type: %s", type);
	}
    ads_relrb(edata);
    return 1;
}


extern int WriteExportList(FILE *fp, DLL_LIST expList,
						   char *basename, char *matName, char *radFName)
{
    long num = 0, numTot;
    time_t now;
	int res;
    DLL_LIST list;
    ads_namep ename;

    now = time(NULL);

	fprintf(fp, "## Radiance geometry file %s for model: %s\n",
            radFName, basename);
    fprintf(fp, "## Created: %s", ctime(&now));

    fprintf(fp, "## Material \"%s\" should be previously defined.\n",
            matName);
    fprintf(fp, "## Geometry from layer or color \"%s\" follows.\n",
            matName);
    if ((numTot = DllGetListSize(expList)) <= 0)
        return 0;
    for (list = DllFirst(expList, NULL); list != NULL;
            list = DllNext(expList, NULL), num++) {
        /* get entity off top of hierarchy list */
        ename = (long *)DllPopF(list, NULL);
        if (num % 20 == 0)
            ads_printf("Writing entity: %ld/%ld\r", num, numTot);
#ifdef DEBUG
        fprintf(stderr, "WriteExportList: %lx : %d\n", *ename, num);
#endif
        res = WriteEntity(fp, ename, list, matName, num);
        free(ename);
        if (res < 0 || ads_usrbrk())
            return 0;
    }
    return 1;
}


int WriteGeometry(Options *opts)

{
    long i;
    DLL_LIST expList;
    char *layer, *basename, lay[80], matName[80], radFName[80];
    FILE *fp;

    if (opts->exportMode == EXP_BYCOLOR) {
        for (i = 1; i < MAXCOLORS; i++) {
            if ((expList = exportColorLists[i]) == NULL)
                continue;
            if (DllGetListSize(expList) <= 0)
                continue;
            sprintf(lay, "%s%ld", options.colorprefix, i);
            sprintf(radFName, "%s%s.rad", opts->basename, lay);
            if ((fp = fopen(radFName, "w")) == NULL) {
                WarnSys("WriteGeometry: can't open file: %s\n");
                return 0;
            }
            ads_printf("Writing file: %s\n", radFName);
            basename = StripPrefix(opts->basename);
            sprintf(matName, "%s%s", basename, lay);
            strcat(strcpy(radFName, matName), ".rad");
            if (!WriteExportList(fp, expList, basename, matName,
                    radFName)) {
                strFREE(basename);
                (void)fclose(fp);
                return 0;
            }
			strFREE(basename);
            (void)fclose(fp);
        }
        return 1;
    }
    /* otherwise we output by layers */
    while ((layer = (char*)DllPopF(exportLayerLists, &expList)) != NULL) {
        if ((i = DllGetListSize(expList)) <= 0)
            continue;
#ifdef DEBUG
        fprintf(stderr, "%s: %d\n", layer, i);
#endif

        sprintf(lay, "%s%s",options.layerprefix, layer);
        RegulateName(lay);
        sprintf(radFName, "%s%s.rad", opts->basename, lay);
        if ((fp = fopen(radFName, "w")) == NULL) {
            WarnSys("WriteGeometry: can't open file: %s\n");
            return 0;
        }
        ads_printf("Writing file: %s\n", radFName);
        basename = StripPrefix(opts->basename);
        sprintf(matName, "%s%s", basename, lay);
        strcat(strcpy(radFName, matName), ".rad");
        if (!WriteExportList(fp, expList, basename, matName, radFName)) {
            strFREE(basename);
            (void)fclose(fp);
            return 0;
        }
		strFREE(basename);
        (void)fclose(fp);
        strFREE(layer);
    }
    return 1;
}


#pragma warning( disable : 4100 )
/* ARGSUSED */
static int Radout(ADSResBuf *args)
{
	int res;

    if (!RadoutSetup()) {
        WarnMsg("error setting up radout.");
        return RTERROR;
    }
    if (!RadoutDLG()) return RTNORM;

	res = export(); 

	/*  WriteGeometry destroys lists, so we call it seperately  */
	/*  maybe we could move list destruction to RadoutFreeAll ? */
	/*  this would make the program more flexible, and all      */
	/*  information would be available until just before we     */
	/*  really want to return to whoever called us */

    if (options.makeGeom  && !WriteGeometry(&options)) {
        WarnMsg("can't WriteGeometry()");
		res = RTERROR;
    }
    RadoutFreeAll();
	return(res);
}
#pragma warning( default : 4100 ) /* reset unused arguments */


int export() /*  seperate function makes it independent of dialog */
{
    long int count = 0L;
    ads_name sset;
    ADSResBuf rbet[2];

    rbet[0].restype = 0;
    rbet[0].resval.rstring = 
		"INSERT,POLYLINE,LWPOLYLINE,LINE,"
		"3DFACE,TRACE,CIRCLE,ARC,SOLID,POINT,"
#ifdef ACIS
		"3DSOLID,REGION,BODY"
#endif  // ACIS
		;
    rbet[0].rbnext = &rbet[1];
    rbet[1].restype = 67;
    rbet[1].resval.rint = 0;
    rbet[1].rbnext = NULL;
    ads_name_clear(sset);
    if (ads_ssget(NULL, NULL, NULL, rbet, sset) != RTNORM) {
		// user cancel or empty pick
	    if(!ads_name_nil(sset)) ads_ssfree(sset);
        return RTNORM;
    }
    if (ads_sslength(sset, &count) != RTNORM)
        return RTERROR;
    if (count <= 0L)
        return RTNORM;

    if (!MakeExportLists(options.exportMode)) {
        WarnMsg("can't make export lists.");
        goto error;
    }
    if (!SampleEnts(sset, options.exportMode)) {
        WarnMsg("can't sample selection set.");
        goto error;
    }
    while (DllGetListSize(exportBlockList) > 0) {
        if (!SampleBlocksLists(exportBlockList)) {
            WarnMsg("can't sample blocks.");
            goto error;
        }
    }

    if ((options.makeMats || options.makeRif) &&
            !WriteMatsAndRif(&options)) {
        WarnMsg("can't WriteMatsAndRif()");
        goto error;
    }
    if (options.makeSun  && !WriteDaylight(&options)) {
        WarnMsg("can't WriteDaylight()");
        goto error;
    }
    ads_ssfree(sset);
    return RTNORM;
error:
    if(!ads_name_nil(sset)) ads_ssfree(sset);
    return RTERROR;
}


// EOF main.cpp
