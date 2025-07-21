/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#ifndef SCALARALARMSUPPORT_H
#define SCALARALARMSUPPORT_H


#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class ScalarAlarmSupport;
typedef std::tr1::shared_ptr<ScalarAlarmSupport> ScalarAlarmSupportPtr;

/**
 * @brief Base interface for a ScalarAlarmSupport.
 *
 */
class epicsShareClass ScalarAlarmSupport :
    PVSupport
{
public:
    POINTER_DEFINITIONS(ScalarAlarmSupport);
    /**
     * The Destructor.
     */
    virtual ~ScalarAlarmSupport();
    /**
     * @brief Connects to contol fields.
     *
     * @param pvValue The field to support.
     * @param pvAlarm The alarm field.
     * @param pvSupport Support specific fields.
     * @return <b>true</b> for success and <b>false</b> for failure.
     */
    virtual bool init(
        epics::pvData::PVFieldPtr const & pvValue,
        epics::pvData::PVStructurePtr const & pvAlarm,
        epics::pvData::PVFieldPtr const & pvSupport);
    /**
     * @brief Honors scalarAlarm fields.
     *
     *
     * @return Returns true is any fields were modified; otherwise false.
     */
    virtual bool process();
    /**
     *  @brief If implementing minSteps it sets isMinStep to false.
     *
     */
    virtual void reset();
    /**
     * @brief create a ScalarAlarm
     *
     * @param pvRecord - The pvRecord to which the support is attached.
     * @return The new ScalarAlarm
     */
    static ScalarAlarmSupportPtr create(PVRecordPtr const & pvRecord);
    /**
     * @brief create a scalarAlarm required by ScalarAlarm
     *
     * @return The scalarAlarmField introspection structure.
     */
    static epics::pvData::StructureConstPtr scalarAlarmField();
private:

    ScalarAlarmSupport(PVRecordPtr const & pvRecord);
    enum {
        range_Lolo = 0,
        range_Low,
        range_Normal,
        range_High,
        range_Hihi,
        range_Invalid,
        range_Undefined
    } AlarmRange;
    void setAlarm(
        epics::pvData::PVStructurePtr const & pvAlarm,
        int alarmRange);
    PVRecordPtr pvRecord;
    int prevAlarmRange;
    epics::pvData::PVScalarPtr pvValue;
    epics::pvData::PVStructurePtr pvAlarm;
    epics::pvData::PVStructurePtr pvScalarAlarm;
    epics::pvData::PVBooleanPtr pvActive;
    epics::pvData::PVDoublePtr pvLowAlarmLimit;
    epics::pvData::PVDoublePtr pvLowWarningLimit;
    epics::pvData::PVDoublePtr pvHighWarningLimit;
    epics::pvData::PVDoublePtr pvHighAlarmLimit;
    epics::pvData::PVDoublePtr pvHysteresis;
    double requestedValue;
    double currentValue;
    bool isHystersis;
};

}}

#endif  /* SCALARALARMSUPPORT_H */
