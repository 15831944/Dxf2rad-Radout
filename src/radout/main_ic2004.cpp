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

/*
This is the main file for IntelliCAD 2000
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sds.h>

extern int Radout_LoadFuncs();
extern int Radout_DoFuncs();
extern int Radout_CleanUp();


extern int main(int argc, char **argv)
{
    int status;
    short scode = RSRSLT;       /* This is the default result code */

    ads_init(argc, argv);       /* Initialize the interface */
    /* mtrace(); */
    while (1) {                 /* Note loop conditions */
        if ((status = ads_link(scode)) < 0) {
            fprintf(stderr, "main: bad status from ads_link() = %d\n",
                    status);
            exit(1);
        }
        scode = RSRSLT;         /* Default return value */
        switch (status) {
        case RQXLOAD:
            scode = (Radout_LoadFuncs() ? RSRSLT : RSERR);
            break;
        case RQSUBR:            /* This case is normally expanded to */
            scode = (Radout_DoFuncs() == RTNORM ? RSRSLT : RSERR);
            break;              /* select one of the application's
                                 * external functions */
        case RQXUNLD:           /* Do C program cleanup here. */
            scode = (Radout_CleanUp() ? RSRSLT : RSERR);
            break;
        case RQSAVE:            /* AutoCAD SAVE command notification. */
        case RQQUIT:            /* AutoCAD QUIT command notification. */
        case RQEND:             /* AutoCAD END command notification. */
        default:
            break;
        }
    }
    return(0);
}



/*** end main_ic2000.c ***/
