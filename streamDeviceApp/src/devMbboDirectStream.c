/***************************************************************************
* Stream Device record interface for multibit binary output direct records *
*                                                                          *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)                      *
***************************************************************************/

#include <devStream.h>
#include <mbboDirectRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    mbboDirectRecord *mbboD = (mbboDirectRecord *) record;
    long rbv;
    
    if (format->type == DBF_LONG)
    {
        if (devStreamScanf (record, format, &rbv)) return ERROR;
        mbboD->rbv = rbv & mbboD->mask;
        return OK;
    }
    return ERROR;
}

static long writeData (dbCommon *record, format_t *format)
{
    mbboDirectRecord *mbboD = (mbboDirectRecord *) record;
    
    if (format->type == DBF_LONG)
    {
        return devStreamPrintf (record, format, mbboD->rval & mbboD->mask);
    }
    return ERROR;
}

static long initRecord (dbCommon *record)
{
    mbboDirectRecord *mbboD = (mbboDirectRecord *) record;
    long status;
    
    mbboD->mask <<= mbboD->shft;
    status = devStreamInitRecord (record, &mbboD->out, readData, writeData);
    return status ? status : DO_NOT_CONVERT;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_mbboDirect;
} devMbboDirectStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamWrite
};

#ifdef ABOVE_EPICS_R3_13
#include "epicsExport.h"
epicsExportAddress(dset, devMbboDirectStream);
#endif
