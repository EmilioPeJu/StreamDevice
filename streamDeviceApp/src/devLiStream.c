/********************************************************
* Stream Device record interface for long input records *
*                                                       *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)   *
********************************************************/

#include <devStream.h>
#include <longinRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    longinRecord *li = (longinRecord *) record;
    
    if ((format->type == DBF_LONG ||
        format->type == DBF_ENUM))
    {
        return devStreamScanf (record, format, &li->val);
    }
    return ERROR;
}

#define writeData NULL

static long initRecord (dbCommon *record)
{
    longinRecord *li = (longinRecord *) record;
    
    return devStreamInitRecord (record, &li->inp, readData, writeData);
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_longin;
} devLiStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamRead
};

#ifdef ABOVE_EPICS_R3_13
#include "epicsExport.h"
epicsExportAddress(dset, devLiStream);
#endif
