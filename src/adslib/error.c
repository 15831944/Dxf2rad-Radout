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

/*

*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>      /* ANSI C header file */
#include <stdlib.h>

#ifdef UNIX
	#include <unistd.h>
#endif



/*  we provide our own strerror() for sunOS 4.x */
#ifdef sun
#ifndef __sysv__
extern char *sys_errlist[];
extern int sys_nerr;
char* strerror(int n)
{ return (0 <= n && n < sys_nerr ) ? sys_errlist[n] : ""; }
#endif
#endif


char *progName = NULL;

/* Print a message and return to caller.
 * Caller specifies "errnoflag".
 */
static void err_doit(int errnoflag, const char *fmt, va_list ap)
{
    int     errno_save;
    char    buf[512];

    errno_save = errno;     /* value caller might want printed */
    if (progName)
        fprintf(stderr, "%s: ", progName);
    vsprintf(buf, fmt, ap);
    if (errnoflag)
        sprintf(buf+strlen(buf), ": %s", strerror(errno_save));
    strcat(buf, "\n");
    fflush(stdout);     /* in case stdout and stderr are the same */
    fputs(buf, stderr);
    fflush(NULL);       /* flushes all stdio output streams */
    return;
}


/* Nonfatal error related to a system call.
 * Print a message and return.
 */
extern void WarnSys(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    return;
}


/* Fatal error related to a system call.
 * Print a message and terminate.
 */
extern void ErrorSys(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    exit(2);
}


/* Fatal error related to a system call.
 * Print a message, dump core, and terminate.
 */
extern void ErrorDump(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    abort();        /* dump core and terminate */
    exit(2);        /* shouldn't get here */
}


/* Nonfatal error unrelated to a system call.
 * Print a message and return.
 */
extern void WarnMsg(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    return;
}


/* Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
extern void ErrorMsg(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    exit(1);
}
/*** end error.c ***/
