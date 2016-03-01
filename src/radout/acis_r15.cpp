/*
This file is part of

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

#ifdef ACIS // only compile this when ACIS is enabled

#include <stdio.h>

#include "adesk.h"
#include "acestext.h"
#include "acdb.h"
#include "acdbabb.h"
#include "adslib.h"
#include "dbmain.h"
//#include "brent.h"
#include "brbrep.h"
#include "brgbl.h"
#include "brm2dctl.h"
#include "brmesh2d.h"
#include "brentrav.h"
#include "brmetrav.h"
#include "brnode.h"

#include "adtools.h"
#include "geomtypes.h"
//#include "geomdefs.h"
#include "geomproto.h"
#include "radout.h"
#include "resbuf.h"

extern "C"  Options options;


void
errorReport(AcBr::ErrorStatus errorCode)
{
    switch (errorCode) {
	case(AcBr::eBrepChanged):
		acutPrintf(" Brep Changed\n");
		break;        
    case(AcBr::eUnsuitableTopology):
		acutPrintf(" Unsuitable Topology\n");
		break;        
    case(AcBr::eDegenerateTopology):
		acutPrintf(" Degenerate Topology\n");
		break;        
    case(AcBr::eUninitialisedObject):
		acutPrintf(" Uninitialised Object\n");
		break;        
	default: 
		acutPrintf(" AutoCAD Error Code: %d\n%s\n",
			errorCode,
			acadErrorStatusText((Acad::ErrorStatus)errorCode));
		break;    
	}
	return;
}


extern int
SolidMeshToPoly(AcBrMesh2d &brepMesh, Poly3 ** outpolys)
{
	AcBr::ErrorStatus res = (AcBr::ErrorStatus)AcBr::eOk;
    Poly3 *polys = NULL, *poly; /* polygon data for export */
	Point3 *plist;
	int polylen = 0;
	AcBrMesh2dElement2dTraverser meshElemTrav;
	AcBrElement2dNodeTraverser polyNodeTrav;
    AcGePoint3dArray pts;
	AcGePoint3d point;
	
	//Iterate over mesh
	// make a global element traverser
	if ((res = meshElemTrav.setMesh(brepMesh)) != AcBr::eOk) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n Unable to initialize mesh traverser in SolidMeshToPoly()");
		goto error;
	}
	
	
	while (!meshElemTrav.done() && (res == AcBr::eOk)) {
		if(acedUsrBrk()) goto error;
				
		if ((res = polyNodeTrav.setElement(meshElemTrav)) != AcBr::eOk) {
			if(res == Acad::eUserBreak) goto cancel;
			acutPrintf("\n Error in AcBrElement2dNodeTraverser::setElement()");
			goto error;
		}
		// Iterate over polygon, collect points
		while (!polyNodeTrav.done() && (res == AcBr::eOk)) { 
			AcBrNode node;
			res = polyNodeTrav.getNode(node);
			if (res != AcBr::eOk) {	
				if(res == Acad::eUserBreak) goto cancel;
				acutPrintf("\n Error in AcBrElement2dNodeTraverser::getNode:"); 
				goto error;
			}
			// get the point
			res = node.getPoint(point);
			if ((res = polyNodeTrav.next()) != AcBr::eOk) {
				if(res == Acad::eUserBreak) goto cancel;
				acutPrintf("\n Error in AcBrElement2dNodeTraverser::next:");
				goto error;
			}
			pts.append(point);
		} // poly traversal
		
		// convert polygon for output list
		polylen = pts.length();
		if ((poly = Poly3Alloc(polylen, 1, NULL)) == NULL) {
			if(res == Acad::eUserBreak) goto cancel;
			acutPrintf("Write3DSolid: can't alloc poly");
			goto error;
		}
		plist = poly->verts;
		int i;
		for(i = 0; i < polylen; i++) {
			point = pts.at(i);
			plist[i].x	= point.x;
			plist[i].y	= point.y;
			plist[i].z	= point.z;
		}
		// add to chain of polys for export 
		poly->next = polys;
		polys = poly;
		
		// empty array for next iteration
		pts.removeSubArray(0, polylen-1);
		
		if ((res = meshElemTrav.next()) != AcBr::eOk) {
			if(res == Acad::eUserBreak) goto cancel;
			acutPrintf("\n Error in AcBrMesh2dElement2dTraverser::next:");  
			goto error;
		}
	} // mesh traversal
	
	*outpolys = polys;
	return 0;
	
