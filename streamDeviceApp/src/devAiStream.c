/**********************************************************
* Stream Device record interface for analog input records *
*                                                         *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)     *
**********************************************************/

#include <devStream.h>
#include <aiRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    aiRecord *ai = (aiRecord *) record;
    double val;
    
    if (format->type == DBF_DOUBLE)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (ai->aslo != 0) val *= ai->aslo;
        ai->val = val + ai->aoff;
        return DO_NOT_CONVERT;
    }
    if (format->type == DBF_LONG)
    {
        return devStreamScanf (record, format, &ai->rval);
    }
    return ERROR;
}

#define writeData NULL

static long initRecord (dbCommon *record)
{
    aiRecord *ai = (aiRecord *) record;
    
    return devStreamInitRecord (record, &ai->inp, readData, writeData);
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
} devAiStream = {
    6,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamRead,
    NULL
};

#ifdef ABOVE_EPICS_R3_13
#include "epicsExport.h"
epicsExportAddress(dset, devAiStream);
#endif
