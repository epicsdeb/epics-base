/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.04.11
 */
#ifndef PVDBCRPROCESSARRAY_H
#define PVDBCRPROCESSARRAY_H
#include <epicsThread.h>
#include <epicsGuard.h>
#include <pv/event.h>
#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

typedef std::tr1::shared_ptr<epicsThread> EpicsThreadPtr;
class PvdbcrProcessRecord;
typedef std::tr1::shared_ptr<PvdbcrProcessRecord> PvdbcrProcessRecordPtr;

/**
 * @brief  PvdbcrProcessRecord A record that processes other records in the master database.
 *
 */
class epicsShareClass PvdbcrProcessRecord :
     public PVRecord,
     public epicsThreadRunable
{
private:
    PvdbcrProcessRecord(
        std::string const & recordName,epics::pvData::PVStructurePtr const & pvStructure,
        double delay,
        int asLevel,std::string const & asGroup);
    double delay;
    EpicsThreadPtr thread;
    epics::pvData::Event runStop;
    epics::pvData::Event runReturn;
    PVDatabasePtr pvDatabase;
    PVRecordMap pvRecordMap;
    epics::pvData::PVStringPtr pvCommand;
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVStringPtr pvResult;
    epics::pvData::Mutex mutex;
public:
    POINTER_DEFINITIONS(PvdbcrProcessRecord);
    /**
     * The Destructor.
     */
    virtual ~PvdbcrProcessRecord() {}
    /**
     * @brief Create a record.
     *
     * @param recordName The record name.
     * @param asLevel  The access security level.
     * @param asGroup  The access security group.
     * @return The PVRecord
     */
     static PvdbcrProcessRecordPtr create(
        std::string const & recordName,
        double delay= 1.0,
        int asLevel=0,std::string const & asGroup = std::string("DEFAULT"));
    /**
     * @brief set the delay between prcocessing.
     *
     * @param delay in seconds
     */    
    void setDelay(double delay);
     /**
     * @brief get the delay between prcocessing.
     *
     * @return delay in seconds
     */    
    double getDelay();    
    /**
     *  @brief a PVRecord method
     * @return success or failure
     */
    virtual bool init();
    /**
     *  @brief method that processes other records in the master database.
     */
    virtual void process();
    /**
     *  @brief thread method
     */
    virtual void run();
    /**
     *  @brief thread method
     */
    void startThread();
    /**
     *  @brief thread method
     */
    void stop();
};

}}

#endif  /* PVDBCRPROCESSARRAY_H */
