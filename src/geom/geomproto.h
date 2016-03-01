/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


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

#ifndef _GEOMPROTO_H
#define _GEOMPROTO_H
#ifdef __cplusplus
    extern "C" {
#endif


/* adcolor.c */
extern int GetAcadRGB (int cindex, RGB *color);
/* ads_perr.c */
extern int ads_perror (void);
/* error.c */
extern void WarnSys (const char *fmt, ...);
extern void ErrorSys (const char *fmt, ...);
extern void ErrorDump (const char *fmt, ...);
extern void WarnMsg (const char *fmt, ...);
extern void ErrorMsg (const char *fmt, ...);

/* m4geom.c */
extern void M4DirectionMatrix (Matrix4 mat, Matrix4 res);
extern int M4NormalMatrix (Matrix4 mat, Matrix4 res);
extern void M4VecScale (Matrix4 mat, Vector3 *scale);
extern void M4Rotate (Matrix4 resMat, Vector3 *axis, double angle_in_radians);
extern void M4RotateAboutAxis (Vector3 *dir, double rotang, Matrix4 mat);
extern void M4Mirror (Matrix4 resMat, Vector3 *normal);
extern void M4Shear (Vector3 *normal, Vector3 *shear, Matrix4 result);
extern int M4Orient (Vector3 *from, Vector3 *at, Vector3 *up, Matrix4 resMat);
extern int M4Span (Vector3 *from, Vector3 *at, Vector3 *up, int vol_preserve, Matrix4 resMat);
extern int M4GetAxisAndAngle (Matrix4 mat, Vector3 *axis, double *angle);
extern void M4Align (Vector3 *vec1, Vector3 *vec2, Matrix4 resMat);
extern void V3PerpVec (Vector3 *vec, int is_unit, Vector3 *resVec);
extern void M4GetAcadXForm(Matrix4 mx, Vector3 *zaxis,
			  int isinsert, Vector3 *inpt, double rotang,
			  double xscl, double yscl, double zscl, Vector3 *bpt);
extern void M4TransformPolys(Poly3 *polys, Matrix4 mx);
extern Poly3 *M4TransformPolysCopy(Poly3 *polys, Matrix4 mx);
extern void M4TransformCyls(Cyl3 *cyls, Matrix4 mx);
extern Cyl3 *M4TransformCylsCopy(Cyl3 *cyls, Matrix4 mx);
extern void M4TransformSimpleTexts(SimpleText *texts, Matrix4 mx);
/* refcopy dosen't copy the actual text data! */
extern SimpleText *M4TransformSimpleTextRefcopy(SimpleText *texts, Matrix4 mx);

/* m4inv.c */
extern int M4Invert (Matrix4 mat, Matrix4 resMat);
extern int AffineMatrix4Inverse (register Matrix4 in, register Matrix4 out);
extern int AnglePreservingMatrix4Inverse (Matrix4 in, Matrix4 out);
/* m4mat.c */
extern void M4SetFromPoints (Matrix4 m, Point3 *p1, Point3 *p2, Point3 *p3);
extern void M4SetIdentity (Matrix4 m);
extern void M3SetIdentity (Matrix3 mat);
extern void M4Zero (Matrix4 m);
extern double M4Determinant (Matrix4 m);
extern void M4Copy (Matrix4 source, Matrix4 target);
extern void M4MatMult (Matrix4 mat1, Matrix4 mat2, Matrix4 prod);
extern void M4Transpose (Matrix4 mat, Matrix4 trans);
extern void M4PrintFormatted (FILE *fp, Matrix4 mat, char *title, char *head, char *format, char *tail);
extern void M4Print (FILE *fp, Matrix4 mat);
extern int M4IsEqual (Matrix4 m1, Matrix4 m2, double epsilon);
extern double M4Trace (Matrix4 mat);
extern int M4Power (Matrix4 resMat, Matrix4 mat, int power);
extern int M4ColumnReduce (Matrix4 resMat, Matrix4 mat, double epsilon);
extern int M4KernelBasis (Matrix4 resMat, Matrix4 mat, double epsilon);
extern void M4ScalarMult (Matrix4 m, double scalar, Matrix4 resMat);
extern void M4Add (Matrix4 A, Matrix4 B, Matrix4 resMat);
/* m4post.c */
extern void M4RotateX (Matrix4 m, double a);
extern void M4RotateY (Matrix4 m, double a);
extern void M4RotateZ (Matrix4 m, double a);
extern void M4Scale (Matrix4 m, double sx, double sy, double sz);
extern void M4Translate (Matrix4 m, double tx, double ty, double tz);
/* misc.c */
extern char *StrDup (char *str);
/* poly.c */
extern SimpleText *SimpleTextAlloc (char *s, SimpleText *next);
extern void SimpleTextFree (SimpleText *text);
extern Cyl3 *Cyl3Alloc (Cyl3 *next);
extern void Cyl3Free (Cyl3 **cyl);
extern void Cyl3FreeList (Cyl3 *cyls);
extern Poly3 *Poly3Alloc (int nverts, int closed, Poly3 *next);
extern void Poly3Free (Poly3 **poly);
extern void Poly3FreeList (Poly3 *polys);
extern Poly3 *Poly3GetLast (Poly3 *polys);
extern void Poly3Print (FILE *fp, Poly3 *poly, int pntFlag);
extern void Poly3PrintList (FILE *fp, Poly3 *polys);
extern Poly3 *FaceSubDivide(Poly3 *poly);
extern Poly3 *CopyPolyUp(Poly3 *poly, double dist);
extern Poly3 *WidePlist(Poly3 *poly, double hwdth);
extern Poly3 *CreateSideWalls(Poly3 *poly, double thick);
/* bulge.c */
extern double StartTangent(double ang);
extern int ArcApprox(double radius, double a1, double a2,
					 double curveTolerance, double angleTolerance,
					 double *segLen, double *initialAngle, double *incrAngle);
extern Poly3 *SegmentArc(Point3 *center, int dir,
						 double distTolerance, double angleTolerance,
						 double radius, double a1, double a2);
extern double BulgeToArc(Point3 *p1, Point3 *p2, double bulge, int *dir,
						 Point3 *center, double *a1, double *a2);
/* polycheck.c */
extern int PolyCheckCoincident (Poly3 *poly);
extern int PolyCheckColinear (Poly3 *poly);
extern int PolyCheckCoplanar (Poly3 *poly);
extern int FaceCheckCoplanar (Poly3 *face);
extern double PolyGetArea (Poly3 *poly);
extern double TriGetArea (Point3 *p0, Point3 *p1, Point3 *p2);
/* v3vec.c */
#define V3ISEQUAL(a,b) (((a)->x==(b)->x)&&((a)->y==(b)->y)&&((a)->z==(b)->z))
#define V3SET(a,b,c,v) (((v)->x=(a)),((v)->y=(b)),((v)->z=(c)))
#define V3ZERO(v) ((v)->x=(v)->y=(v)->z=0.0)
extern double V3SquaredLength (Vector3 *a);
extern double V3Length (Vector3 *v);
extern Vector3 *V3Negate (Vector3 *v);
extern Vector3 *V3Scale (Vector3 *v, double s);
extern double V3Normalize (Vector3 *v);
extern Vector3 *V3Normalize2 (Vector3 *v, double newlen);
extern Vector3 *V3Add (Vector3 *a, Vector3 *b, Vector3 *c);
extern Vector3 *V3Sub (Vector3 *a, Vector3 *b, Vector3 *c);
extern double V3Dot (Vector3 *a, Vector3 *b);
extern double V3GetAngle2D (Point3 *pt1, Point3 *pt2);
extern double V3CosineAngle (Point3 *a, Point3 *b, Point3 *c);
extern Point3 *V3AddScalarMult (Point3 *a, double b, Point3 *c);
extern Point3 *V3AddPolar2D (Point3 *pt, double angle, double dist, Point3 *rslt);
extern Vector3 *V3Lerp (Vector3 *lo, Vector3 *hi, double alpha, Vector3 *result);
extern Vector3 *V3Combine (Vector3 *a, Vector3 *b, Vector3 *result, double ascl, double bscl);
extern Vector3 *V3Mult (Vector3 *a, Vector3 *b, Vector3 *result);
extern double V3DistanceBetween2Points (Point3 *a, Point3 *b);
extern double V3DistancePoint2Plane (Point3 *pnt, Plane3 *pleq);
extern Vector3 *V3Cross (Vector3 *a, Vector3 *b, Vector3 *c);
extern Vector3 *V3New (double x, double y, double z);
extern Vector3 *V3Duplicate (Vector3 *a);
extern Vector3 *M4MultVector3 (Vector3 *in, Matrix4 m, Vector3 *out);
extern Point3 *M4MultPoint3 (Point3 *in, Matrix4 m, Point3 *out);
extern Point3 *V3MulPointByProjMatrix (Point3 *in, Matrix4 m, Point3 *out);
extern Point3 *V3Translate (Point3 *point, Point3 *dir, double dist, Point3 *result);


#ifdef __cplusplus
    }
#endif
#endif /*  _GEOMPROTO_H */
