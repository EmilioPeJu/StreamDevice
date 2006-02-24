/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is the interface to bus drivers for StreamDevice.       *
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

#ifndef StreamBusInterface_h
#define StreamBusInterface_h

#include <stddef.h>

class StreamBusInterface
{
public:
    enum IoStatus {ioSuccess, ioTimeout, ioNoReply, ioEnd, ioFault};

    class Client
    {
        friend class StreamBusInterface;
        virtual void lockCallback(IoStatus status) = 0;
        virtual void writeCallback(IoStatus status) = 0;
        virtual long readCallback(IoStatus status,
            const void* input, long size) = 0;
        virtual void eventCallback(IoStatus status) = 0;
        virtual long priority() = 0;
        virtual const char* name() = 0;

    protected:
        StreamBusInterface* businterface;
        bool busSetEos(const char* eos, size_t eoslen) {
            return businterface->setEos(eos, eoslen);
        }
        bool busSupportsEvent() {
            return businterface->supportsEvent();
        }
        bool busSupportsAsyncRead() {
            return businterface->supportsAsyncRead();
        }
        bool busAcceptEvent(unsigned long mask,
            unsigned long replytimeout_ms) {
            return businterface->acceptEvent(mask, replytimeout_ms);
        }
        void busRelease() {
            businterface->release();
        }
        bool busLockRequest(unsigned long timeout_ms) {
            return businterface->lockRequest(timeout_ms);
        }
        bool busUnlock() {
            return businterface->unlock();
        }
        bool busWriteRequest(const void* output, size_t size,
            unsigned long timeout_ms) {
            return businterface->writeRequest(output, size, timeout_ms);
        }
        bool busReadRequest(unsigned long replytimeout_ms,
            unsigned long readtimeout_ms, long expectedLength,
            bool async) {
            return businterface->readRequest(replytimeout_ms,
                readtimeout_ms, expectedLength, async);
        }
    };

private:
    friend class StreamBusInterfaceClass; // the iterator
    friend class Client;
    class RegistrarBase
    {
        friend class StreamBusInterfaceClass; // the iterator
        friend class StreamBusInterface;      // the implementation
        static RegistrarBase* first;
        RegistrarBase* next;
        virtual StreamBusInterface* find(Client* client,
            const char* busname, int addr, const char* param) = 0;
    protected:
        const char* name;
        RegistrarBase(const char* name);
    public:
        const char* getName() { return name; }
    };

public:
    template <class C>
    class Registrar : protected RegistrarBase
    {
        StreamBusInterface* find(Client* client, const char* busname,
            int addr, const char* param)
            { return C::getBusInterface(client, busname, addr, param); }
    public:
        Registrar(const char* name) : RegistrarBase(name) {};
    };

    Client* client;
    virtual ~StreamBusInterface() {};

protected:
    const char* eos;
    size_t eoslen;
    StreamBusInterface(Client* client);
    void lockCallback(IoStatus status)
        { client->lockCallback(status); }
    void writeCallback(IoStatus status)
        { client->writeCallback(status); }
    long readCallback(IoStatus status,
        const void* input = NULL, long size = 0)
        { return client->readCallback(status, input, size); }
    void eventCallback(IoStatus status)
        { client->eventCallback(status); }
    long priority() { return client->priority(); }
    const char* clientName() { return client->name(); }

// default implementations
    virtual bool setEos(const char* eos, size_t eoslen);
    virtual bool supportsEvent(); // defaults to false
    virtual bool supportsAsyncRead(); // defaults to false
    virtual bool acceptEvent(unsigned long mask,
        unsigned long replytimeout_ms); // implement if supportsEvents() = true
    virtual void release();

// pure virtual
    virtual bool lockRequest(unsigned long timeout_ms) = 0;
    virtual bool unlock() = 0;
    virtual bool writeRequest(const void* output, size_t size,
        unsigned long timeout_ms) = 0;
    virtual bool readRequest(unsigned long replytimeout_ms,
        unsigned long readtimeout_ms, long expectedLength,
        bool async) = 0;

public:
// static methods
    static StreamBusInterface* find(Client*, const char* busname,
        int addr, const char* param);
};

#define RegisterStreamBusInterface(interface) \
template class StreamBusInterface::Registrar<interface>; \
static StreamBusInterface::Registrar<interface> \
registrar_##interface(#interface);

// Interface class iterator

class StreamBusInterfaceClass
{
    StreamBusInterface::RegistrarBase* ptr;
public:
    StreamBusInterfaceClass () {
        ptr = StreamBusInterface::RegistrarBase::first;
    }
    StreamBusInterfaceClass& operator ++ () {
        ptr = ptr->next; return *this;
    }
    const char* name() {
        return ptr->name;
    }
    operator bool () {
        return ptr != NULL;
    }
};

#endif
