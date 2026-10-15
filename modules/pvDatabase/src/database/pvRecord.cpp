/* pvRecord.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2012.11.21
 */
#include <list>
#include <epicsGuard.h>
#include <epicsThread.h>
#include <pv/status.h>
#include <pv/pvAccess.h>
#include <pv/createRequest.h>
#include <pv/pvaVersion.h>
#include <pv/pvaVersionNum.h>
#include <pv/monitor.h>
#include <pv/convert.h>
#include <pv/rpcService.h>
#include <pv/timeStamp.h>
#include <pv/pvData.h>
#include <pv/rpcService.h>
#include <pv/pvTimeStamp.h>

#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/pvDatabase.h"

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvDatabase;
using namespace std;

namespace epics { namespace pvDatabase {

PVRecordPtr PVRecord::create(
    string const &recordName,
    PVStructurePtr const & pvStructure,
    int asLevel,
    const std::string& asGroup)
{
    PVRecordPtr pvRecord(new PVRecord(recordName,pvStructure,asLevel,asGroup));
    if(!pvRecord->init()) {
        pvRecord.reset();
    }
    return pvRecord;
}


PVRecord::PVRecord(
    string const & recordName,
    PVStructurePtr const & pvStructure,
    int asLevel_,
    const std::string& asGroup_)
: recordName(recordName),
  pvStructure(pvStructure),
  depthGroupPut(0),
  traceLevel(0),
  isAddListener(false),
  asLevel(asLevel_),
  asGroup(asGroup_)
{
}

PVRecord::~PVRecord()
{
    if(traceLevel>0) {
        cout << "~PVRecord() " << recordName << endl;
    }
}

void PVRecord::unlistenClients()
{
    epicsGuard<epics::pvData::Mutex> guard(mutex);
    for(std::list<PVListenerWPtr>::iterator iter = pvListenerList.begin();
         iter!=pvListenerList.end();
         iter++ )
    {
        PVListenerPtr listener = iter->lock();
        if(!listener) continue;
        if(traceLevel>0) {
            cout << "PVRecord::remove() calling listener->unlisten " << recordName << endl;
        }
        listener->unlisten(shared_from_this());
    }
    pvListenerList.clear();
    for (std::list<PVRecordClientWPtr>::iterator iter = clientList.begin();
         iter!=clientList.end();
         iter++ )
    {
        PVRecordClientPtr client = iter->lock();
        if(!client) continue;
        if(traceLevel>0) {
            cout << "PVRecord::remove() calling client->detach " << recordName << endl;
        }
        client->detach(shared_from_this());
    }
    clientList.clear();
}


void PVRecord::remove()
{
    if(traceLevel>0) {
            cout << "PVRecord::remove() " << recordName << endl;
    }
    unlistenClients();
    epicsGuard<epics::pvData::Mutex> guard(mutex);
    PVDatabasePtr pvDatabase(PVDatabase::getMaster());
    if(pvDatabase) pvDatabase->removeFromMap(shared_from_this());
    pvTimeStamp.detach();
}

void PVRecord::initPVRecord()
{
    PVRecordStructurePtr parent;
    pvRecordStructure = PVRecordStructurePtr(
        new PVRecordStructure(pvStructure,parent,shared_from_this()));
    pvRecordStructure->init();
    PVFieldPtr pvField = pvStructure->getSubField("timeStamp");
    if(pvField) pvTimeStamp.attach(pvField);
}

void PVRecord::process()
{
    if(traceLevel>2) {
        cout << "PVRecord::process() " << recordName << endl;
    }
    if(pvTimeStamp.isAttached()) {
        pvTimeStamp.get(timeStamp);
        timeStamp.getCurrent();
        pvTimeStamp.set(timeStamp);
    }
}


PVRecordFieldPtr PVRecord::findPVRecordField(PVFieldPtr const & pvField)
{
    return findPVRecordField(pvRecordStructure,pvField);
}

PVRecordFieldPtr PVRecord::findPVRecordField(
    PVRecordStructurePtr const & pvrs,
        PVFieldPtr const & pvField)
{
    size_t desiredOffset = pvField->getFieldOffset();
    PVFieldPtr pvf = pvrs->getPVField();
    size_t offset = pvf->getFieldOffset();
    if(offset==desiredOffset) return pvrs;
    PVRecordFieldPtrArrayPtr  pvrfpap = pvrs->getPVRecordFields();
    PVRecordFieldPtrArray::iterator iter;
    for (iter = pvrfpap.get()->begin(); iter!=pvrfpap.get()->end(); iter++ ) {
        PVRecordFieldPtr pvrf = *iter;
        pvf = pvrf->getPVField();
        offset = pvf->getFieldOffset();
        if(offset==desiredOffset) return pvrf;
        size_t nextOffset = pvf->getNextFieldOffset();
        if(nextOffset<=desiredOffset) continue;
        return findPVRecordField(
            static_pointer_cast<PVRecordStructure>(pvrf),
            pvField);
    }
    throw std::logic_error(
        recordName + " pvField "
        + pvField->getFieldName() + " not in PVRecord");
}

void PVRecord::lock() {
    if(traceLevel>2) {
        cout << "PVRecord::lock() " << recordName << endl;
    }
    mutex.lock();
}

void PVRecord::unlock() {
    if(traceLevel>2) {
        cout << "PVRecord::unlock() " << recordName << endl;
    }
    mutex.unlock();
}

bool PVRecord::tryLock() {
    if(traceLevel>2) {
        cout << "PVRecord::tryLock() " << recordName << endl;
    }
    return mutex.tryLock();
}

void PVRecord::lockOtherRecord(PVRecordPtr const & otherRecord)
{
    if(traceLevel>2) {
        cout << "PVRecord::lockOtherRecord() " << recordName << endl;
    }
    if(this<otherRecord.get()) {
        otherRecord->lock();
        return;
    }
    unlock();
    otherRecord->lock();
    lock();
}

bool PVRecord::addPVRecordClient(PVRecordClientPtr const & pvRecordClient)
{
    if(traceLevel>1) {
        cout << "PVRecord::addPVRecordClient() " << recordName << endl;
    }
    epicsGuard<epics::pvData::Mutex> guard(mutex);
    // clean clientList
    bool clientListClean = false;
    while(!clientListClean) {
        if(clientList.empty()) break;
        clientListClean = true;
        std::list<PVRecordClientWPtr>::iterator iter;
        for (iter = clientList.begin();
             iter!=clientList.end();
             iter++ )
        {
            PVRecordClientPtr client = iter->lock();
            if(client) continue;
            if(traceLevel>1) {
                 cout << "PVRecord::addPVRecordClient() erasing client"
                      << recordName << endl;
            }
            clientList.erase(iter);
            clientListClean = false;
            break;
        }
    }
    clientList.push_back(pvRecordClient);
    return true;
}

bool PVRecord::addListener(
    PVListenerPtr const & pvListener,
    epics::pvCopy::PVCopyPtr const & pvCopy)
{
    if(traceLevel>1) {
        cout << "PVRecord::addListener() " << recordName << endl;
    }
    epicsGuard<epics::pvData::Mutex> guard(mutex);
    pvListenerList.push_back(pvListener);
    this->pvListener = pvListener;
    isAddListener = true;
    pvCopy->traverseMaster(shared_from_this());
    this->pvListener = PVListenerPtr();;
    return true;
}

void PVRecord::nextMasterPVField(PVFieldPtr const & pvField)
{
     PVRecordFieldPtr pvRecordField = findPVRecordField(pvField);
     PVListenerPtr listener = pvListener.lock();
     if(!listener.get()) return;
     if(isAddListener) {
         pvRecordField->addListener(listener);
     } else {
         pvRecordField->removeListener(listener);
     }
}

bool PVRecord::removeListener(
    PVListenerPtr const & pvListener,
    epics::pvCopy::PVCopyPtr const & pvCopy)
{
    if(traceLevel>1) {
        cout << "PVRecord::removeListener() " << recordName << endl;
    }
    epicsGuard<epics::pvData::Mutex> guard(mutex);
    std::list<PVListenerWPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ )
    {
        PVListenerPtr listener = iter->lock();
        if(!listener.get()) continue;
        if(listener.get()==pvListener.get()) {
            pvListenerList.erase(iter);
            this->pvListener = pvListener;
            isAddListener = false;
            pvCopy->traverseMaster(shared_from_this());
            this->pvListener = PVListenerPtr();
            return true;
        }
    }
    return false;
}

void PVRecord::beginGroupPut()
{
   if(++depthGroupPut>1) return;
    if(traceLevel>2) {
        cout << "PVRecord::beginGroupPut() " << recordName << endl;
    }
   std::list<PVListenerWPtr>::iterator iter;
   for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++)
   {
       PVListenerPtr listener = iter->lock();
       if(!listener.get()) continue;
       listener->beginGroupPut(shared_from_this());
   }
}

