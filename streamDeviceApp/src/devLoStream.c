/*********************************************************
* Stream Device record interface for long output records *
*                                                        *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)    *
*********************************************************/

#include <devStream.h>
#include <longoutRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    longoutRecord *lo = (longoutRecord *) record;
    
    if (format->type == DBF_LONG ||
        format->type == DBF_ENUM)
    {
        return devStreamScanf (record, format, &lo->val);
    }
    return ERROR;
}

static long writeData (dbCommon *record, format_t *format)
{
    longoutRecord *lo = (longoutRecord *) record;
    
    if (format->type == DBF_LONG ||
        format->type == DBF_ENUM)
    {
        return devStreamPrintf (record, format, lo->val);
    }
    return ERROR;
}

static long initRecord (dbCommon *record)
{
    longoutRecord *lo = (longoutRecord *) record;
    long status;
    
    status = devStreamInitRecord (record, &lo->out, readData, writeData);
    return status ? status : DO_NOT_CONVERT;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_longout;
} devLoStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamWrite
};
