/*
This file is part of

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

/*
 *  Tools to operate on ADS result buffers and variables
 *  Philip Thompson
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef UNIX
	#include <unistd.h>
#endif

#include "cadcompat.h"

#include "adtools.h"
#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"


/* RBSearch -- Search an entity buffer chain and return an item
 *   with the specified group code.
 */
ADSResBuf *RBSearch(register ADSResBuf *rb, register int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    return rb;
}


int RBPrint(FILE *fp, ADSResBuf *rb)

{
    int rt;

    if (rb == NULL) {
        WarnMsg("RBPrint: null buffer.");
        return RTNONE;
    }
    if ((rb->restype >= 0) && (rb->restype <= 9))
        rt = RTSTR;
    else if ((rb->restype >= 10) && (rb->restype <= 19)) 
        rt = RT3DPOINT;
    else if ((rb->restype >= 38) && (rb->restype <= 59)) 
        rt = RTREAL;
    else if ((rb->restype >= 60) && (rb->restype <= 79)) 
        rt = RTSHORT;
    else if ((rb->restype >= 210) && (rb->restype <= 239)) 
        rt = RT3DPOINT;
    else if (rb->restype < 0)
        rt = rb->restype;
    else
        rt = RTNONE;
    switch(rt) {
    case RTSHORT:
        if (fp)
            fprintf(stderr, "(%d . %d)", rb->restype, rb->resval.rint);
        else
            ads_printf("(%d . %d)", rb->restype, rb->resval.rint);
        break;
    case RTREAL:
        if (fp)
            fprintf(stderr, "(%d . %0.3f)", rb->restype, rb->resval.rreal);
        else
            ads_printf("(%d . %0.3f)", rb->restype, rb->resval.rreal);
        break;
    case RTSTR:
        if (fp)
            fprintf(stderr,"(%d . %s)", rb->restype, rb->resval.rstring);
        else
            ads_printf("(%d . %s)", rb->restype, rb->resval.rstring);
        break;
    case RT3DPOINT:
        if (fp)
            fprintf(stderr,"(%d . %0.3f %0.3f %0.3f)", rb->restype,
                    rb->resval.rpoint[X], rb->resval.rpoint[Y],
                    rb->resval.rpoint[Z]);
        else
            ads_printf("(%d . %0.3f %0.3f %0.3f)", rb->restype,
                    rb->resval.rpoint[X], rb->resval.rpoint[Y],
                    rb->resval.rpoint[Z]);
        break;
    case RTNONE:
        if (fp)
            fprintf(stderr,"(%d . Unknown type)", rb->restype);
        else
            ads_printf("(%d . Unknown type)", rb->restype);
        break;
    case -1:
    case -2:
        if (fp)
            fprintf(stderr,"(%d . <Entity name: %8lx>)", rb->restype,
                    rb->resval.rlname[0]);
        else
            ads_printf("(%d . <Entity name: %8lx>)", rb->restype,
                    rb->resval.rlname[0]);
    }
    return rb->restype;
}


void RBPrintList(FILE *fp, ADSResBuf *rb)

{
    register ADSResBuf *ri;

    if (rb == NULL) {
        WarnMsg("RBPrintList: no result buffer.\n");
        return;
    }
    for (ri = rb; ri != NULL; ri = ri->rbnext) {
        RBPrint(fp, ri);
        if (fp)
            fprintf(fp, "\n");
        else
            ads_printf("\n");
    }
}


int PrintEntity(FILE *fp, ads_name name)

{
    ADSResBuf *rb;

    if ((rb = ads_entget(name)) == NULL) {
        WarnMsg("PrintEntity: failed to get entity buffer.");
        return 0;
    }
    RBPrintList(fp, rb);
    ads_relrb(rb);
    return 1;
}


void RBPrintHead(FILE *fp, ADSResBuf *rb)

{
    ADSResBuf *ri;

    if (rb == NULL) {
        WarnMsg("RBPrintHead: no result buffer.");
        return;
    }
    if ((ri = RBSearch(rb, -1)) != NULL)
        RBPrint(fp, ri);
    if ((ri = RBSearch(rb, 0)) != NULL)
        RBPrint(fp, ri);
    if ((ri = RBSearch(rb, 8)) != NULL)
        RBPrint(fp, ri);
    if ((ri = RBSearch(rb, 39)) != NULL)
        RBPrint(fp, ri);
    if ((ri = RBSearch(rb, 62)) != NULL)
        RBPrint(fp, ri);
    if ((ri = RBSearch(rb, 67)) != NULL)
        RBPrint(fp, ri);
    if ((ri = RBSearch(rb, 210)) != NULL)
        RBPrint(fp, ri);
    if (fp)
        fprintf(stderr, "\n");
    else
        ads_printf("\n");
}


