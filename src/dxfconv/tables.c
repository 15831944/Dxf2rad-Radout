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

#include <string.h>
#include <stdlib.h>

#include "dlltypes.h"
#include "dllproto.h"
#include "geomtypes.h"

#include "tables.h"


static DLL_LIST BlockTable;
static DLL_LIST LayerTable;

BlockDef *CurrentBlockDef = NULL;
char *Layer0 = NULL;

int InitTables(void)
{
	DllSetup *BlockTableSetup;
	DllSetup *LayerTableSetup;

	BlockTableSetup = (DllSetup*)DllSetupList((int(*)(void*,void*))strcmp,
			NULL, NULL, NULL); /* we'll never deallocate any of this */
	if (!BlockTableSetup) {
        fprintf(stderr, "Can't allocate block table setup\n.");
		return -1;
	}
    if ((BlockTable = DllNewList(BlockTableSetup)) == NULL) {
        fprintf(stderr, "Can't allocate block table\n.");
        return -1;
    }
	LayerTableSetup = (DllSetup*)DllSetupList((int(*)(void*,void*))strcmp,
			NULL, NULL, NULL); /* we'll never deallocate any of this */
	if (!LayerTableSetup) {
        fprintf(stderr, "Can't allocate layer table setup\n.");
		return -1;
	}
    if ((LayerTable = DllNewList(LayerTableSetup)) == NULL) {
        fprintf(stderr, "Can't allocate layer table\n.");
        return -1;
    }
	Layer0 = AddLayerDef("l_0");
	return 0;
}

char *AddLayerDef(char *layer)
{
	char *newlayer = NULL;
	size_t len;

	len = strlen(layer);
	if(len <= 0) return NULL;

	newlayer = (char*)malloc(len+1);
	if(newlayer == NULL) return NULL;

	strncpy(newlayer, layer, len+1);
	DllInsert(LayerTable, newlayer, newlayer);
	return newlayer;
}


char *GetLayerDef(char *layer)
{	
	char *data = NULL;

	(void)DllSearch(LayerTable, layer, (void**)&data);
	if(data == NULL) {
		data = AddLayerDef(layer);
	}
	return data;
}


BlockDef *BlockAlloc(char *name)
{
	char *newname = NULL;
	size_t len;
	BlockDef *blockdef = NULL;

	len = strlen(name);
	if(len <= 0) return NULL;

	blockdef = (BlockDef *)malloc(sizeof(BlockDef));
	if(blockdef == NULL) return NULL;

	newname = (char*)malloc(len+1);
	strncpy(newname, name, len+1);
	if(newname == NULL) {
		free(blockdef);
		return NULL;
	}
	blockdef->name = newname;
	blockdef->inserts = NULL;
	blockdef->polys = NULL;
	blockdef->texts = NULL;
	blockdef->cyls = NULL;

	return blockdef;
}


void AddBlockDef(BlockDef *block)
{
	DllInsert(BlockTable, block->name, block);
}


BlockDef *GetBlockDef(char *name)
{	
	BlockDef *block = NULL;

	(void)DllSearch(BlockTable, name, (void**)&block);
	return block;
}

InsertDef *InsertAlloc(char *name)
{
	InsertDef *insertdef = NULL;
	BlockDef *blockdef = NULL;

	blockdef = GetBlockDef(name);
	if(blockdef == NULL) {
		/* the block may be defined lower down in the file */
		blockdef = BlockAlloc(name);
		AddBlockDef(blockdef);
	}
	if(blockdef == NULL) return NULL;

	insertdef = (InsertDef *)malloc(sizeof(InsertDef));
	if(insertdef == NULL) return NULL;

	insertdef->layer = NULL;
	insertdef->color = -1;
	insertdef->blockdef = blockdef;
	insertdef->next = NULL;
	insertdef->container = NULL;

	return insertdef;
}


int BlockAddInsert(BlockDef *block, InsertDef *insert)
{
	InsertDef *cur_id = insert;
	
	if(block == NULL) {
		return -1;
	}
	while(cur_id->next != NULL) {
		cur_id = cur_id->next;
	}
	cur_id->next = block->inserts;
	block->inserts = insert;

	return 0;
}


int BlockAddPoly(BlockDef *block, Poly3 *poly)
{
	Poly3 *cur_id = poly;
	
	if(block == NULL) {
		return -1;
	}
	while(cur_id->next != NULL) {
		cur_id = cur_id->next;
	}
	cur_id->next = block->polys;
	block->polys = poly;

	return 0;
}


int BlockAddCyl(BlockDef *block, Cyl3 *cyl)
{
	Cyl3 *cur_id = cyl;
	
	if(block == NULL) {
		return -1;
	}
	while(cur_id->next != NULL) {
		cur_id = cur_id->next;
	}
	cur_id->next = block->cyls;
	block->cyls = cyl;

	return 0;
}


int BlockAddText(BlockDef *block, SimpleText *text)
{
	SimpleText *cur_id = text;
	
	if(block == NULL) {
		return -1;
	}
	while(cur_id->next != NULL) {
		cur_id = cur_id->next;
	}
	cur_id->next = block->texts;
	block->texts = text;

	return 0;
}


