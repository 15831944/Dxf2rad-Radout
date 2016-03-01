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
#include "readdxf.h"
#include "convert.h"
#include "tables.h"

#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"
#include "writerad.h"


int id_index = 0;

static Matrix4 ScaleMatrix;
Acadvars_Type Acadvars = {
	0,   /* screenh */
	0,   /* screenv */
	0.0, /* pdsize  */
	0.0, /* viewsize */
};


void InitConvert(void)
{
	/* initialize a module wide scaling matrix  */
	M4SetIdentity(ScaleMatrix);
	if ((Options.scale != 0.0) && (Options.scale != 1.0))
		M4Scale(ScaleMatrix, Options.scale, Options.scale, Options.scale);
}


int GetInsertdefToWCS(InsertDef *insertdef, Matrix4 matrix)
{
	Matrix4 nxform;
	BlockDef *blockdef = insertdef->blockdef;

	if(insertdef == NULL) return -1;
	M4GetAcadXForm(matrix, &insertdef->zvect, 1,
		&insertdef->inspt, insertdef->zrot,
		insertdef->xscale, insertdef->yscale, insertdef->zscale,
		&blockdef->basept);

	while(insertdef->container) {
		insertdef = insertdef->container;
		blockdef = insertdef->blockdef;
		M4GetAcadXForm(nxform, &insertdef->zvect, 1,
			&insertdef->inspt, insertdef->zrot,
			insertdef->xscale, insertdef->yscale, insertdef->zscale,
			&blockdef->basept);
		M4MatMult(nxform, matrix, matrix);
	}
	return 0;
}


void TransformInsertContents(FILE *outf, InsertDef *insertdef)
{
	BlockDef *blockdef = insertdef->blockdef;
	Poly3 *newpoly = NULL, *curpoly = NULL;
	Cyl3 *newcyl = NULL, *curcyl = NULL;
	SimpleText *newtext = NULL;
	Matrix4 matrix;
	InsertDef *curins = NULL, *xcurins = NULL;
	char *blocklayer = NULL;
	int blockcolor = -1;

	/* get floating layer/color and check block recursion */
	for(curins = insertdef; curins; curins = curins->container) {
		if(curins != insertdef && curins->blockdef == blockdef) {
			fprintf(stderr, "Recursive block reference detected.\n");
			fprintf(stderr, "Skipping insert: %s", blockdef->name);
			for(xcurins = insertdef->container; xcurins;
					xcurins = xcurins->container) {
				fprintf(stderr, " - %s", xcurins->blockdef->name);
			}
			fprintf(stderr, " - WORLD\n");
			return;
		}
		if(curins->layer != Layer0 && blocklayer == NULL) {
			blocklayer = curins->layer;
		}
		if(curins->color != -1 && blockcolor == -1) {
			blockcolor = curins->color;
		}
	}
	if(blocklayer == NULL) blocklayer = Layer0;
	if(blockcolor == -1) blockcolor = 7; /* default white */

	/* get general transformation */
	GetInsertdefToWCS(insertdef, matrix);
	/* scale everything for output */
	if ((Options.scale != 0.0) && (Options.scale != 1.0))
		M4Scale(matrix, Options.scale, Options.scale, Options.scale);

	/* blah blah */
	if(Options.verbose > 1) {
		fprintf(stderr, "    Transforming: %s", blockdef->name);
		for(curins = insertdef->container; curins; curins = curins->container)
			fprintf(stderr, " >> %s", curins->blockdef->name);
		fprintf(stderr, " >> WORLD\n");
	}
	/* transform and write polys */
	if(blockdef->polys != NULL) {
		newpoly = M4TransformPolysCopy(blockdef->polys, matrix);
		/* XXX assumes BYLAYER  */
		/* fix up floating layers  */
		for(curpoly = newpoly; curpoly; curpoly = curpoly->next) {
			if(curpoly->material == Layer0) {
				curpoly->material = blocklayer;
			}
		}
		WritePoly(outf, NULL, id_index++, newpoly);
	}
	/* transform and write cyls */
	if(blockdef->cyls != NULL) {
		newcyl = M4TransformCylsCopy(blockdef->cyls, matrix);
		/* XXX assumes BYLAYER  */
		/* fix up floating layers  */
		for(curcyl = newcyl; curcyl; curcyl = curcyl->next) {
			if(curcyl->material == Layer0) {
				curcyl->material = blocklayer;
			}
		}
		WriteCyl(outf, NULL, id_index++, newcyl);
	}
	/* transform and write text */
	if(blockdef->texts != NULL) {
		/* refcopy dosen't copy the actual text data! */
		newtext = M4TransformSimpleTextRefcopy(blockdef->texts, matrix);
		WriteSimpleText(outf, newtext);
	}
	/* recurse down into child inserts */
	for(curins = blockdef->inserts; curins; curins = curins->next) {
		curins->container = insertdef;
		TransformInsertContents(outf, curins);
	}
}


