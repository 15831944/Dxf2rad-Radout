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

#ifndef _READDXF_H
#define _READDXF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#ifdef WIN32 /* shut up some of the warnings */
#define strncat(dst,src,len) strncat_s(dst,len,src,len)
#define strncpy(dst,src,len) strncpy_s(dst,len,src,len)
#endif

#include "geomtypes.h" /* for Point3 */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif
/* Maximum accepted file path length */
#define MAXPATH 4096
/* This is the maximum length for symbol names in Autocad 2000 */
#define MAXSTRING 256
/* The maximum line length we'll accept without truncating */
#define MAXLINE 4096
/* our static tables take a total of 544 bytes per entry,
	let's allocate in chunks of about 1 MB */
#define CHUNKSIZE  2049    /* 350 */
#ifndef MAXFLOAT
#define MAXFLOAT   3.4E38
#endif
#ifndef MINFLOAT
#define MINFLOAT   -3.4E-38
#endif
#ifndef M_PI
#define M_PI       3.14159265358
#endif
#define DEG2RAD    (M_PI/180.0)

#define SECTION    "SECTION"
#define ENDSEC     "ENDSEC"
#define HEADER     "HEADER"
#define CLASSES    "CLASSES"
#define TABLES     "TABLES"
#define TABLE      "TABLE"
#define BLOCKS     "BLOCKS"
#define BLOCK      "BLOCK"
#define ENTITIES   "ENTITIES"
#define OBJECTS    "OBJECTS"
#define VIEW       "VIEW"
#define VPORT      "VPORT"
#define ENDTAB     "ENDTAB"
#define ENDBLK     "ENDBLK"
#define SEQEND     "SEQEND"
#define CIRCLE     "CIRCLE"
#define POINT      "POINT"
#define FACE3D     "3DFACE"
#define TRACE      "TRACE"
#define SOLID      "SOLID"
#define LINE       "LINE"
#define ARC        "ARC"
#define POLYLINE   "POLYLINE"
#define LWPOLYLINE "LWPOLYLINE"
#define TEXT       "TEXT"
#define INSERT     "INSERT"
#define	FILEEND    "EOF"

#define Magnitude(v) sqrt(v.x*v.x+v.y*v.y+v.z*v.z)   /* |v|   */
#define Dot(v1,v2)   (v1.x*v2.x+v1.y*v2.y+v1.z*v2.z) /* v1.v2   */


#ifdef ACIS
#undef ACIS
#endif

typedef enum {
		et_NONE,
		et_3DFACE, et_SOLID, et_TRACE,
		et_PLINE, et_WPLINE,et_POLYGON,et_PMESH, et_PFACE,
		et_LINE, et_ARC, et_CIRCLE, et_POINT, et_TEXT,
#ifdef ACIS
		et_3DSOLID, et_BODY, et_REGION,
#endif /* ACIS */
		et_LAST
} EntityType;


typedef enum {
	none, bylayer, bycolor
} ExportMode;


enum EndMode   {Open,Closed};

typedef struct {
  int   line;
  int   code;
  char  value[MAXLINE];
} Group_Type;


typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  char  Text[MAXSTRING];
  Point3 Location;
} Text_Type;

typedef struct {
  char  Name[MAXSTRING];
  int Mode;
  Point3 Center;
  Point3 Target;
  Point3 Direction;
  double Height;
  double Width;
  double Twist;
  double Lens;
  double Fclip;
  double Bclip;
} View_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  int   Flags;
  Point3 Location;
  double Bulge;
  int   Face[4];  /* Four vertices of a face.   */
  unsigned int   VCount;   /* How many vertices in the face.   */
} Vertex_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  Point3 Normal;
  double Thickness;
  Point3 Start;
  Point3 End;
} Line_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  Point3 Normal;
  double Thickness;
  Point3 Center;
  double Radius;
  double Startangle;
  double Endangle;
} Arc_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  Point3 Normal;
  double Thickness;
  Point3 Center;
  double Radius;
} Circle_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  Point3 Center;
  double Thickness;
} Point_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  Point3 p[4];
} Face3D_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  Point3 p[4];
  Point3 Normal;
  double Thickness;
} Trace_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  int   Flags;
  unsigned int   M_Count;
  unsigned int   N_Count;
  unsigned int   V_Count;  /* Vertex count   */
  unsigned int   F_Count;  /* Face count   */
  double Elevation;
  double Thickness;
  double Width;
  Point3 Normal;
  EntityType Type; /* our own classification   */
} PolyLine_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  int   Attributes;
  char  Name[MAXSTRING];
  Point3 Insertion;
  Point3 Scale;
  Point3 Normal;
  double Rotation;
  unsigned int   ColumnCount;
  unsigned int   RowCount;
  double ColumnSpacing;
  double RowSpacing;
} Insert_Type;

typedef struct {
  char  Layer[MAXSTRING];
  char  Handle[MAXSTRING];
  int   Colour;
  char  Name[MAXSTRING];
  int   Flags;
  Point3 Base;
} Block_Type;


extern Group_Type    Group;
extern View_Type     View;
extern Vertex_Type   Vertex;
extern Line_Type     Line;
extern Arc_Type      Arc;
extern Circle_Type   Circle;
extern Point_Type    Point;
extern Face3D_Type   Face3D;
extern Trace_Type    Trace;
extern PolyLine_Type PolyLine;
extern Block_Type    Block;
extern Insert_Type   Insert;

void IgnoreSection();
void HeaderSection();
void TablesSection();
void BlocksSection();
void EntitiesSection();

void ReadText();
void ReadBlocks();
void ReadVertex();
void ReadCircle();
void Read3DFace();
void ReadPolyLine();
void ReadInsert();
void ReadEntities(char* Terminate);
int next_group(FILE *fp, Group_Type *m);

#ifdef __cplusplus
	}
#endif
#endif /* _READDXF_H */
