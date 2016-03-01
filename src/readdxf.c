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
#include <stdio.h>
#include <ctype.h>

#include "geomtypes.h"
#include "geomdefs.h"
#include "geomproto.h"
#include "readdxf.h"
#include "convert.h"


extern FILE *infp;

#define IS_SAMEPT(p,q) (((p.x)==(q.x))&&((p.y)==(q.y))&&((p.z)==(q.z)))


Point3 ZeroVector = {0.0, 0.0, 0.0};
Point3 ZUnit = {0.0, 0.0, 1.0};

#ifdef WITH_SMOOTHING
Point3 Unit(Point3);
int   operator ==(Point3,Point3);
Point3 operator +(Point3,Point3);
Point3 operator -(Point3,Point3);
Point3 operator *(Point3,Point3);
Point3 operator *(double,Point3);
#endif


/* read in line, accept \n, \r\n, and \r  */
char *
fgetline(char *s, int n, FILE *fp) 
{
	char  *cp = s;
	int  c = EOF;

	while (--n > 0 && (c = getc(fp)) != EOF) {
		if (c == '\r' && (c = getc(fp)) != '\n') {
			ungetc(c, fp);  /* must be Apple file */
			c = '\n';
		}
		if (c == '\n') {
			*cp++ = c;
			break;
		}
		*cp++ = c;
	}
	if (cp == s && c == EOF)
		return(NULL);
	
	*cp = '\0';
	return(s);
}


int
next_group(FILE *fp, Group_Type *m)
{
	static char buffer[MAXLINE];
	char *cp;
	size_t ccount, dcount, dumpcount;

	cp = fgetline((char*)&buffer, MAXLINE, fp);
	if(feof(fp)) return -1;
	ccount = strlen(cp);
	if(buffer[ccount-1] != '\n') {
		fprintf(stderr,
			"Error: Max line length exceeded on code line %d - exiting.\n",
				m->line);
		exit(-1);
	}
	m->code = atoi(cp);
	m->line++;
	
	cp = fgetline((char*)&m->value, MAXLINE, fp);
	if(feof(fp)) return -1;
	dcount = strlen(cp);
	/* fprintf(stderr, "newline: [%c]\n", m->value[dcount-1]); */
	if(m->value[dcount-1] != '\n') {
		fprintf(stderr,
			"Warning: Max line length exceeded on data line %d - truncating.\n",
				m->line);
		/*exit(-1);*/
		dumpcount = dcount;
		while(buffer[dumpcount-1] != '\n') {
			cp = fgetline((char*)&buffer, MAXLINE, fp);
			if(feof(fp)) return -1;
			dumpcount = strlen(cp);
			/*fprintf(stderr, "truncating %ld chars (%s).\n", dumpcount, cp);*/
		}
	}
	for(dcount--; dcount >= 0; dcount --) { /* remove spurious trailing stuff */
		if(isspace(m->value[dcount])) {
			m->value[dcount] = '\0';
		}
		else break;
	}
	m->line++;
	return 0;
}


/*  Transpose certain characters to create an acceptable primitive id.
 */
void RegulateName(char *name)
{
	char *ptr;

	for (ptr = name; *ptr != '\0';  ptr++) {
		if(!isalnum(*ptr) && (*ptr != '.') && (*ptr != '-'))
			*ptr = '_';
		else
			*ptr = (char)tolower(*ptr);
	}
}

#define READ_THICKNESS(X)  case 39: \
	X.Thickness = Options.ignorethickness ? 0.0 : atof(Group.value); break
#define READ_WIDTH(X)  case 40: \
	X.Width = Options.ignorepolywidth ? 0.0 : atof(Group.value); break
#define READ_FLAGS(X)      case 70:  X.Flags = atoi(Group.value); break
#define READ_ROTATION(C,X) case C:   X = atof(Group.value); X *= DEG2RAD; break
#define READ_DOUBLE(C,X)   case C:   X = atof(Group.value); break
#define READ_INT(C,X)      case C:   X = atoi(Group.value); break

#define READ_TEXT(X,C)\
	case C:\
		strncpy(X, Group.value,sizeof(X));\
		X[sizeof(X)-1] = '\0';\
		break

#define READ_NAME(X)\
	case 2:\
		strncpy(X.Name, Group.value,sizeof(X.Name));\
		X.Name[sizeof(X.Name)-1] = '\0';\
		break

#define READ_COORDINATE(X,C1,C2,C3)\
	case C1:   X.x = atof(Group.value); break;\
	case C2:   X.y = atof(Group.value); break;\
	case C3:   X.z = atof(Group.value); break

