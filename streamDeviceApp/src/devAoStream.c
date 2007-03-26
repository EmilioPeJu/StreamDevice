/***********************************************************
* Stream Device record interface for analog output records *
*                                                          *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)      *
***********************************************************/

#include <devStream.h>
#include <aoRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    aoRecord *ao = (aoRecord *) record;
    double val;
    
    if (format->type == DBF_DOUBLE)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (ao->aslo != 0) val *= ao->aslo;
        ao->val = val + ao->aoff;
        return DO_NOT_CONVERT;
    }
    if (format->type == DBF_LONG)
    {
        return devStreamScanf (record, format, &ao->rbv);
    }
    return ERROR;
}

static long writeData (dbCommon *record, format_t *format)
{
    aoRecord *ao = (aoRecord *) record;
    double val;
    
    if (format->type == DBF_DOUBLE)
    {
        val = ao->oval - ao->aoff;
        if (ao->aslo != 0) val /= ao->aslo;
        return devStreamPrintf (record, format, val);
    }
    if (format->type == DBF_LONG)
    {
        return devStreamPrintf (record, format, ao->rval);
    }
    return ERROR;
}

static long initRecord (dbCommon *record)
{
    aoRecord *ao = (aoRecord *) record;
    long status;
    
    status = devStreamInitRecord (record, &ao->out, readData, writeData);
    return status ? status : DO_NOT_CONVERT;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_ao;
    DEVSUPFUN special_linconv;
} devAoStream = {
    6,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamWrite,
    NULL
};
