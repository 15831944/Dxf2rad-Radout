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



#include  <stdio.h>
#include  <string.h>

#ifdef DOS
  #include "dir.h"
  #include "process.h"
  #include "signal.h"
  #include "stdlib.h"
#endif

#ifdef UNIX
  #ifdef UNIX_CDIR
	#include <unistd.h>
	#include <limits.h>
	#define MAXPATH PATH_MAX
  #endif
  #include  <dirent.h>
#endif

#ifdef _WIN32
  #include <direct.h>
  #include <stdlib.h>
#endif /* _WIN32 */

#include "cadcompat.h"

#include "adtools.h"
#include "geomtypes.h"
#include "geomdefs.h"
#include "radout.h"
#include "dlltypes.h"
#include "dllproto.h"
#include "geomproto.h"
#include "cproto.h"
#include "lispapi.h"

#ifdef _WIN32
#define PATHSEPCHAR '\\'
#else
#define PATHSEPCHAR '/'
#endif /* _WIN32 */

extern Options options;
extern DLL_LIST exportColorLists[MAXCOLORS], exportLayerLists;
extern char *validEntTypeList[];

#ifdef DOS
  extern int errno;
#endif

void layer_traversal(void *key, void *laylist, struct resbuf *rbret);

/***********************************************************************/
/* return list of exported files to autolisp */
static void return_color_mats() 
{
	struct resbuf *rb, *rbret = NULL;
	char col_str[80], *nprefix;
	DLL_LIST expList = NULL;
	int i;

	nprefix = strrchr(options.basename, PATHSEPCHAR);

	for (i = 1; i < MAXCOLORS; i++) {
		if ((expList = exportColorLists[i]) == NULL) continue;
		if (DllGetListSize(expList) <= 0) continue;

		sprintf(col_str, "%s%s%d", nprefix ? nprefix+1 : "",
							 options.colorprefix, i);
		rb = ads_newrb(RTSTR);
		/* though rb->resval.rstring may not be NULL, it is invalid! */
		rb->resval.rstring = StrDup(col_str);
		if (NULL == rbret) {
			rbret = rb;
		} else {
			rb->rbnext = rbret;
			rbret = rb;
		}
	}
	if (rbret) {
		ads_retlist(rbret);
		ads_relrb(rbret);
	}
}


void layer_traversal(void *key, void *laylist, struct resbuf *rbret)
/* visit each list */
{
	struct resbuf *rb;
	char  lay_str[128];

	if (DllGetListSize(laylist) > 0L) {
		sprintf(lay_str, "%s%s%s",
						 rbret->resval.rstring ? rbret->resval.rstring+1 : "",
						 options.layerprefix,
						 (char*)key);
		RegulateName(lay_str);
		rb = ads_newrb(RTSTR);
		/* though rb->resval.rstring may not be NULL, it is invalid! */
		rb->resval.rstring = StrDup(lay_str);
		/* insert resbuf into output list */
		rb->rbnext = rbret->rbnext;
		rbret->rbnext = rb;
	}
}

		
/* return list of exported file names to autolisp */
static void return_layer_mats() 
{
	struct resbuf rbret;

	rbret.resval.rstring = strrchr(options.basename, PATHSEPCHAR);
	rbret.rbnext = NULL;

	DllTraverse(exportLayerLists, (TraversalProc)layer_traversal, (void*)&rbret);
	if (rbret.rbnext) {
		ads_retlist(rbret.rbnext);
		ads_relrb(rbret.rbnext);
	}
}


