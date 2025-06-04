/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.04.11
 */
#ifndef PVDBCRSCALARARRAYRECORD_H
#define PVDBCRSCALARARRAYRECORD_H

#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class PvdbcrScalarArrayRecord;
typedef std::tr1::shared_ptr<PvdbcrScalarArrayRecord> PvdbcrScalarArrayRecordPtr;

/**
 * @brief  PvdbcrScalarArrayRecord creates a record with a scalar array value, alarm, and timeStamp.
 *
 */
class epicsShareClass PvdbcrScalarArrayRecord :
     public PVRecord
{
private:
  PvdbcrScalarArrayRecord(
    std::string const & recordName,epics::pvData::PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup);
public:
    POINTER_DEFINITIONS(PvdbcrScalarArrayRecord);
    /**
     * The Destructor.
     */
    virtual ~PvdbcrScalarArrayRecord() {}
    /**
     * @brief Create a record.
     *
     * @param recordName The record name.
     * @param scalarType The type for the value field
     * @param asLevel  The access security level.
     * @param asGroup  The access security group.
     * @return The PVRecord
     */
     static PvdbcrScalarArrayRecordPtr create(
        std::string const & recordName,std::string const &  scalarType,
        int asLevel=0,std::string const & asGroup = std::string("DEFAULT"));
};

}}

#endif  /* PVDBCRSCALARARRAYRECORD_H */
