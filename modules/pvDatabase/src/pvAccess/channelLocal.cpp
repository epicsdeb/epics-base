/* channelLocal.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */

#include <sstream>
#include <vector>

#include <asLib.h>
#include <epicsGuard.h>
#include <epicsThread.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/timeStamp.h>
#include <pv/createRequest.h>
#include <pv/pvaVersion.h>
#include <pv/pvaVersionNum.h>
#include <pv/serverContext.h>
#include <pv/pvSubArrayCopy.h>
#include <pv/security.h>

#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/pvDatabase.h"
#include "pv/channelProviderLocal.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvCopy;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;
using std::string;

namespace epics { namespace pvDatabase {

static StructureConstPtr nullStructure;

class ChannelProcessLocal;
typedef std::tr1::shared_ptr<ChannelProcessLocal> ChannelProcessLocalPtr;
class ChannelGetLocal;
typedef std::tr1::shared_ptr<ChannelGetLocal> ChannelGetLocalPtr;
class ChannelPutLocal;
typedef std::tr1::shared_ptr<ChannelPutLocal> ChannelPutLocalPtr;
class ChannelPutGetLocal;
typedef std::tr1::shared_ptr<ChannelPutGetLocal> ChannelPutGetLocalPtr;
class ChannelRPCLocal;
typedef std::tr1::shared_ptr<ChannelRPCLocal> ChannelRPCLocalPtr;
class ChannelArrayLocal;
typedef std::tr1::shared_ptr<ChannelArrayLocal> ChannelArrayLocalPtr;

static bool getProcess(PVStructurePtr pvRequest,bool processDefault)
{
    PVFieldPtr pvField = pvRequest->getSubField("record._options.process");
    if(!pvField || pvField->getField()->getType()!=scalar) {
        return processDefault;
    }
    ScalarConstPtr scalar = static_pointer_cast<const Scalar>(
        pvField->getField());
    if(scalar->getScalarType()==pvString) {
        PVStringPtr pvString = static_pointer_cast<PVString>(pvField);
        return  pvString->get().compare("true")==0 ? true : false;
    } else if(scalar->getScalarType()==pvBoolean) {
        PVBooleanPtr pvBoolean = static_pointer_cast<PVBoolean>(pvField);
        return pvBoolean->get();
    }
    return processDefault;
}

class ChannelProcessLocal :
    public epics::pvAccess::ChannelProcess,
    public std::tr1::enable_shared_from_this<ChannelProcessLocal>
{
public:
    POINTER_DEFINITIONS(ChannelProcessLocal);
    virtual ~ChannelProcessLocal();
    static ChannelProcessLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void process();
    virtual std::tr1::shared_ptr<Channel> getChannel();
    virtual void cancel(){}
    virtual void lock();
    virtual void unlock();
    virtual void lastRequest() {}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelProcessLocal(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVRecordPtr const &pvRecord,
        int nProcess)
    :
      channelLocal(channelLocal),
      channelProcessRequester(channelProcessRequester),
      pvRecord(pvRecord),
      nProcess(nProcess)
    {
    }
    ChannelLocalWPtr channelLocal;
    ChannelProcessRequester::weak_pointer channelProcessRequester;
    PVRecordWPtr pvRecord;
    int nProcess;
    Mutex mutex;
};

ChannelProcessLocalPtr ChannelProcessLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelProcessRequester::shared_pointer const & channelProcessRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVFieldPtr pvField;
    PVStructurePtr pvOptions;
    int nProcess = 1;
    if(pvRequest) pvField = pvRequest->getSubField("record._options");
    if(pvField) {
        pvOptions = static_pointer_cast<PVStructure>(pvField);
        pvField = pvOptions->getSubField("nProcess");
        if(pvField) {
            PVStringPtr pvString = pvOptions->getSubField<PVString>("nProcess");
            if(pvString) {
                int size=0;
                std::stringstream ss;
                ss << pvString->get();
                ss >> size;
                nProcess = size;
            }
        }
    }
    ChannelProcessLocalPtr process(new ChannelProcessLocal(
        channelLocal,
        channelProcessRequester,
        pvRecord,
        nProcess));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelProcessLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelProcessRequester->channelProcessConnect(Status::Ok, process);
    return process;
}

ChannelProcessLocal::~ChannelProcessLocal()
{
//cout << "~ChannelProcessLocal()\n";
}

std::tr1::shared_ptr<Channel> ChannelProcessLocal::getChannel()
{
   ChannelLocalPtr channel(channelLocal.lock());
   return channel;
}

void ChannelProcessLocal::lock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->lock();
}
void ChannelProcessLocal::unlock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->unlock();
}

void ChannelProcessLocal::process()
{
    ChannelProcessRequester::shared_pointer requester = channelProcessRequester.lock();
    if(!requester) return;
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>1)
    {
        cout << "ChannelProcessLocal::process";
        cout << " nProcess " << nProcess << endl;
    }
    try {
        for(int i=0; i< nProcess; i++) {
            epicsGuard <PVRecord> guard(*pvr);
            pvr->beginGroupPut();
            pvr->process();
            pvr->endGroupPut();
        }
        requester->processDone(Status::Ok,getPtrSelf());
    } catch(std::exception& ex) {
        Status status = Status(Status::STATUSTYPE_FATAL, ex.what());
        requester->processDone(status,getPtrSelf());
    }
}