void ConvertBlockStart(Block_Type Block)
{
	BlockDef *blockdef;

	if(Options.verbose > 1) {
		fprintf(stderr, "    Reading block definition: %s\n", Block.Name);
	}
	blockdef = GetBlockDef(Block.Name);
	if(blockdef == NULL) {
		blockdef = BlockAlloc(Block.Name);
		AddBlockDef(blockdef);
	}
	if(blockdef == NULL) return;

	blockdef->basept = Block.Base;
	CurrentBlockDef = blockdef;

	return;
}

void ConvertBlockEnd(Block_Type Block)
{
	CurrentBlockDef = NULL;
	return;
}

void get_screensize(View_Type View, double *vh, double *vv)
{
	double d_side, h_side, v_side, aspang;
	if(View.Mode & 1) { /* perspective view */
		d_side = 21.6333 / View.Lens;  /* diagonal of 24/36mm */
		aspang = atan(View.Height/View.Width);
		v_side = d_side * sin(aspang);
		h_side = d_side * cos(aspang);
		*vv = atan(v_side) * 360.0 / M_PI;
		*vh = atan(h_side) * 360.0 / M_PI;
	} else {
		*vv = View.Height;
		*vh = View.Width;
	}
}

void ConvertView(View_Type View)
{
	FILE *vf;
	char vfn[MAXPATH];
	double vlen = 0;
	double vo = 0, va = 0, vv, vh;
	Vector3 vp, vd, vu;
	Vector3 tmppt, xvect, target;
	Vector3 vnull = {0.0, 0.0, 0.0};
	Vector3 zunit = {0.0, 0.0, 1.0};
	Matrix4 matrix;

	if (Options.views == 0) return;
	if ((sizeof(vfn)
		- Options.viewprefixlen
		- strlen(View.Name)
		- 4) < 0) {
		fprintf(stderr,
			"Error: File name path for view \"%s\" too long. Skipping.\n",
			View.Name);
		return;
	}

	vlen = V3Length(&View.Direction);
	if(View.Mode & (2 + 16)){
		vo = vlen - View.Fclip;
	} else {
		vo = 0.0;
	}
	if(View.Mode & 4){
		va = vlen - View.Bclip;
	} else {
		va = 0.0;
	}
	if(View.Mode & 1){
		target = View.Target;
	} else {
		M4GetAcadXForm(matrix, &View.Center, 1, &View.Center, View.Twist,
			1.0, 1.0, 1.0, &vnull);
		M4MultPoint3(&View.Target, matrix, &target);
	}
	V3Cross(&zunit, &View.Direction, &xvect);
	if (V3EQUAL(vnull, xvect, FUZZ )) {
		/* looking straight down or up */
		vu.x = 0.0;
		vu.y = 1.0;
		vu.z = 0.0;
	} else {
		V3Cross(&View.Direction, &xvect, &vu);
	}
	V3Translate(&target, &View.Direction, 1.0, &vp);
	vd = View.Direction;
	V3Negate(&vd);
	V3Normalize(&vd);

	if(View.Twist != 0.0) {
		M4RotateAboutAxis(&vd, View.Twist, matrix);
		M4MultPoint3(&vu, matrix, &tmppt);
		vu = tmppt;
	}
	if(View.Twist == 0.0 && vu.z > 0.0) {
		/* simplify z vector to zenith */
		vu = zunit;
	}
	get_screensize(View, &vh, &vv);

	strncpy(vfn, Options.viewprefix, sizeof(vfn));
	strncat(vfn, View.Name, sizeof(vfn)-Options.viewprefixlen-4);
	strncat(vfn, ".vf", sizeof(vfn)-Options.viewprefixlen-1);
	vfn[sizeof(vfn)-1] = '\0'; /* paranoia */
	if(Options.verbose > 2) {
		fprintf(stderr, "      Writing view file \"%s\"\n", vfn);
	}
	errno = 0;
	vf = fopen(vfn, "wb");
	if(vf == NULL) {
		fprintf(stderr,"Error: Can't open file \"%s\" (E%d: %s). Skipping.\n",
			View.Name, errno, strerror(errno));
		return;
	}
	V3Scale(&vp, Options.scale);
	va *= Options.scale;
	vo *= Options.scale;
	if(!(View.Mode & 1)) {
		vh *= Options.scale;
		vv *= Options.scale;
	}
	fprintf(vf, "rpict -vt%c", (View.Mode & 1) ? 'v':'l');
	fprintf(vf, " -vp %g %g %g", vp.x, vp.y, vp.z);
	fprintf(vf, " -vd %g %g %g", vd.x, vd.y, vd.z);
	fprintf(vf, " -vu %g %g %g", vu.x, vu.y, vu.z);
	fprintf(vf, " -vh %g -vv %g -vo %g -va %g\n", vh, vv, vo, va);
	fclose(vf);
}


