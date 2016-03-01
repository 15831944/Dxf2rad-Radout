/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.


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

#ifndef _BLOCKS_H
#define _BLOCKS_H
#ifdef __cplusplus
    extern "C" {
#endif

#include "geomtypes.h"

#define MAXSTRING 256

typedef struct _BlockDef *BlockDefPtr;
typedef struct _InsertDef *InsertDefPtr;

typedef struct _InsertDef{
	BlockDefPtr blockdef;
	char *layer;
	int color;
	Point3 inspt;
	Point3 zvect;
	double zrot;
	double xscale;
	double yscale;
	double zscale;
	InsertDefPtr next;
	InsertDefPtr container;
} InsertDef;


typedef struct _BlockDef{
	Point3 basept;
	InsertDefPtr inserts;
	Poly3 *polys;
	Cyl3  *cyls;
	SimpleText *texts;
	char *name;
} BlockDef;

extern BlockDef *CurrentBlockDef;
extern char *Layer0;

extern int InitTables(void);
extern char *AddLayerDef(char *layer);
extern char *GetLayerDef(char *layer);
extern void AddBlockDef(BlockDef *block);
extern BlockDef *GetBlockDef(char *name);
extern BlockDef *BlockAlloc(char *name);
extern InsertDef *InsertAlloc(char *name);
extern int BlockAddInsert(BlockDef *block, InsertDef *insert);
extern int BlockAddPoly(BlockDef *block, Poly3 *poly);
extern int BlockAddCyl(BlockDef *block, Cyl3 *cyl);
extern int BlockAddText(BlockDef *block, SimpleText *text);


#ifdef __cplusplus
    }
#endif
#endif /* BLOCKS_H */