class ChannelGetLocal :
    public epics::pvAccess::ChannelGet,
    public std::tr1::enable_shared_from_this<ChannelGetLocal>
{
public:
    POINTER_DEFINITIONS(ChannelGetLocal);
    virtual ~ChannelGetLocal();
    static ChannelGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelGetRequester::shared_pointer const & channelGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void get();
    virtual std::tr1::shared_ptr<Channel> getChannel();
    virtual void cancel(){}
    virtual void lock();
    virtual void unlock();
    virtual void lastRequest() {}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelGetLocal(
        bool callProcess,
        ChannelLocalPtr const &channelLocal,
        ChannelGetRequester::shared_pointer const & channelGetRequester,
        PVCopyPtr const &pvCopy,
        PVStructurePtr const&pvStructure,
        BitSetPtr const & bitSet,
        PVRecordPtr const &pvRecord)
    :
      firstTime(true),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelGetRequester(channelGetRequester),
      pvCopy(pvCopy),
      pvStructure(pvStructure),
      bitSet(bitSet),
      pvRecord(pvRecord)
    {
    }
    bool firstTime;
    bool callProcess;
    ChannelLocalWPtr channelLocal;
    ChannelGetRequester::weak_pointer channelGetRequester;
    PVCopyPtr pvCopy;
    PVStructurePtr pvStructure;
    BitSetPtr bitSet;
    PVRecordWPtr pvRecord;
    Mutex mutex;
};

ChannelGetLocalPtr ChannelGetLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelGetRequester::shared_pointer const & channelGetRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVCopyPtr pvCopy = PVCopy::create(
        pvRecord->getPVRecordStructure()->getPVStructure(),
        pvRequest,
        "");
    if(!pvCopy) {
        Status status(
            Status::STATUSTYPE_ERROR,
            "invalid pvRequest");
        ChannelGet::shared_pointer channelGet;
        channelGetRequester->channelGetConnect(
            status,
            channelGet,
            nullStructure);
        ChannelGetLocalPtr localGet;
        return localGet;
    }
    PVStructurePtr pvStructure = pvCopy->createPVStructure();
    BitSetPtr   bitSet(new BitSet(pvStructure->getNumberFields()));
    ChannelGetLocalPtr get(new ChannelGetLocal(
        getProcess(pvRequest,false),
        channelLocal,
        channelGetRequester,
        pvCopy,
        pvStructure,
        bitSet,
        pvRecord));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelGetLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelGetRequester->channelGetConnect(
        Status::Ok,get,pvStructure->getStructure());
    return get;
}

ChannelGetLocal::~ChannelGetLocal()
{
//cout << "~ChannelGetLocal()\n";
}

std::tr1::shared_ptr<Channel> ChannelGetLocal::getChannel()
{
   ChannelLocalPtr channel(channelLocal.lock());
   return channel;
}

void ChannelGetLocal::lock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->lock();
}
void ChannelGetLocal::unlock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->unlock();
}


void ChannelGetLocal::get()
{
    ChannelGetRequester::shared_pointer requester = channelGetRequester.lock();
    if(!requester) return;
    ChannelLocalPtr channel(channelLocal.lock());
    if(!channel) throw std::logic_error("channel is deleted");
    if(!channel->canRead()) {
        Status status = Status::error("ChannelGet::get is not allowed");
        requester->getDone(status,getPtrSelf(),PVStructurePtr(),BitSetPtr());
        return;
    }
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    try {
        bool notifyClient = true;
        bitSet->clear();
        {
            epicsGuard <PVRecord> guard(*pvr);
            if(callProcess) {
                pvr->beginGroupPut();
                pvr->process();
                pvr->endGroupPut();
            }
            notifyClient = pvCopy->updateCopySetBitSet(pvStructure, bitSet);
        }
        if(firstTime) {
            bitSet->clear();
            bitSet->set(0);
            firstTime = false;
            notifyClient = true;
        }
        if(notifyClient) {
            requester->getDone(
                Status::Ok,
                getPtrSelf(),
                pvStructure,
                bitSet);
            bitSet->clear();
        } else {
            BitSetPtr temp(new BitSet(bitSet->size()));
            requester->getDone(
                Status::Ok,
                getPtrSelf(),
                pvStructure,
                temp);
        }
        if(pvr->getTraceLevel()>1)
        {
            cout << "ChannelGetLocal::get" << endl;
        }
    } catch(std::exception& ex) {
        Status status = Status(Status::STATUSTYPE_FATAL, ex.what());
        requester->getDone(status,getPtrSelf(),pvStructure,bitSet);
    }

}

class ChannelPutLocal :
    public epics::pvAccess::ChannelPut,
    public std::tr1::enable_shared_from_this<ChannelPutLocal>
{
public:
    POINTER_DEFINITIONS(ChannelPutLocal);
    virtual ~ChannelPutLocal();
    static ChannelPutLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutRequester::shared_pointer const & channelPutRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void put(PVStructurePtr const &pvStructure,BitSetPtr const &bitSet);
    virtual void get();
    virtual std::tr1::shared_ptr<Channel> getChannel();
    virtual void cancel(){}
    virtual void lock();
    virtual void unlock();
    virtual void lastRequest() {}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelPutLocal(
        bool callProcess,
        ChannelLocalPtr const &channelLocal,
        ChannelPutRequester::shared_pointer const & channelPutRequester,
        PVCopyPtr const &pvCopy,
        PVRecordPtr const &pvRecord)
    :
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelPutRequester(channelPutRequester),
      pvCopy(pvCopy),
      pvRecord(pvRecord)
    {
    }
    bool callProcess;
    ChannelLocalWPtr channelLocal;
    ChannelPutRequester::weak_pointer channelPutRequester;
    PVCopyPtr pvCopy;
    PVRecordWPtr pvRecord;
    Mutex mutex;
};