#define READ_ENTITY_OPTIONAL(X)\
	case 8:\
		if(Options.prefixlen) strncpy(X.Layer,Options.prefix,sizeof(X.Layer));\
		else X.Layer[0] = '\0';\
		strncat(X.Layer, Group.value, MAXSTRING - Options.prefixlen - 1);\
		X.Layer[sizeof(X.Layer)-1] = '\0';\
		RegulateName(X.Layer);\
		break;\
	case 5:\
		strncpy(X.Handle, Group.value,sizeof(X.Handle));\
		X.Handle[sizeof(X.Handle)-1] = '\0';\
		break;\
	case 62:  X.Colour = atoi(Group.value); break

#define READ_ENTITY_NORMAL(X)\
	case 210: X.x = atof(Group.value); break;\
	case 220: X.y = atof(Group.value); break;\
	case 230: X.z = atof(Group.value); break


Group_Type    Group;                   /* A group : Code and value  */

Text_Type     Text;                    /* A line of text  */
View_Type     View;                    /* A view  */
Vertex_Type   Vertex;                  /* A vertex  */
Line_Type     Line;                    /* A line  */
Arc_Type      Arc;                     /* An arc  */
Circle_Type   Circle;                  /* A circle/cylinder  */
Point_Type    Point;                   /* A point (sphere)  */
Face3D_Type   Face3D;                  /* A 3D face  */
Trace_Type    Trace;                   /* A Trace or 2D-Solid  */
PolyLine_Type PolyLine;                /* A polygon/polyface mesh  */
Block_Type    Block;                   /* A block  */
Insert_Type   Insert;                  /* An insert  */

/* we dynamically allocate memory for polyline vertex data,
   grow it when needed, but never shrink or release it */
typedef int xface[4];
size_t mesh_size = 0;
Point3        *Mesh = NULL;           /* vertices  */
double        *Bulges = NULL;         /* bulge per vertex   */
size_t faces_size = 0;
xface	      *Faces = NULL;          /* faces (3 or 4 vertices each)  */
unsigned int           *VCount = NULL;         /* vertice count per face  */
xface         Fc;
size_t normals_size = 0;
Point3	      *Normals = NULL;        /* vertice normals  */

/* ------------------------------------------------------------------------ */


/* The exclude lists contains names of blocks and inserts that must be  */
/* ignored.  */

#define ExcludeListSize 5
char *ExcludeList[ExcludeListSize] =
{"AME_SOL","AME_NIL","AME_FRZ","AVE_RENDER","AME_JNK"};

/* Checks if name is in the exclude list. Excluded names are also those  */
/* that begin with *.  */

int InExcludeList(char *Name) {
	int i;
	
	if (Name[0] == '*') return 1;
	for(i=0;i<ExcludeListSize;i++)
		if (strcmp(Name,ExcludeList[i]) == 0) return 1;
		return 0;
}


/* ------------------------------------------------------------------------ */
void IgnoreSection()	/* Ignore everything  */
{
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value,ENDSEC)  != 0)
			&& (strcmp(Group.value,SECTION) != 0)))) {
		next_group(infp, &Group);
	}
}

/* ------------------------------------------------------------------------ */

void HeaderSection()	/* Ignore everything except $PDSIZE  */
{
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value,ENDSEC)  != 0)
			&& (strcmp(Group.value,SECTION) != 0)))) {
		if(Group.code == 9 && Group.value[1] == 'P') {
			if(strcmp(Group.value, "$PDSIZE") == 0) {
				next_group(infp, &Group);
				if(Group.code == 40) Acadvars.pdsize = atof(Group.value);
			}
		}
		next_group(infp, &Group);
	}
}

/* ------------------------------------------------------------------------ */

void findTable()
{
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value, TABLE)  != 0)
			&& (strcmp(Group.value, ENDSEC) != 0)
			&& (strcmp(Group.value, SECTION)!= 0)))) {
		next_group(infp, &Group);
	}
}

void findVport()
{
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value, VPORT)   != 0)
			&& (strcmp(Group.value, TABLE)  != 0)
			&& (strcmp(Group.value, ENDTAB) != 0)
			&& (strcmp(Group.value, ENDSEC) != 0)
			&& (strcmp(Group.value, SECTION)!= 0)))) {
		next_group(infp, &Group);
	}
}

