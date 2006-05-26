/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is the interface to EPICS for StreamDevice.             *
* Please refer to the HTML files in ../doc/ for a detailed     *
* documentation.                                               *
*                                                              *
* If you do any changes in this file, you are not allowed to   *
* redistribute it any more. If there is a bug or a missing     *
* feature, send me an email and/or your patch. If I accept     *
* your changes, they will go to the next release.              *
*                                                              *
* DISCLAIMER: If this software breaks something or harms       *
* someone, it's your problem.                                  *
*                                                              *
***************************************************************/

#include "StreamCore.h"
#include "StreamError.h"
#include "devStream.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <asynDriver.h>
#include <dbAccess.h>
#include <dbStaticLib.h>
#include <devSup.h>
#include <drvSup.h>
#include <recSup.h>
#include <recGbl.h>
#include <devLib.h>
#include <dbScan.h>
#include <alarm.h>
#include <callback.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <registryFunction.h>

#include <iocsh.h>

#include <epicsExport.h>

#if defined(__vxworks) || defined(vxWorks)
#include <symLib.h>
#include <sysSymTbl.h>
#endif

enum MoreFlags {
    // 0x0000FFFF used by StreamCore
    InDestructor  = 0x0001000,
    ValueReceived = 0x0002000
};

extern "C" void streamExecuteCommand(CALLBACK *pcallback);

class Stream : protected StreamCore, epicsTimerNotify
{
    dbCommon* record;
    struct link *ioLink;
    streamIoFunction readData;
    streamIoFunction writeData;
    epicsTimerQueueActive* timerQueue;
    epicsTimer* timer;
    epicsMutex mutex;
    epicsEvent initDone;
    StreamBuffer fieldBuffer;
    int status;
    int convert;
    long currentValueLength;
    IOSCANPVT ioscanpvt;
    CALLBACK commandCallback;


// epicsTimerNotify methods
    expireStatus expire(const epicsTime&);

// StreamCore methods
    // void protocolStartHook(); // Nothing to do here?
    void protocolFinishHook(ProtocolResult);
    void startTimer(unsigned short timeout);
    bool getFieldAddress(const char* fieldname,
        StreamBuffer& address);
    bool formatValue(const StreamFormat&,
        const void* fieldaddress);
    bool matchValue(const StreamFormat&,
        const void* fieldaddress);
    void lockMutex();
    void releaseMutex();
    bool execute();
    friend void streamExecuteCommand(CALLBACK *pcallback);

// Stream Epics methods
    long initRecord();
    Stream(dbCommon* record, struct link *ioLink,
        streamIoFunction readData, streamIoFunction writeData,
        IOSCANPVT ioscanpvt);
    ~Stream();
    bool print(format_t *format, va_list ap);
    bool scan(format_t *format, void* pvalue, size_t maxStringSize);
    bool process();

// device support functions
    friend long streamInitRecord(dbCommon *record, struct link *ioLink,
        streamIoFunction readData, streamIoFunction writeData);
    friend long streamReadWrite(dbCommon *record);
    friend long streamGetIointInfo(int cmd, dbCommon *record,
        IOSCANPVT *ppvt);
    friend long streamPrintf(dbCommon *record, format_t *format, ...);
    friend long streamScanfN(dbCommon *record, format_t *format,
        void*, size_t maxStringSize);
    friend long streamScanSep(dbCommon *record);
    friend long streamReload(char* recordname);

public:
    long priority() { return record->prio; };
    static long report(int interest);
    static long drvInit();
};


// shell functions ///////////////////////////////////////////////////////

epicsExportAddress(int, streamDebug);

#ifdef MEMGUARD
static const iocshFuncDef memguardReportDef =
    { "memguardReport", 0, NULL };

static void memguardReportFunc (const iocshArgBuf *args)
{
    memguardReport();
}
#endif

long streamReloadSub()
{
    // for subroutine record
    return streamReload(NULL);
}