error:
	if(res != eOk) errorReport(res);
cancel:
	if(polys)Poly3FreeList(polys);
	return -1;
}


extern int
SolidToPoly(ADSResBuf *edata, Poly3 **outpolys)
{
	ads_name ename;
	AcBr::ErrorStatus res = (AcBr::ErrorStatus)AcBr::eOk;;
	Acad::ErrorStatus ares;
	AcDbObjectId objId;
    AcDbFullSubentPath *subPath = NULL;
	AcBrMesh2dControl meshCtrl;
	AcBrEntity *meshEnt = NULL;
	AcBrMesh2dFilter meshFilter;
	AcBrMesh2d brepMesh;

    ads_name_set(RBSearch(edata, -1)->resval.rlname, ename);

	if ((ares = acdbGetObjectId(objId, ename)) != Acad::eOk) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n Invalid entity name in SolidToPoly()");
		errorReport((AcBr::ErrorStatus)ares);
		goto error;
	}
	if ((subPath = new AcDbFullSubentPath(objId, kNullSubentId)) == NULL) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n Unable to create AcDbFullSubentPath in SolidToPoly()");
		goto error;
	}
	if ((res = meshCtrl.setAngTol(options.angleTolerance)) != eOk) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n Angle tolerance out of range in SolidToPoly():");
		goto error;
	}
	if ((res = meshCtrl.setDistTol(options.distTolerance)) != eOk) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n Distance tolerance out of range in SolidToPoly()");
		goto error;
	}
	// make the mesh filter from the topology entity and the mesh controls
	if ((meshEnt = new AcBrBrep()) == NULL) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n Unable to create AcBrBrep in SolidToPoly()");
		goto error;
	}
	if ((res = meshEnt->set(*subPath)) != eOk) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n setSubentPath() failed in SolidToPoly()");
		goto error;
	}
	
	meshFilter.insert(make_pair(meshEnt, meshCtrl));

	/* last exit before expensive operaion */
	if(acedUsrBrk()) goto cancel;

    // generate the mesh, display any errors (most errors are not fatal
	// so we want to do the best we can with whatever subset of the brep
	// was meshed).
	if ((res = brepMesh.generate(meshFilter)) != eOk) {
		if(res == Acad::eUserBreak) goto cancel;
		acutPrintf("\n Error in AcBrMesh2d::generate:");
		goto severe;
	}
	delete subPath;
	delete meshEnt;
	return SolidMeshToPoly(brepMesh, outpolys);

severe:
	if(res != eOk) errorReport(res);
	if(subPath) delete subPath;
	if(meshEnt) delete meshEnt;
	acedAlert(
"WARNING: Radout has encountered a problem when requesting the \n\
meshed representation of an ACIS solid from the geometry library.\n\
This is possibly caused by an internal problem with Autocad, or\n\
by a bug in Radout.\n\
\n\
In case the problem was caused by insufficient memory, it might\n\
be preferrable to save your drawing to a different file, restart\n\
Autocad, and check if everything with the new file is OK.");
	return -1;

error:
	if(res != eOk) errorReport(res);
	acedAlert("Error trying to mesh ACIS solid entity.");
cancel:
	acutPrintf("*cancel*\n");
	if(subPath) delete subPath;
	if(meshEnt) delete meshEnt;
	return -1;
}

#endif // ACIS
