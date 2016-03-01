/*
 * Geom3d.h
 * 3d geometry types
 * originally from "Graphics Gems", Academic Press, 1990
 */


#ifndef G3D_H
#define G3D_H 1
#ifdef __cplusplus
    extern "C" {
#endif

#include <sys/types.h>

/*********************/

typedef struct _Point3D {   /* 3d point */
    double x, y, z;
} Point3, Vector3;


typedef struct _RGB {
    double red, grn, blu;
} RGB;

typedef struct _IntPoint3D {/* 3d integer point */
    int x, y, z;
} IntPoint3;

typedef double Matrix3[3][3];   /* 3-by-3 matrix */

typedef double Matrix4[4][4];   /* 4-by-4 matrix */

typedef struct _Box3D {    /* 3d box */
    Point3 min, max;
} Box3;


typedef struct _Poly3D {
    unsigned short closed, nverts;
    Point3 *verts;
    Vector3 *normals;   /* normal at each vertex */
    Vector3 normal;     /* normal for polygon */
    struct _Poly3D *next;
	char *material; /* not required for radout, but still useful */
} Poly3;


typedef struct _Plane3D {
    Vector3 dir;
    double d;
} Plane3;


typedef struct _Cyl3D {
    Point3 svert, evert;        /* start & end vertices */
	Point3 normal;
    double length, srad, erad;          /* start & end radii */
    struct _Cyl3D *next;
	char *material; /* not required for radout, but still useful */
} Cyl3;


typedef struct _SimpleText {
	Point3 position;
	char *s;
	struct _SimpleText *next;
	struct _SimpleText *ref;
} SimpleText;

#ifdef __cplusplus
    }
#endif
#endif        /* G3D_H */                  