long streamReload(char* recordname)
{
    DBENTRY	dbentry;
    dbCommon*   record;
    long status;

    if(!pdbbase) {
        error("No database has been loaded\n");
        return ERROR;
    }
    debug("streamReload(%s)\n", recordname);
    dbInitEntry(pdbbase,&dbentry);
    for (status = dbFirstRecordType(&dbentry); status == OK;
        status = dbNextRecordType(&dbentry))
    {
        for (status = dbFirstRecord(&dbentry); status == OK;
            status = dbNextRecord(&dbentry))
        {
            char* value;
            if (dbFindField(&dbentry, "DTYP") != OK)
                continue;
            if ((value = dbGetString(&dbentry)) == NULL)
                continue;
            if (strcmp(value, "stream") != 0)
                continue;
            record=(dbCommon*)dbentry.precnode->precord;
            if (recordname && strcmp(recordname, record->name) != 0)
                continue;

            // This cancels any running protocol and reloads
            // the protocol file
            record->dset->init_record(record);

            printf("%s: protocol reloaded\n", record->name);
        }
    }
    dbFinishEntry(&dbentry);
    StreamProtocolParser::free();
    return OK;
}

static const iocshArg streamReloadArg0 =
    { "recordname", iocshArgString };
static const iocshArg * const streamReloadArgs[] =
    { &streamReloadArg0 };
static const iocshFuncDef reloadDef =
    { "streamReload", 1, streamReloadArgs };

extern "C" void streamReloadFunc (const iocshArgBuf *args)
{
    streamReload(args[0].sval);
}

static void streamRegistrar ()
{
#ifdef MEMGUARD
    iocshRegister(&memguardReportDef, memguardReportFunc);
#endif
    iocshRegister(&reloadDef, streamReloadFunc);
    // make streamReload available for subroutine records
    registryFunctionAdd("streamReload",
        (REGISTRYFUNCTION)streamReloadSub);
}

epicsExportRegistrar(streamRegistrar);

// driver support ////////////////////////////////////////////////////////

struct {
    long number;
    long (*report)(int);
    DRVSUPFUN init;
} stream = {
    2,
    Stream::report,
    Stream::drvInit
};

epicsExportAddress(drvet, stream);

long Stream::
report(int interest)
{
    debug("Stream::report(interest=%d)\n", interest);
    printf("  %s\n", StreamVersion);
    printf("  registered bus interfaces:\n");

    StreamBusInterfaceClass interface;
    while (interface)
    {
        printf("    %s\n", interface.name());
        ++interface;
    }
    if (interest < 1) return OK;
    Stream* pstream;
    printf("  connected records:\n");
    for (pstream = static_cast<Stream*>(first); pstream;
        pstream = static_cast<Stream*>(pstream->next))
    {
        if (interest == 2)
        {
            printf("\n%s: %s\n", pstream->name(),
                pstream->ioLink->value.instio.string);
            pstream->printProtocol();
        }
        else
        {
            printf("    %s: %s\n", pstream->name(),
                pstream->ioLink->value.instio.string);
        }
    }
    return OK;
}

long Stream::
drvInit()
{
    char* path;
    debug("drvStreamInit()\n");
    path = getenv("STREAM_PROTOCOL_PATH");
#ifdef __vxworks
    // for compatibility reasons look for global symbol
    if (!path)
    {
        char* symbol;
        SYM_TYPE type;
        if (symFindByName(sysSymTbl,
            "STREAM_PROTOCOL_DIR", &symbol, &type) == OK)
        {
            path = *(char**)symbol;
        }
    }
#endif
    if (!path)
    {
        fprintf(stderr,
            "drvStreamInit: Warning! STREAM_PROTOCOL_PATH not set. "
            "Defaults to \"%s\"\n", StreamProtocolParser::path);
    }
    else
    {
        StreamProtocolParser::path = path;
    }
    debug("StreamProtocolParser::path = %s\n",
        StreamProtocolParser::path);
    return OK;
}

