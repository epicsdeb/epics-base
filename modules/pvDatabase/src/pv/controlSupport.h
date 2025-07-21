/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#ifndef CONTROLSUPPORT_H
#define CONTROLSUPPORT_H

#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class ControlSupport;
typedef std::tr1::shared_ptr<ControlSupport> ControlSupportPtr;

/**
 * @brief Base interface for a ControlSupport.
 *
 */
class epicsShareClass ControlSupport :
    PVSupport
{
public:
    POINTER_DEFINITIONS(ControlSupport);
    /**
     * The Destructor.
     */
    virtual ~ControlSupport();
    /**
     * @brief Connects to contol fields.
     *
     * @param pvValue The field to support.
     * @param pvSupport Support specific fields.
     * @return <b>true</b> for success and <b>false</b> for failure.
     */
    virtual bool init(
        epics::pvData::PVFieldPtr const & pvValue,
        epics::pvData::PVFieldPtr const & pvSupport);
    /**
     * @brief Honors control fields.
     *
     *
     * @return Returns true is any fields were modified; otherwise false.
     */
    virtual bool process();
    /**
     *  @brief If implementing minSteps it sets isMinStep to false.
     *
     * @return Returns true is any fields were modified; otherwise false.
     */
    virtual void reset();
    /**
     * @brief create a ControlSupport
     *
     * @param pvRecord - The pvRecord to which the support is attached.
     * @return The new ControlSupport
     */
    static ControlSupportPtr create(PVRecordPtr const & pvRecord);
    /**
     * @brief create a controlSupport required by ControlSupport
     *
     * @param scalarType The type for outputValue.
     * @return The controlField introspection structure.
     */
    static epics::pvData::StructureConstPtr controlField(epics::pvData::ScalarType scalarType);
private:
    ControlSupport(PVRecordPtr const & pvRecord);
    PVRecordPtr pvRecord;
    epics::pvData::PVScalarPtr pvValue;
    epics::pvData::PVStructurePtr pvControl;
    epics::pvData::PVDoublePtr pvLimitLow;
    epics::pvData::PVDoublePtr pvLimitHigh;
    epics::pvData::PVDoublePtr pvMinStep;
    epics::pvData::PVScalarPtr pvOutputValue;
    double currentValue;
    bool isMinStep;
};

}}

#endif  /* CONTROLSUPPORT_H */