ChannelPutLocalPtr ChannelPutLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelPutRequester::shared_pointer const & channelPutRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVCopyPtr pvCopy = PVCopy::create(
        pvRecord->getPVRecordStructure()->getPVStructure(),
        pvRequest,
        "");
    if(!pvCopy) {
        Status status(
            Status::STATUSTYPE_ERROR,
            "invalid pvRequest");
        ChannelPut::shared_pointer channelPut;
        PVStructurePtr pvStructure;
        BitSetPtr bitSet;
        channelPutRequester->channelPutConnect(
            status,
            channelPut,
            nullStructure);
        ChannelPutLocalPtr localPut;
        return localPut;
    }
    ChannelPutLocalPtr put(new ChannelPutLocal(
        getProcess(pvRequest,true),
        channelLocal,
        channelPutRequester,
        pvCopy,
        pvRecord));
    channelPutRequester->channelPutConnect(
        Status::Ok, put, pvCopy->getStructure());
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelPutLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    return put;
}

ChannelPutLocal::~ChannelPutLocal()
{
//cout << "~ChannelPutLocal()\n";
}

std::tr1::shared_ptr<Channel> ChannelPutLocal::getChannel()
{
   ChannelLocalPtr channel(channelLocal.lock());
   return channel;
}

void ChannelPutLocal::lock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->lock();
}
void ChannelPutLocal::unlock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->unlock();
}


void ChannelPutLocal::get()
{
    ChannelPutRequester::shared_pointer requester = channelPutRequester.lock();
    if(!requester) return;
    ChannelLocalPtr channel(channelLocal.lock());
    if(!channel) throw std::logic_error("channel is deleted");
    if(!channel->canRead()) {
        Status status = Status::error("ChannelPut::get is not allowed");
        requester->getDone(status,getPtrSelf(),PVStructurePtr(),BitSetPtr());
        return;
    }
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    try {
        PVStructurePtr pvStructure = pvCopy->createPVStructure();
         BitSetPtr bitSet(new BitSet(pvStructure->getNumberFields()));
         bitSet->clear();
         bitSet->set(0);
         {
             epicsGuard <PVRecord> guard(*pvr);
             pvCopy->updateCopyFromBitSet(pvStructure, bitSet);
         }
         requester->getDone(
            Status::Ok,getPtrSelf(),pvStructure,bitSet);
         if(pvr->getTraceLevel()>1)
         {
             cout << "ChannelPutLocal::get" << endl;
         }
    } catch(std::exception& ex) {
        Status status = Status(Status::STATUSTYPE_FATAL, ex.what());
        PVStructurePtr pvStructure;
        BitSetPtr bitSet;
        requester->getDone(status,getPtrSelf(),pvStructure,bitSet);
    }
}

void ChannelPutLocal::put(
    PVStructurePtr const &pvStructure,BitSetPtr const &bitSet)
{
    ChannelPutRequester::shared_pointer requester = channelPutRequester.lock();
    if(!requester) return;
    ChannelLocalPtr channel(channelLocal.lock());
    if(!channel) throw std::logic_error("channel is deleted");
    if(!channel->canWrite()) {
        Status status = Status::error("ChannelPut::put is not allowed");
        requester->putDone(status,getPtrSelf());
        return;
    }

    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    try {
        {
            epicsGuard <PVRecord> guard(*pvr);
            pvr->beginGroupPut();
            pvCopy->updateMaster(pvStructure, bitSet);
            if(callProcess) {
                 pvr->process();
            }
            pvr->endGroupPut();
        }
        requester->putDone(Status::Ok,getPtrSelf());
        if(pvr->getTraceLevel()>1)
        {
            cout << "ChannelPutLocal::put" << endl;
        }
    } catch(std::exception& ex) {
        Status status = Status(Status::STATUSTYPE_FATAL, ex.what());
        requester->putDone(status,getPtrSelf());
    }
}


class ChannelPutGetLocal :
    public epics::pvAccess::ChannelPutGet,
    public std::tr1::enable_shared_from_this<ChannelPutGetLocal>
{
public:
    POINTER_DEFINITIONS(ChannelPutGetLocal);
    virtual ~ChannelPutGetLocal();
    static ChannelPutGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void putGet(
        PVStructurePtr const &pvPutStructure,
        BitSetPtr const &putBitSet);
    virtual void getPut();
    virtual void getGet();
    virtual std::tr1::shared_ptr<Channel> getChannel();
    virtual void cancel(){}
    virtual void lock();
    virtual void unlock();
    virtual void lastRequest() {}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelPutGetLocal(
        bool callProcess,
        ChannelLocalPtr const &channelLocal,
        ChannelPutGetRequester::weak_pointer const & channelPutGetRequester,
        PVCopyPtr const &pvPutCopy,
        PVCopyPtr const &pvGetCopy,
        PVStructurePtr const&pvGetStructure,
        BitSetPtr const & getBitSet,
        PVRecordPtr const &pvRecord)
    :
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelPutGetRequester(channelPutGetRequester),
      pvPutCopy(pvPutCopy),
      pvGetCopy(pvGetCopy),
      pvGetStructure(pvGetStructure),
      getBitSet(getBitSet),
      pvRecord(pvRecord)
    {
    }
    bool callProcess;
    ChannelLocalWPtr channelLocal;
    ChannelPutGetRequester::weak_pointer channelPutGetRequester;
    PVCopyPtr pvPutCopy;
    PVCopyPtr pvGetCopy;
    PVStructurePtr pvGetStructure;
    BitSetPtr getBitSet;
    PVRecordWPtr pvRecord;
    Mutex mutex;
};