// device support (C interface) //////////////////////////////////////////

long streamInit(int after)
{
    if (after)
    {
        StreamProtocolParser::free();
    }
    return OK;
}

long streamInitRecord(dbCommon* record, struct link *ioLink,
    streamIoFunction readData, streamIoFunction writeData)
{
    debug("streamInitRecord(%s): SEVR=%d\n", record->name, record->sevr);
    IOSCANPVT ioscanpvt = NULL;
    Stream* pstream = (Stream*)record->dpvt;
    record->dpvt = NULL;
    if (pstream)
    {
        // we have been called by streamReload
        // keep old ioscanpvt
        // but delete old stream
        ioscanpvt = pstream->ioscanpvt;
        if (record->pact) pstream->finishProtocol(Stream::Abort);
        pstream->ioscanpvt = NULL;
        debug("streamInitRecord(%s): deleting old stream. ioscanpvt=%p\n",
            record->name, ioscanpvt);
        delete pstream;
    }
    debug("streamInitRecord(%s): creating new stream (ioscanpvt=%p)\n",
        record->name, ioscanpvt);
    pstream = new Stream(record, ioLink, readData, writeData, ioscanpvt);
    long status = pstream->initRecord();
    if (status != OK && status != DO_NOT_CONVERT)
    {
        error("%s: streamInitRecord failed\n", record->name);
        delete pstream;
        debug("streamInitRecord(%s): stream object deleted\n",
            record->name);
        return status;
    }
    record->dpvt = pstream;
    return status;
}

long streamReadWrite(dbCommon *record)
{
    Stream* pstream = (Stream*)record->dpvt;
    if (!pstream || pstream->status == ERROR)
    {
        (void) recGblSetSevr(record, UDF_ALARM, INVALID_ALARM);
        error("%s: Record not initialised correctly\n", record->name);
        return ERROR;
    }
    return pstream->process() ? pstream->convert : ERROR;
}

long streamGetIointInfo(int cmd, dbCommon *record, IOSCANPVT *ppvt)
{
    debug("streamGetIointInfo(%s,cmd=%d)\n",
        record->name, cmd);
    Stream* pstream = (Stream*)record->dpvt;
    if (!pstream) return ERROR;
    *ppvt = pstream->ioscanpvt;
    if (cmd == 0)
    {
        /* SCAN has been set to "I/O Intr" */
        if (!pstream->startProtocol(Stream::StartAsync))
        {
            error("%s: Can't start \"I/O Intr\" protocol\n",
                record->name);
            return ERROR;
        }
    }
    else
    {
        /* SCAN is no longer "I/O Intr" */
        pstream->finishProtocol(Stream::Abort);
    }
    return OK;
}

long streamPrintf(dbCommon *record, format_t *format, ...)
{
    debug("streamPrintf(%s,format=%%%c)\n",
        record->name, format->priv->conv);
    Stream* pstream = (Stream*)record->dpvt;
    if (!pstream) return ERROR;
    va_list ap;
    va_start(ap, format);
    bool success = pstream->print(format, ap);
    va_end(ap);
    return success ? OK : ERROR;
}

long streamScanSep(dbCommon* record)
{
    // depreciated
    debug("streamScanSep(%s)\n", record->name);
    Stream* pstream = (Stream*)record->dpvt;
    if (!pstream) return ERROR;
    return pstream->scanSeparator() ? OK : ERROR;
}

long streamScanfN(dbCommon* record, format_t *format,
    void* value, size_t maxStringSize)
{
    debug("streamScanfN(%s,format=%%%c,maxStringSize=%d)\n",
        record->name, format->priv->conv, maxStringSize);
    Stream* pstream = (Stream*)record->dpvt;
    if (!pstream) return ERROR;
    if (!pstream->scan(format, value, maxStringSize))
    {
        return ERROR;
    }
#ifndef NO_TEMPORARY
    debug("streamScanfN(%s) success, value=\"%s\"\n",
        record->name, StreamBuffer((char*)value).expand()());
#endif
    return OK;
}

