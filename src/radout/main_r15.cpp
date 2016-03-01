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



// ADS and ARX interfaces
#include <adslib.h>
#include <rxdefs.h>
#include "rxdlinkr.h"
#include "rxregsvc.h"

// event handlers
extern int Radout_LoadFuncs(void);
extern int Radout_DoFuncs(void);
extern int Radout_CleanUp(void);


/*-----------------------------------------------------------------------*/
/* ACRXENTRYPOINT -- This function replaces main() for an ADSRX program. */

extern "C" AcRx::AppRetCode
acrxEntryPoint(AcRx::AppMsgCode msg, void* ptr)
{
	static char acbr_path[512] = "acbr15.dll";
    switch(msg) {
    case AcRx::kInitAppMsg:
		// load b-rep dll
		// the dll MUST be in the autocad directory!
		if (!(acrxClassDictionary->at("AcBrEntity"))) {
				acrxDynamicLinker->loadModule(acbr_path, 0);
		}
        // otherwise it is only SDI
		acrxRegisterAppMDIAware (ptr);
        acrxUnlockApplication(ptr);
        break;
    case AcRx::kInvkSubrMsg:
        Radout_DoFuncs();
        break;
    case AcRx::kLoadADSMsg:
        Radout_LoadFuncs();
		break;
	case AcRx::kUnloadAppMsg:
		Radout_CleanUp();
		break;
    }
    return AcRx::kRetOK;
}