ChannelPutGetLocalPtr ChannelPutGetLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVCopyPtr pvPutCopy = PVCopy::create(
        pvRecord->getPVRecordStructure()->getPVStructure(),
        pvRequest,
        "putField");
    PVCopyPtr pvGetCopy = PVCopy::create(
        pvRecord->getPVRecordStructure()->getPVStructure(),
        pvRequest,
        "getField");
    if(!pvPutCopy || !pvGetCopy) {
        Status status(
            Status::STATUSTYPE_ERROR,
            "invalid pvRequest");
        ChannelPutGet::shared_pointer channelPutGet;
        channelPutGetRequester->channelPutGetConnect(
            status,
            channelPutGet,
            nullStructure,
            nullStructure);
        ChannelPutGetLocalPtr localPutGet;
        return localPutGet;
    }
    PVStructurePtr pvGetStructure = pvGetCopy->createPVStructure();
    BitSetPtr   getBitSet(new BitSet(pvGetStructure->getNumberFields()));
    ChannelPutGetLocalPtr putGet(new ChannelPutGetLocal(
        getProcess(pvRequest,true),
        channelLocal,
        channelPutGetRequester,
        pvPutCopy,
        pvGetCopy,
        pvGetStructure,
        getBitSet,
        pvRecord));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelPutGetLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelPutGetRequester->channelPutGetConnect(
        Status::Ok, putGet, pvPutCopy->getStructure(),pvGetCopy->getStructure());
    return putGet;
}

ChannelPutGetLocal::~ChannelPutGetLocal()
{
//cout << "~ChannelPutGetLocal()\n";
}

std::tr1::shared_ptr<Channel> ChannelPutGetLocal::getChannel()
{
   ChannelLocalPtr channel(channelLocal.lock());
   return channel;
}

void ChannelPutGetLocal::lock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->lock();
}
void ChannelPutGetLocal::unlock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->unlock();
}


void ChannelPutGetLocal::putGet(
    PVStructurePtr const &pvPutStructure,BitSetPtr const &putBitSet)
{
    ChannelPutGetRequester::shared_pointer requester = channelPutGetRequester.lock();
    if(!requester) return;
    ChannelLocalPtr channel(channelLocal.lock());
    if(!channel) throw std::logic_error("channel is deleted");
    if(!channel->canWrite()||!channel->canRead() ) {
        Status status = Status::error("ChannelPutGet::putGet is not allowed");
        requester->putGetDone(status,getPtrSelf(),PVStructurePtr(),BitSetPtr());
        return;
    }
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    try {
        {
            epicsGuard <PVRecord> guard(*pvr);
            pvr->beginGroupPut();
            pvPutCopy->updateMaster(pvPutStructure, putBitSet);
            if(callProcess) pvr->process();
            getBitSet->clear();
            pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet);
            pvr->endGroupPut();
        }
        requester->putGetDone(
            Status::Ok,getPtrSelf(),pvGetStructure,getBitSet);
        if(pvr->getTraceLevel()>1)
        {
            cout << "ChannelPutGetLocal::putGet" << endl;
        }
    } catch(std::exception& ex) {
        Status status = Status(Status::STATUSTYPE_FATAL, ex.what());
        requester->putGetDone(status,getPtrSelf(),pvGetStructure,getBitSet);
    }
}

void ChannelPutGetLocal::getPut()
{
    ChannelPutGetRequester::shared_pointer requester = channelPutGetRequester.lock();
    if(!requester) return;
    ChannelLocalPtr channel(channelLocal.lock());
    if(!channel) throw std::logic_error("channel is deleted");
    if(!channel->canRead()) {
        Status status = Status::error("ChannelPutGet::getPut is not allowed");
        requester->getPutDone(status,getPtrSelf(),PVStructurePtr(),BitSetPtr());
        return;
    }
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    try {
        PVStructurePtr pvPutStructure = pvPutCopy->createPVStructure();
        BitSetPtr putBitSet(new BitSet(pvPutStructure->getNumberFields()));
        {
            epicsGuard <PVRecord> guard(*pvr);
            pvPutCopy->initCopy(pvPutStructure, putBitSet);
        }
        requester->getPutDone(
            Status::Ok,getPtrSelf(),pvPutStructure,putBitSet);
        if(pvr->getTraceLevel()>1)
        {
            cout << "ChannelPutGetLocal::getPut" << endl;
        }
    } catch(std::exception& ex) {
        Status status = Status(Status::STATUSTYPE_FATAL, ex.what());
        PVStructurePtr pvPutStructure;
        BitSetPtr putBitSet;
        requester->getPutDone(status,getPtrSelf(),pvGetStructure,getBitSet);
    }
}

void ChannelPutGetLocal::getGet()
{
    ChannelPutGetRequester::shared_pointer requester = channelPutGetRequester.lock();
    if(!requester) return;
    ChannelLocalPtr channel(channelLocal.lock());
    if(!channel) throw std::logic_error("channel is deleted");
    if(!channel->canRead()) {
        Status status = Status::error("ChannelPutGet::getGet is not allowed");
        requester->getPutDone(status,getPtrSelf(),PVStructurePtr(),BitSetPtr());
        return;
    }
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    try {
         getBitSet->clear();
         {
             epicsGuard <PVRecord> guard(*pvr);
             pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet);
         }
         requester->getGetDone(
             Status::Ok,getPtrSelf(),pvGetStructure,getBitSet);
         if(pvr->getTraceLevel()>1)
         {
             cout << "ChannelPutGetLocal::getGet" << endl;
         }
    } catch(std::exception& ex) {
        Status status = Status(Status::STATUSTYPE_FATAL, ex.what());
        PVStructurePtr pvPutStructure;
        BitSetPtr putBitSet;
        requester->getGetDone(status,getPtrSelf(),pvGetStructure,getBitSet);
    }
}


