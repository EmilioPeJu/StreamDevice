/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is error and debug message handling of StreamDevice.    *
* Please refer to the HTML files in ../doc/ for a detailed     *
* documentation.                                               *
*                                                              *
* If you do any changes in this file, you are not allowed to   *
* redistribute it any more. If there is a bug or a missing     *
* feature, send me an email and/or your patch. If I accept     *
* your changes, they will go to the next release.              *
*                                                              *
* DISCLAIMER: If this software breaks something or harms       *
* someone, it's your problem.                                  *
*                                                              *
***************************************************************/

#include "StreamError.h"
#include <stdarg.h>
#include <string.h>

int streamDebug = 0;
FILE* StreamDebugFile = stderr;

void StreamError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\033[31;1m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m");
    if (StreamDebugFile != stderr)
    {
        fprintf(StreamDebugFile, "\033[31;1m");
        vfprintf(StreamDebugFile, fmt, args);
        fprintf(StreamDebugFile, "\033[0m");
        fflush(StreamDebugFile);
    }
    va_end(args);
}

int StreamDebugClass::
print(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const char* f = strrchr(file, '/');
    if (f) f++; else f = file;
    fprintf(StreamDebugFile, "%s:%d: ", f, line);
    vfprintf(StreamDebugFile, fmt, args);
    fflush(StreamDebugFile);
    va_end(args);
    return 1;
}