void ConvertFace(Face3D_Type Face,int Vertices)
{
	Poly3 *poly = NULL, *polys = NULL;
	char *layerdef = NULL;

	layerdef = GetLayerDef(Face.Layer);
	if(layerdef == NULL) return;
	if (Vertices < 3 )
		fprintf(stderr,"Warning: Too few vertices in face. Ignored.\n");
	else if (Vertices > 4)
		fprintf(stderr,"Warning: Too many vertices in face. Ignored.\n");
	else {
		if (Vertices == 3) {
			if ((poly = Poly3Alloc(3, 1, NULL)) == NULL)
				return;
			poly->verts[0] = Face.p[0];
			poly->verts[1] = Face.p[1];
			poly->verts[2] = Face.p[2];
		} else if (Vertices == 4) {
			if ((poly = Poly3Alloc(4, 1, NULL)) == NULL)
				return;
			poly->verts[0] = Face.p[0];
			poly->verts[1] = Face.p[1];
			poly->verts[2] = Face.p[2];
			poly->verts[3] = Face.p[3];
		}
		poly->material = layerdef;
		if (!PolyCheckColinear(poly) || (PolyGetArea(poly) <= 0.0)) {
			free(poly->verts);
			free(poly);
			return;
		}
		if ((Vertices == 4) && (Options.smooth || !FaceCheckCoplanar(poly))) {
			if ((polys = FaceSubDivide(poly)) != NULL) {
				Poly3Free(&poly);
				poly = polys;
			}
		}
		if(CurrentBlockDef != NULL) {
			BlockAddPoly(CurrentBlockDef, poly);
		} else {
			if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
				M4TransformPolys(poly, ScaleMatrix);
			}
			WritePoly(outf, Face.Layer, id_index++, poly);
		}
	}
}

void ConvertSmoothFace(Face3D_Type Face,Face3D_Type Normal,int Vertices)
{
	/* shortcut for now...  */
	ConvertFace(Face, Vertices);
}


void ConvertTextEntity(Text_Type Text)
{
	SimpleText *text;

	text = SimpleTextAlloc(Text.Text, NULL);
	text->position = Text.Location;
	if(CurrentBlockDef != NULL) {
		BlockAddText(CurrentBlockDef, text);
	} else {
		if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
			M4TransformSimpleTexts(text, ScaleMatrix);
		}
		WriteSimpleText(outf, text);
	}
}