class ChannelRPCLocal :
    public epics::pvAccess::ChannelRPC,
    public epics::pvAccess::RPCResponseCallback,
    public std::tr1::enable_shared_from_this<ChannelRPCLocal>
{
public:
    POINTER_DEFINITIONS(ChannelRPCLocal);
    static ChannelRPCLocalPtr create(
        ChannelLocalPtr const & channelLocal,
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const & pvRecord);

    ChannelRPCLocal(
        ChannelLocalPtr const & channelLocal,
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        RPCServiceAsync::shared_pointer const & service,
        PVRecordPtr const & pvRecord) :
        channelLocal(channelLocal),
        channelRPCRequester(channelRPCRequester),
        service(service),
        pvRecord(pvRecord)
    {
    }

    virtual ~ChannelRPCLocal();
    void processRequest(RPCService::shared_pointer const & service,
                        PVStructurePtr const & pvArgument);

    virtual void requestDone(Status const & status,
        PVStructurePtr const & result)
    {
        ChannelRPCRequester::shared_pointer requester = channelRPCRequester.lock();
        if(!requester) return;
        requester->requestDone(status, getPtrSelf(), result);
    }

    void processRequest(RPCServiceAsync::shared_pointer const & service,
                        PVStructurePtr const & pvArgument);

    virtual void request(PVStructurePtr const & pvArgument);
    virtual Channel::shared_pointer getChannel();
    virtual void cancel() {}
    virtual void lock() {}
    virtual void unlock() {}
    virtual void lastRequest() {}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelLocalWPtr channelLocal;
    ChannelRPCRequester::weak_pointer channelRPCRequester;
    RPCServiceAsync::shared_pointer service;
    PVRecordWPtr pvRecord;
};

ChannelRPCLocalPtr ChannelRPCLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelRPCRequester::shared_pointer const & channelRPCRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    RPCServiceAsync::shared_pointer service = pvRecord->getService(pvRequest);
    if (!service)
    {
        Status status(Status::STATUSTYPE_ERROR,
            "ChannelRPC not supported");
            channelRPCRequester->channelRPCConnect(status,ChannelRPCLocalPtr());
        return ChannelRPCLocalPtr();
    }

    if (!channelRPCRequester)
        throw std::invalid_argument("channelRPCRequester == null");

    // TODO use std::make_shared
    ChannelRPCLocalPtr rpc(
        new ChannelRPCLocal(channelLocal, channelRPCRequester, service, pvRecord)
    );
    channelRPCRequester->channelRPCConnect(Status::Ok, rpc);
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelRPCLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    return rpc;
}

ChannelRPCLocal::~ChannelRPCLocal()
{
//cout << "~ChannelRPCLocal()\n";
}

std::tr1::shared_ptr<Channel> ChannelRPCLocal::getChannel()
{
   ChannelLocalPtr channel(channelLocal.lock());
   return channel;
}


void ChannelRPCLocal::processRequest(
    RPCService::shared_pointer const & service,
    PVStructurePtr const & pvArgument)
{
    PVStructurePtr result;
    Status status = Status::Ok;
    bool ok = true;
    try
    {
        result = service->request(pvArgument);
    }
    catch (epics::pvAccess::RPCRequestException& rre)
    {
        status = Status(rre.getStatus(), rre.what());
        ok = false;
    }
    catch (std::exception& ex)
    {
        status = Status(Status::STATUSTYPE_FATAL, ex.what());
        ok = false;
    }
    catch (...)
    {
        // handle user unexpected errors
        status = Status(Status::STATUSTYPE_FATAL, "Unexpected exception caught while calling RPCService.request(PVStructure).");
        ok = false;
    }

    // check null result
    if (ok && !result)
    {
        status = Status(Status::STATUSTYPE_FATAL, "RPCService.request(PVStructure) returned null.");
    }
    ChannelRPCRequester::shared_pointer requester = channelRPCRequester.lock();
    if(requester) requester->requestDone(status, getPtrSelf(), result);
}

void ChannelRPCLocal::processRequest(
    RPCServiceAsync::shared_pointer const & service,
    PVStructurePtr const & pvArgument)
{
    try
    {
        service->request(pvArgument, getPtrSelf());
    }
    catch (std::exception& ex)
    {
        // handle user unexpected errors
        Status errorStatus(Status::STATUSTYPE_FATAL, ex.what());
        ChannelRPCRequester::shared_pointer requester = channelRPCRequester.lock();
        if(requester) requester->requestDone(errorStatus, getPtrSelf(), PVStructurePtr());
    }
    catch (...)
    {
        // handle user unexpected errors
        Status errorStatus(Status::STATUSTYPE_FATAL,
                           "Unexpected exception caught while calling RPCServiceAsync.request(PVStructure, RPCResponseCallback).");
        ChannelRPCRequester::shared_pointer requester = channelRPCRequester.lock();
        if(requester) requester->requestDone(errorStatus, shared_from_this(), PVStructurePtr());
    }

    // we wait for callback to be called
}


void ChannelRPCLocal::request(PVStructurePtr const & pvArgument)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(pvr && pvr->getTraceLevel()>0) {
        cout << "ChannelRPCLocal::request " << pvr->getRecordName() << endl;
    }
    RPCService::shared_pointer rpcService =
            std::tr1::dynamic_pointer_cast<RPCService>(service);
    if (rpcService)
    {
        processRequest(rpcService, pvArgument);
        return;
    }

    RPCServiceAsync::shared_pointer rpcServiceAsync =
            std::tr1::dynamic_pointer_cast<RPCServiceAsync>(service);
    if (rpcServiceAsync)
    {
         processRequest(rpcServiceAsync, pvArgument);
         return;
    }
}


typedef std::tr1::shared_ptr<PVArray> PVArrayPtr;

