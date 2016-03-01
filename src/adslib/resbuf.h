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

#ifdef __cplusplus
    extern "C" {
#endif
#ifndef _RESBUF_H
#define _RESBUF_H 1


#include "geomtypes.h"

/* resbuf.c */
extern ADSResBuf *RBSearch (register ADSResBuf *rb, register int gcode);
extern int RBPrint (FILE *fp, ADSResBuf *rb);
extern void RBPrintList (FILE *fp, ADSResBuf *rb);
extern int PrintEntity (FILE *fp, ads_name name);
extern void RBPrintHead (FILE *fp, ADSResBuf *rb);
extern int PrintEntityHeader (FILE *fp, ads_name name);
extern int GetADSPointVar (char *str, ads_point point);
extern Point3 *GetPoint3Var (char *str, Point3 *point);
extern char *GetStrVar (char *str, char *resstr);
extern int GetIntVar (int *val, char *str);
extern int GetDoubleVar (double *real, char *str);
extern void SetPointVar (char *str, ads_point point, int dimflag);
extern void SetIntVar (char *str, int val);
extern ADSResBuf *RBGetName (ads_name ename, ADSResBuf *rb, int gcode);
extern char *RBGetNewStr (ADSResBuf *rb, int gcode);
extern ADSResBuf *RBGetStr (char *str, ADSResBuf *rb, int gcode);
extern ADSResBuf *RBGetInt (int *ival, ADSResBuf *rb, int gcode);
extern ADSResBuf *RBGetDouble (double *rval, ADSResBuf *rb, int gcode);
extern ADSResBuf *RBGetADSPoint (ads_point point, ADSResBuf *rb, int gcode);
extern ADSResBuf *RBGetPoint3 (Point3 *point, ADSResBuf *rb, int gcode);

#endif /* _RESBUF_H */
#ifdef __cplusplus
    }
#endif

