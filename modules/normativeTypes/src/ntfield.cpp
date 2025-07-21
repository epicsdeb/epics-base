/* ntfield.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <pv/lock.h>
#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntfield.h>

using namespace epics::pvData;
using std::tr1::static_pointer_cast;

namespace epics { namespace nt {

NTFieldPtr NTField::get()
{
    static Mutex mutex;
    static NTFieldPtr ntstructureField;
    Lock xx(mutex);
    if(ntstructureField.get()==NULL) {
         ntstructureField = NTFieldPtr(new NTField());
    }
    return ntstructureField;
}

NTField::NTField()
: fieldCreate(getFieldCreate()),
  standardField(getStandardField())
{
}

Result& NTField::isEnumerated(Result& result)
{
    return result
        .has<Scalar>("index")
        .has<ScalarArray>("choices");
}

bool NTField::isEnumerated(FieldConstPtr const & field)
{
    Result result(field);
    return isEnumerated(result.is<Structure>()).valid();
}

Result& NTField::isTimeStamp(Result& result)
{
    return result
        .has<Scalar>("secondsPastEpoch")
        .has<Scalar>("nanoseconds")
        .has<Scalar>("userTag");
}

bool NTField::isTimeStamp(FieldConstPtr const & field)
{
    Result result(field);
    return isTimeStamp(result.is<Structure>()).valid();
}

Result& NTField::isAlarm(Result& result)
{
    return result
        .has<Scalar>("severity")
        .has<Scalar>("status")
        .has<Scalar>("message");
}

bool NTField::isAlarm(FieldConstPtr const & field)
{
    Result result(field);
    return isAlarm(result.is<Structure>()).valid();
}

Result& NTField::isDisplay(Result& result)
{
    return result
        .has<Scalar>("limitLow")
        .has<Scalar>("limitHigh")
        .has<Scalar>("description")
        .has<Scalar>("format")
        .has<Scalar>("units");

}

bool NTField::isDisplay(FieldConstPtr const & field)
{
    Result result(field);
    return isDisplay(result.is<Structure>()).valid();
}

Result& NTField::isAlarmLimit(Result& result)
{
    return result
        .has<Scalar>("active")
        .has<Scalar>("lowAlarmLimit")
        .has<Scalar>("lowWarningLimit")
        .has<Scalar>("highWarningLimit")
        .has<Scalar>("highAlarmLimit")
        .has<Scalar>("lowAlarmSeverity")
        .has<Scalar>("lowWarningSeverity")
        .has<Scalar>("highWarningSeverity")
        .has<Scalar>("highAlarmSeverity")
        .has<Scalar>("hysteresis");
}

bool NTField::isAlarmLimit(FieldConstPtr const & field)
{
    Result result(field);
    return isAlarmLimit(result.is<Structure>()).valid();
}

Result& NTField::isControl(Result& result)
{
    return result
        .has<Scalar>("limitLow")
        .has<Scalar>("limitHigh")
        .has<Scalar>("minStep");
}

bool NTField::isControl(FieldConstPtr const & field)
{
    Result result(field);
    return isControl(result.is<Structure>()).valid();
}

StructureConstPtr NTField::createEnumerated()
{
    return standardField->enumerated();
}

StructureConstPtr NTField::createTimeStamp()
{
    return standardField->timeStamp();
}

StructureConstPtr NTField::createAlarm()
{
    return standardField->alarm();
}

StructureConstPtr NTField::createDisplay()
{
    return standardField->display();
}

StructureConstPtr NTField::createControl()
{
    return standardField->control();
}

StructureArrayConstPtr NTField::createEnumeratedArray()
{
    return fieldCreate->createStructureArray(createEnumerated());
}

StructureArrayConstPtr NTField::createTimeStampArray()
{
    StructureConstPtr st = createTimeStamp();
    return fieldCreate->createStructureArray(st);
}

StructureArrayConstPtr NTField::createAlarmArray()
{
    StructureConstPtr st = createAlarm();
    return fieldCreate->createStructureArray(st);
}

PVNTFieldPtr PVNTField::get()
{
    static Mutex mutex;
    static PVNTFieldPtr pvntstructureField;
    Lock xx(mutex);
    if(pvntstructureField.get()==NULL) {
         pvntstructureField = PVNTFieldPtr(new PVNTField());
    }
    return pvntstructureField;
}

PVNTField::PVNTField()
: pvDataCreate(getPVDataCreate()),
  standardField(getStandardField()),
  standardPVField(getStandardPVField()),
  ntstructureField(NTField::get())
{
}


PVStructurePtr PVNTField::createEnumerated(
    StringArray const & choices)
{
    return standardPVField->enumerated(choices);
}

PVStructurePtr PVNTField::createTimeStamp()
{
    StructureConstPtr timeStamp = standardField->timeStamp();
    return pvDataCreate->createPVStructure(timeStamp);
}

PVStructurePtr PVNTField::createAlarm()
{
    StructureConstPtr alarm = standardField->alarm();
    return pvDataCreate->createPVStructure(alarm);
}

PVStructurePtr PVNTField::createDisplay()
{
    StructureConstPtr display = standardField->display();
    return pvDataCreate->createPVStructure(display);
}


PVStructurePtr PVNTField::createControl()
{
    StructureConstPtr control = standardField->control();
    return pvDataCreate->createPVStructure(control);
}

PVStructureArrayPtr PVNTField::createEnumeratedArray()
{
    StructureArrayConstPtr sa =
        ntstructureField->createEnumeratedArray();
    return pvDataCreate->createPVStructureArray(sa);
}

PVStructureArrayPtr PVNTField::createTimeStampArray()
{
    StructureArrayConstPtr sa =
         ntstructureField->createTimeStampArray();
    return pvDataCreate->createPVStructureArray(sa);
}

PVStructureArrayPtr PVNTField::createAlarmArray()
{
    StructureArrayConstPtr sa = ntstructureField->createAlarmArray();
    return pvDataCreate->createPVStructureArray(sa);
}

}}