void PVRecord::endGroupPut()
{
   if(--depthGroupPut>0) return;
    if(traceLevel>2) {
        cout << "PVRecord::endGroupPut() " << recordName << endl;
    }
   std::list<PVListenerWPtr>::iterator iter;
   for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++)
   {
       PVListenerPtr listener = iter->lock();
       if(!listener.get()) continue;
       listener->endGroupPut(shared_from_this());
   }
}

std::ostream& operator<<(std::ostream& o, const PVRecord& record)
{
    o << format::indent() << "record " << record.getRecordName() << endl;
    {
        format::indent_scope s(o);
        o <<  *record.getPVRecordStructure()->getPVStructure();
    }
    return o;
}

PVRecordField::PVRecordField(
    PVFieldPtr const & pvField,
    PVRecordStructurePtr const &parent,
    PVRecordPtr const & pvRecord)
:  pvField(pvField),
   isStructure(pvField->getField()->getType()==structure ? true : false),
   master(),
   parent(parent),
   pvRecord(pvRecord)
{
}

void PVRecordField::init()
{
    fullFieldName = pvField.lock()->getFieldName();
    PVRecordStructurePtr pvParent(parent.lock());
    while(pvParent) {
        string parentName = pvParent->getPVField()->getFieldName();
        if(parentName.size()>0) {
            fullFieldName = pvParent->getPVField()->getFieldName()
                + '.' + fullFieldName;
        }
        pvParent = pvParent->getParent();
    }
    PVRecordPtr pvRecord(this->pvRecord.lock());
    if(fullFieldName.size()>0) {
        fullName = pvRecord->getRecordName() + '.' + fullFieldName;
    } else {
        fullName = pvRecord->getRecordName();
    }
    pvField.lock()->setPostHandler(shared_from_this());
}

