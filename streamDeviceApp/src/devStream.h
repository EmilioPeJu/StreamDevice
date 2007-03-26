#ifndef __devStream_h
#define __devStream_h

/***************************************************************
* Stream Device Support                                        *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
***************************************************************/

#include <devStreamProtocol.h>
#include <callback.h>
#include <dbScan.h>
#include <devSup.h>

#define DO_NOT_CONVERT 2

typedef struct {
    CALLBACK callback;   /* don't touch */
    dbCommon *record;    /* associated EPICS record */
    protocol_t protocol; /* access to timeouts and terminators */
    ulong_t channel;     /* number of the output stream for locking */
    int haveBus;         /* TRUE while record has locked the output */
    int acceptInput;     /* TRUE while record expects input */
    int acceptEvent;     /* TRUE while record expects evants */
    void *busPrivate;    /* store your data here */
} stream_t;

typedef struct {
    long (* initRecord) (stream_t *stream, char *busName, char *param);
    long (* writePart) (stream_t *stream, char* data, long size);
    void (* startInput) (stream_t *stream);      /* may be NULL */
    void (* stopInput) (stream_t *stream);       /* may be NULL */
    void (* startProtocol) (stream_t *stream);   /* may be NULL */
    void (* stopProtocol) (stream_t *stream);    /* may be NULL */
    ulong_t nChannels;        /* number of channels for locking */
} streamBusInterface_t;

/* device support functions */
#define devStreamReport NULL
long devStreamInit (int after);
long devStreamWrite (dbCommon *record);
long devStreamRead (dbCommon *record);
long devStreamGetIointInfo (int cmd, dbCommon *record, IOSCANPVT *ppvt);

/* record interface functions */
long devStreamInitRecord (dbCommon *record, struct link *ioLink,
    long (* readData) (dbCommon *, format_t *),
    long (* writeData) (dbCommon *, format_t *));
long devStreamScanf (dbCommon *record, format_t *format, void *value);
long devStreamPrintf (dbCommon *record, format_t *format, ...);
long devStreamScanSep (dbCommon *record);
long devStreamPrintSep (dbCommon *record);

/* bus interface functions (can be called from interrupt level) */
void devStreamReceive (stream_t *stream, char *data, long size, int endflag);
void devStreamStartTimer (stream_t *stream, ushort_t timeout, FUNCPTR callback);
void devStreamWriteTimeout (stream_t *stream);
void devStreamReadTimeout (stream_t *stream);
void devStreamBusError (stream_t *public);
void devStreamWriteFinished (stream_t *public);
void devStreamEvent (stream_t *public);

#endif
