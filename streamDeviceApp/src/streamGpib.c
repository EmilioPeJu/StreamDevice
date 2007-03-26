/**************************************************************
* Stream Device bus interface to GPIB                         *
*                                                             *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)         *
*                                                             *
* This interface depends on my Universal GPIB Driver package. *
**************************************************************/

#include <devStream.h>
#include <drvUniGpib.h>
#include <stdlib.h>
#include <devLib.h>

long streamGpibInitRecord (stream_t *stream, char *busName, char *linkString);
long streamGpibWritePart (stream_t *stream, char* data, long length);
void streamGpibStartProtocol (stream_t *stream);
void streamGpibStopProtocol (stream_t *stream);
void streamGpibStartInput (stream_t *stream);
#define streamGpibStopInput NULL

streamBusInterface_t streamGpib = {
    streamGpibInitRecord,
    streamGpibWritePart,
    streamGpibStartInput,
    streamGpibStopInput,
    streamGpibStartProtocol,
    streamGpibStopProtocol,
    31
};

typedef struct streamlist_s {
    struct streamlist_s *next;
    void *driverId;
    stream_t *connected[31];
} streamlist_t;

typedef struct {
    stream_t *next;   /* streams connected to the same channel */
    void *driverId;
    ushort_t address;
    char inpSRQ, errSRQ, evtSRQ;
} streamGpibPrivate_t;

LOCAL streamlist_t *streamlists = NULL;

LOCAL void SRQCallback (stream_t **streamlist, int statusByte, int size);
LOCAL void written (stream_t *stream, int reason, int size);
LOCAL void receive (stream_t **streamlist, int reason, int size);
LOCAL void receiveOne (stream_t *stream, int reason, int size);

/* Link Format:
   @protocolFile protocol busname address [inpSRQ [errSRQ [evtSRQ]]] 

   protocolFile is the name of a file in STREAM_PROTOCOL_DIR
   protocol is one of the protocols in the protocolFile
   busname is one of the gpib busses
   address is the GPIB address on the bus (0...30 [+256*secondary_address])
   xxxSRQ are optional. They are bitmasks for the status byte that signal
   available input, error, and events, respectively. If they are omitted,
   no SRQ is used and the bus is blocked while waiting for reply.
   Always use SRQ if the SCAN field is "I/O Intr" otherwise the bus could
   be blocked for a long time! Make sure that the device will send SRQs!

   STREAM_PROTOCOL_DIR should be set in the startup file (default "/")
*/
long streamGpibInitRecord (stream_t *stream, char *busName, char *linkString)
{
    streamGpibPrivate_t *busPrivate;
    ushort_t address, inp = 0, err = 0, evt = 0;
    streamlist_t *list;
    int i;

    busPrivate = (streamGpibPrivate_t *) malloc (sizeof (streamGpibPrivate_t));
    if (busPrivate == NULL)
    {
        printErr ("streamGpibInitRecord %s: out of memory\n",
            stream->record->name);
        return S_dev_noMemory;
    }
    busPrivate->driverId = gpibDriverId (busName);
    if (busPrivate->driverId == NULL)
    {
        printErr ("streamGpibInitRecord %s: can't open bus \"%s\"\n",
            stream->record->name, busName);
        return S_db_badField;
    }
    if (sscanf (linkString, "%hu%hu%hu%hu", &address, &inp, &err, &evt) < 1 || 
        (address & 0xFF) > 30 || inp > 0xFF || err > 0xFF || evt > 0xFF)
    {
        printErr ("streamGpibInitRecord %s: illegal link parameter '%s'\n",
            stream->record->name, linkString);
        return S_db_badField;
    }
    busPrivate->address = address;
    address &= 0xFF;
    busPrivate->inpSRQ = inp;
    busPrivate->errSRQ = err;
    busPrivate->evtSRQ = evt;
    /* make 31 linked lists for each driver */
    for (list = streamlists; list != NULL; list = list->next)
    {
        if (list->driverId == busPrivate->driverId) break;
    } 
    if (list == NULL)
    {
        #ifdef DEBUG
        printErr ("streamGpibInitRecord %s: new entry for bus %s\n",
            stream->record->name, busName);
        #endif
        list = (streamlist_t *) malloc (sizeof (streamlist_t));
        if (list == NULL)
        {
            printErr ("streamGpibInitRecord %s: out of memory\n",
                stream->record->name);
            return S_dev_noMemory;
        }
        list->driverId = busPrivate->driverId;
        for (i = 0; i < 31; i++)
        {
            list->connected[i] = NULL;
        }
        list->next = streamlists;
        streamlists = list;
    }
    /* found the list for this driver, now insert this instance */
    busPrivate->next = list->connected[address];
    list->connected[address] = stream;
    if (busPrivate->next == NULL)
    {
        gpibInstallSRQHandler (busPrivate->driverId, address,
            (gpibCallback *) SRQCallback, &list->connected[address]);
    }
    stream->busPrivate = busPrivate;
    stream->channel = address;
    return OK;
}