PVRecordStructurePtr PVRecordField::getParent()
{
    return parent.lock();
}

PVFieldPtr PVRecordField::getPVField() {return pvField.lock();}

string PVRecordField::getFullFieldName() {return fullFieldName; }

string PVRecordField::getFullName() {return fullName; }

PVRecordPtr PVRecordField::getPVRecord() {return pvRecord.lock();}

bool PVRecordField::addListener(PVListenerPtr const & pvListener)
{
    PVRecordPtr pvRecord(this->pvRecord.lock());
    if(pvRecord && pvRecord->getTraceLevel()>1) {
         cout << "PVRecordField::addListener() " << getFullName() << endl;
    }
    pvListenerList.push_back(pvListener);
    return true;
}

void PVRecordField::removeListener(PVListenerPtr const & pvListener)
{
    PVRecordPtr pvRecord(this->pvRecord.lock());
    if(pvRecord && pvRecord->getTraceLevel()>1) {
         cout << "PVRecordField::removeListener() " << getFullName() << endl;
    }
    std::list<PVListenerWPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ ) {
        PVListenerPtr listener = iter->lock();
        if(!listener.get()) continue;
        if(listener.get()==pvListener.get()) {
            pvListenerList.erase(iter);
            return;
        }
    }
}

void PVRecordField::postPut()
{
    PVRecordStructurePtr parent(this->parent.lock());;
    if(parent) {
        parent->postParent(shared_from_this());
    }
    postSubField();
}