void ConvertTraceEntity(Trace_Type Trace)
{
	Poly3 *poly = NULL;
	char *layerdef = NULL;
	Matrix4 mx;

	layerdef = GetLayerDef(Trace.Layer);
	if(layerdef == NULL) return;
	if ((poly = Poly3Alloc(4, 1, NULL)) == NULL)
		return;
	poly->verts[0] = Trace.p[0];
	poly->verts[1] = Trace.p[1];
	poly->verts[2] = Trace.p[2];
	poly->verts[3] = Trace.p[3];
	if (!PolyCheckColinear(poly) || (PolyGetArea(poly) <= 0.0)) {
		free(poly->verts);
		free(poly);
		return;
	}
	poly->material = layerdef;
	if(Trace.Thickness != 0.0) {
		poly->next = CopyPolyUp(poly, Trace.Thickness);
		poly->next->next = CreateSideWalls(poly, Trace.Thickness);
		poly->next->material = layerdef;
		poly->next->next->material = layerdef;
	}
	M4GetAcadXForm(mx, &Trace.Normal, 0, NULL, 0.0, 1.0, 1.0, 1.0, NULL);
	M4TransformPolys(poly, mx);
	if(CurrentBlockDef != NULL) {
		BlockAddPoly(CurrentBlockDef, poly);
	} else {
		if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
			M4TransformPolys(poly, ScaleMatrix);
		}
		WritePoly(outf, Trace.Layer, id_index++, poly);
	}
}


void ConvertInsertEntity(Insert_Type Insert)
{
	InsertDef *insertdef = NULL;
	char *layerdef = NULL;

	layerdef = GetLayerDef(Insert.Layer);
	if(layerdef == NULL) return;

	insertdef = InsertAlloc(Insert.Name);
	if(insertdef == NULL) return;

	insertdef->layer = layerdef;
	insertdef->inspt = Insert.Insertion;
	insertdef->zvect = Insert.Normal;
	insertdef->zrot = Insert.Rotation;
	insertdef->xscale = Insert.Scale.x;
	insertdef->yscale = Insert.Scale.y;
	insertdef->zscale = Insert.Scale.z;

	if(CurrentBlockDef != NULL) {
		BlockAddInsert(CurrentBlockDef, insertdef);
	} else {
		TransformInsertContents(outf, insertdef);
	}
}

void ConvertLineEntity(Line_Type Line)
{
	Poly3 *poly;
	char *layerdef = NULL;

	if(!Options.ignorethickness && Line.Thickness == 0.0) return;
	layerdef = GetLayerDef(Line.Layer);
	if(layerdef == NULL) return;	
	poly = Poly3Alloc(4, 1, NULL);
	if (poly == NULL) return;
	
	poly->verts[0] = Line.Start;
	poly->verts[1] = Line.End;
	if(Line.Thickness == 0.0) {
		poly->nverts = 2;
	} else {
		(void)V3Translate(&poly->verts[0], &Line.Normal,
						  Line.Thickness, &poly->verts[3]);
		(void)V3Translate(&poly->verts[1], &Line.Normal,
						  Line.Thickness, &poly->verts[2]);
	}
	poly->material = layerdef;
	
	if(CurrentBlockDef != NULL) {
		BlockAddPoly(CurrentBlockDef, poly);
	} else {
		if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
			M4TransformPolys(poly, ScaleMatrix);
		}
		WritePoly(outf, Line.Layer, id_index++, poly);
	}
}


void ConvertArcEntity(Arc_Type Arc)
{
	int i;
	Poly3 *poly, *arc, *polys;
	char *layerdef = NULL;
	Matrix4 mx;

	if(!Options.ignorethickness && Arc.Thickness == 0.0) return;
	layerdef = GetLayerDef(Arc.Layer);
	if(layerdef == NULL) return;	

	arc = SegmentArc(&Arc.Center, CW, Options.disttol, Options.angtol,
			Arc.Radius, Arc.Startangle, Arc.Endangle);
	if(arc == NULL) return;
	poly = Poly3Alloc(arc->nverts+2, 0, arc);
	if (poly == NULL) {
		Poly3FreeList(arc);
		return;
	}
	for(i = 0; i < arc->nverts; i++) {
		poly->verts[i+1] = arc->verts[i];
	}
	poly->material = layerdef;
	(void)V3AddPolar2D(&Arc.Center, Arc.Startangle, Arc.Radius,
					   &poly->verts[poly->nverts - 1]);
	(void)V3AddPolar2D(&Arc.Center, Arc.Endangle, Arc.Radius,
					   &poly->verts[0]);
	if (Arc.Thickness == 0) {
		polys = poly;
	} else {
		if ((polys = CreateSideWalls(poly, Arc.Thickness)) == NULL)
			return;
		Poly3FreeList(poly);
	}

	M4GetAcadXForm(mx, &Arc.Normal, 0, NULL, 0.0, 1.0, 1.0, 1.0, NULL);
	M4TransformPolys(polys, mx);
	if(CurrentBlockDef != NULL) {
		BlockAddPoly(CurrentBlockDef, polys);
	} else {
		if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
			M4TransformPolys(polys, ScaleMatrix);
		}
		WritePoly(outf, Arc.Layer, id_index++, polys);
	}
}


