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

#if (!defined(lint) && !defined(SABER))
#endif

typedef enum _Boolean {
    False = 0, True = 1
} Boolean;
typedef struct resbuf ADSResBuf;
typedef struct _FuncEntry {
    char *funcName;
    int (*func)(ADSResBuf*);
} FuncEntry;

#define ELEMENTS(array)(sizeof(array)/sizeof(array[0]))
#define Cmdecho  False
#define CommandB()  { ADSResBuf rBc, rBb, rBu, rBh; \
    ads_getvar("CMDECHO", &rBc); \
    ads_getvar("BLIPMODE", &rBb); \
    ads_getvar("HIGHLIGHT", &rBh); \
    rBu.restype = RTSHORT; \
    rBu.resval.rint = (int)Cmdecho; \
    ads_setvar("CMDECHO", &rBu); \
    rBu.resval.rint = (int)False; \
    ads_setvar("BLIPMODE", &rBu); \
    ads_setvar("HIGHLIGHT", &rBu)

#define CommandE() ads_setvar("CMDECHO", &rBc); \
    ads_setvar("BLIPMODE", &rBb); \
    ads_setvar("HIGHLIGHT", &rBh); }

#ifdef __cplusplus
    }
#endif
/*** end adtools.h ***/
