/* ADS_PERR.C
      Copyright (C) 1988-1992 by Autodesk, Inc.
      Permission to use, copy, modify, and distribute this software
      for any purpose and without fee is hereby granted, provided
      that the above copyright notice appears in all copies and that
      both that copyright notice and this permission notice appear in
      all supporting documentation.
      THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED
      WARRANTY.  ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR
      PURPOSE AND OF MERCHANTABILITY ARE HEREBY DISCLAIMED.

    ADS_PERR.C - ADS routine to print the error message
      associated with the value of "errno" upon failure of a call
      to AutoCAD.  This routine is meant to be invoked by other
      ADS programs, and contains a table of the current error
      messages as found in ol_errno.h.  This routine is meant to
      parallel the Unix perror() function. 

      ads_perror() is a Lisp-callable or invokable function that
      takes an optional single argument and prints the message on
      the AutoCAD command line.  The syntax is: 

      (ads_perror "string")

      The string is optional; if provided, it is printed on a
      separate line before the system error message.  To be of
      most use, the argument string should include the name of
      the program that incurred the error.  The error number is
      taken from the system variable "errno", which is set when
      most errors occur, but not cleared when non-erroneous calls
      are made. 
 */
#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#ifdef UNIX
	#include <unistd.h>
#endif
#include "cadcompat.h"


/* The error message table */

char *errmsg[] = {             /* ERRNO code */
    /*MSG1*/"Invalid symbol table name.",
    /*MSG2*/"Invalid name (entity or selection set) value.",
    /*MSG3*/"Exceeded max number of selection sets.",
    /*MSG4*/"Invalid selection set.",
    /*MSG5*/"Improper use of Block Definition entity.",
    /*MSG6*/"Improper use of Xref entity.",
    /*MSG7*/"Entity selection failure by pick.",
    /*MSG8*/"End of entity file.",
    /*MSG9*/"End of Block Definition file.",
    /*MSG10*/"Failure of eidlast.",
    /*MSG11*/"Illegal entdel of Viewport.",
    /*MSG12*/"Operation not allowed during PLINE.",
    /*MSG13*/"Invalid handle.",
    /*MSG14*/"Handles not enabled.",
    /*MSG15*/"Invalid TRANS request.",
    /*MSG16*/"Invalid space trans request.",
    /*MSG17*/"Invalid use of deleted entity.",
    /*MSG18*/"Invalid table name.",
    /*MSG19*/"Invalid table function argument.",
    /*MSG20*/"Attempt to set read-only variable.",
    /*MSG21*/"Zero value invalid.",
    /*MSG22*/"Value out of range.",
    /*MSG23*/"Complex regen in progress.",
    /*MSG24*/"Attempt to change entity type.",
    /*MSG25*/"Bad layer name.",
    /*MSG26*/"Bad linetype name.",
    /*MSG27*/"Bad color name.",
    /*MSG28*/"Bad text style name.",
    /*MSG29*/"Bad shape name.",
    /*MSG30*/"Bad field for entity type.",
    /*MSG31*/"Attempted entmod of deleted entity.",
    /*MSG32*/"Attempted entmod of SEQEND.",
    /*MSG33*/"Attempt to change handle.",
    /*MSG34*/"Illegal modification of viewport visibility.",
    /*MSG35*/"Entity on locked layer.",
    /*MSG36*/"Bad entity type.",
    /*MSG37*/"Bad PLINE entity.",
    /*MSG38*/"Incomplete complex entity in block.",
    /*MSG39*/"Invalid block name field.",
    /*MSG40*/"Duplicate block flag fields.",
    /*MSG41*/"Duplicate block name fields.",
    /*MSG42*/"Bad normal vector.",
    /*MSG43*/"Missing block name.",
    /*MSG44*/"Missing block flags.",
    /*MSG45*/"Invalid anonymous block.",
    /*MSG46*/"Invalid Block Definition entity.",
    /*MSG47*/"Mandatory field missing.",
    /*MSG48*/"Unrecognized extended data type.",
    /*MSG49*/"Improper nesting of list in Xdata.",
    /*MSG50*/"Improper location of APPID field.",
    /*MSG51*/"Exceeded maximum Xdata size.",
    /*MSG52*/"Entity selection failure by null response.",
    /*MSG53*/"Duplicate application name in Xdata.",
    /*MSG56*/"Attempt to make or modify Viewport entity.",
    /*MSG57*/"Attempt to make an Xref or dependent symbol.",
    /*MSG58*/"Bad ssget filter: unterminated clause.",
    /*MSG59*/"Bad ssget filter: missing test operand.",
    /*MSG60*/"Bad ssget filter: invalid test operation string.",
    /*MSG61*/"Bad ssget filter: empty clause or improper nesting.",
    /*MSG62*/"Bad ssget filter: begin/end clause mismatch.",
    /*MSG63*/"Bad ssget filter: wrong number of XOR/NOT operands.",
    /*MSG64*/"Bad ssget filter: maximum nesting level exceeded.",
    /*MSG65*/"Bad ssget filter: invalid group code.",
    /*MSG66*/"Bad ssget filter: invalid string test.",
    /*MSG67*/"Bad ssget filter: invalid vector test.",
    /*MSG68*/"Bad ssget filter: invalid real test.",
    /*MSG69*/"Bad ssget filter: invalid integer test.",
    /*MSG70*/"Digitizer isn't a tablet",
    /*MSG71*/"Tablet isn't calibrated.",
    /*MSG72*/"Invalid arguments to (TABLET) function.",
    /*MSG73*/"Not enough memory to allocate resbuf.",
    /*MSG74*/"NULL Pointer was provided as an argument.",
    /*MSG75*/"The specified file can't be opened.",
    /*MSG76*/"The specified application is already loaded.",
    /*MSG77*/"The maximum number of loaded ADS apps has been reached.",
    /*MSG78*/"The specified application could not be executed.",
    /*MSG79*/"The ADS app has an incompatible version number.",
    /*MSG80*/"The ADS app is active or nested and can't be unloaded.",
    /*MSG81*/"The ADS application refused to XUNLOAD.",
    /*MSG82*/"The specified ADS application is not loaded.",
    /*MSG83*/"Insuficient memory to load ADS application.",
    /*MSG84*/"Invalid transformation matrix.",
    /*MSG85*/"Invalid symbol name.",
    /*MSG86*/"Invalid symbol value.",
    /*MSG87*/"Operation not allowed while a dialogue box is active.",
};

#define ELEMENTS(array)(sizeof(array)/sizeof(array[0]))

int ads_perror()
{
    struct resbuf *argl, errval;

    argl = ads_getargs();
    if (ads_getfuncode() != 0)      /* This is the ONLY function here */
        return RSRSLT;
    if (ads_getvar("ERRNO", &errval) == RTERROR)
        return RSRSLT;
    if ((errval.resval.rint < 1) || (errval.resval.rint > ELEMENTS(errmsg)
#ifdef MAX_OL_ERRNO
		|| errval.resval.rint > MAX_OL_ERRNO
#endif
		))
        return RSRSLT;
    if (argl && argl->restype == RTSTR && strlen(argl->resval.rstring))
        ads_printf("%s: ", argl->resval.rstring);
    ads_printf("%s\n", errmsg[errval.resval.rint - 1]);
    ads_relrb(argl);
    ads_retvoid();
    return RSRSLT;
}
/*** end ads_perr.c ***/
