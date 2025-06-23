/* monitorFactory.cpp */
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

#include <epicsGuard.h>
#include <pv/thread.h>
#include <pv/bitSetUtil.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/serverContext.h>
#include <pv/timeStamp.h>

#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/pvDatabase.h"
#include "pv/channelProviderLocal.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvCopy;
using std::tr1::static_pointer_cast;
using std::cout;
using std::endl;
using std::string;

namespace epics { namespace pvDatabase {

class MonitorLocal;
typedef std::tr1::shared_ptr<MonitorLocal> MonitorLocalPtr;

static MonitorPtr nullMonitor;
static MonitorElementPtr NULLMonitorElement;
static Status failedToCreateMonitorStatus(
    Status::STATUSTYPE_ERROR,"failed to create monitor");
static Status alreadyStartedStatus(Status::STATUSTYPE_ERROR,"already started");
static Status notStartedStatus(Status::STATUSTYPE_ERROR,"not started");
static Status deletedStatus(Status::STATUSTYPE_ERROR,"record is deleted");

class MonitorElementQueue;
typedef std::tr1::shared_ptr<MonitorElementQueue> MonitorElementQueuePtr;

class  MonitorElementQueue
{
private:
    MonitorElementPtrArray elements;
    // TODO use size_t instead
    int size;
    int numberFree;
    int numberUsed;
    int nextGetFree;
    int nextSetUsed;
    int nextGetUsed;
    int nextReleaseUsed;
public:
    POINTER_DEFINITIONS(MonitorElementQueue);

    MonitorElementQueue(std::vector<MonitorElementPtr> monitorElementArray)
    :  elements(monitorElementArray),
       size(monitorElementArray.size()),
       numberFree(size),
       numberUsed(0),
       nextGetFree(0),
       nextSetUsed(0),
       nextGetUsed(0),
       nextReleaseUsed(0)
    {
    }

    virtual ~MonitorElementQueue() {}

    void clear()
    {
        numberFree = size;
        numberUsed = 0;
        nextGetFree = 0;
        nextSetUsed = 0;
        nextGetUsed = 0;
        nextReleaseUsed = 0;
    }

    MonitorElementPtr getFree()
    {
        if(numberFree==0) return MonitorElementPtr();
        numberFree--;
        int ind = nextGetFree;
        MonitorElementPtr queueElement = elements[nextGetFree++];
        if(nextGetFree>=size) nextGetFree = 0;
        return elements[ind];
    }

    void setUsed(MonitorElementPtr const &element)
    {
       if(element!=elements[nextSetUsed++]) {
           throw std::logic_error("not correct queueElement");
        }
        numberUsed++;
        if(nextSetUsed>=size) nextSetUsed = 0;
    }

    MonitorElementPtr getUsed()
    {
        if(numberUsed==0) return MonitorElementPtr();
        int ind = nextGetUsed;
        MonitorElementPtr queueElement = elements[nextGetUsed++];
        if(nextGetUsed>=size) nextGetUsed = 0;
        return elements[ind];
    }
    void releaseUsed(MonitorElementPtr const &element)
    {
        if(element!=elements[nextReleaseUsed++]) {
            throw std::logic_error(
               "not queueElement returned by last call to getUsed");
        }
        if(nextReleaseUsed>=size) nextReleaseUsed = 0;
        numberUsed--;
        numberFree++;
    }
};


typedef std::tr1::shared_ptr<MonitorRequester> MonitorRequesterPtr;


class MonitorLocal :
    public Monitor,
    public PVListener,
    public std::tr1::enable_shared_from_this<MonitorLocal>
{
    enum MonitorState {idle,active,deleted};
public:
    POINTER_DEFINITIONS(MonitorLocal);
    virtual ~MonitorLocal();
    virtual Status start();
    virtual Status stop();
    virtual MonitorElementPtr poll();
    virtual void detach(PVRecordPtr const & pvRecord){}
    virtual void release(MonitorElementPtr const & monitorElement);
    virtual void dataPut(PVRecordFieldPtr const & pvRecordField);
    virtual void dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField);
    virtual void beginGroupPut(PVRecordPtr const & pvRecord);
    virtual void endGroupPut(PVRecordPtr const & pvRecord);
    virtual void unlisten(PVRecordPtr const & pvRecord);
    MonitorElementPtr getActiveElement();
    void releaseActiveElement();
    bool init(PVStructurePtr const & pvRequest);
    MonitorLocal(
        MonitorRequester::shared_pointer const & channelMonitorRequester,
        PVRecordPtr const &pvRecord);
    PVCopyPtr getPVCopy() { return pvCopy;}
private:
    MonitorLocalPtr getPtrSelf()
    {
        return shared_from_this();
    }
    MonitorRequester::weak_pointer monitorRequester;
    PVRecordPtr pvRecord;
    MonitorState state;
    PVCopyPtr pvCopy;
    MonitorElementQueuePtr queue;
    MonitorElementPtr activeElement;
    bool isGroupPut;
    bool dataChanged;
    Mutex mutex;
    Mutex queueMutex;
};

MonitorLocal::MonitorLocal(
    MonitorRequester::shared_pointer const & channelMonitorRequester,
    PVRecordPtr const &pvRecord)
: monitorRequester(channelMonitorRequester),
  pvRecord(pvRecord),
  state(idle),
  isGroupPut(false),
  dataChanged(false)
{
}

MonitorLocal::~MonitorLocal()
{
//cout << "MonitorLocal::~MonitorLocal()" << endl;
}


Status MonitorLocal::start()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "MonitorLocal::start state " << state << endl;
    }
    {
        Lock xx(mutex);
        if(state==active) return alreadyStartedStatus;
        if(state==deleted) return deletedStatus;
    }
    pvRecord->addListener(getPtrSelf(),pvCopy);
    epicsGuard <PVRecord> guard(*pvRecord);
    Lock xx(mutex);
    state = active;
    queue->clear();
    isGroupPut = false;
    activeElement = queue->getFree();
    activeElement->changedBitSet->clear();
    activeElement->overrunBitSet->clear();
    activeElement->changedBitSet->set(0);
    releaseActiveElement();
    return Status::Ok;
}

