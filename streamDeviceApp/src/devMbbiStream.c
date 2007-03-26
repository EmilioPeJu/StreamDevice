/*******************************************************************
* Stream Device record interface for multibit binary input records *
*                                                                  *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)              *
*******************************************************************/

#include <devStream.h>
#include <mbbiRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    mbbiRecord *mbbi = (mbbiRecord *) record;
    long val;
    
    if (format->type == DBF_LONG)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (mbbi->mask == 0)
        {
            if (val >= 16) return ERROR;
            mbbi->val = val;
            return DO_NOT_CONVERT;
        }
        mbbi->rval = val & mbbi->mask;
        return OK;
    }
    if (format->type == DBF_ENUM)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (val >= 16) return ERROR;
        mbbi->val = val;
        return DO_NOT_CONVERT;
    }
    return ERROR;
}

#define writeData NULL

static long initRecord (dbCommon *record)
{
    mbbiRecord *mbbi = (mbbiRecord *) record;
    
    mbbi->mask <<= mbbi->shft;
    return devStreamInitRecord (record, &mbbi->inp, readData, writeData);
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbi;
} devMbbiStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamRead
};
