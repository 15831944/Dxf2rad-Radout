/*
This file is part of

* Radout  - Export geometry from Autocad to Radiance scene files.


The MIT License (MIT)

Copyright (c) 1999-2016 Georg Mischler
(Originally acquired from Philip Thompson)

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

/*  Miscellaneous stuff
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef UNIX
	#include <unistd.h>
#endif

#if defined(R14)||defined(R15)
/* if we don't allocate our strings from the autocad heap
   when running as ARX, bad things will happen in ads_relrb */
#include "ads.h"
  #define our_malloc acad_malloc
#else
  #define our_malloc malloc
#endif /* R14 || R15 */

/*  Our own version of a function that is sometimes missing from
 *  machines or is implimentated without malloc and causes problems.
 */
char *StrDup(char *str)

{
    char *to = NULL;
	int len = 0;

    if ((str == NULL) || ((len = strlen(str)) <= 0))
        return NULL;
    if ((to = our_malloc((unsigned)len + 1)) == NULL) {
        perror("StrDup: can't malloc()");
        return NULL;
    }
    return strcpy(to, str);
}




/*** end misc.c ***/
/* strcpy */