void readVport()
{
	double viewaspect = 0.0;
	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_NAME(View); /* "*Active" */
			READ_COORDINATE(View.Center,12,22,32);
			READ_COORDINATE(View.Direction,16,26,36);
			READ_COORDINATE(View.Target,17,27,37);
			READ_DOUBLE(40, View.Height);
			READ_DOUBLE(41, viewaspect);
			READ_DOUBLE(42, View.Lens);
			READ_DOUBLE(43, View.Fclip);
			READ_DOUBLE(44, View.Bclip);
			READ_ROTATION(51, View.Twist);
			READ_INT(71, View.Mode);
		}
		next_group(infp, &Group);
	}
	View.Width = View.Height * viewaspect;
	View.Center.z = 0.0;
	ConvertView(View);
}

void findView()
{
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value, VIEW)   != 0)
			&& (strcmp(Group.value, TABLE)  != 0)
			&& (strcmp(Group.value, ENDTAB) != 0)
			&& (strcmp(Group.value, ENDSEC) != 0)
			&& (strcmp(Group.value, SECTION)!= 0)))) {
		next_group(infp, &Group);
	}
}

void readView()
{
	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_NAME(View);
			READ_COORDINATE(View.Center,10,20,30);
			READ_COORDINATE(View.Direction,11,21,31);
			READ_COORDINATE(View.Target,12,22,32);
			READ_DOUBLE(40, View.Height);
			READ_DOUBLE(41, View.Width);
			READ_DOUBLE(42, View.Lens);
			READ_DOUBLE(43, View.Fclip);
			READ_DOUBLE(44, View.Bclip);
			READ_ROTATION(50, View.Twist);
			READ_INT(71, View.Mode);
		}
		next_group(infp, &Group);
	}
	View.Center.z = 0.0;
	ConvertView(View);
}

#define T_NONE 0
#define T_VPORT 1
#define T_VIEW 2

void TablesSection()
{
	int tablesEnd = 0;
	int tableSection = 0;
	int entryRead = 0;

	next_group(infp, &Group);
	while(!feof(infp) && tablesEnd == 0) {
		findTable();
		if(strcmp(Group.value,ENDSEC)  == 0) {
			tablesEnd = 1;
		} else if(strcmp(Group.value,SECTION) == 0) {
			tablesEnd = 1;
		} else if(strcmp(Group.value,TABLE)  == 0) {
			next_group(infp, &Group);
			if(Group.code == 2) {
				if(Options.verbose > 1) {
					fprintf(stderr, "    Reading table: %s\n", Group.value);
				}
				if(strcmp(Group.value,VIEW) == 0) {
					tableSection = T_VIEW;
				/* Unfortunately, we can't determine the "current" viewport */
				/*} else if(strcmp(Group.value,VPORT) == 0) {
					tableSection = T_VPORT;*/
				}
				if (tableSection) {
					while(!feof(infp) && tablesEnd == 0) {
						entryRead = 0;
						/*
						if(tableSection == T_VPORT) {
							findVport();
							if(strcmp(Group.value,VPORT)  == 0) {
								readVport();
								entryRead = 1;
							}
						}
						*/
						if(tableSection == T_VIEW) {
							findView();
							if(strcmp(Group.value,VIEW)  == 0) {
								readView();
								entryRead = 1;
							}
						}
						if(entryRead) {
							;
						} else if(strcmp(Group.value,ENDTAB)  == 0) {
							break;
						} else if(strcmp(Group.value,TABLE)  == 0) {
							break;
						} else if(strcmp(Group.value,ENDSEC)  == 0) {
							tablesEnd = 1;
							break;
						} else if(strcmp(Group.value,SECTION) == 0) {
							tablesEnd = 1;
							break;
						} else { /* huh ??  */
							next_group(infp, &Group);
							break;
						}
					}
				}
				tableSection = T_NONE;
			}
		} else { /* huh ?  */
			next_group(infp, &Group);
			break;
		}
	}
	/* Read rest  */
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value, ENDSEC)  != 0)
			&& (strcmp(Group.value, SECTION) != 0)))) {
		next_group(infp, &Group);
	}
}


/* ------------------------------------------------------------------------ */


void ReadText()
{
	Text.Colour = -1;

	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Text);
			READ_TEXT(Text.Text,1);
			/* READ_FLAGS(Text); */
			READ_COORDINATE(Text.Location,10,20,30);
		}
		next_group(infp, &Group);
	}		
}

void ReadLine()
{
	Line.Colour = -1;
	Line.Thickness = 0;
	Line.Normal = ZUnit;
	
	next_group(infp, &Group); /* skip group 0 */
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Line);
			READ_THICKNESS(Line);
			READ_COORDINATE(Line.Start,10,20,30);
			READ_COORDINATE(Line.End,11,21,31);
			READ_ENTITY_NORMAL(Line.Normal);
		}
		next_group(infp, &Group);
	}
}


