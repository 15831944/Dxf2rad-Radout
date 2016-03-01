/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


As a derivative work from code placed in the public domain by its
original author, this file is also in the public domain.
*/

/* dltypes.h
 * Private declarations for a doubly-linked list implementation
 */

#ifndef _DLTYPES_H_
#define _DLTYPES_H_

#ifdef __cplusplus
    extern /*MSG0*/"C" {
#endif

typedef void (*TraversalProc)(void *,void *,void*);
#ifdef __STDC__
typedef int (*CompProc)(void *, void *);
typedef void (*FreeProc)(void *, void *);
typedef void *(*CopyProc)(void *, void *);
#else
typedef int (*CompProc)();
typedef void (*FreeProc)();
typedef void *(*CopyProc)();
#endif

typedef struct DllNode {
    struct DllNode *next, *prev;
    void *key, *data;
} DllNode;

typedef struct DllList {
    DllNode *last, *current;
    CompProc comp;
    FreeProc freeData, freeKey;
    short nextOk, changed, ordered;
	unsigned long size;
    void *data;
} DllList;

typedef struct DllSetup {
    CompProc comp;
    FreeProc freeData, freeKey;
    void *data;
} DllSetup;

/* Hidden (opaque) data types */
typedef void *DLL_LIST;
typedef void *DLL_SETUP;

/* Peek/push/pop locations */
#define DLL_FRONT       1
#define DLL_BACK        2
#define DLL_FIRST(lst) (lst->last->next)
#define DLL_LAST(lst)  (lst->last)


#ifdef __cplusplus
    }
#endif

#endif /* _DLTYPES_H_ */
/*** end dltypes.h ***/
