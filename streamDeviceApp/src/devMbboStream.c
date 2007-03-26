/********************************************************************
* Stream Device record interface for multibit binary output records *
*                                                                   *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)               *
********************************************************************/

#include <devStream.h>
#include <mbboRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    mbboRecord *mbbo = (mbboRecord *) record;
    long val;
    
    if (format->type == DBF_LONG)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        mbbo->rbv = val & mbbo->mask;
        return OK;
    }
    if (format->type == DBF_ENUM)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (val >= 16) return ERROR;
        mbbo->val = val;
        return DO_NOT_CONVERT;
    }
    return ERROR;     
}

static long writeData (dbCommon *record, format_t *format)
{
    mbboRecord *mbbo = (mbboRecord *) record;
    
    if (format->type == DBF_LONG)
    {
        return devStreamPrintf (record, format, mbbo->rval & mbbo->mask);
    }
    if (format->type == DBF_ENUM)
    {
        return devStreamPrintf (record, format, (long) mbbo->val);
    }
    if (format->type == DBF_STRING)
    {
        return devStreamPrintf (record, format,
            mbbo->zrst + sizeof(mbbo->zrst) * mbbo->val);
    }
    return ERROR;
}

static long initRecord (dbCommon *record)
{
    mbboRecord *mbbo = (mbboRecord *) record;
    long status;
    
    mbbo->mask <<= mbbo->shft;
    status = devStreamInitRecord (record, &mbbo->out, readData, writeData);
    return status ? status : DO_NOT_CONVERT;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbbo;
} devMbboStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamWrite
};
