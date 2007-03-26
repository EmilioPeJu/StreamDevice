/*******************************************************************************
* streamTty.c - Stream Device bus interface to TTY devices
*
* (C) 2001 Pete Owens (p.h.owens@dl.ac.uk)
*
* This interface is for serial devices with VxWorks tty drivers  
*
* Link Format:
*   @protocolFile protocol busname address [inpSRQ [errSRQ [evtSRQ]]] 
*
*   protocolFile is the name of a file in STREAM_PROTOCOL_DIR
*   protocol is one of the protocols in the protocolFile
*   busname is one of the tty busses eg tyCo_1 => /tyCo/1
*           busname is put on the symbol table so cannot contain a '/'
*           the convention for converting a tty device name to busname
*           is to omit the first '/' and replace any others with '_'
*
*   STREAM_PROTOCOL_DIR should be set in the startup file (default "/")
*/

#include <devStream.h>
#include <ioLib.h>
#include <stdlib.h>
#include <devLib.h>
#include <taskLib.h>
#include <string.h>

int streamTtyDebug = FALSE;

LOCAL long initRecord (stream_t *stream, char *busName, char *linkString);
LOCAL long writePart  (stream_t *stream, char* data, long length);

#define startInput    NULL
#define stopInput     NULL
#define startProtocol NULL
#define stopProtocol  NULL
#define numChannels   1

streamBusInterface_t streamTty = {
    initRecord,
    writePart,
    startInput,
    stopInput,
    startProtocol,
    stopProtocol,
    numChannels
};

typedef struct streamList_s {
    struct streamList_s *next;
    stream_t            *stream;
} streamList_t;

typedef struct streamTtyPrivate_s {
    struct streamTtyPrivate_s *next;
    streamList_t              *streamList;
    char                      *name;
    int                        fd;      /* file descriptor for open device */
    int                        tid;     /* task id for reading task        */
} streamTtyPrivate_t;

LOCAL streamTtyPrivate_t *busList = NULL;

LOCAL void timeout    (stream_t *stream);
LOCAL void getInput   (streamTtyPrivate_t *bus);

/*
* use default proprties for reading task
*/
#define TASK_NAME  "tStreamTty"
#define TASK_PRIO  100
#define TASK_OPTS  (VX_SUPERVISOR_MODE | VX_FP_TASK | VX_STDIO)
#define TASK_STACK 20000

/*******************************************************************************
* initRecord   
*/
LOCAL long initRecord (stream_t *stream, char *busName, char *linkString)
{
    streamTtyPrivate_t *bus;
    streamList_t *streamList;
    char fileName[80];  /* should be long enough for any reasonable device */
    char *f = fileName;

    if (streamTtyDebug)
        printf ("streamTty initRecord: %s\n", stream->record->name);  

    /*
    * Find the bus matching name from the list
    */
    for (bus = busList; bus != NULL; bus = bus->next)
        if (strcmp (bus->name, busName) == 0)
            break;

    /*
    * If no match then create a new one
    */
    if (bus == NULL)
    {
        bus = (streamTtyPrivate_t *) malloc (sizeof (streamTtyPrivate_t));
        if (bus == NULL)
        {
            printErr ("streamTtyInitRecord %s: out of memory\n", stream->record->name);
            return S_dev_noMemory;
        }

        bus->streamList = NULL;
    
        bus->name = malloc (strlen (busName) + 1);
        if (bus->name == NULL)
        {
            printErr ("streamTtyInitRecord %s: out of memory\n", stream->record->name);
            return S_dev_noMemory;
        }
        strcpy (bus->name, busName);
    
        for (f = fileName, *f++ = '/'; *busName; busName++)
            *f++ = (*busName == '_') ? '/' : *busName;
        *f = '\0';
        bus->fd = open (fileName, O_RDWR, 0);
        if (bus->fd == ERROR)
        {
            printErr ("streamTtyInitRecord %s: can't open device: %s\n", stream->record->name, fileName);
            return S_db_badField;
        }

        bus->tid = taskSpawn (TASK_NAME, TASK_PRIO, TASK_OPTS, TASK_STACK,
                              (FUNCPTR) getInput, (int) bus, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (bus->tid == ERROR)
        {
            printErr ("streamTtyInitRecord %s: can't start getInput task\n", stream->record->name);
            return S_dev_badInitRet;
        }

        /*
        * Add bus to busList
        */
        bus->next = busList;
        busList = bus;
    }

    stream->channel = 0;
    /*
    * Add stream to streamList
    */
    streamList = (streamList_t *) malloc (sizeof (streamList_t));
    if (streamList == NULL)
    {
        printErr ("streamTtyInitRecord %s: out of memory\n", stream->record->name);
        return S_dev_noMemory;
    }
    streamList->stream = stream;
    streamList->next = bus->streamList;
    bus->streamList = streamList;

    stream->busPrivate = bus;
    return OK;
}
/*******************************************************************************
* writePart   
*/
LOCAL long writePart (stream_t *stream, char* data, long length)
{
    
    streamTtyPrivate_t *busPrivate = (streamTtyPrivate_t *)stream->busPrivate;
    int written;

    written = write (busPrivate->fd, data, length);

    if (streamTtyDebug)
        printf ("streamTty writePart %s: writen %d of %d bytes to tty\n", 
                   stream->record->name, written, length);

    if (written == length)
    {
        devStreamWriteFinished (stream);
        if (streamTtyDebug)
            printErr("Write Finished\n");
    }
    else if (written == ERROR)
    {
        devStreamBusError (stream);
        if (streamTtyDebug)
            printErr("Write Error\n");
    }

    return written;
}
/*******************************************************************************
* getInput    reads any available input from the device
*             and send it on to all accepting streams
*/
LOCAL void getInput (streamTtyPrivate_t *bus)
{
    char c = '\0';
    stream_t *stream = NULL;
    streamList_t *list = NULL;
    int endflag = FALSE;
    int termlen = 0;

    if (streamTtyDebug)
        printf ("streamTty getInput \n");  

    for (;;)
    {
        /* 
        * arrange for a callback on timeout 
        */
        for (list = bus->streamList; list; list = list->next)
        {
            stream = list->stream;
            if (stream->acceptInput && stream->protocol.readTimeout)
    	        devStreamStartTimer (stream, 
                                     stream->protocol.readTimeout, 
                                     (FUNCPTR) timeout);
        }

        /*
        * read a single character
        */
        if (read (bus->fd, &c, 1) == ERROR)
        {
            for (list = bus->streamList; list; list = list->next)
                if (list->stream->acceptInput)
                   devStreamBusError (list->stream);
            break;
        }

        /*
        * send character to all accepting streams
        * must be atomic to ensure protocol separation
        */
        taskLock ();
        for (list = bus->streamList; list; list = list->next)
        {
            stream = list->stream;
            if (stream->acceptInput)
            {
                termlen = stream->protocol.inTerminator[0];
                if (termlen > 0 && c == stream->protocol.inTerminator[termlen])
                    endflag = TRUE;
                else
                    endflag = FALSE; 
                devStreamReceive (stream, &c, 1, endflag);
            }
        }
        taskUnlock();
    }
}
/*******************************************************************************
* timeout    Call-back function in cas of no input (probably not needed now)  
*/
LOCAL void timeout (stream_t *stream)
{
    devStreamReadTimeout (stream);

    if (streamTtyDebug)
        printErr ("streamTty %s: Timed out\n", stream->record->name);
}
/*******************************************************************************
*/