// Stream methods ////////////////////////////////////////////////////////

Stream::
Stream(dbCommon* _record, struct link *ioLink,
    streamIoFunction readData, streamIoFunction writeData,
    IOSCANPVT ioscanpvt)
:record(_record), ioLink(ioLink), readData(readData), writeData(writeData),
    ioscanpvt(ioscanpvt)
{
    streamname = record->name;
    timerQueue = &epicsTimerQueueActive::allocate(true);
    timer = &timerQueue->createTimer();
    status = ERROR;
    convert = DO_NOT_CONVERT;
}

Stream::
~Stream()
{
    debug ("Stream destructor\n");
    flags |= InDestructor;;
    debug("~Stream(%s)\n", name());
    if (record->dpvt)
    {
        finishProtocol(Abort);
        debug("~Stream(%s): protocol finished\n", name());
        record->dpvt = NULL;
        debug("~Stream(%s): dpvt cleared\n", name());
    }
    timer->cancel();
    timer->destroy();
    debug("~Stream(%s): timer destroyed\n", name());
    timerQueue->release();
    debug("~Stream(%s): timer queue released\n", name());
}

long Stream::
initRecord()
{
    // scan link parameters: filename protocol busname addr busparam
    char filename[80];
    char protocol[80];
    char busname[80];
    int addr = -1;
    char busparam[80];
    int n=0;
    if (ioLink->type != INST_IO)
    {
        error("%s: Wrong link type %s\n", name(),
            pamaplinkType[ioLink->type].strvalue);
        return ERROR;
    }
    int items = sscanf(ioLink->value.instio.string, "%79s%79s%79s%n%i%n",
        filename, protocol, busname, &n, &addr, &n);
    if (items < 3)
    {
        error("%s: Wrong link format\n"
            "  expect \"file protocol bus addr params\"\n"
            "  in \"%s\"\n", name(),
            ioLink->value.instio.string);
        return S_dev_badInitRet;
    }
    memset(busparam, 0 ,80);
    while (isspace((unsigned char)ioLink->value.instio.string[n])) n++;
    strncpy (busparam, ioLink->value.constantStr+n, 79);

    // attach to bus interface
    if (!attachBus(busname, addr, busparam))
    {
        error("%s: Can't attach to bus %s %d\n",
            name(), busname, addr);
        return S_dev_noDevice;
    }

    // parse protocol file
    if (!parse(filename, protocol))
    {
        error("%s: Protocol parse error\n",
            name());
        return S_dev_noDevice;
    }

    // record is ready to use
    status = NO_ALARM;

    if (ioscanpvt)
    {
        // we have been called by streamReload
        debug("Stream::initRecord %s: initialize after streamReload\n",
            name());
        if (record->scan == SCAN_IO_EVENT)
        {
            debug("Stream::initRecord %s: restarting \"I/O Intr\" after reload\n",
                name());
            if (!startProtocol(StartAsync))
            {
                error("%s: Can't restart \"I/O Intr\" protocol\n",
                    name());
            }
        }
        return OK;
    }

    debug("Stream::initRecord %s: initialize the first time\n",
        name());
    // initialize the first time
    scanIoInit(&ioscanpvt);

    if (!onInit) return DO_NOT_CONVERT; // no @init handler, keep DOL

    // initialize the record from hardware
    if (!startProtocol(StartInit))
    {
        error("%s: Can't start init run\n",
            name());
        return ERROR;
    }
    debug("Stream::initRecord %s: waiting for initDone\n",
        name());
    initDone.wait();
    debug("Stream::initRecord %s: initDone.\n",
        name());

    // init run has set status and convert
    if (status != NO_ALARM)
    {
        record->stat = status;
        error("%s: Initializing record from hardware failed!\n",
            name());
        return ERROR;
    }
    debug("Stream::initRecord %s: initialized. convert=%d\n",
        name(), convert);
    return convert;
}