void ConvertCircleEntity(Circle_Type Circle)
{
	Cyl3 *cyl;
	Matrix4 mx;
	char *layerdef = NULL;

	layerdef = GetLayerDef(Circle.Layer);
	if(layerdef == NULL) return;	
	cyl = Cyl3Alloc(NULL);
	if (cyl == NULL) return;
	cyl->srad = cyl->erad = Circle.Radius;
	cyl->length = Circle.Thickness;
	
	cyl->normal = Circle.Normal;
	cyl->svert = Circle.Center;
	cyl->evert = Circle.Center;
	if (Circle.Thickness != 0) {
		cyl->evert.z += Circle.Thickness;
	} else {
		cyl->evert.z += Circle.Normal.z;
	}
	cyl->material = layerdef;

	/* transform points from ECS to next higher level CS */
	M4GetAcadXForm(mx, &Circle.Normal, 0, NULL, 0.0, 1.0, 1.0, 1.0, NULL);
	M4TransformCyls(cyl, mx);
	if(CurrentBlockDef != NULL) {
		BlockAddCyl(CurrentBlockDef, cyl);
	} else {
		if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
			M4TransformCyls(cyl, ScaleMatrix);
		}
		WriteCyl(outf, Circle.Layer, id_index++, cyl);
	}
}


void ConvertPointEntity(Point_Type Point)
{
	Cyl3 *cyl;
	char *layerdef = NULL;

	layerdef = GetLayerDef(Point.Layer);
	if(layerdef == NULL) return;	
	if(Point.Thickness == 0.0) Point.Thickness = Acadvars.pdsize;
	if(Point.Thickness == 0.0) return;
	cyl = Cyl3Alloc(NULL);
	if (cyl == NULL) return;

	cyl->srad = Point.Thickness;
	cyl->length = Point.Thickness;	
	cyl->svert = Point.Center;
	cyl->material = layerdef;

	if(CurrentBlockDef != NULL) {
		BlockAddCyl(CurrentBlockDef, cyl);
	} else {
		if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
			M4TransformCyls(cyl, ScaleMatrix);
		}
		WriteCyl(outf, Point.Layer, id_index++, cyl);
	}
}


void Convert3DFaceEntity(Face3D_Type Face3D)
{
	ConvertFace(Face3D,4);
}