/* sample and export entities */
int sample_n_export(struct resbuf *rb)
{
	struct resbuf *rbname, *rbxmode, *rbdist, *rbang, *rbscale, *rbtype;
	int i ;
	char  **elist;
	char *temp_basename = NULL;

	/* current interface is :
	 * (sample_n_export <basename> "Color"|"Layer"
	 *              distTolerance angleTolerance scaleFactor 
	 * 				 ("3DFACE" "ARC" "CIRCLE" "LINE" "PFACE" "PLINE"
	 * 			   "PMESH" "POINT" "POLYGON" "SOLID" "TRACE" "WPLINE")  )
	 *   if one of the types is not spelled correctly (usually "")
	 *    this type is skipped.
	 *
	 * This api could probably live with some enhancements like
	 * using a bytemap for the type information.
	 */

	if (!RadoutSetup()) {
		ads_fail("error setting up radout.");
		return RTERROR;
	}

	/* unfold the list into named variables and check if they are all there */
	if (NULL == (rbname = rb)) {
		ads_fail("\nNo arguments! ");
		return RTERROR;
	}
	if (RTSTR != rbname->restype ) {
		ads_fail("\nInvalid argument type for basename! ");
		return RTERROR;
	}

	if (NULL == (rbxmode = rbname->rbnext)) {
		ads_fail("\nNot enough arguments! ");
		return RTERROR;
	}
	if (RTSTR != rbxmode->restype ) {
		ads_fail("\nInvalid argument type for export mode! ");
		return RTERROR;
	}

	if (NULL == (rbdist = rbxmode->rbnext)) {
		ads_fail("\nNot enough arguments! ");
		return RTERROR;
	}
	if (RTREAL != rbdist->restype ) {
		ads_fail("\nInvalid argument type for arc distance tolerance ! ");
		return RTERROR;
	}

	if (NULL == (rbang = rbdist->rbnext)) {
		ads_fail("\nNot enough arguments! ");
		return RTERROR;
	}
	if (RTREAL != rbang->restype ) {
		ads_fail("\nInvalid argument type for arc angle tolerance ! ");
		return RTERROR;
	}

	if (NULL == (rbscale = rbang->rbnext)) {
		ads_fail("\nNot enough arguments! ");
		return RTERROR;
	}
	if (RTREAL != rbscale->restype ) {
		ads_fail("\nInvalid argument type for scale factor ! ");
		return RTERROR;
	}

	if (NULL == rbscale->rbnext
			|| NULL == (rbtype = rbscale->rbnext->rbnext)) {
		ads_fail("\nNot enough arguments! ");
		return RTERROR;
	}

	/* set options the way we like em */
	options.makeRif = 0;
	options.makeSun = 0;
	options.makeMats = 0;
	options.smooth = NULL;
	options.makeGeom = 1;

	/* list of type names to export */
	/* requires all items present, only correct spelling gets exported */
	for (i = 0, elist = validEntTypeList; *elist != NULL; elist++) {
		if (NULL == rbtype || RTLE == rbtype->restype) {
			ads_fail("Unexpected end of type list !");
			goto error;
		} else if (RTSTR == rbtype->restype) {
			if  (0 == strcmp(*elist, rbtype->resval.rstring)) {
				options.entTypes[i++] = StrDup(*elist);
			} /* if not "spelled correctly", skip type. */
		}
		rbtype = rbtype->rbnext;
	} /* We ignore any trailing names. */

	if (0 == (options.entTypesCount = i)) {
		/* nothing to do, so skip the rest ... */
		goto end;
	}

	/* input prefix must supply trailing "_" if required */
	if ('\0' != rbname->resval.rstring[0]) {
		strFREE(options.basename);
	    options.basename = StrDup(rbname->resval.rstring);
	}

	switch (rbxmode->resval.rstring[0]) {
		case 'c': case 'C':
			options.exportMode = EXP_BYCOLOR;
			strcpy(options.colorprefix, "c_");
			break;
		case 'l': case 'L':
			options.exportMode = EXP_BYLAYER;
			strcpy(options.layerprefix, "l_");
			break;
		default:
			/* hmmm, wasn't there a "toplayer" mode somewhere..? */
			ads_fail("Unknown export mode");
			goto error;
	}

	if (0.0 >= (options.distTolerance = rbdist->resval.rreal)) {
		ads_fail("Arc distance tolerance must be greater than zero! ");
		goto error;
	}
	if (0.0 >= (options.angleTolerance = (rbang->resval.rreal/180.0)*M_PI)) {
		ads_fail("Arc angle tolerance must be greater than zero! ");
		goto error;
	}

	if (0.0 >= (options.scaleFactor = rbscale->resval.rreal)) {
		ads_fail("Scale Factor must be greater than zero! ");
		goto error;
	}

	/* have Philip do the real work... */
	i = export();  /* doesn't really export anything ... hehe */

	/* return the list of exported files  */
	if (EXP_BYCOLOR == options.exportMode) return_color_mats();
	if (EXP_BYLAYER == options.exportMode) return_layer_mats();

	/* list destruction happens while writing, so we need to do this as */
	/* the last thing. Why not put destruction into RadoutFreeAll() ? */
	if (!WriteGeometry(&options)) {
		ads_fail("Error while trying to write geometry files !");
		goto error;
	}

end:
	options.basename = temp_basename;
	RadoutFreeAll();
	return(i);

error:
	options.basename = temp_basename;
	RadoutFreeAll();
	return RTERROR;
	
}

#ifndef PATH_MAX /* why doesn't limits.h work on solaris 2.4 ? */
#define PATH_MAX 1024
#endif


extern int
rf_osexec(struct resbuf *rb)
{
    char old_dir[PATH_MAX];
    int result = 0;
    char *exec_dir = NULL;
    char *cmdstr = NULL;
	int background = 0;

	/* Make sure that there are enough arguments */
    if ((rb == NULL) || (rb->rbnext == NULL)) {
		ads_fail("\nNot enough arguments.");
		return RTERROR;
    }

	/* Grab the directory argument */
    if (rb->restype == RTSTR) {
		exec_dir = (char *)rb->resval.rstring;
		rb = rb->rbnext;
    } else {
		ads_fail("\nWrong argument type.");
		return RTERROR;
    }
 
	/* Grab the command string */
    if (rb->restype == RTSTR) {
		cmdstr = (char *)rb->resval.rstring;
		rb = rb->rbnext;
    } else {
		ads_fail("\nWrong argument type.");
		return RTERROR;
    }
 

    if (NULL == getcwd(old_dir,PATH_MAX)) {
		ads_retint(errno);
		ads_printf("Could not determine current directory. %s\n",
						  strerror(errno));
		return RTNORM;
	}

	/* change directory if specified */
	if ('\0' != exec_dir[0]) {
		if (0 != (result = chdir(exec_dir))) {
			ads_retint(result);
			ads_printf("Could not change directory to \"%s\". %s\n",
							  exec_dir, strerror(errno));
			return RTNORM;
		}
	}

    result = system(cmdstr);
	ads_retint(result);

	if (result) {
		ads_printf("Couldn't execute system command \"%s\" (%d)\n",
							 cmdstr, result);
    }

	/* change directory back */
    if (0 != (result = chdir(old_dir))) {
		ads_retint(result);
		ads_printf("Could not change directory back to \"%s\". %s\n",
						  cmdstr, strerror(errno));
	}

    return RTNORM;
}



/* -------------------------------------------------------------------------- */
/*  eof */