bool Stream::
process()
{
    MutexLock lock(this);
    if (record->pact || record->scan == SCAN_IO_EVENT)
    {
        if (status != NO_ALARM)
        {
            debug("Stream::process(%s) error status=%d\n",
                name(), status);
            (void) recGblSetSevr(record, status, INVALID_ALARM);
            return false;
        }
        debug("Stream::process(%s) ready. %s\n",
            name(), convert==2 ?
            "convert" : "don't convert");
        return true;
    }
    if (flags & InDestructor)
    {
        error("%s: Try to process while in stream destructor (try later)\n",
            name());
        (void) recGblSetSevr(record, UDF_ALARM, INVALID_ALARM);
        return false;
    }
    debug("Stream::process(%s) start\n", name());
    status = NO_ALARM;
    convert = OK;
    record->pact = true;
    if (!startProtocol(StreamCore::StartNormal))
    {
        debug("Stream::process(%s): could not start, status=%d\n",
            name(), status);
        (void) recGblSetSevr(record, status, INVALID_ALARM);
        return false;
    }
    debug("Stream::process(%s): protocol started\n", name());
    return true;
}

bool Stream::
print(format_t *format, va_list ap)
{
    long lval;
    double dval;
    char* sval;
    switch (format->type)
    {
        case DBF_ENUM:
        case DBF_LONG:
            lval = va_arg(ap, long);
            return printValue(*format->priv, lval);
        case DBF_DOUBLE:
            dval = va_arg(ap, double);
            return printValue(*format->priv, dval);
        case DBF_STRING:
            sval = va_arg(ap, char*);
            return printValue(*format->priv, sval);
    }
    error("INTERNAL ERROR (%s): Illegal format type\n", name());
    return false;
}

bool Stream::
scan(format_t *format, void* value, size_t maxStringSize)
{
    // called by streamScanfN
    long* lptr;
    double* dptr;
    char* sptr;

    // first remove old value from inputLine (if we are scanning arrays)
    consumedInput += currentValueLength;
    currentValueLength = 0;
    switch (format->type)
    {
        case DBF_LONG:
        case DBF_ENUM:
            lptr = (long*)value;
            currentValueLength = scanValue(*format->priv, *lptr);
            break;
        case DBF_DOUBLE:
            dptr = (double*)value;
            currentValueLength = scanValue(*format->priv, *dptr);
            break;
        case DBF_STRING:
            sptr = (char*)value;
            currentValueLength  = scanValue(*format->priv, sptr, maxStringSize);
            break;
        default:
            error("INTERNAL ERROR (%s): Illegal format type\n", name());
            return false;
    }
    if (currentValueLength < 0) return false;
    // Don't remove scanned value from inputLine yet, because
    // we might need the string in a later error message.
    return true;
}

// epicsTimerNotify virtual method ///////////////////////////////////////

Stream::expireStatus Stream::
expire(const epicsTime&)
{
    timerCallback();
    return noRestart;
}

// StreamCore virtual methods ////////////////////////////////////////////

