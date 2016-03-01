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

/*  Dialog box call for radout
 */



#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef UNIX
	#include <unistd.h>
#endif

#include "cadcompat.h"
#if defined(R14)||defined(R15)
  #include <adsdlg.h>
#endif

#include "adtools.h"
#include "geomtypes.h"
#include "radout.h"
#include "dlltypes.h"
#include "dllproto.h"
#include "geomdefs.h"
#include "cproto.h"
#include "geomproto.h"

extern Options options;
extern char *validEntTypeList[];


static int GetDoubleVal(ads_callback_packet *cbpkt, char *name, double *val)

{
    char buf[17];
    double dbl;

    /* ??? */ ads_get_tile(cbpkt->dialog, name, buf, 16);
    if ((sscanf(buf, "%lf", &dbl)) == 1) {
        *val = dbl;
        return 1;
    }
    return 0;
}


static int GetToggleVal(ads_callback_packet *cbpkt, char *name)

{
    char val[3];

    /* ??? */ ads_get_tile(cbpkt->dialog, name, val, 3);
    if (strcmp(val, "1") == 0)
        return 1;
    return 0;
}


/* extract data from dialog box and close if possible
 */
static int GetRadDlgValues(ads_callback_packet *cbpkt)

{
    int i = 0;
    double dblval;
    char **elist, strval[80];
    ads_hdlg hDlg = cbpkt->dialog;
    /* these must be ordered according to the defines */
    static char *sunVals[6] = {"Month", "Day", "Hour", "Long", "Lat", "TZ"};
#ifdef DEBUG
    fprintf(stderr,"GetRadDlgValues(%p) entered\n", cbpkt);
#endif
    if ((options.makeGeom = GetToggleVal(cbpkt, "geometry")) == 1) {
        for (i = 0, elist = validEntTypeList; *elist != NULL; elist++)
            if (GetToggleVal(cbpkt, *elist))
                options.entTypes[i++] = StrDup(*elist);
        if ((options.entTypesCount = i) == 0)
            options.makeGeom = 0;
    } else
        options.entTypesCount = 0;
    if (GetDoubleVal(cbpkt, "distTolerance", &dblval) && (dblval > 0.0))
        options.distTolerance = dblval;
    else {
        sprintf(strval, "%.8g", options.distTolerance);
        ads_set_tile(hDlg, "distTolerance", strval);
        return 0;
    }
    if (GetDoubleVal(cbpkt, "angleTolerance", &dblval) && (dblval > 0.0))
        options.angleTolerance = (dblval/180.0)*M_PI;
    else {
        sprintf(strval, "%.8g", (options.angleTolerance/M_PI)*180.0);
        ads_set_tile(hDlg, "angleTolerance", strval);
        return 0;
    }
    /* get various toggle*/
    options.makeMats = GetToggleVal(cbpkt, "materials");
    options.makeRif = GetToggleVal(cbpkt, "rif");
#ifdef SMOOTHING
    if (GetToggleVal(cbpkt, "smoothing")) {
        options.smooth = InitSmooth();
        if (GetDoubleVal(cbpkt, "tolerance", &dblval))
            EnableEdgePreservation(options.smooth,
                    cos(dblval*DEG_TO_RAD));
        else
            EnableEdgePreservation(options.smooth, 0.0);
    } else
        options.smooth = NULL;
#endif
    if (GetToggleVal(cbpkt, "Color")) {      /* get color/layer mode */
        options.exportMode = EXP_BYCOLOR;
		strcpy(options.colorprefix, "_c");
	}
    else
        options.exportMode = EXP_BYLAYER; {
		strcpy(options.layerprefix, "_l");
	}

    /* get the scale factor */
    if (GetDoubleVal(cbpkt, "scale", &dblval))
         options.scaleFactor = dblval;
    else {
        sprintf(strval, "%.8g", options.scaleFactor);
        ads_set_tile(hDlg, "scale", strval);
        return 0;
    }
    ads_get_tile(hDlg, "basename", strval, 80);
    FREE(options.basename);
    options.basename = StrDup(strval);
    if ((options.makeSun = GetToggleVal(cbpkt, "sunlight")) == 1) {
        for (i = 0; i < 6; i++) {
            if (GetDoubleVal(cbpkt, sunVals[i], &dblval))
                options.sunlight[i] = dblval;
            else {
                sprintf(strval, "%.8g", options.sunlight[i]);
                ads_set_tile(hDlg, sunVals[i], strval);
                return 0;
            }
        }
    }
#ifdef DEBUG
    fprintf(stderr, "GetRadDlgValues() done\n");
#endif
    return 1;
}


static void CBOk(ads_callback_packet *cbpkt)

{
    if (ads_usrbrk())
        ads_term_dialog();
    if (GetRadDlgValues(cbpkt)) {
        ads_done_dialog(cbpkt->dialog, DLGOK);
        return;
    }
    return;
}


/* dialog was cancelled */
static void CBCancel(ads_callback_packet *cbpkt)

{
    ads_done_dialog(cbpkt->dialog, DLGCANCEL);
}


/* callback for exterior sunlight toggle */
static void CBToggleSunlight(ads_callback_packet *cbpkt)

{
    if (GetToggleVal(cbpkt, "sunlight"))
        ads_mode_tile(cbpkt->dialog, "sunvals", MODE_ENABLE);
    else
        ads_mode_tile(cbpkt->dialog, "sunvals", MODE_DISABLE);
}