void ReadArc()
{
	Arc.Colour = -1;
	Arc.Thickness = 0;
	Arc.Normal = ZUnit;
	
	next_group(infp, &Group); /* skip group 0 */
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Arc);
			READ_COORDINATE(Arc.Center,10,20,30);
			READ_ENTITY_NORMAL(Arc.Normal);
			READ_THICKNESS(Arc);
			READ_DOUBLE(40, Arc.Radius);
			READ_ROTATION(50, Arc.Startangle);
			READ_ROTATION(51, Arc.Endangle);
		}
		next_group(infp, &Group);
	}
}


void ReadCircle()
{
	Circle.Colour = -1;
	Circle.Thickness = 0;
	Circle.Normal = ZUnit;
	
	next_group(infp, &Group); /* skip group 0 */
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Circle);
			READ_THICKNESS(Circle);
			READ_DOUBLE(40, Circle.Radius);
			READ_COORDINATE(Circle.Center,10,20,30);
			READ_ENTITY_NORMAL(Circle.Normal);
		}
		next_group(infp, &Group);
	}
}


void ReadPoint()
{
	Point.Colour = -1;
	Point.Thickness = 0.0;
	
	next_group(infp, &Group); /* skip group 0 */
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Point);
			READ_THICKNESS(Point);
			READ_COORDINATE(Point.Center,10,20,30);
		}
		next_group(infp, &Group);
	}
}


void Read3DFace()
{
	Face3D.Colour = -1;
	
	next_group(infp, &Group); /* skip group 0 */
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Face3D);
			READ_COORDINATE(Face3D.p[0],10,20,30);
			READ_COORDINATE(Face3D.p[1],11,21,31);
			READ_COORDINATE(Face3D.p[2],12,22,32);
			READ_COORDINATE(Face3D.p[3],13,23,33);
		}
		next_group(infp, &Group);
	}
}


void ReadTrace()
{
	Trace.Colour = -1;
	Trace.Thickness = 0.0;
	Trace.Normal =  ZUnit;
	
	next_group(infp, &Group); /* skip group 0 */
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Trace);
			READ_COORDINATE(Trace.p[0],10,20,30);
			READ_COORDINATE(Trace.p[1],11,21,31);
			READ_COORDINATE(Trace.p[3],12,22,32);
			READ_COORDINATE(Trace.p[2],13,23,33);
			READ_ENTITY_NORMAL(Trace.Normal);
			READ_THICKNESS(Trace);
		}
		next_group(infp, &Group);
	}
}

/* ------------------------------------------------------------------------ */

void ReadVertex() /* PolyLine vertices only  */
{
	int i;
	
	Vertex.Colour   = -1;
	Vertex.Flags    = 0;
	Vertex.VCount   = 0;
	Vertex.Bulge    = 0.0;
	Vertex.Location = ZeroVector;
	for(i=0;i<4;i++) Vertex.Face[i] = 0;
	
	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Vertex);
			READ_DOUBLE(42, Vertex.Bulge);
			READ_FLAGS(Vertex);
		case 71: Vertex.Face[0] = atoi(Group.value); Vertex.VCount++; break;
		case 72: Vertex.Face[1] = atoi(Group.value); Vertex.VCount++; break;
		case 73: Vertex.Face[2] = atoi(Group.value); Vertex.VCount++; break;
		case 74: Vertex.Face[3] = atoi(Group.value); Vertex.VCount++; break;
			READ_COORDINATE(Vertex.Location,10,20,30);
		}
		next_group(infp, &Group);
	}
}

/* Read in a polyline. If the polyline represents a 3D shape then read in  */
/* the following vertices.  */