void PVRecordField::postParent(PVRecordFieldPtr const & subField)
{
    PVRecordStructurePtr pvrs = static_pointer_cast<PVRecordStructure>(shared_from_this());
    std::list<PVListenerWPtr>::iterator iter;
    for(iter = pvListenerList.begin(); iter != pvListenerList.end(); ++iter)
    {
        PVListenerPtr listener = iter->lock();
        if(!listener.get()) continue;
        listener->dataPut(pvrs,subField);
    }
    PVRecordStructurePtr parent(this->parent.lock());
    if(parent) {
        parent->postParent(subField);
    }
}

void PVRecordField::postSubField()
{
    // Master field pointer will be set in only one subfield
    PVRecordStructurePtr master(this->master.lock());
    if(master) {
        master->callListener();
    }
    callListener();
    if(isStructure) {
        PVRecordStructurePtr pvrs =
            static_pointer_cast<PVRecordStructure>(shared_from_this());
        PVRecordFieldPtrArrayPtr pvRecordFields = pvrs->getPVRecordFields();
        PVRecordFieldPtrArray::iterator iter;
        for(iter = pvRecordFields->begin() ; iter !=pvRecordFields->end(); iter++) {
             (*iter)->postSubField();
        }
    }
}

void PVRecordField::callListener()
{
    std::list<PVListenerWPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ ) {
        PVListenerPtr listener = iter->lock();
        if(!listener.get()) continue;
        listener->dataPut(shared_from_this());
    }
}

PVRecordStructure::PVRecordStructure(
    PVStructurePtr const &pvStructure,
    PVRecordStructurePtr const &parent,
    PVRecordPtr const & pvRecord)
:
    PVRecordField(pvStructure,parent,pvRecord),
    pvStructure(pvStructure),
    pvRecordFields(new PVRecordFieldPtrArray)
{
}

void PVRecordStructure::init()
{
    PVRecordField::init();
    const PVFieldPtrArray & pvFields = pvStructure.lock()->getPVFields();
    size_t numFields = pvFields.size();
    pvRecordFields->reserve( numFields);
    PVRecordStructurePtr self =
        static_pointer_cast<PVRecordStructure>(shared_from_this());
    PVRecordPtr pvRecord = getPVRecord();
    static bool masterFieldCallbackSet = false;
    bool isMasterField = (!getFullFieldName().size());
    if (isMasterField) {
        masterFieldCallbackSet = false;
    }
    for(size_t i=0; i<numFields; i++) {
        PVFieldPtr pvField = pvFields[i];
        if(pvField->getField()->getType()==structure) {
            PVStructurePtr xxx = static_pointer_cast<PVStructure>(pvField);
            PVRecordStructurePtr pvRecordStructure(
                 new PVRecordStructure(xxx,self,pvRecord));
            pvRecordFields->push_back(pvRecordStructure);
            pvRecordStructure->init();
        } else {
            PVRecordFieldPtr pvRecordField(
                new PVRecordField(pvField,self,pvRecord));
            pvRecordFields->push_back(pvRecordField);
            pvRecordField->init();
            // Master field listeners will be called before
            // calling listeners for the first subfield
            if (!masterFieldCallbackSet) {
                masterFieldCallbackSet = true;
                // Find master field
                PVRecordStructurePtr p = pvRecordField->parent.lock();
                while (p) {
                    PVRecordStructurePtr p2 = p->parent.lock();
                    if (!p2) {
                        pvRecordField->master = p;
                    }
                    p = p2;
                }
            }
        }
    }
}

PVRecordFieldPtrArrayPtr PVRecordStructure::getPVRecordFields()
{
    return pvRecordFields;
}

PVStructurePtr PVRecordStructure::getPVStructure() {return pvStructure.lock();}

}}