Status MonitorLocal::stop()
{
    if(pvRecord->getTraceLevel()>0){
        cout << "MonitorLocal::stop state " << state << endl;
    }
    {
        Lock xx(mutex);
        if(state==idle) return notStartedStatus;
        if(state==deleted) return deletedStatus;
        state = idle;
    }
    pvRecord->removeListener(getPtrSelf(),pvCopy);
    return Status::Ok;
}

MonitorElementPtr MonitorLocal::poll()
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::poll state  " << state << endl;
    }
    {
        Lock xx(queueMutex);
        if(state!=active) return NULLMonitorElement;
        return queue->getUsed();
    }
}

void MonitorLocal::release(MonitorElementPtr const & monitorElement)
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::release state  " << state << endl;
    }
    {
        Lock xx(queueMutex);
        if(state!=active) return;
        queue->releaseUsed(monitorElement);
    }
}

void MonitorLocal::releaseActiveElement()
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::releaseActiveElement  state  " << state << endl;
    }
    {
        Lock xx(queueMutex);
        if(state!=active) return;
        bool result = pvCopy->updateCopyFromBitSet(activeElement->pvStructurePtr,activeElement->changedBitSet);
        if(!result) return;
        MonitorElementPtr newActive = queue->getFree();
        if(!newActive) return;
        BitSetUtil::compress(activeElement->changedBitSet,activeElement->pvStructurePtr);
        BitSetUtil::compress(activeElement->overrunBitSet,activeElement->pvStructurePtr);
        queue->setUsed(activeElement);
        activeElement = newActive;
        activeElement->changedBitSet->clear();
        activeElement->overrunBitSet->clear();
    }
    MonitorRequesterPtr requester = monitorRequester.lock();
    if(!requester) return;
    requester->monitorEvent(getPtrSelf());
    return;
}

void MonitorLocal::dataPut(PVRecordFieldPtr const & pvRecordField)
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::dataPut(pvRecordField)" << endl;
    }
    // If this record field is the master field, and the master field was not
    // requested, we do not proceed with copy
    bool isMasterField = pvRecordField->getPVRecord()->getPVStructure()->getFieldOffset()==0;
    if (isMasterField && !pvCopy->isMasterFieldRequested()) {
        return;
    }
    if(state!=active) return;
    {
        Lock xx(mutex);
        size_t offset = pvCopy->getCopyOffset(pvRecordField->getPVField());
        BitSetPtr const &changedBitSet = activeElement->changedBitSet;
        BitSetPtr const &overrunBitSet = activeElement->overrunBitSet;
        bool isSet = changedBitSet->get(offset);
        changedBitSet->set(offset);
        if(isSet) overrunBitSet->set(offset);
        dataChanged = true;
    }
    if(!isGroupPut) {
        releaseActiveElement();
        dataChanged = false;
    }
}

