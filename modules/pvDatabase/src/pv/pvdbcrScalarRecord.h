/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.04.11
 */
#ifndef PVDBCRSCALARRECORD_H
#define PVDBCRSCALARRECORD_H

#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class PvdbcrScalarRecord;
typedef std::tr1::shared_ptr<PvdbcrScalarRecord> PvdbcrScalarRecordPtr;

/**
 * @brief  PvdbcrScalarRecord creates a record with a scalar value, alarm, and timeStamp.
 *
 */
class epicsShareClass PvdbcrScalarRecord :
     public PVRecord
{
private:
  PvdbcrScalarRecord(
    std::string const & recordName,epics::pvData::PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup);
public:
    POINTER_DEFINITIONS(PvdbcrScalarRecord);
    /**
     * The Destructor.
     */
    virtual ~PvdbcrScalarRecord() {}
    /**
     * @brief Create a record.
     *
     * @param recordName The record name.
     * @param scalarType The type for the value field
     * @param asLevel  The access security level.
     * @param asGroup  The access security group.
     * @return The PVRecord
     */
     static PvdbcrScalarRecordPtr create(
        std::string const & recordName,std::string const &  scalarType,
        int asLevel=0,std::string const & asGroup = std::string("DEFAULT"));
};

}}

#endif  /* PVDBCRSCALARRECORD_H */
