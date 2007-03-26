/******************************************************
* Stream Device record interface for waveform records *
*                                                     *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de) *
******************************************************/
#include <devStream.h>
#include <waveformRecord.h>

static long readData (dbCommon *record, format_t *format)
{
    waveformRecord *wf = (waveformRecord *) record;
    double dval;
    long lval;
    ulong_t nord;
    
    for (nord = 0; nord < wf->nelm; nord++)
    {
        if (nord > 0 && devStreamScanSep (record) != OK)
        {
            #ifdef DEBUG
            printErr ("readData %s: no more separator after %d fields\n",
                record->name, nord);
            #endif
            break;
        }        
        if (format->type == DBF_DOUBLE)
        {
            if (devStreamScanf (record, format, &dval))
            {
                printErr ("readData %s: no float found at index %d\n",
                    record->name, nord);
                return ERROR;
            }
            switch (wf->ftvl)
            {
                case DBF_DOUBLE:
                    ((double *)wf->bptr)[nord] = dval;
                    break;
                case DBF_FLOAT:
                    ((float *)wf->bptr)[nord] = dval;
                    break;
                default:
                    printErr ("readData %s: can't convert from double to %s\n",
                        record->name, pamapdbfType[wf->ftvl].strvalue);
                    return ERROR;
            }
            #ifdef DEBUG
            printErr ("readData %s: field %d = %#g\n",
                record->name, nord, dval);
            #endif
        }
        else if (format->type == DBF_LONG || format->type == DBF_ENUM)
        {
            if (devStreamScanf (record, format, &lval))
            {
                printErr ("readData %s: no int found at index %d\n",
                    record->name, nord);
                return ERROR;
            }
            switch (wf->ftvl)
            {
                case DBF_DOUBLE:
                    ((double *)wf->bptr)[nord] = lval;
                    break;
                case DBF_FLOAT:
                    ((float *)wf->bptr)[nord] = lval;
                    break;
                case DBF_LONG:
                case DBF_ULONG:
                    ((long *)wf->bptr)[nord] = lval;
                    break;
                case DBF_SHORT:
                case DBF_USHORT:
                case DBF_ENUM:
                    ((short *)wf->bptr)[nord] = lval;
                    break;
                case DBF_CHAR:
                case DBF_UCHAR:
                    ((char *)wf->bptr)[nord] = lval;
                    break;
                default:
                    printErr ("readData %s: can't convert from long to %s\n",
                        record->name, pamapdbfType[wf->ftvl].strvalue);
                    return ERROR;
            }
            #ifdef DEBUG
            printErr ("readData %s: field %d = %li\n",
                record->name, nord, lval);
            #endif
        }
        else if (format->type == DBF_STRING && wf->ftvl == DBF_STRING)
        {
            if (devStreamScanf (record, format,
                ((char *)wf->bptr) + nord * MAX_STRING_SIZE))
                return ERROR;
            #ifdef DEBUG
            printErr ("readData %s: field %d = \"%s\"\n",
                record->name, nord,
                ((char *)wf->bptr) + nord * MAX_STRING_SIZE);
            #endif
        }
        else
        {
            printErr ("readData %s: can't convert from %s to %s\n",
                record->name, pamapdbfType[format->type].strvalue, 
                pamapdbfType[wf->ftvl].strvalue);
            return ERROR;
        }
    }
    #ifdef DEBUG
    printErr ("readData %s: %d fields found\n",
        record->name, nord);
    #endif
    wf->nord = nord;
    wf->rarm = 0;
    return OK;
}

static long writeData (dbCommon *record, format_t *format)
{
    waveformRecord *wf = (waveformRecord *) record;
    double dval;
    long lval;
    ulong_t nowd;
    
    for (nowd = 0; nowd < wf->nord; nowd++)
    {
        if (nowd > 0 && devStreamPrintSep (record) != OK)
        {
            return ERROR;
        }       
        if (format->type == DBF_DOUBLE)
        {
            switch (wf->ftvl)
            {
                case DBF_DOUBLE:
                    dval = ((double *)wf->bptr)[nowd];
                    break;
                case DBF_FLOAT:
                    dval = ((float *)wf->bptr)[nowd];
                    break;
                case DBF_LONG:
                    dval = ((long *)wf->bptr)[nowd];
                    break;
                case DBF_ULONG:
                    dval = ((ulong_t *)wf->bptr)[nowd];
                    break;
                case DBF_SHORT:
                    dval = ((short *)wf->bptr)[nowd];
                    break;
                case DBF_USHORT:
                case DBF_ENUM:
                    dval = ((ushort_t *)wf->bptr)[nowd];
                    break;
                case DBF_CHAR:
                    dval = ((char *)wf->bptr)[nowd];
                    break;
                case DBF_UCHAR:
                    dval = ((uchar_t *)wf->bptr)[nowd];
                    break;
                default:
                    printErr ("writeData %s: can't convert from %s to double\n",
                        record->name, pamapdbfType[wf->ftvl].strvalue);
                    return ERROR;
            }
            if (devStreamPrintf (record, format, dval))
                return ERROR;
        }
        else if (format->type == DBF_LONG || format->type == DBF_ENUM)
        {
            switch (wf->ftvl)
            {
                case DBF_LONG:
                case DBF_ULONG:
                    lval = ((long *)wf->bptr)[nowd];
                    break;
                case DBF_SHORT:
                    lval = ((short *)wf->bptr)[nowd];
                    break;
                case DBF_USHORT:
                case DBF_ENUM:
                    lval = ((ushort_t *)wf->bptr)[nowd];
                    break;
                case DBF_CHAR:
                    lval = ((char *)wf->bptr)[nowd];
                    break;
                case DBF_UCHAR:
                    lval = ((uchar_t *)wf->bptr)[nowd];
                    break;
                default:
                    printErr ("writeData %s: can't convert from %s to long\n",
                        record->name, pamapdbfType[wf->ftvl].strvalue);
                    return ERROR;
            }
            if (devStreamPrintf (record, format, lval))
                return ERROR;
        }
        else if (format->type == DBF_STRING && wf->ftvl == DBF_STRING)
        {
            if (devStreamPrintf (record, format,
                ((char *)wf->bptr) + nowd * MAX_STRING_SIZE))
                return ERROR;
        }
        else
        {
            printErr ("writeData %s: can't convert from %s to %s\n",
                record->name, pamapdbfType[wf->ftvl].strvalue,
                pamapdbfType[format->type].strvalue);
            return ERROR;
        }
    }
    return OK;
}

static long initRecord (dbCommon *record)
{
    waveformRecord *wf = (waveformRecord *) record;
    
    return devStreamInitRecord (record, &wf->inp, readData, writeData);
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_wf;
} devWfStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamRead
};

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_wf;
} devWfOutStream = {
    5,
    devStreamReport,
    devStreamInit,
    initRecord,
    devStreamGetIointInfo,
    devStreamWrite
};