class ChannelArrayLocal :
    public epics::pvAccess::ChannelArray,
    public std::tr1::enable_shared_from_this<ChannelArrayLocal>
{
public:
    POINTER_DEFINITIONS(ChannelArrayLocal);
    virtual ~ChannelArrayLocal();
    static ChannelArrayLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelArrayRequester::shared_pointer const & channelArrayRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void getArray(size_t offset, size_t count, size_t stride);
    virtual void putArray(
         PVArrayPtr const &putArray,
         size_t offset, size_t count, size_t stride);
    virtual void getLength();
    virtual void setLength(size_t length);
    virtual std::tr1::shared_ptr<Channel> getChannel();
    virtual void cancel(){}
    virtual void lock();
    virtual void unlock();
    virtual void lastRequest() {}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelArrayLocal(
        ChannelLocalPtr const &channelLocal,
        ChannelArrayRequester::shared_pointer const & channelArrayRequester,
        PVArrayPtr const &pvArray,
        PVArrayPtr const &pvCopy,
        PVRecordPtr const &pvRecord)
    :
      channelLocal(channelLocal),
      channelArrayRequester(channelArrayRequester),
      pvArray(pvArray),
      pvCopy(pvCopy),
      pvRecord(pvRecord)
    {
    }

    ChannelLocalWPtr channelLocal;
    ChannelArrayRequester::weak_pointer channelArrayRequester;
    PVArrayPtr pvArray;
    PVArrayPtr pvCopy;
    PVRecordWPtr pvRecord;
    Mutex mutex;
};


ChannelArrayLocalPtr ChannelArrayLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelArrayRequester::shared_pointer const & channelArrayRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVFieldPtrArray const & pvFields = pvRequest->getPVFields();
    if(pvFields.size()!=1) {
        Status status(
            Status::STATUSTYPE_ERROR,"invalid pvRequest");
        ChannelArrayLocalPtr channelArray;
        ArrayConstPtr array;
        channelArrayRequester->channelArrayConnect(status,channelArray,array);
        return channelArray;
    }
    PVFieldPtr pvField = pvFields[0];
    string fieldName("");
    while(true) {
        string name = pvField->getFieldName();
        if(fieldName.size()>0) fieldName += '.';
        fieldName += name;
        PVStructurePtr pvs = static_pointer_cast<PVStructure>(pvField);
        PVFieldPtrArray const & pvfs = pvs->getPVFields();
        if(pvfs.size()!=1) break;
        pvField = pvfs[0];
    }
    size_t indfield = fieldName.find_first_of("field.");
    if(indfield==0) {
         fieldName = fieldName.substr(6);
    }
    pvField = pvRecord->getPVRecordStructure()->getPVStructure()->getSubField(fieldName);
    if(!pvField) {
        Status status(
            Status::STATUSTYPE_ERROR,fieldName +" not found");
        ChannelArrayLocalPtr channelArray;
        ArrayConstPtr array;
        channelArrayRequester->channelArrayConnect(
            status,channelArray,array);
        return channelArray;
    }
    if(pvField->getField()->getType()!=scalarArray
    && pvField->getField()->getType()!=structureArray
    && pvField->getField()->getType()!=unionArray)
    {
        Status status(
            Status::STATUSTYPE_ERROR,fieldName +" not array");
        ChannelArrayLocalPtr channelArray;
        ArrayConstPtr array;
        channelArrayRequester->channelArrayConnect(
           status,channelArray,array);
        return channelArray;
    }
    PVArrayPtr pvArray = static_pointer_cast<PVArray>(pvField);
    PVArrayPtr pvCopy;
    if(pvField->getField()->getType()==scalarArray) {
        PVScalarArrayPtr xxx = static_pointer_cast<PVScalarArray>(pvField);
        pvCopy = getPVDataCreate()->createPVScalarArray(
            xxx->getScalarArray()->getElementType());
    } else if(pvField->getField()->getType()==structureArray) {
        PVStructureArrayPtr xxx = static_pointer_cast<PVStructureArray>(pvField);
        pvCopy = getPVDataCreate()->createPVStructureArray(
            xxx->getStructureArray()->getStructure());
    } else {
        PVUnionArrayPtr xxx = static_pointer_cast<PVUnionArray>(pvField);
        pvCopy = getPVDataCreate()->createPVUnionArray(
            xxx->getUnionArray()->getUnion());
    }
    ChannelArrayLocalPtr array(new ChannelArrayLocal(
        channelLocal,
        channelArrayRequester,
        pvArray,
        pvCopy,
        pvRecord));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelArrayLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelArrayRequester->channelArrayConnect(
        Status::Ok, array, pvCopy->getArray());
    return array;
}

ChannelArrayLocal::~ChannelArrayLocal()
{
//cout << "~ChannelArrayLocal()\n";
}

std::tr1::shared_ptr<Channel> ChannelArrayLocal::getChannel()
{
   ChannelLocalPtr channel(channelLocal.lock());
   return channel;
}

void ChannelArrayLocal::lock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->lock();
}
void ChannelArrayLocal::unlock()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    pvr->unlock();
}

void ChannelArrayLocal::getArray(size_t offset, size_t count, size_t stride)
{
    ChannelArrayRequester::shared_pointer requester = channelArrayRequester.lock();
    if(!requester) return;
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>1)
    {
       cout << "ChannelArrayLocal::getArray" << endl;
    }
    const char *exceptionMessage = NULL;
    try {
        bool ok = false;
        epicsGuard <PVRecord> guard(*pvr);
        while(true) {
            size_t length  = pvArray->getLength();
            if(length<=0) break;
            if(count<=0) {
                 count = (length -offset + stride -1)/stride;
                 if(count>0) ok = true;
                 break;
            }
            size_t maxcount = (length -offset + stride -1)/stride;
            if(count>maxcount) count = maxcount;
            ok = true;
            break;
        }
        if(ok) {
            pvCopy->setLength(count);
            copy(pvArray,offset,stride,pvCopy,0,1,count);
        }
    } catch(std::exception& e) {
        exceptionMessage = e.what();
    }
    Status status = Status::Ok;
    if(exceptionMessage!=NULL) {
      status = Status(Status::STATUSTYPE_ERROR,exceptionMessage);
    }
    requester->getArrayDone(status,getPtrSelf(),pvCopy);
}