long streamGpibWritePart (stream_t *stream, char* data, long length)
{
    streamGpibPrivate_t *busPrivate = (streamGpibPrivate_t *)stream->busPrivate;

    #ifdef DEBUG
    printErr ("streamGpibWritePart %s: writing %d bytes to bus %p\n", 
            stream->record->name, length, busPrivate->driverId);
    #endif
    if (gpibAsyncWrite (busPrivate->driverId, busPrivate->address,
        data, length, stream->protocol.writeTimeout,
        (gpibCallback *)written, stream) != OK)
    {
        printErr ("streamGpibWritePart %s: write error\n",
            stream->record->name);
        return ERROR;
    }
    #ifdef DEBUG
    printErr ("streamGpibWritePart %s: %d bytes written\n", 
            stream->record->name, length);
    #endif
    return length;
}

void streamGpibStartInput (stream_t *stream)
{
    streamGpibPrivate_t *busPrivate = (streamGpibPrivate_t *)stream->busPrivate;
    int termlen;
    int eos = -1;

    if (stream->acceptInput)
    {
        if (busPrivate->inpSRQ == 0)
        {
            termlen = stream->protocol.inTerminator[0];
            if (termlen == 1) {
                eos = stream->protocol.inTerminator[1];
            }
            gpibAsyncRead (busPrivate->driverId, busPrivate->address,
                NULL, 0, /* use the driver's local buffer */
                eos,
                stream->protocol.replyTimeout,
                stream->protocol.readTimeout,
                (gpibCallback *) receiveOne, stream);
        }
    }
    if (stream->acceptEvent)
    {
        if (busPrivate->evtSRQ == 0)
        {
            devStreamBusError (stream);
        }
    }
}

void streamGpibStartProtocol (stream_t *stream)
{
    streamGpibPrivate_t *busPrivate = (streamGpibPrivate_t *)stream->busPrivate;

    gpibREN (busPrivate->driverId, TRUE);
}

void streamGpibStopProtocol (stream_t *stream)
{
    streamGpibPrivate_t *busPrivate = (streamGpibPrivate_t *)stream->busPrivate;

    gpibREN (busPrivate->driverId, FALSE);
}

