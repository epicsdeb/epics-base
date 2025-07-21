/* ntaggregate.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntaggregate.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt {

static NTFieldPtr ntField = NTField::get();

namespace detail {

StructureConstPtr NTAggregateBuilder::createStructure()
{
    FieldBuilderPtr builder =
            getFieldCreate()->createFieldBuilder()->
               setId(NTAggregate::URI)->
               add("value", pvDouble)->
               add("N", pvLong);

    if (dispersion)
        builder->add("dispersion", pvDouble);

    if (first)
        builder->add("first", pvDouble);

    if (firstTimeStamp)
        builder->add("firstTimeStamp", ntField->createTimeStamp());

    if (last)
        builder->add("last" , pvDouble);

    if (lastTimeStamp)
        builder->add("lastTimeStamp", ntField->createTimeStamp());

    if (max)
        builder->add("max", pvDouble);

    if (min)
        builder->add("min", pvDouble);

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

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addDispersion()
{
    dispersion = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addFirst()
{
    first = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addFirstTimeStamp()
{
    firstTimeStamp = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addLast()
{
    last = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addLastTimeStamp()
{
    lastTimeStamp = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addMax()
{
    max = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addMin()
{
    min = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}

PVStructurePtr NTAggregateBuilder::createPVStructure()
{
    return getPVDataCreate()->createPVStructure(createStructure());
}

NTAggregatePtr NTAggregateBuilder::create()
{
    return NTAggregatePtr(new NTAggregate(createPVStructure()));
}

NTAggregateBuilder::NTAggregateBuilder()
{
    reset();
}

void NTAggregateBuilder::reset()
{
    dispersion = false;
    first = false;
    firstTimeStamp = false;
    last = false;
    lastTimeStamp = false;
    max = false;
    min = false;

    descriptor = false;
    alarm = false;
    timeStamp = false;

    extraFieldNames.clear();
    extraFields.clear();
}

NTAggregateBuilder::shared_pointer NTAggregateBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}


}

const std::string NTAggregate::URI("epics:nt/NTAggregate:1.0");

NTAggregate::shared_pointer NTAggregate::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTAggregate::shared_pointer NTAggregate::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTAggregate(pvStructure));
}

bool NTAggregate::is_a(StructureConstPtr const & structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTAggregate::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}


bool NTAggregate::isCompatible(StructureConstPtr const &structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
        .is<Structure>()
        .has<Scalar>("value")
        .has<Scalar>("N")
        .maybeHas<Scalar>("dispersion")
        .maybeHas<Scalar>("first")
        .maybeHas<&NTField::isTimeStamp, Structure>("firstTimeStamp")
        .maybeHas<Scalar>("last")
        .maybeHas<&NTField::isTimeStamp, Structure>("lastTimeStamp")
        .maybeHas<Scalar>("max")
        .maybeHas<Scalar>("min")
        .maybeHas<Scalar>("descriptor")
        .maybeHas<&NTField::isAlarm, Structure>("alarm")
        .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
        .valid();
}

bool NTAggregate::isCompatible(PVStructurePtr const & pvStructure)
{
    if(!pvStructure) return false;

    return isCompatible(pvStructure->getStructure());
}

bool NTAggregate::isValid()
{
    return true;
}

NTAggregateBuilderPtr NTAggregate::createBuilder()
{
    return NTAggregateBuilderPtr(new detail::NTAggregateBuilder());
}

bool NTAggregate::attachTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTAggregate::attachAlarm(PVAlarm &pvAlarm) const
{
    PVStructurePtr al = getAlarm();
    if (al)
        return pvAlarm.attach(al);
    else
        return false;
}

PVStructurePtr NTAggregate::getPVStructure() const
{
    return pvNTAggregate;
}

PVStringPtr NTAggregate::getDescriptor() const
{
    return pvNTAggregate->getSubField<PVString>("descriptor");
}

PVStructurePtr NTAggregate::getTimeStamp() const
{
    return pvNTAggregate->getSubField<PVStructure>("timeStamp");
}

PVStructurePtr NTAggregate::getAlarm() const
{
    return pvNTAggregate->getSubField<PVStructure>("alarm");
}

PVDoublePtr NTAggregate::getValue() const
{
    return pvValue;
}

PVLongPtr NTAggregate::getN() const
{
    return pvNTAggregate->getSubField<PVLong>("N");
}

PVDoublePtr NTAggregate::getDispersion() const
{
    return pvNTAggregate->getSubField<PVDouble>("dispersion");
}

PVDoublePtr NTAggregate::getFirst() const
{
    return pvNTAggregate->getSubField<PVDouble>("first");
}

PVStructurePtr NTAggregate::getFirstTimeStamp() const
{
    return pvNTAggregate->getSubField<PVStructure>("firstTimeStamp");
}

PVDoublePtr NTAggregate::getLast() const
{
    return pvNTAggregate->getSubField<PVDouble>("last");
}

PVStructurePtr NTAggregate::getLastTimeStamp() const
{
    return pvNTAggregate->getSubField<PVStructure>("lastTimeStamp");
}

PVDoublePtr NTAggregate::getMax() const
{
    return pvNTAggregate->getSubField<PVDouble>("max");
}

PVDoublePtr NTAggregate::getMin() const
{
    return pvNTAggregate->getSubField<PVDouble>("min");
}

NTAggregate::NTAggregate(PVStructurePtr const & pvStructure) :
    pvNTAggregate(pvStructure), pvValue(pvNTAggregate->getSubField<PVDouble>("value"))
{}


}}
