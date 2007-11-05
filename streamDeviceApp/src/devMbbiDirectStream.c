/**************************************************************************
* Stream Device record interface for multibit binary input direct records *
*                                                                         *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)                     *
**************************************************************************/

#include <devStream.h>
#include <mbbiDirectRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    mbbiDirectRecord *mbbiD = (mbbiDirectRecord *) record;
    long val;
   
    if (format->type == DBF_LONG)
    {
        if (devStreamScanf (record, format, &val)) return ERROR;
        if (mbbiD->mask == 0)
        {
/***** BUG FIX - Pete Owens - 14-9-04 (mbbiDirect can hold 16 bit values)
            if (val >= 16) return ERROR;
*****/
            if (val >= (1<<16)) return ERROR;
            mbbiD->val = val;
            return DO_NOT_CONVERT;
        }
        mbbiD->rval = val & mbbiD->mask;
        return OK;
    }
    return ERROR;
}

#define writeData NULL

static long initRecord (dbCommon *record)
{
    mbbiDirectRecord *mbbiD = (mbbiDirectRecord *) record;
   
    mbbiD->mask <<= mbbiD->shft;
    return devStreamInitRecord (record, &mbbiD->inp, readData, writeData);
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_mbbiDirect;
} devMbbiDirectStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamRead
};

#ifdef ABOVE_EPICS_R3_13
#include "epicsExport.h"
epicsExportAddress(dset, devMbbiDirectStream);
#endif
