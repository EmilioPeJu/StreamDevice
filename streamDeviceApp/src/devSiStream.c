/**********************************************************
* Stream Device record interface for string input records *
*                                                         *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)     *
**********************************************************/

#include <devStream.h>
#include <stringinRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    stringinRecord *si = (stringinRecord *) record;
    
    if (format->type == DBF_STRING)
    {
        return devStreamScanf (record, format, &si->val);
    }
    return ERROR;
}

#define writeData NULL

static long initRecord (dbCommon *record)
{
    stringinRecord *si = (stringinRecord *) record;
    
    return devStreamInitRecord (record, &si->inp, readData, writeData);
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_stringin;
} devSiStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamRead
};