/* Print a quick summary of an entity */
int PrintEntityHeader(FILE *fp, ads_name name)

{
    ADSResBuf *rb;

    if ((rb = ads_entget(name)) == NULL) {
        WarnMsg("PrintQEntity: failed to get entity buffer.");
        return 0;
    }
    RBPrintHead(fp, rb);
    ads_relrb(rb);
    return 1;
}

int GetADSPointVar(char *str, ads_point point)

{
    ADSResBuf rb;

    if (ads_getvar(str, &rb) != RTNORM)
        return 0;
    ads_point_set(rb.resval.rpoint, point);
    return 1;
}


Point3 *GetPoint3Var(char *str, Point3 *point)

{
    ADSResBuf rb;

    if (ads_getvar(str, &rb) != RTNORM)
        return NULL;
    point->x = rb.resval.rpoint[X];
    point->y = rb.resval.rpoint[Y];
    point->z = rb.resval.rpoint[Z];
    return point;
}


char *GetStrVar(char *str, char *resstr)

{
    ADSResBuf rb;

    if (ads_getvar(str, &rb) != RTNORM)
        return NULL;
    return strcpy(resstr, rb.resval.rstring);
}


int GetIntVar(int *val, char *str)

{
    ADSResBuf rb;

    if (ads_getvar(str, &rb) != RTNORM)
        return 0;
    *val = rb.resval.rint;
    return 1;
}


int GetDoubleVar(double *real, char *str)

{
    ADSResBuf rb;

    if (ads_getvar(str, &rb) != RTNORM)
        return 0;
    *real = rb.resval.rreal;
    return 1;
}


/* SETVARS * */

void SetPointVar(char *str, ads_point point, int dimflag)

{
    ADSResBuf rb;

    rb.rbnext = NULL;
    if (dimflag == 2)
        rb.restype = RTPOINT;       /* flag for 2D-point */
    if (dimflag == 3)
        rb.restype = RT3DPOINT;     /* flag for 3D-point */
    /* put point in result-buffer */
    ads_point_set(point, rb.resval.rpoint);
    ads_setvar(str, &rb);
}


void SetIntVar(char *str, int val)

{
    ADSResBuf rb;

    rb.rbnext = NULL;
    rb.restype = RTSHORT;
    rb.resval.rint = (short)val;
    ads_setvar(str, &rb);
}


/* retreive entity-name */
ADSResBuf *RBGetName(ads_name ename, ADSResBuf *rb, int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    if (rb)
        ads_name_set(rb->resval.rlname, ename);
    return rb;
}


/* retreive and alloc string */
char *RBGetNewStr(ADSResBuf *rb, int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    if (rb)
        return StrDup(rb->resval.rstring);
    return NULL;
}


/* retreive string */
ADSResBuf *RBGetStr(char *str, ADSResBuf *rb, int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    if (rb != NULL)
        (void)strcpy(str, rb->resval.rstring);
    return rb;
}


/* retreive integer */
ADSResBuf *RBGetInt(int *ival, ADSResBuf *rb, int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    if (rb != NULL)
        *ival = (int)((short)rb->resval.rint);
    return rb;
}


/* retreive double */
ADSResBuf *RBGetDouble(double *rval, ADSResBuf *rb, int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    if (rb != NULL)
        *rval = rb->resval.rreal;
    return rb;
}


/* retreive point */
ADSResBuf *RBGetADSPoint(ads_point point, ADSResBuf *rb, int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    if (rb != NULL)
        ads_point_set(rb->resval.rpoint, point);
    return rb;
}


ADSResBuf *RBGetPoint3(Point3 *point, ADSResBuf *rb, int gcode)

{
    while ((rb != NULL) && (rb->restype != gcode))
        rb = rb->rbnext;
    if (rb != NULL) {
        point->x = rb->resval.rpoint[X];
        point->y = rb->resval.rpoint[Y];
        point->z = rb->resval.rpoint[Z];
    }
    return rb;
}

/*** end resbuf.c ***/
