/**********************************************************
* Stream Device record interface for binary input records *
*                                                         *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)     *
**********************************************************/

#include <devStream.h>
#include <biRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    biRecord *bi = (biRecord *) record;
    ulong_t val;
    
    if (format->type == DBF_LONG)
    {
        return devStreamScanf (record, format, &bi->rval);
    }
    if (format->type == DBF_ENUM)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (val > 1) return ERROR;
        bi->val = val;
        return DO_NOT_CONVERT;
    }
    return ERROR;
}

#define writeData NULL

static long initRecord (dbCommon *record)
{
    biRecord *bi = (biRecord *) record;
    
    return devStreamInitRecord (record, &bi->inp, readData, writeData);
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbi;
} devBiStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamRead
};

#ifdef ABOVE_EPICS_R3_13
#include "epicsExport.h"
epicsExportAddress(dset, devBiStream);
#endif