void ChannelArrayLocal::putArray(
     PVArrayPtr const & pvArray, size_t offset, size_t count, size_t stride)
{
    ChannelArrayRequester::shared_pointer requester = channelArrayRequester.lock();
    if(!requester) return;
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>1)
    {
       cout << "ChannelArrayLocal::putArray" << endl;
    }
    size_t newLength = offset + count*stride;
    if(newLength<pvArray->getLength()) pvArray->setLength(newLength);
    const char *exceptionMessage = NULL;
    try {
        epicsGuard <PVRecord> guard(*pvr);
        copy(pvArray,0,1,this->pvArray,offset,stride,count);
    } catch(std::exception& e) {
        exceptionMessage = e.what();
    }
    Status status = Status::Ok;
    if(exceptionMessage!=NULL) {
        status = Status(Status::STATUSTYPE_ERROR,exceptionMessage);
    }
    requester->putArrayDone(status,getPtrSelf());
}

void ChannelArrayLocal::getLength()
{
    ChannelArrayRequester::shared_pointer requester = channelArrayRequester.lock();
    if(!requester) return;
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    size_t length = 0;
    const char *exceptionMessage = NULL;
    try {
        epicsGuard <PVRecord> guard(*pvr);
        length = pvArray->getLength();
    } catch(std::exception& e) {
        exceptionMessage = e.what();
    }
    Status status = Status::Ok;
    if(exceptionMessage!=NULL) {
        status = Status(Status::STATUSTYPE_ERROR,exceptionMessage);
    }
    requester->getLengthDone(status,getPtrSelf(),length);
}

void ChannelArrayLocal::setLength(size_t length)
{
    ChannelArrayRequester::shared_pointer requester = channelArrayRequester.lock();
    if(!requester) return;
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>1)
    {
       cout << "ChannelArrayLocal::setLength" << endl;
    }
    try {
         {
             epicsGuard <PVRecord> guard(*pvr);
             if(pvArray->getLength()!=length) pvArray->setLength(length);
         }
         requester->setLengthDone(Status::Ok,getPtrSelf());
    } catch(std::exception& e) {
        string exceptionMessage = e.what();
        Status status = Status(Status::STATUSTYPE_ERROR,exceptionMessage);
        requester->setLengthDone(status,getPtrSelf());
    }
}


ChannelLocal::ChannelLocal(
    ChannelProviderLocalPtr const & provider,
    ChannelRequester::shared_pointer const & requester,
    PVRecordPtr const & pvRecord)
:
    requester(requester),
    provider(provider),
    pvRecord(pvRecord),
    asLevel(pvRecord->getAsLevel()),
    asGroup(getAsGroup(pvRecord)),
    asUser(getAsUser(requester)),
    asHost(getAsHost(requester)),
    asMemberPvt(0),
    asClientPvt(0)
{
    if(pvRecord->getTraceLevel()>0) {
         cout << "ChannelLocal::ChannelLocal()"
              << " recordName " << pvRecord->getRecordName()
              << " requester exists " << (requester ? "true" : "false")
              << endl;
    }
    if (pvRecord->getAsGroup().empty() || asAddMember(&asMemberPvt, &asGroup[0]) != 0) {
        asMemberPvt = 0;
    } 
    if (asMemberPvt) {
        asAddClient(&asClientPvt, asMemberPvt, asLevel, &asUser[0], &asHost[0]);
    }
}

std::vector<char> ChannelLocal::toCharArray(const std::string& s)
{
    std::vector<char> v(s.begin(), s.end());
    v.push_back('\0');
    return v;
}

std::vector<char> ChannelLocal::getAsGroup(const PVRecordPtr& pvRecord)
{
    return toCharArray(pvRecord->getAsGroup());
}

std::vector<char> ChannelLocal::getAsUser(const ChannelRequester::shared_pointer& requester)
{
    PeerInfo::const_shared_pointer info(requester->getPeerInfo());
    std::string user;
    if(info && info->identified) {
        if(info->authority=="ca") {
            user = info->account;
            size_t first = user.find_last_of('/');
            if(first != std::string::npos) {
                // prevent CA accounts like "<authority>/<user>"
                user = user.substr(first+1);
            }
        } 
        else {
            user = info->authority + "/" + info->account;
        }
    } 
    return toCharArray(user);
}

std::vector<char> ChannelLocal::getAsHost(const epics::pvAccess::ChannelRequester::shared_pointer& requester)
{
    PeerInfo::const_shared_pointer info(requester->getPeerInfo());
    std::string host;
    if(info && info->identified) {
        host= info->peer;
    } 
    else {
        // anonymous
        host = requester->getRequesterName();
    }

    // handle form "ip:port"
    size_t last = host.find_first_of(':');
    if(last == std::string::npos) {
        last = host.size();
    }
    host.resize(last);
    return toCharArray(host);
}

bool ChannelLocal::canWrite() 
{
    if(!asActive || (asClientPvt && asCheckPut(asClientPvt))) {
        return true;
    }
    return false;
}

