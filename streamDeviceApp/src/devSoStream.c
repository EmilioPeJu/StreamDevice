/***********************************************************
* Stream Device record interface for string output records *
*                                                          *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)      *
***********************************************************/

#include <devStream.h>
#include <stringoutRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    stringoutRecord *so = (stringoutRecord *) record;
    
    if (format->type == DBF_STRING)
    {
        return devStreamScanf (record, format, &so->val);
    }
    return ERROR;
}

static long writeData (dbCommon *record, format_t *format)
{
    stringoutRecord *so = (stringoutRecord *) record;
    
    if (format->type == DBF_STRING)
    {
        return devStreamPrintf (record, format, so->val);
    }
    return ERROR;
}

static long initRecord (dbCommon *record)
{
    stringoutRecord *so = (stringoutRecord *) record;
    long status;
    
    status = devStreamInitRecord (record, &so->out, readData, writeData);
    return status ? status : DO_NOT_CONVERT;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_stringout;
} devSoStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamWrite
};