void Stream::
protocolFinishHook(ProtocolResult result)
{
    switch (result)
    {
        case Success:
            status = NO_ALARM;
            if (flags & ValueReceived)
            {
                record->udf = false;
                if (flags & InitRun)
                {
                    // records start with UDF/INVALID,
                    // but now this record has a value
                    record->sevr = NO_ALARM;
                    record->stat = NO_ALARM;
                }
            }
            break;
        case LockTimeout:
        case ReplyTimeout:
            status = TIMEOUT_ALARM;
            break;
        case WriteTimeout:
            status = WRITE_ALARM;
            break;
        case ReadTimeout:
            status = READ_ALARM;
            break;
        case ScanError:
        case FormatError:
            status = CALC_ALARM;
            break;
        case Abort:
        case Fault:
            status = UDF_ALARM;
            if (record->pact || record->scan == SCAN_IO_EVENT)
                error("%s: Protocol aborted\n", name());
            break;
        default:
            status = UDF_ALARM;
            error("INTERNAL ERROR (%s): Illegal protocol result\n",
                name());
            break;

    }
    if (flags & InitRun)
    {
        initDone.signal();
        return;
    }
    if (record->pact || record->scan == SCAN_IO_EVENT)
    {
        debug("Stream::protocolFinishHook(stream=%s,result=%d) "
            "processing record\n", name(), result);
        // process record
        // This will call streamReadWrite.
        dbScanLock(record);
        ((DEVSUPFUN)record->rset->process)(record);
        dbScanUnlock(record);

        debug("Stream::protocolFinishHook(stream=%s,result=%d) done.\n",
            name(), result);
    }
    if ((record->scan == SCAN_IO_EVENT))
    {
        // restart protocol for next turn
        debug("Stream::process(%s) restart async protocol\n",
            name());
        if (!startProtocol(StartAsync))
        {
            error("%s: Can't restart \"I/O Intr\" protocol\n",
                name());
        }
    }
}

void Stream::
startTimer(unsigned short timeout)
{
    debug("Stream::startTimer(stream=%s, timeout=%hu) = %f seconds\n",
        name(), timeout, timeout * 0.001);
    timer->start(*this, timeout * 0.001);
}

bool Stream::
getFieldAddress(const char* fieldname, StreamBuffer& address)
{
    DBADDR dbaddr;
    char fullname[PVNAME_SZ + 1];
    sprintf(fullname, "%s.%s", name(), fieldname);
    if (dbNameToAddr(fullname, &dbaddr) != OK) return false;
    address.append(&dbaddr, sizeof(dbaddr));
    return true;
}

static const short dbfMapping[] =
    {0, DBF_LONG, DBF_ENUM, DBF_DOUBLE, DBF_STRING};
static const short typeSize[] =
    {0, sizeof(epicsInt32), sizeof(epicsUInt16),
        sizeof(epicsFloat64), MAX_STRING_SIZE};

bool Stream::
formatValue(const StreamFormat& format, const void* fieldaddress)
{
    debug("Stream::formatValue(%s, format=%%%c, fieldaddr=%p\n",
        name(), format.conv, fieldaddress);

// --  If SCAN is "I/O Intr" and record has not been processed,     --
// --  do it now to get the latest value (only for output records?) --

    if (fieldaddress)
    {
        DBADDR* pdbaddr = (DBADDR*)fieldaddress;
        long i;
        long nelem = pdbaddr->no_elements;
        size_t size = nelem * typeSize[format.type];
        char* buffer = fieldBuffer.clear().reserve(size);
        if (dbGet(pdbaddr, dbfMapping[format.type], buffer,
            NULL, &nelem, NULL) != 0)
        {
            error("%s: dbGet() failed\n", name());
            return false;
        }
        for (i = 0; i < nelem; i++)
        {
            switch (format.type)
            {
                case enum_format:
                    if (!printValue(format,
                        (long)((epicsUInt16*)buffer)[i]))
                        return false;
                    break;
                case long_format:
                    if (!printValue(format,
                        (long)((epicsInt32*)buffer)[i]))
                        return false;
                    break;
                case double_format:
                    if (!printValue(format,
                        (double)((epicsFloat64*)buffer)[i]))
                        return false;
                    break;
                case string_format:
                    if (!printValue(format, buffer+MAX_STRING_SIZE*i))
                        return false;
                    break;
                default:
                    error("INTERNAL ERROR %s: Illegal format.type=%s\n",
                        name(),
                        StreamProtocolParser::formatTypeStr(format.type));
                    return false;
            }
        }
        return true;
    }
    format_s fmt;
    fmt.type = dbfMapping[format.type];
    fmt.priv = &format;
    debug("Stream::formatValue(%s) format=%%%c type=%s\n",
        name(), format.conv, pamapdbfType[fmt.type].strvalue);
    if (!writeData)
    {
        error("%s: No writeData() function provided\n", name());
        return false;
    }
    if (writeData(record, &fmt) == ERROR)
    {
        debug("Stream::formatValue(%s): writeData failed\n", name());
        return false;
    }
    return true;
}

