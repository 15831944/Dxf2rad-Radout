/*
This file is part of

* Radout  - Export geometry from Autocad to Radiance scene files.


The MIT License (MIT)

Copyright (c) 1999-2016 Georg Mischler
(Originally acquired from Philip Thompson)

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


/* dialog.c */
extern int RadoutDLG (void);
/* geom.c */
extern Poly3 *PFaceToPoly (ADSResBuf *edata);
extern Poly3 *TraceToPoly (ADSResBuf *edata);
extern Poly3 *ArcToPoly (ADSResBuf *edata);
extern Poly3 *MeshToPoly (ADSResBuf *edata);
extern Poly3 *LineToPoly (ADSResBuf *edata);
extern Poly3 *PlineToPoly (ADSResBuf *edata, int typ);
extern Poly3 *LWPolyToPoly (ADSResBuf *edata, int typ);
extern Poly3 *FaceToPoly (ADSResBuf *edata);
/* lists.c */
extern int MakeExportLists (ExportMode mode);
extern int AddEntToLayerLists (DLL_LIST blist, char *layer);
extern char *GetEntType (ADSResBuf *rb, char *type);
extern int SampleEnts (ads_name selset, ExportMode expMode);
extern int SampleBlocksLists (DLL_LIST exportlists);
extern void GetEntXForm (ADSResBuf *edata, Matrix4 mx);
extern Poly3 *TransformEnt (Poly3 *polys, ADSResBuf *edata);
/* main.c */
extern int RadoutSetup (void);
extern void RadoutFreeAll (void);
#ifndef R15
extern int main (int argc, char *argv[]);
#endif
extern int export (void);
/* radout.c */
extern int WriteGeometry (Options *opts);
extern int WriteMatsAndRif (Options *opts);
extern int WriteDaylight (Options *opts);
/* smooth.c */
extern void IncludePolygon (int nverts, Point3 *verts, Smooth smooth);
extern void SmoothSetPolyList (Poly3 *polys, Smooth smooth);
extern void EnableEdgePreservation (Smooth smooth, double minDot);
extern void DisableEdgePreservation (Smooth smooth);
extern void SetFuzzFraction (Smooth smooth, double fuzzFraction);
extern Smooth InitSmooth (void);
extern void SmoothMakeVertexNormals (Smooth smooth);
extern void SmoothFreeData (Smooth smooth);
extern void SmoothFreeAll (Smooth smooth);
extern void WriteBaryTriangle (FILE *fp, char *matName, int num, int cnt, Poly3 *poly);
/* utils.c */
extern ads_namep NewADSName (ads_name name);
extern void *CopyNameProc (void *data, void *info);
extern int CompIntKey (void *key1, void *key2);
extern int CompStrKey (void *key1, void *key2);
extern void FreePtrProc (void *ptr, void *info);
extern void FreeStrProc (void *ptr, void *info);
extern void FreeListProc (void *data, void *info);
extern void PrintStrProc (void *key, void *data, void *info);
extern void PrintPtrProc (void *key, void *data, void *info);
extern void PrintNameProc (void *key, void *data, void *info);
extern void PrintListListsProc (void *key, void *data, void *info);
extern void PrintLayerListsProc (void *key, void *data, void *info);
extern int BinarySearch (char *name, int l, int h, char *array[]);
extern int GetColorVal (ADSResBuf *data, DLL_LIST contblks, int level);
extern int GetLayerVal (char layer[], ADSResBuf *data, DLL_LIST contblks, int level);
extern char *StripPrefix (char *name);
extern void RegulateName (char *name);


#ifdef __cplusplus
    }
#endif