void ReadLWPolyLine() {
	int use_vertex = 1;
	
	PolyLine.Colour = -1;
	PolyLine.Width = 0.0;
	PolyLine.Thickness = 0.0;
	PolyLine.Elevation = 0.0;
	PolyLine.Normal =  ZUnit;
	PolyLine.Flags = 0;
	PolyLine.Type = et_PLINE; /* default: simple pline */
	PolyLine.M_Count = 0;
	PolyLine.N_Count = 0;
	PolyLine.V_Count = 0;
	PolyLine.F_Count = 0;

	if(Mesh == NULL) {
		mesh_size = CHUNKSIZE;
		Mesh = malloc(sizeof(Point3) * CHUNKSIZE);
		Bulges = malloc(sizeof(double) * CHUNKSIZE);
	} else {
		mesh_size = sizeof(Mesh) / sizeof(Point3);
	}

	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(PolyLine);
			READ_ENTITY_NORMAL(PolyLine.Normal);
			READ_DOUBLE(38, PolyLine.Elevation);
			READ_THICKNESS(PolyLine);
			/*READ_DOUBLE(40, PolyLine.Width);*/
			READ_WIDTH(PolyLine);
			READ_FLAGS(PolyLine);
		case 10: /* count starts with 1: preincrement */
			if(PolyLine.V_Count + 2 > mesh_size) {
				Point3 *new_mesh;
				double *new_bulges;
				mesh_size += CHUNKSIZE;
				new_mesh = realloc(Mesh, sizeof(Point3) * mesh_size);
				new_bulges = realloc(Bulges, sizeof(double) * mesh_size);
				if(new_mesh == NULL || new_bulges == NULL) {
					fprintf(stderr, "Error: Out of memory.\n");
					exit(1);
				}
				Mesh = new_mesh;
				Bulges = new_bulges;
			}
			Mesh[++PolyLine.V_Count].x = atof(Group.value);
			/* initialize the rest of the vertex to something */
			Mesh[PolyLine.V_Count].y = 0.0;
			Mesh[PolyLine.V_Count].z = PolyLine.Elevation;
			Bulges[PolyLine.V_Count] = 0.0;
			use_vertex = 1;
			break;
		case 20:
			if(PolyLine.V_Count == 0) {
				fprintf(stderr,
					"Group 20 before first group 10 in LWPOLYLINE."
					" Skipping group on line %d.\n",
					Group.line);
				continue;
			}
			Mesh[PolyLine.V_Count].y = atof(Group.value);
			if(PolyLine.V_Count > 1
				&& (fabs(Mesh[PolyLine.V_Count].x - (Mesh[PolyLine.V_Count-1]).x)
					< EPSILON /* one axis diff is enough */
					&& fabs(Mesh[PolyLine.V_Count].y - (Mesh[PolyLine.V_Count-1]).y)
						< EPSILON)) {
				/* two virtually identical points, ignore the last one */
				PolyLine.V_Count--;
				use_vertex = 0;
			}
			break;
		case 42:
			if(PolyLine.V_Count == 0) {
				fprintf(stderr,
					"Group 42 before first group 10 in LWPOLYLINE."
					" Skipping group on line %d.\n",
					Group.line);
				continue;
			}
			if(use_vertex) Bulges[PolyLine.V_Count] = atof(Group.value);
			break;
		}
		next_group(infp, &Group);
	}
	if(PolyLine.Width > 0.0) PolyLine.Type = et_WPLINE;
	else if(PolyLine.Flags & 1) PolyLine.Type = et_POLYGON;
	else if(PolyLine.Thickness == 0.0) PolyLine.Type = et_NONE;
	/* 1.1 - if it is open, unwide, and flat, don't convert it */
	if(PolyLine.V_Count) PolyLine.V_Count++;
}


/* Read in a polyline. If the polyline represents a 3D shape then read in  */
/* the following vertices.  */

void ReadPolyLine() {
	unsigned int i,VerticesFollow;
	unsigned int Face_Count=0;
	unsigned int Vertex_Count = 1; /* Vertices are indexed from 1  */ 
#ifdef WITH_SMOOTHING	
	unsigned int M,N,M_wrap,N_wrap,j,k;
	Point3 p,p1,p2,p3,nold;
#endif
	
	PolyLine.Colour = -1;
	PolyLine.Width = 0.0;
	PolyLine.Thickness = 0.0;
	PolyLine.Normal =  ZUnit;
	PolyLine.Flags = 0;
	PolyLine.Type = et_PLINE; /* default: simple pline */
	PolyLine.M_Count = 0;
	PolyLine.N_Count = 0;
	PolyLine.V_Count = 0;
	PolyLine.F_Count = 0;
	VerticesFollow = 0;

	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(PolyLine);
			READ_ENTITY_NORMAL(PolyLine.Normal);
			READ_THICKNESS(PolyLine);
			/*READ_DOUBLE(40, PolyLine.Width);*/
			READ_WIDTH(PolyLine);
			READ_INT(66, VerticesFollow);
			READ_FLAGS(PolyLine);
			READ_INT(71, PolyLine.M_Count);
			READ_INT(72, PolyLine.N_Count);
		}
		next_group(infp, &Group);
	}
	
	if(Mesh == NULL) {
		mesh_size = CHUNKSIZE;
		Mesh = malloc(sizeof(Point3) * mesh_size);
		Bulges = malloc(sizeof(double) * mesh_size);
	}
	if(Faces == NULL) {
		faces_size = CHUNKSIZE;
		Faces = malloc(sizeof(xface) * faces_size);
		VCount = malloc(sizeof(int) * faces_size);
	}
	if(Mesh == NULL || Bulges == NULL || Faces == NULL || VCount == NULL) {
		fprintf(stderr, "Error: Out of memory\n");
		exit(1);
	}