/* callback for smoothing toggle */
static void CBToggleSmoothing(ads_callback_packet *cbpkt)

{
    if (GetToggleVal(cbpkt, "smoothing"))
        ads_mode_tile(cbpkt->dialog, "tolerance", MODE_ENABLE);
    else
        ads_mode_tile(cbpkt->dialog, "tolerance", MODE_DISABLE);
}


/* callback for geometry files toggle */
static void CBToggleFiles(ads_callback_packet *cbpkt)

{
    if (GetToggleVal(cbpkt, "geometry")) {
        ads_mode_tile(cbpkt->dialog, "fileList", MODE_ENABLE);
        ads_mode_tile(cbpkt->dialog, "modes", MODE_ENABLE);
    } else {
        ads_mode_tile(cbpkt->dialog, "fileList", MODE_DISABLE);
        ads_mode_tile(cbpkt->dialog, "modes", MODE_DISABLE);
    }
}


static void CBTextVerifyDouble(ads_callback_packet *cbpkt)

{
    static int oldReason = -1;
    double val = 0.0;
    char tile[TILE_STR_LIMIT];

    ads_get_attr_string(cbpkt->tile, "key", tile, TILE_STR_LIMIT);
    if (sscanf(cbpkt->value, "%lf", &val) == 1) {
        ads_set_tile(cbpkt->dialog, "error",
                "                               ");
        oldReason = cbpkt->reason;
        return;
    }
    if ((cbpkt->reason == CBR_LOST_FOCUS) &&
                 (cbpkt->reason == oldReason))
        return;
    ads_set_tile(cbpkt->dialog, "error", "Plese enter a NUMBER value!");
    ads_mode_tile(cbpkt->dialog, tile, MODE_SETFOCUS);
    oldReason = cbpkt->reason;
}


int RadoutDLG()
{
    int dcl_id, dlgStatus;
    char temp[80], **elist;
    ads_hdlg hDlg = NULL;

#ifdef DEBUG
    fprintf(stderr, "RadoutDLG() entered\n");
#endif
    if (ads_load_dialog("radout.dcl", &dcl_id) != RTNORM) {
        ads_fail("can't load dialog box");
        return 0;
    }
    if (dcl_id < 0) {
        ads_fail("error loading \"radout.dcl\"");
        return 0;
    }
    if (ads_new_dialog("radout", dcl_id, NULLCB, &hDlg) != RTNORM) {
        ads_fail("can't open dialog box");
        return 0;
    }
    if (hDlg == NULL) {
        ads_fail("ads_new_dialog: failed");
        return 0;
    }
    /* set up entity types */
    for (elist = validEntTypeList; *elist != NULL; elist++)
        ads_set_tile(hDlg, *elist, "1");
    ads_set_tile(hDlg, "POLYGON", "0");
    ads_set_tile(hDlg, "POINT", "0");
    /* set up filetypes section */
    ads_mode_tile(hDlg, "sunvals", MODE_DISABLE);
    ads_mode_tile(hDlg, "accept", MODE_SETFOCUS);
    /* set up default values */
    ads_set_tile(hDlg, "basename", options.basename);
    sprintf(temp, "%.8g", options.distTolerance);
    ads_set_tile(hDlg, "distTolerance", temp);
    sprintf(temp, "%.8g", (options.angleTolerance/M_PI)*180.0);
    ads_set_tile(hDlg, "angleTolerance", temp);
    sprintf(temp, "%.8g", options.scaleFactor);
    ads_set_tile(hDlg, "scale", temp);
    /* initialize callback functions */
    ads_action_tile(hDlg, "Hour", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "Day", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "Month", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "Lat", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "Long", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "TZ", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "scale", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "geometry", (CLIENTFUNC)CBToggleFiles);
    ads_action_tile(hDlg, "sunlight", (CLIENTFUNC)CBToggleSunlight);
    ads_action_tile(hDlg, "distTolerance", (CLIENTFUNC)CBTextVerifyDouble);
    ads_action_tile(hDlg, "angleTolerance", (CLIENTFUNC)CBTextVerifyDouble);
#ifdef SMOOTHING
    ads_mode_tile(hDlg, "tolerance", MODE_DISABLE);
    ads_action_tile(hDlg, "smoothing", (CLIENTFUNC)CBToggleSmoothing);
    /* ads_mode_tile(hDlg, "smoothing", MODE_DISABLE); */
#endif /* SMOOTHING */
#if !defined(ACIS)
    ads_mode_tile(hDlg, "3DSOLID", MODE_DISABLE);
    ads_mode_tile(hDlg, "BODY", MODE_DISABLE);
    ads_mode_tile(hDlg, "REGION", MODE_DISABLE);
#endif /* (defined(R14)||defined(R15))&&!defined(ACIS) */
    ads_action_tile(hDlg, "accept", (CLIENTFUNC)CBOk);
    ads_action_tile(hDlg, "cancel", (CLIENTFUNC)CBCancel);
    /* go for it! */
    ads_start_dialog(hDlg, &dlgStatus);
    ads_unload_dialog(dcl_id);
#ifdef DEBUG
    fprintf(stderr, "RadoutDLG(%d) done\n", dlgStatus);
#endif
    if (dlgStatus == DLGOK)
        return 1;
    return 0;
}
/*** end dialog.c ***/
