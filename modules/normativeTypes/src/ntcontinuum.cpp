/* ntcontinuum.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntcontinuum.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt {

static NTFieldPtr ntField = NTField::get();

namespace detail {

StructureConstPtr NTContinuumBuilder::createStructure()
{
    FieldBuilderPtr builder =
            getFieldCreate()->createFieldBuilder()->
               setId(NTContinuum::URI)->
               addArray("base", pvDouble)->
               addArray("value", pvDouble)->
               addArray("units", pvString);

    if (descriptor)
        builder->add("descriptor", pvString);

    if (alarm)
        builder->add("alarm", ntField->createAlarm());

    if (timeStamp)
        builder->add("timeStamp", ntField->createTimeStamp());

    size_t extraCount = extraFieldNames.size();
    for (size_t i = 0; i< extraCount; i++)
        builder->add(extraFieldNames[i], extraFields[i]);


    StructureConstPtr s = builder->createStructure();

    reset();
    return s;
}

NTContinuumBuilder::shared_pointer NTContinuumBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTContinuumBuilder::shared_pointer NTContinuumBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTContinuumBuilder::shared_pointer NTContinuumBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}


PVStructurePtr NTContinuumBuilder::createPVStructure()
{
    return getPVDataCreate()->createPVStructure(createStructure());
}

NTContinuumPtr NTContinuumBuilder::create()
{
    return NTContinuumPtr(new NTContinuum(createPVStructure()));
}

NTContinuumBuilder::NTContinuumBuilder()
{
    reset();
}

void NTContinuumBuilder::reset()
{
    descriptor = false;
    alarm = false;
    timeStamp = false;
    extraFieldNames.clear();
    extraFields.clear();
}

NTContinuumBuilder::shared_pointer NTContinuumBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}

}

const std::string NTContinuum::URI("epics:nt/NTContinuum:1.0");

NTContinuum::shared_pointer NTContinuum::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTContinuum::shared_pointer NTContinuum::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTContinuum(pvStructure));
}

bool NTContinuum::is_a(StructureConstPtr const & structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTContinuum::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}

bool NTContinuum::isCompatible(StructureConstPtr const & structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
        .is<Structure>()
        .has<ScalarArray>("base")
        .has<ScalarArray>("value")
        .has<ScalarArray>("units")
        .maybeHas<Scalar>("descriptor")
        .maybeHas<&NTField::isAlarm, Structure>("alarm")
        .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
        .valid();
}

bool NTContinuum::isCompatible(PVStructurePtr const & pvStructure)
{
    if(!pvStructure) return false;

    return isCompatible(pvStructure->getStructure());
}

bool NTContinuum::isValid()
{
    return ((getUnits()->getLength()-1)*getBase()->getLength() ==
            getValue()->getLength());
}

NTContinuumBuilderPtr NTContinuum::createBuilder()
{
    return NTContinuumBuilderPtr(new detail::NTContinuumBuilder());
}

bool NTContinuum::attachTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTContinuum::attachAlarm(PVAlarm &pvAlarm) const
{
    PVStructurePtr al = getAlarm();
    if (al)
        return pvAlarm.attach(al);
    else
        return false;
}

PVStructurePtr NTContinuum::getPVStructure() const
{
    return pvNTContinuum;
}

PVStringPtr NTContinuum::getDescriptor() const
{
    return pvNTContinuum->getSubField<PVString>("descriptor");
}

PVStructurePtr NTContinuum::getTimeStamp() const
{
    return pvNTContinuum->getSubField<PVStructure>("timeStamp");
}

PVStructurePtr NTContinuum::getAlarm() const
{
    return pvNTContinuum->getSubField<PVStructure>("alarm");
}

PVDoubleArrayPtr NTContinuum::getBase() const
{
    return pvNTContinuum->getSubField<PVDoubleArray>("base");
}

PVDoubleArrayPtr NTContinuum::getValue() const
{
    return pvValue;
}

PVStringArrayPtr NTContinuum::getUnits() const
{
    return pvNTContinuum->getSubField<PVStringArray>("units");
}

NTContinuum::NTContinuum(PVStructurePtr const & pvStructure) :
    pvNTContinuum(pvStructure),
    pvValue(pvNTContinuum->getSubField<PVDoubleArray>("value"))
{}


}}