#ifdef WITH_SMOOTHING
	if(Normals == NULL) {
		normals_size = CHUNK_SIZE;
		Normals = malloc(sizeof(Point3) * mesh_size);
	}
	if(Normals == NULL) {
		fprintf(stderr, "Error: Out of memory\n");
		exit(1);
	}
#endif

	if(PolyLine.Flags & 8) PolyLine.Type = et_NONE; /* 3d poly */
	else if(PolyLine.Flags & 16) PolyLine.Type = et_PMESH;
	else if(PolyLine.Flags & 64) PolyLine.Type = et_PFACE;
	else if(PolyLine.Width > 0.0) PolyLine.Type = et_WPLINE;
	else if(PolyLine.Flags & 1)  PolyLine.Type = et_POLYGON;
	/* else if (PolyLine.Thickness == 0.0) PolyLine.Type = et_NONE; */
	/* 1.1 - if it is open, unwide, and flat, don't convert it */

	Mesh[0] = ZeroVector;
#ifdef WITH_SMOOTHING	
	M=PolyLine.M_Count;
	N=PolyLine.N_Count;
	if (PolyLine.Flags & 1)  M_wrap = 1; else M_wrap = 0;
	if (PolyLine.Flags & 32) N_wrap = 1; else N_wrap = 0;
#endif
	
	/* Read Vertices and faces, and calculate normals  */
	if (VerticesFollow) {
		while (!feof(infp) && strcmp(Group.value,SEQEND) != 0) {
			ReadVertex();
			
			/* vertices  */
			if (Vertex.Flags & 128 && !(Vertex.Flags & 64)) {
				/* Polyface mesh face  */
				/* Make sure our arrays are still big enough */
				if(Face_Count >= faces_size) {
					void *newfaces, *newvcount;
					faces_size += CHUNKSIZE;
					newfaces = realloc(Faces, sizeof(xface) * faces_size);
					newvcount = realloc(VCount, sizeof(int) * faces_size);
					if(newfaces == NULL || newvcount == NULL) {
						fprintf(stderr, "Error: Out of memory\n");
						exit(1);
					}
					Faces = newfaces;
					VCount = newvcount;
				}
#ifdef WITH_SMOOTHING
				if(Face_Count >= normals_size) {
					void *newnormals;
					normals_size += CHUNKSIZE;
					newnormals = realloc(Normals, sizeof(Point3) * normals_size);
					if(newnormals == NULL) {
						fprintf(stderr, "Error: Out of memory\n");
						exit(1);
					}
					Normals = newnormals
				}
#endif
				/* copy vertex indices and number */
				for (i=0;i<Vertex.VCount;i++) {
					Faces[Face_Count][i] = Vertex.Face[i];
					VCount[Face_Count]   = Vertex.VCount;
				}
#ifdef WITH_SMOOTHING	
				if(Options.smooth) { /* Calculate normal  */
					for (i=0;i<Vertex.VCount;i++) {
						j = i-1;
						if (j==-1) j=Vertex.VCount-1;
						nold = Normals[abs(Faces[Face_Count][i])];
						p   = Mesh[abs(Faces[Face_Count][i])];
						p1  = Mesh[abs(Faces[Face_Count][j])];
						p2  = Mesh[abs(Faces[Face_Count][(i+1)%Vertex.VCount])];
						p3  = Unit((p1-p)*(p-p2));
						Normals[abs(Faces[Face_Count][i])] = Unit(p3+nold);
					}
				}
#endif
				Face_Count++;
			} else if (!(Vertex.Flags & 16)) {
				/* vertex point  */
				if(PolyLine.Type == et_PFACE
					|| PolyLine.Type == et_PMESH
					|| Vertex_Count == 1 /* fist one */
						/* only use a pline or polygon vertex if it is different
						from the previous one */
					|| (Vertex_Count > 1
						&& (fabs(Mesh[Vertex_Count].x - (Mesh[Vertex_Count-1]).x)
							> EPSILON /* one axis diff is enough */
							|| fabs(Mesh[Vertex_Count].y - (Mesh[Vertex_Count-1]).y)
								> EPSILON)
					)) {
					/* Make sure our arrays are still big enough */
					if(Vertex_Count >= mesh_size) {
						void *newmesh, *newbulges;
						mesh_size += CHUNKSIZE;
						newmesh = realloc(Mesh, sizeof(Point3) * mesh_size);
						newbulges = realloc(Bulges, sizeof(double) * mesh_size);
						if(newmesh == NULL || newbulges == NULL) {
							fprintf(stderr, "Error: Out of memory\n");
							exit(1);
						}
						Mesh = newmesh;
						Bulges = newbulges;
					}
#ifdef WITH_SMOOTHING	
					if(Vertex_Count >= normals_size) {
						void *newnormals;
						normals_size += CHUNKSIZE;
						newnormals = realloc(Normals, sizeof(Point3) * normals_size);
						if(newnormals == NULL) {
							fprintf(stderr, "Error: Out of memory\n");
							exit(1);
						}
						Normals = newnormals;
					}
					Normals[Vertex_Count] = ZeroVector;
#endif
					Mesh[Vertex_Count]    = Vertex.Location;
					Bulges[Vertex_Count]  = Vertex.Bulge;
					Vertex_Count++;
				}
			}
		}
	}
	
	/* Calculate normals for polygon mesh  */
