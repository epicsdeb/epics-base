/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.04.11
 */
#ifndef PVDBCRTRACEARRAY_H
#define PVDBCRTRACEARRAY_H

#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class PvdbcrTraceRecord;
typedef std::tr1::shared_ptr<PvdbcrTraceRecord> PvdbcrTraceRecordPtr;

/**
 * @brief  PvdbcrTraceRecord A record sets trace level for a record in the master database.
 *
 */
class epicsShareClass PvdbcrTraceRecord :
     public PVRecord
{
private:
  PvdbcrTraceRecord(
    std::string const & recordName,epics::pvData::PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup);
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVIntPtr pvLevel;
    epics::pvData::PVStringPtr pvResult;
public:
    POINTER_DEFINITIONS(PvdbcrTraceRecord);
    /**
     * The Destructor.
     */
    virtual ~PvdbcrTraceRecord() {}
    /**
     * @brief Create a record.
     *
     * @param recordName The record name.
     * @param asLevel  The access security level.
     * @param asGroup  The access security group.
     * @return The PVRecord
     */
     static PvdbcrTraceRecordPtr create(
        std::string const & recordName,
        int asLevel=0,std::string const & asGroup = std::string("DEFAULT"));
    /**
     *  @brief a PVRecord method
     * @return success or failure
     */
    virtual bool init();
    /**
     *  @brief process method that sets trace level for a record in the master database.
     */
    virtual void process();
};

}}

#endif  /* PVDBCRTRACEARRAY_H */