bool ChannelLocal::canRead()
{
    if(!asActive || (asClientPvt && asCheckGet(asClientPvt))) {
        return true;
    }
    return false;
}
ChannelLocal::~ChannelLocal()
{
    if(asMemberPvt) {
        asRemoveMember(&asMemberPvt);
        asMemberPvt = 0;
    }
    if(asClientPvt) {
        asRemoveClient(&asClientPvt);
        asClientPvt = 0;
    }
}

ChannelProvider::shared_pointer ChannelLocal::getProvider()
{
    return provider.lock();
}

void ChannelLocal::detach(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>0) {
         cout << "ChannelLocal::detach() "
         << " recordName " << pvRecord->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }
    if(!requester) return;
    requester->channelStateChange(shared_from_this(),Channel::DESTROYED);
}


string ChannelLocal::getRequesterName()
{
    PVRecordPtr pvr(pvRecord.lock());
    if(pvr && pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::getRequesterName() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }

    if(!requester) return string();
    return requester->getRequesterName();
}

void ChannelLocal::message(
        string const &message,
        MessageType messageType)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(pvr && pvr->getTraceLevel()>1) {
         cout << "ChannelLocal::message() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }
    if(requester) {
        requester->message(message,messageType);
        return;
    }
    string recordName("record deleted");
    if(pvr) recordName = pvr->getRecordName();
    cout << recordName
         << " message " << message
         << " messageType " << getMessageTypeName(messageType)
         << endl;
}

string ChannelLocal::getRemoteAddress()
{
    return string("local");
}

Channel::ConnectionState ChannelLocal::getConnectionState()
{
    return Channel::CONNECTED;
}

string ChannelLocal::getChannelName()
{
    PVRecordPtr pvr(pvRecord.lock());
    string name("record deleted");
    if(pvr) name = pvr->getRecordName();
    return name;
}

ChannelRequester::shared_pointer ChannelLocal::getChannelRequester()
{
    return requester;
}

bool ChannelLocal::isConnected()
{
    return true;
}

void ChannelLocal::getField(GetFieldRequester::shared_pointer const &requester,
        string const &subField)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(subField.size()<1) {
        StructureConstPtr structure =
            pvr->getPVRecordStructure()->getPVStructure()->getStructure();
        requester->getDone(Status::Ok,structure);
        return;
    }
    PVFieldPtr pvField =
        pvr->getPVRecordStructure()->getPVStructure()->getSubField(subField);
    if(pvField) {
        requester->getDone(Status::Ok,pvField->getField());
        return;
    }
    Status status(Status::STATUSTYPE_ERROR,
        "client asked for illegal field");
    requester->getDone(status,FieldConstPtr());
}

AccessRights ChannelLocal::getAccessRights(
        PVField::shared_pointer const &pvField)
{
    throw std::logic_error("Not Implemented");
}

ChannelProcess::shared_pointer ChannelLocal::createChannelProcess(
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructure::shared_pointer const & pvRequest)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::createChannelProcess() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }
    ChannelProcessLocalPtr channelProcess =
       ChannelProcessLocal::create(
            getPtrSelf(),
            channelProcessRequester,
            pvRequest,
            pvr);
    return channelProcess;
}

ChannelGet::shared_pointer ChannelLocal::createChannelGet(
        ChannelGetRequester::shared_pointer const &channelGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::createChannelGet() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }
    ChannelGetLocalPtr channelGet =
       ChannelGetLocal::create(
            getPtrSelf(),
            channelGetRequester,
            pvRequest,
            pvr);
    return channelGet;
}

ChannelPut::shared_pointer ChannelLocal::createChannelPut(
        ChannelPutRequester::shared_pointer const &channelPutRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::createChannelPut() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }

    ChannelPutLocalPtr channelPut =
       ChannelPutLocal::create(
            getPtrSelf(),
            channelPutRequester,
            pvRequest,
            pvr);
    return channelPut;
}

ChannelPutGet::shared_pointer ChannelLocal::createChannelPutGet(
        ChannelPutGetRequester::shared_pointer const &channelPutGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::createChannelPutGet() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }

    ChannelPutGetLocalPtr channelPutGet =
       ChannelPutGetLocal::create(
            getPtrSelf(),
            channelPutGetRequester,
            pvRequest,
            pvr);
    return channelPutGet;
}

ChannelRPC::shared_pointer ChannelLocal::createChannelRPC(
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        PVStructure::shared_pointer const & pvRequest)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::createChannelRPC() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }

    ChannelRPCLocalPtr channelRPC =
        ChannelRPCLocal::create(
            getPtrSelf(),
            channelRPCRequester,
            pvRequest,
            pvr);
     return channelRPC;
}

Monitor::shared_pointer ChannelLocal::createMonitor(
        MonitorRequester::shared_pointer const &monitorRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::createMonitor() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }

    MonitorPtr monitor = createMonitorLocal(
            pvr,
            monitorRequester,
            pvRequest);
    return monitor;
}

ChannelArray::shared_pointer ChannelLocal::createChannelArray(
        ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    PVRecordPtr pvr(pvRecord.lock());
    if(!pvr) throw std::logic_error("pvRecord is deleted");
    if(pvr->getTraceLevel()>0) {
         cout << "ChannelLocal::createChannelArray() "
         << " recordName " << pvr->getRecordName()
         << " requester exists " << (requester ? "true" : "false")
         << endl;
    }
    ChannelArrayLocalPtr channelArray =
       ChannelArrayLocal::create(
            getPtrSelf(),
            channelArrayRequester,
            pvRequest,
            pvr);
    return channelArray;
}

void ChannelLocal::printInfo()
{
    printInfo(std::cout);
}

void ChannelLocal::printInfo(std::ostream& out)
{
    out << "ChannelLocal provides access to a record in the local PVDatabase";
}

}}
