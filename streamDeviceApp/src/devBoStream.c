/***********************************************************
* Stream Device record interface for binary output records *
*                                                          *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)      *
***********************************************************/

#include <devStream.h>
#include <boRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    boRecord *bo = (boRecord *) record;
    ulong_t val;
    
    if (format->type == DBF_LONG)
    {
        return devStreamScanf (record, format, &bo->rbv);
    }
    if (format->type == DBF_ENUM)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (val > 1) return ERROR;
        bo->val = val;
        return DO_NOT_CONVERT;
    }
    return ERROR;
}

static long writeData (dbCommon *record, format_t *format)
{
    boRecord *bo = (boRecord *) record;
    
    if (format->type == DBF_LONG)
    {
        return devStreamPrintf (record, format, bo->rval);
    }
    if (format->type == DBF_ENUM)
    {
        return devStreamPrintf (record, format, (long) bo->val);
    }
    if (format->type == DBF_STRING)
    {
        return devStreamPrintf (record, format, bo->val ? bo->onam : bo->znam);
    }
    return ERROR;
}

static long initRecord (dbCommon *record)
{
    boRecord *bo = (boRecord *) record;
    long status;
    
    status = devStreamInitRecord (record, &bo->out, readData, writeData);
    return status ? status : DO_NOT_CONVERT;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_bo;
} devBoStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamWrite
};

#ifdef ABOVE_EPICS_R3_13
#include "epicsExport.h"
epicsExportAddress(dset, devBoStream);
#endif
