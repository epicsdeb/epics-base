/* recordClient.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */
#ifndef RECORDCLIENT_H
#define RECORDCLIENT_H


#ifdef epicsExportSharedSymbols
#   define pvRecordClientEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/timeStamp.h>
#include <pv/alarm.h>
#include <pv/pvTimeStamp.h>
#include <pv/pvAlarm.h>
#include <pv/pvDatabase.h>

#ifdef pvRecordClientEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#       undef pvRecordClientEpicsExportSharedSymbols
#endif

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class RecordClient;
typedef std::tr1::shared_ptr<RecordClient> RecordClientPtr;

class RecordClient :
    public PVRecordClient,
    public std::tr1::enable_shared_from_this<RecordClient>
{
public:
    POINTER_DEFINITIONS(RecordClient);
    static RecordClientPtr create(
        PVRecordPtr const & pvRecord)
    {
        RecordClientPtr pvRecordClient(new RecordClient(pvRecord));
        pvRecord->addPVRecordClient(pvRecordClient);
        return pvRecordClient;
    }
    virtual ~RecordClient()
    {
         std::string recordName("pvRecord was destroyed");
         if(pvRecord) recordName = pvRecord->getRecordName();
         std::cout << "RecordClient::~RecordClient " << recordName << std::endl;
    }
    virtual void detach(PVRecordPtr const & pvRecord)
    {
         std::cout << "RecordClient::detach record " << pvRecord->getRecordName() << std::endl;
         this->pvRecord.reset();
    }

private:
    RecordClient(PVRecordPtr const & pvRecord)
    : pvRecord(pvRecord)
    {}
    PVRecordPtr pvRecord;
};

}}

#endif  /* RECORDCLIENT_H */
