/*
This file is part of

* dxf2rad - convert from DXF to Radiance scene files.
* Radout  - Export geometry from Autocad to Radiance scene files.


As a derivative work from code placed in the public domain by its
original author, this file is also in the public domain.
*/

#ifndef _DLLPROTO_H
#define _DLLPROTO_H
#ifdef __cplusplus
    extern "C" {
#endif

#include <stdio.h>

#include "dlltypes.h"

/* dll.c */
void DllTouch (DllList *list);
DLL_LIST DllNewList (DLL_SETUP psetup);
DLL_SETUP DllSetupList (CompProc comp, FreeProc freeKey, FreeProc freeData, void *data);
void DllFreeSetup (DLL_SETUP setup);
void *DllDelete (DLL_LIST plist, void *key, void **data);
void *DllDelRank (DLL_LIST plist, unsigned long rank, void **data);
DLL_LIST DllCopyList (DLL_LIST plist, DLL_SETUP psetup, CopyProc copyKey, CopyProc copyData, void *info);
void DllDestroyList (DLL_LIST plist, FreeProc freeKey, FreeProc freeData, void *info);
void DllTraverseAndDelete (DLL_LIST plist, CompProc findKey, CompProc findData, void *info);
void DllDump (DLL_LIST plist, void (*key_dump)(FILE*,void*,void*,void*), void *info, FILE *fp);
int DllInsert (DLL_LIST plist, void *key, void *data);
void *DllFirst (DLL_LIST plist, void **data);
void *DllLast (DLL_LIST plist, void **data);
void *DllPrev (DLL_LIST plist, void **data);
void *DllNext (DLL_LIST plist, void **data);
void *DllPeekF (DLL_LIST plist, void **data);
void *DllPeekR (DLL_LIST plist, void **data);
void *DllPeek (DLL_LIST plist, int where, void **data);
void *DllPushF (DLL_LIST plist, void *key, void *data);
void *DllPushR (DLL_LIST plist, void *key, void *data);
void *DllPush (DLL_LIST plist, int where, void *key, void *data);
void *DllPopF (DLL_LIST plist, void **data);
void *DllPopR (DLL_LIST plist, void **data);
void *DllPop (DLL_LIST plist, int where, void **data);
void *DllRank (DLL_LIST plist, unsigned long rank, void **data);
void *DllSearch (DLL_LIST plist, void *target, void **data);
void DllTraverse (DLL_LIST plist, void (*)(void *,void *,void*), void *parms);
void *DllGetListData (DLL_LIST plist);
void DllSetListData (DLL_LIST plist, void *data);
long DllGetListSize (DLL_LIST plist);
/* dltest.c */

#ifdef __cplusplus
    }
#endif
#endif /* _DLLPROTO_H */