void ConvertPline(PolyLine_Type Pline, Point3 Mesh[],
				double Bulges[])
{
	int i, jj, k;
	int vertnum = Pline.V_Count -1;
	int nvertnum = vertnum;
	Poly3 *poly = NULL;
	Poly3 *arcs = NULL, *curarc = NULL, *lastarc = NULL;
	Matrix4 mx;
	char *layerdef = NULL;

	layerdef = GetLayerDef(Pline.Layer);
	if(layerdef == NULL) return;
	for(i = 1; i <= vertnum; i++) {
		/* let's expand all the bulges first, so that we
			know how many vertices there will be */
		if(Bulges[i] != 0.0 
			&& (Pline.Type == et_POLYGON || i <= vertnum)) {
			int dir;
			Point3 center, *p2;
			Poly3 *arc;
			double radius, a1, a2;

			if(i == vertnum) p2 = &Mesh[1];
			else p2 = &Mesh[i+1];
			radius = BulgeToArc(&Mesh[i], p2, Bulges[i],
				&dir, &center, &a1, &a2);
			arc = SegmentArc(&center, dir, Options.disttol,
				Options.angtol, radius, a1, a2);
			nvertnum += arc->nverts;
			if(lastarc == NULL) {
				arcs = lastarc = arc;
			} else {
				lastarc->next = arc;
				lastarc = arc;
			}
		}
	}
	poly = Poly3Alloc(nvertnum, Pline.Flags & 1, NULL);
	poly->material = layerdef; /* points into table */
	curarc = arcs;
	for(i = 1, jj = 0; i <= vertnum; i++) {
		poly->verts[jj++] = Mesh[i];
		if(Bulges[i] != 0.0) {
			for(k = 0; k < curarc->nverts; k++)
				poly->verts[jj++] = curarc->verts[k];
			curarc = curarc->next;
		}
	}
	Poly3FreeList(arcs);

	/* wide pline, replace with new shape */
	if(Pline.Type == et_WPLINE) {
		Poly3 *wpoly;

		wpoly = WidePlist(poly, Pline.Width/2.0);
		Poly3FreeList(poly);
		poly = wpoly;
	}
	/* thick pline */
	if (Pline.Thickness != 0.0) {
		Poly3 *walls;

		walls = CreateSideWalls(poly, Pline.Thickness);
		if(Pline.Type == et_PLINE) {
			Poly3FreeList(poly);
			poly = walls;
		} else  { /* polygon and wpline are both closed */
			poly->next = CopyPolyUp(poly, Pline.Thickness);
			poly->next->next = walls;
		}
	}
	M4GetAcadXForm(mx, &Pline.Normal, 0, NULL, 0.0, 1.0, 1.0, 1.0, NULL);
	M4TransformPolys(poly, mx);

	if(CurrentBlockDef != NULL) {
		BlockAddPoly(CurrentBlockDef, poly);
	} else {
		if ((Options.scale != 0.0) && (Options.scale != 1.0)) {
			M4TransformPolys(poly, ScaleMatrix);
		}
		WritePoly(outf, Pline.Layer, id_index++, poly);
	}
}



void  ConvertMesh(PolyLine_Type PolyLine,Point3 Mesh[],
				Point3 Normals[], int Faces[][4],unsigned int VCount[])
{
	
	unsigned int i,j,N,M,N_wrap,M_wrap;
	Face3D_Type Face;
#ifdef WITH_SMOOTHING
	Face3D_Type Normal;
#endif
	
	strncpy(Face.Layer, PolyLine.Layer, MAXSTRING);
	if (PolyLine.Flags & 16 || PolyLine.Flags & 64) {
		
		N=PolyLine.N_Count,M=PolyLine.M_Count;
		if (PolyLine.Flags & 1)  M_wrap = 1; else M_wrap = 0;
		if (PolyLine.Flags & 32) N_wrap = 1; else N_wrap = 0;
		
		/* Polyface mesh: regular mesh  */
		if (PolyLine.Flags & 16) {
			for (i=1;i<N+N_wrap;i++) {
				for (j=1;j<M+M_wrap;j++) {
					Face.p[0] = Mesh[(j%M)*N+(i%N)+1];
					Face.p[1] = Mesh[(j-1)%M*N+(i%N)+1];
					Face.p[2] = Mesh[(j-1)%M*N+(i-1)%N+1];
					Face.p[3] = Mesh[(j%M)*N+(i-1)%N+1];
#ifdef WITH_SMOOTHING
					Normal.p[0] = Normals[(j%M)*N+(i%N)+1];
					Normal.p[1] = Normals[(j-1)%M*N+(i%N)+1];
					Normal.p[2] = Normals[(j-1)%M*N+(i-1)%N+1];
					Normal.p[3] = Normals[(j%M)*N+(i-1)%N+1];
					if (Options.smooth)
						ConvertSmoothFace(Face,Normal,4);
					else
#endif
						ConvertFace(Face,4);
				}
			}
		}
		/* Polygon mesh: irregular mesh  */
		else if (PolyLine.Flags & 64) {
			for (i=0;i<PolyLine.F_Count;i++) {
				for (j=0;j<VCount[i];j++) {
					Face.p[j] = Mesh[abs(Faces[i][j])];
#ifdef WITH_SMOOTHING
					Normal.p[j] = Normals[abs(Faces[i][j])];
#endif
				}
#ifdef WITH_SMOOTHING
				if (Options.smooth)
					ConvertSmoothFace(Face,Normal,VCount[i]);
				else
#endif
					ConvertFace(Face,VCount[i]);
			}
		}
		
	}
}