void MonitorLocal::dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField)
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::dataPut(requested,pvRecordField)" << endl;
    }
    if(state!=active) return;
    {
        Lock xx(mutex);
        BitSetPtr const &changedBitSet = activeElement->changedBitSet;
        BitSetPtr const &overrunBitSet = activeElement->overrunBitSet;
        size_t offsetCopyRequested = pvCopy->getCopyOffset(
            requested->getPVField());
        size_t offset = offsetCopyRequested
             + (pvRecordField->getPVField()->getFieldOffset()
                 - requested->getPVField()->getFieldOffset());
        bool isSet = changedBitSet->get(offset);
        changedBitSet->set(offset);
        if(isSet) overrunBitSet->set(offset);
        dataChanged = true;
    }
    if(!isGroupPut) {
        releaseActiveElement();
        dataChanged = false;
    }
}

void MonitorLocal::beginGroupPut(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::beginGroupPut()" << endl;
    }
    if(state!=active) return;
    {
        Lock xx(mutex);
        isGroupPut = true;
        dataChanged = false;
    }
}

void MonitorLocal::endGroupPut(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::endGroupPut dataChanged " << dataChanged << endl;
    }
    if(state!=active) return;
    {
        Lock xx(mutex);
        isGroupPut = false;
    }
    if(dataChanged) {
        dataChanged = false;
        releaseActiveElement();
    }
}

void MonitorLocal::unlisten(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::unlisten\n";
    }
    {
        Lock xx(mutex);
        state = deleted;
    }
    MonitorRequesterPtr requester = monitorRequester.lock();
    if(requester) {
        if(pvRecord->getTraceLevel()>1)
        {
            cout << "MonitorLocal::unlisten calling requester->unlisten\n";
        }
        requester->unlisten(getPtrSelf());
    }
}


bool MonitorLocal::init(PVStructurePtr const & pvRequest)
{
    PVFieldPtr pvField;
    size_t queueSize = 2;
    PVStructurePtr pvOptions = pvRequest->getSubField<PVStructure>("record._options");
    MonitorRequesterPtr requester = monitorRequester.lock();
    if(!requester) return false;
    if(pvOptions) {
        PVStringPtr pvString  = pvOptions->getSubField<PVString>("queueSize");
        if(pvString) {
            try {
                int32 size;
                std::stringstream ss;
                ss << pvString->get();
                ss >> size;
                queueSize = size;
            } catch (...) {
                 requester->message("queueSize " +pvString->get() + " illegal",errorMessage);
                 return false;
            }
        }
    }
    pvField = pvRequest->getSubField("field");
    if(!pvField) {
        pvCopy = PVCopy::create(
            pvRecord->getPVRecordStructure()->getPVStructure(),
            pvRequest,"");
        if(!pvCopy) {
            requester->message("illegal pvRequest",errorMessage);
            return false;
        }
    } else {
        if(pvField->getField()->getType()!=structure) {
            requester->message("illegal pvRequest",errorMessage);
            return false;
        }
        pvCopy = PVCopy::create(
            pvRecord->getPVRecordStructure()->getPVStructure(),
            pvRequest,"field");
        if(!pvCopy) {
            requester->message("illegal pvRequest",errorMessage);
            return false;
        }
    }
    if(queueSize<2) queueSize = 2;
    std::vector<MonitorElementPtr> monitorElementArray;
    monitorElementArray.reserve(queueSize);
    for(size_t i=0; i<queueSize; i++) {
         PVStructurePtr pvStructure = pvCopy->createPVStructure();
         MonitorElementPtr monitorElement(
             new MonitorElement(pvStructure));
         monitorElementArray.push_back(monitorElement);
    }
    queue = MonitorElementQueuePtr(new MonitorElementQueue(monitorElementArray));
    requester->monitorConnect(
        Status::Ok,
        getPtrSelf(),
        pvCopy->getStructure());
    return true;
}

MonitorPtr createMonitorLocal(
    PVRecordPtr const & pvRecord,
    MonitorRequester::shared_pointer const & monitorRequester,
    PVStructurePtr const & pvRequest)
{
    MonitorLocalPtr monitor(new MonitorLocal(
        monitorRequester,pvRecord));
    bool result = monitor->init(pvRequest);
    if(!result) {
        MonitorPtr monitor;
        StructureConstPtr structure;
        monitorRequester->monitorConnect(
            failedToCreateMonitorStatus,monitor,structure);
        return nullMonitor;
    }
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "MonitorFactory::createMonitor"
        << " recordName " << pvRecord->getRecordName() << endl;
    }
    return monitor;
}

}}
