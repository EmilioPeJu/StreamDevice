#ifndef __devStreamProtocol_h
#define __devStreamProtocol_h

/***************************************************************
* Stream Device Support                                        *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
***************************************************************/

#include <vxWorks.h>
#include <stdio.h>

#define NUL    (0x00)
#define LF     (0x0a)
#define CR     (0x0d)
#define OUT    (0x0e)
#define IN     (0x0f)
#define WAIT   (0x10)
#define EVENT  (0x11)
#define SKIP   (0x19)
#define FORMAT (0x1a)
#define ESC    (0x1b)

#define FLAG_IGNORE_EXTRA_INPUT (0x0001)

#define FORMAT_FLAG_LEFT  (0x01)
#define FORMAT_FLAG_SIGN  (0x02)
#define FORMAT_FLAG_SPACE (0x04)
#define FORMAT_FLAG_ALT   (0x08)
#define FORMAT_FLAG_ZERO  (0x10)
#define FORMAT_FLAG_SKIP  (0x20)

typedef struct {
    ushort_t bufferSize;
    ushort_t codeSize;
    ushort_t readTimeout;
    ushort_t replyTimeout;
    ushort_t lockTimeout;
    ushort_t writeTimeout;
    ushort_t flags;
    uchar_t inTerminator [5];
    uchar_t outTerminator [5];
    uchar_t separator [6];
    uchar_t *code;
} protocol_t;

typedef struct {
    uchar_t type;
    char conversion;
    char flags;
    signed char prec;
    uchar_t width;
    char string [1];
} format_t;

/*****************************************
* parseProtocol
*
* returns OK or linenr of parse error or ERROR if pattern not found
* or -2 if out of memory
*
*/

int devStreamParseProtocol (protocol_t *protocol, FILE *file, char *patternname);

#endif