bool Stream::
matchValue(const StreamFormat& format, const void* fieldaddress)
{
    // this function must increase consumedInput
    long consumed;
    if (fieldaddress)
    {
        DBADDR* pdbaddr = (DBADDR*)fieldaddress;
        long nord;
        long nelem = pdbaddr->no_elements;
        size_t size = nelem * typeSize[format.type];
        char* buffer = fieldBuffer.clear().reserve(size);
        for (nord = 0; nord < nelem; nord++)
        {
            switch (format.type)
            {
                case long_format:
                {
                    long lval;
                    consumed = scanValue(format, lval);
                    if (consumed < 0) goto noMoreElements;
                    ((epicsInt32*)buffer)[nord] = lval;
                    break;
                }
                case enum_format:
                {
                    long lval;
                    consumed = scanValue(format, lval);
                    if (consumed < 0) goto noMoreElements;
                    ((epicsUInt16*)buffer)[nord] = lval;
                    break;
                }
                case double_format:
                {
                    double dval;
                    consumed = scanValue(format, dval);
                    if (consumed < 0) goto noMoreElements;
                    ((epicsFloat64*)buffer)[nord] = dval;
                    break;
                }
                case string_format:
                {
                    consumed = scanValue(format,
                        buffer+MAX_STRING_SIZE*nord, MAX_STRING_SIZE);
                    if (consumed < 0) goto noMoreElements;
                    break;
                }
                default:
                    error("INTERNAL ERROR: Stream::matchValue %s: "
                        "Illegal format type\n", name());
                    return false;
            }
            consumedInput += consumed;
        }
noMoreElements:
        if (!nord) return false;
        if (dbPut(pdbaddr, dbfMapping[format.type],
            fieldBuffer(), nord) != 0)
        {
            flags &= ~ScanTried;
            error("%s: dbPut failed\n", name());
            return false;
        }
        return true;
    }
    // no fieldaddress
    format_s fmt;
    fmt.type = dbfMapping[format.type];
    fmt.priv = &format;
    if (!readData)
    {
        error("%s: No readData() function provided\n", name());
        return false;
    }
    currentValueLength = 0;
    convert = readData(record, &fmt); // this will call scan()
    if (convert == ERROR)
    {
        debug("Stream::matchValue(%s): readData failed\n", name());
        if (currentValueLength > 0)
        {
            error("%s: Record does not accept input \"%s%s\"\n",
                name(), inputLine.expand(consumedInput, 19)(),
                inputLine.length()-consumedInput > 20 ? "..." : "");
            flags &= ~ScanTried;
        }
        return false;
    }
    flags |= ValueReceived;
    consumedInput += currentValueLength;
    return true;
}

// There is no header file for this
extern "C" int iocshCmd (const char *cmd);

extern "C" void streamExecuteCommand(CALLBACK *pcallback)
{
    Stream* pstream = static_cast<Stream*>(pcallback->user);
    
    if (iocshCmd(pstream->outputLine()) != OK)
    {
        pstream->execCallback(StreamBusInterface::ioFault);
    }
    else
    {
        pstream->execCallback(StreamBusInterface::ioSuccess);
    }
}

bool Stream::
execute()
{
    callbackSetCallback(streamExecuteCommand, &commandCallback);
    callbackSetUser(priority(), &commandCallback);
    callbackSetUser(this, &commandCallback);
    callbackRequest(&commandCallback);
    return true;
}

void Stream::
lockMutex()
{
    mutex.lock();
}

void Stream::
releaseMutex()
{
    mutex.unlock();
}