#ifdef WITH_SMOOTHING	
	if (Options.smooth && PolyLine.Flags & 16) {
		for (i=1;i<N+N_wrap;i++) {
			for (j=1;j<M+M_wrap;j++) {
				Fc[0] = (j%M)*N+(i%N)+1;
				Fc[1] = (j-1)%M*N+(i%N)+1;
				Fc[2] = (j-1)%M*N+(i-1)%N+1;
				Fc[3] = (j%M)*N+(i-1)%N+1;
				
				/* Account for meshes that don't wrap  */
				if (!((!N_wrap && i==N) || (!M_wrap && j==M))) {
					
					p    = Mesh[Fc[1]];
					p1   = Mesh[Fc[0]];
					p2   = Mesh[Fc[2]];
					/* Avoid degeneracy  */
					if IS_SAMEPT(p,p1) p1 = Mesh[Fc[3]];
					else if IS_SAMEPT(p,p2) p2 = Mesh[Fc[3]];
					p3 = Unit((p2-p)*(p-p1));
					
					/* Assume all quads are planar since we  */
					/* don't know how they are triangulated.  */
					for (k=0;k<4;k++) {
						nold = Normals[Fc[k]];
						Normals[Fc[k]] = Unit(p3+nold);
					}
				}
			}
		}
	}
#endif
	PolyLine.V_Count = Vertex_Count;
	PolyLine.F_Count = Face_Count;
}

void ReadInsert()
{
	Insert.Attributes = 0;
	Insert.Rotation = 0;
	Insert.ColumnCount = 1;
	Insert.RowCount = 1;
	Insert.ColumnSpacing = 0;
	Insert.RowSpacing = 0;
	Insert.Scale.x = 1.0;
	Insert.Scale.y = 1.0;
	Insert.Scale.z = 1.0;
	Insert.Normal  = ZUnit;
	
	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Insert);
			READ_INT(66, Insert.Attributes);
			READ_NAME(Insert);
			READ_ROTATION(50, Insert.Rotation);
			READ_INT(70, Insert.ColumnCount);
			READ_INT(71, Insert.RowCount);
			READ_DOUBLE(44, Insert.ColumnSpacing);
			READ_DOUBLE(45, Insert.RowSpacing);
			READ_COORDINATE(Insert.Insertion,10,20,30);
			READ_COORDINATE(Insert.Scale,41,42,43);
			READ_COORDINATE(Insert.Normal,210,220,230);
		}
		next_group(infp, &Group);
	}

	/* Attribute entities  */
	if (Insert.Attributes == 1) {
		while (!feof(infp) && Group.code != 0
			&& strcmp(Group.value,SEQEND) != 0) {
			next_group(infp, &Group);
		}
		next_group(infp, &Group); /* move over SEQEND data */
		while (!feof(infp) && Group.code != 0) {
			next_group(infp, &Group);
		}
	}
}


void findBlock()
{
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value, BLOCK)  != 0)
			&& (strcmp(Group.value, ENDSEC) != 0)
			&& (strcmp(Group.value, SECTION)!= 0)))) {
		next_group(infp, &Group);
	}
}

void findEndblk()
{
	while (!feof(infp) && (Group.code != 0
		|| (   (strcmp(Group.value, ENDBLK)   != 0)
			&& (strcmp(Group.value, BLOCK)  != 0)
			&& (strcmp(Group.value, ENDSEC) != 0)
			&& (strcmp(Group.value, SECTION)!= 0)))) {
		next_group(infp, &Group);
	}
}

