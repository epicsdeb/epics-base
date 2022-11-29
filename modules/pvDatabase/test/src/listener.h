/* listener.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */
#ifndef LISTENER_H
#define LISTENER_H


#ifdef epicsExportSharedSymbols
#   define listenerEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/timeStamp.h>
#include <pv/alarm.h>
#include <pv/pvTimeStamp.h>
#include <pv/pvAlarm.h>
#include <pv/pvDatabase.h>
#include <pv/pvStructureCopy.h>

#ifdef listenerEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#       undef listenerEpicsExportSharedSymbols
#endif

#include <shareLib.h>

//epicsShareFunc epics::pvData::PVStructurePtr createListener();

namespace epics { namespace pvDatabase {
using namespace epics::pvData;
using namespace std;

class Listener;
typedef std::tr1::shared_ptr<Listener> ListenerPtr;

class Listener :
    public PVListener,
    public std::tr1::enable_shared_from_this<Listener>
{
public:
    POINTER_DEFINITIONS(Listener);
    static ListenerPtr create(
        PVRecordPtr const & pvRecord)
    {
        ListenerPtr listener(new Listener(pvRecord));
        pvRecord->addListener(listener,listener->pvCopy);
        return listener;
    }
    virtual ~Listener(){}
    virtual void detach(PVRecordPtr const & pvRecord)
    {
    }
    virtual void dataPut(PVRecordFieldPtr const & pvRecordField)
    {
        cout << "Listener::dataPut record " << recordName
        << " pvRecordField " << pvRecordField->getPVField()->getFullName()
        << endl;
    }
    virtual void dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField)
    {
        cout << "Listener::dataPut record " << recordName
        << " requested " << requested->getPVStructure()->getFullName()
        << " pvRecordField " << pvRecordField->getPVField()->getFullName()
        << endl;
    }
    virtual void beginGroupPut(PVRecordPtr const & pvRecord)
    {
        cout << "Listener::beginGroupPut record " << recordName << endl;
    }
    virtual void endGroupPut(PVRecordPtr const & pvRecord)
    {
         cout << "Listener::endGroupPut record " << recordName << endl;
    }
    virtual void unlisten(PVRecordPtr const & pvRecord)
    {
         cout << "Listener::unlisten record " << recordName << endl;
    }

private:
    Listener(PVRecordPtr const & pvRecord)
    : pvCopy(
          epics::pvCopy::PVCopy::create(
              getPVDataCreate()->createPVStructure(
                  pvRecord->getPVRecordStructure()->getPVStructure()),
              CreateRequest::create()->createRequest(""),
              pvRecord->getRecordName())),
          pvStructure(pvCopy->createPVStructure()),
          recordName(pvRecord->getRecordName())
    {
    }
    epics::pvCopy::PVCopyPtr pvCopy;
    PVStructurePtr pvStructure;
    string recordName;
};

}}

#endif  /* LISTENER_H */
