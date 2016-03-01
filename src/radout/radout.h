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

/* radout.h */

#ifdef __cplusplus
    extern "C" {
#endif

#include "dlltypes.h"

/* Our export mode */
typedef enum _ExportMode { EXP_BYCOLOR, EXP_BYLAYER } ExportMode;

/* WARNING: Must reflect the validEntTypeList elements in utils.c */
#define TYPECOUNT 16

typedef struct HashNodestruct {
    Poly3 *polygon;     /* polygon for this vertex */
    int vertexNum;      /* which vertex this is */
    int marked;         /* vertex status */
    struct HashNodestruct *next;
} HashNodeStruct, *HashNode;

#define HASH_TABLE_SIZE        1000

typedef struct SmoothStruct {
    HashNode hashTable[HASH_TABLE_SIZE];
    Poly3 *polygonTable;
    Poly3 *polyTail;
    double fuzz;            /* distance for vertex equality */
    double fuzzFraction;    /* fraction of model size for fuzz */
    int edgeTest;           /* apply edging test using minDot */
    double minDot;          /* if > this, make sharp edge; see above */
} SmoothStruct, *Smooth;


typedef struct _Options {
    char dwgName[160], *basename;
	char colorprefix[8], layerprefix[8];
    char *entTypes[TYPECOUNT];
    int entTypesCount;
    ExportMode exportMode;
    int makeRif, makeSun, makeGeom, makeMats;
    double distTolerance, angleTolerance, scaleFactor, sunlight[6];
    Smooth smooth;
} Options;


/* defines for the exterior sun */
#define MONTH   0
#define DAY     1
#define HOUR    2
#define LONG    3
#define LAT     4
#define TZ      5

#define MAXCOLORS 256    /* 0 - 255 */
#define BYBLOCK 0
#define BYLAYER 256
#define RED     1
#define YELLOW  2
#define GREEN   3
#define CYAN    4
#define BLUE    5
#define MAGENTA 6
#define WHITE   7
#define DIST_TOLERANCE 0.1
#define ANGLE_TOLERANCE (15.0/180.0)*M_PI
#define SMOOTH_TOLERANCE 0.001
#define XtNumber(arr)   ((int)(sizeof(arr)/sizeof(arr[0])))
#define FREE(x)  if (x) free(x), x = (void*)NULL

/* prototypes required for main.cpp on NT  */
extern Cyl3 *CircleToCyl(ADSResBuf *edata);
extern Cyl3 *PointToCyl(ADSResBuf *edata);
extern Poly3 *TransformPolyToWCS(Poly3 *polys, DLL_LIST contblks);
extern Cyl3 *TransformCylToWCS(Cyl3 *cyls, DLL_LIST contblks);

#if defined(R14)||defined(R15)
  /* we allocate all strings on the autocad heap
     to make sure that we never crash in ads_relrb() */
  #define strFREE(x) if(x)acad_free(x),x=((char*)NULL)
#else
  #define strFREE(x) if(x)free(x),x=((char*)NULL)
#endif /* defined(R14)||defined(R15) */


#ifdef __cplusplus
    }
#endif