void readBlock()
{
	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		switch (Group.code) {
			READ_ENTITY_OPTIONAL(Block);
			READ_NAME(Block);
			READ_FLAGS(Block);
			READ_COORDINATE(Block.Base,41,42,43);
		}
		next_group(infp, &Group);
	}		
}

void readEndblk()
{
	next_group(infp, &Group);
	while (!feof(infp) && Group.code != 0) {
		next_group(infp, &Group);
	}		
}

void ReadBlocks() {
	int BlocksEnd = FALSE;
	
	while(!feof(infp) && BlocksEnd == 0) {
		findBlock();
		if(strcmp(Group.value,ENDSEC)  == 0) {
			BlocksEnd = 1;
		} else if(strcmp(Group.value,SECTION) == 0) {
			BlocksEnd = 1;
		} else if(strcmp(Group.value,BLOCK)  == 0) {
			/*next_group(infp, &Group);*/
			readBlock();
			/* current group must be 0 now */
			if (!InExcludeList(Block.Name)) {
				ConvertBlockStart(Block);
				ReadEntities(ENDBLK);
				ConvertBlockEnd(Block);
			} else {
				findEndblk();
			}
			readEndblk();
		} else if(strcmp(Group.value,ENDBLK) == 0) {
			readEndblk();
		} else  BlocksEnd = TRUE; /* something strange happened... */
	}
}

void BlocksSection() {
	next_group(infp, &Group); /* first block */
	ReadBlocks();
}

/* ------------------------------------------------------------------------ */

void ReadEntities(char* Terminate)
{
	
	while (!feof(infp)) {		
		if (strcmp(Group.value,Terminate) == 0) {
			break;
		} else if (strcmp(Group.value,TEXT) == 0) {
			ReadText();
			if(Options.etypes[et_TEXT] > 0) {
				ConvertTextEntity(Text);
			}
		} else if (strcmp(Group.value,ARC) == 0) {
			ReadArc();
			if((Options.etypes[et_ARC] > 0)
					&& (Options.ignorethickness || Arc.Thickness)) {
				ConvertArcEntity(Arc);
			}
		} else if (strcmp(Group.value,LINE) == 0) {
			ReadLine();
			if((Options.etypes[et_LINE] > 0)
					&& (Options.ignorethickness || Line.Thickness)) {
				ConvertLineEntity(Line);
			}
		} else if (strcmp(Group.value,CIRCLE) == 0) {
			ReadCircle();
			if(Options.etypes[et_CIRCLE] > 0) {
				ConvertCircleEntity(Circle);
			}
		} else if (strcmp(Group.value,POINT) == 0) {
			ReadPoint();
			if(Options.etypes[et_POINT] > 0) {
				ConvertPointEntity(Point);
			}
		} else if (strcmp(Group.value,FACE3D) == 0) {
			Read3DFace();
			if(Options.etypes[et_3DFACE] > 0) {
				Convert3DFaceEntity(Face3D);
			}
		} else if (strcmp(Group.value,TRACE) == 0) {
			ReadTrace();
			if(Options.etypes[et_TRACE] > 0) {
				ConvertTraceEntity(Trace);
			}
		} else if (strcmp(Group.value,SOLID) == 0) {
			ReadTrace();
			if(Options.etypes[et_SOLID] > 0) {
				ConvertTraceEntity(Trace);
			}
		} else if (strcmp(Group.value,POLYLINE) == 0) {
			ReadPolyLine();
			if(Options.etypes[PolyLine.Type] > 0) {
				if(PolyLine.Type == et_PMESH
					|| PolyLine.Type == et_PFACE) {
					ConvertMesh(PolyLine,Mesh,Normals,Faces,VCount);
				} else {
					ConvertPline(PolyLine,Mesh,Bulges);
				}
			}
		} else if (strcmp(Group.value,LWPOLYLINE) == 0) {
			ReadLWPolyLine();
			if(Options.etypes[PolyLine.Type] > 0) {
				ConvertPline(PolyLine,Mesh,Bulges);
			}
		} else if (strcmp(Group.value,INSERT) == 0) {
			ReadInsert();
			if(!InExcludeList(Insert.Name)) {
				ConvertInsertEntity(Insert);
			}
		} else next_group(infp, &Group);
	}
}

void EntitiesSection()
{
	next_group(infp, &Group); /* first entity */
	ReadEntities(ENDSEC);
}


/* ------------------------------------------------------------------------ */