LOCAL void SRQCallback (stream_t **streamlist, int statusByte, int size)
{
    stream_t *stream;
    streamGpibPrivate_t *busPrivate;
    int termlen;
    int eos = -1;
    void *driverId = NULL;
    ushort_t address = 0;
    ushort_t timeout = 0;
    
    #ifdef DEBUG
    printErr ("SRQCallback: status = 0x%02X\n", statusByte);
    #endif
    for (stream = *streamlist;
        stream != NULL;
        stream = busPrivate->next)
    {
        /* who is interest in the SRQ? */
        busPrivate = (streamGpibPrivate_t *)stream->busPrivate;
        if (stream->acceptInput)
        {
            if (statusByte & busPrivate->inpSRQ)
            {
                #if 1
                printErr ("SRQCallback: input for record %s\n", stream->record->name);
                #endif
                termlen = stream->protocol.inTerminator[0];
                if (termlen == 1) {
                    eos = stream->protocol.inTerminator[1];
                }
                address = busPrivate->address;
                driverId = busPrivate->driverId;
                if (stream->protocol.readTimeout > timeout)
                {
                    timeout = stream->protocol.readTimeout;
                }
            }
            else if (statusByte & busPrivate->errSRQ)
            {
                #ifdef DEBUG
                printErr ("SRQCallback: error for record %s\n", stream->record->name);
                #endif
                devStreamBusError (stream);
            }
        }
        if (stream->acceptEvent)
        {
            if (statusByte & busPrivate->evtSRQ)
            {
                #ifdef DEBUG
                printErr ("SRQCallback: event for record %s\n", stream->record->name);
                #endif
                devStreamEvent (stream);
            }
        }
    }
    /* at least one record has pending input */
    if (driverId != NULL)
    {
        gpibAsyncRead (driverId, address,
            NULL, 0, /* use the driver's local buffer */
            eos, timeout, timeout,
            (gpibCallback *)receive, streamlist);
    }
}

LOCAL void written (stream_t *stream, int reason, int size)
{
    switch (reason)
    {
        case GPIB_REASON_TIMEOUT:
            #ifdef DEBUG
            printErr ("streamGpib writeCallback %s: reason TIMEOUT\n",
                stream->record->name);
            #endif
            devStreamWriteTimeout (stream);
            break;
        case GPIB_REASON_EOI:
            #ifdef DEBUG
            printErr ("streamGpib writeCallback %s: reason EOI\n",
                stream->record->name);
            #endif
            devStreamWriteFinished (stream);
            break;
        default:
            #ifdef DEBUG
            printErr ("streamGpib writeCallback %s: reason %d\n",
                stream->record->name, reason);
            #endif
            devStreamBusError (stream);
    }
}

LOCAL void receive (stream_t **streamlist, int reason, int size)
{
    stream_t *stream;
    streamGpibPrivate_t *busPrivate;

    for (stream = *streamlist;
        stream != NULL;
        stream = busPrivate->next)
    {
        /* copy input to all waiting records */
        busPrivate = (streamGpibPrivate_t *)stream->busPrivate;
        receiveOne (stream, reason, size);
    }
}

LOCAL void receiveOne (stream_t *stream, int reason, int size)
{
    streamGpibPrivate_t *busPrivate = (streamGpibPrivate_t *)stream->busPrivate;
    int endOfInput = FALSE;

    if (stream->acceptInput)
    {
        switch (reason)
        {
            case GPIB_REASON_TIMEOUT:
                #ifdef DEBUG
                printErr ("streamGpib receiveOne %s: %d bytes, reason TIMEOUT\n",
                    stream->record->name, size);
                #endif
                break;
            case GPIB_REASON_EOI:
                #ifdef DEBUG
                printErr ("streamGpib receiveOne %s: %d bytes, reason EOI\n",
                    stream->record->name, size);
                #endif
                endOfInput = TRUE;
                break;
            case GPIB_REASON_EOS:
                #ifdef DEBUG
                printErr ("streamGpib receiveOne %s: %d bytes, reason EOS\n",
                    stream->record->name, size);
                #endif
                endOfInput = TRUE;
                break;
            case GPIB_REASON_FULL:
                #ifdef DEBUG
                printErr ("streamGpib receiveOne %s: %d bytes, reason FULL\n",
                    stream->record->name, size);
                #endif
                break;
            default:
                #ifdef DEBUG
                printErr ("streamGpib receiveOne %s: %d bytes, reason %d\n",
                    stream->record->name, reason);
                #endif
                devStreamBusError (stream);
                return;
        }
        devStreamReceive (stream, gpibLocalBuffer(busPrivate->driverId),
            size, endOfInput);
/*
 *         if (reason == GPIB_REASON_TIMEOUT)
 *         {
 *             devStreamReadTimeout (stream);
 *         }
 */
    }
}

