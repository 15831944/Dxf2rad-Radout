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

#ifndef _CONVERT_H
#define _CONVERT_H
#ifdef __cplusplus
    extern "C" {
#endif

extern FILE *outf;

typedef struct {
	int screenh;
	int screenv;
	double pdsize;
	double viewsize;
} Acadvars_Type;
extern Acadvars_Type Acadvars;

typedef struct Opt_Def{
	int verbose;
	double scale;
	ExportMode exportmode;
	int etypes[et_LAST];
	double disttol;
	double angtol;
	int skipfrozen;
	int geom;
	char *prefix;
	size_t prefixlen;
	int views;
	char *viewprefix;
	size_t viewprefixlen;
	int smooth;
	int ignorepolywidth;
	int ignorethickness;
} Options_Type;

extern void InitConvert(void);
extern Options_Type Options;

void ConvertTextEntity(Text_Type);
void ConvertBlockStart(Block_Type);
void ConvertBlockEnd(Block_Type);
void ConvertView(View_Type);
void ConvertInsertEntity(Insert_Type);
void ConvertLineEntity(Line_Type);
void ConvertArcEntity(Arc_Type);
void ConvertCircleEntity(Circle_Type);
void ConvertPointEntity(Point_Type);
void Convert3DFaceEntity(Face3D_Type);
void ConvertTraceEntity(Trace_Type);
void ConvertMesh(PolyLine_Type,
				Point3[],Point3[],
				int[][4],unsigned int[]);
void ConvertPline(PolyLine_Type, Point3[],
				double[]);


#ifdef __cplusplus
    }
#endif
#endif /* _CONVERT_H */
