/* ntcontinuum.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/nthistogram.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt {

static NTFieldPtr ntField = NTField::get();

namespace detail {

NTHistogramBuilder::shared_pointer NTHistogramBuilder::value(
        epics::pvData::ScalarType scalarType
        )
{
    valueType = scalarType;
    valueTypeSet = true;

    return shared_from_this();
}

StructureConstPtr NTHistogramBuilder::createStructure()
{
    if (!valueTypeSet)
        throw std::runtime_error("value array element type not set");

    FieldBuilderPtr builder =
            getFieldCreate()->createFieldBuilder()->
               setId(NTHistogram::URI)->
               addArray("ranges", pvDouble)->
               addArray("value", valueType);

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

NTHistogramBuilder::shared_pointer NTHistogramBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTHistogramBuilder::shared_pointer NTHistogramBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTHistogramBuilder::shared_pointer NTHistogramBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}


PVStructurePtr NTHistogramBuilder::createPVStructure()
{
    return getPVDataCreate()->createPVStructure(createStructure());
}

NTHistogramPtr NTHistogramBuilder::create()
{
    return NTHistogramPtr(new NTHistogram(createPVStructure()));
}

NTHistogramBuilder::NTHistogramBuilder()
{
    reset();
}

void NTHistogramBuilder::reset()
{
    valueTypeSet = false;
    descriptor = false;
    alarm = false;
    timeStamp = false;
    extraFieldNames.clear();
    extraFields.clear();
}

NTHistogramBuilder::shared_pointer NTHistogramBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}

}

const std::string NTHistogram::URI("epics:nt/NTHistogram:1.0");

NTHistogram::shared_pointer NTHistogram::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTHistogram::shared_pointer NTHistogram::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTHistogram(pvStructure));
}

bool NTHistogram::is_a(StructureConstPtr const & structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTHistogram::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}

bool NTHistogram::isCompatible(StructureConstPtr const &structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
        .is<Structure>()
        .has<ScalarArray>("ranges")
        .has<ScalarArray>("value")
        .maybeHas<Scalar>("descriptor")
        .maybeHas<&NTField::isAlarm, Structure>("alarm")
        .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
        .valid();
}

bool NTHistogram::isCompatible(PVStructurePtr const & pvStructure)
{
    if(!pvStructure.get()) return false;

    return isCompatible(pvStructure->getStructure());
}

bool NTHistogram::isValid()
{
    return (getValue()->getLength()+1 == getRanges()->getLength());
}

NTHistogramBuilderPtr NTHistogram::createBuilder()
{
    return NTHistogramBuilderPtr(new detail::NTHistogramBuilder());
}

bool NTHistogram::attachTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTHistogram::attachAlarm(PVAlarm &pvAlarm) const
{
    PVStructurePtr al = getAlarm();
    if (al)
        return pvAlarm.attach(al);
    else
        return false;
}

PVStructurePtr NTHistogram::getPVStructure() const
{
    return pvNTHistogram;
}

PVStringPtr NTHistogram::getDescriptor() const
{
    return pvNTHistogram->getSubField<PVString>("descriptor");
}

PVStructurePtr NTHistogram::getTimeStamp() const
{
    return pvNTHistogram->getSubField<PVStructure>("timeStamp");
}

PVStructurePtr NTHistogram::getAlarm() const
{
    return pvNTHistogram->getSubField<PVStructure>("alarm");
}

PVDoubleArrayPtr NTHistogram::getRanges() const
{
    return pvNTHistogram->getSubField<PVDoubleArray>("ranges");
}

PVScalarArrayPtr NTHistogram::getValue() const
{
    return pvValue;
}

NTHistogram::NTHistogram(PVStructurePtr const & pvStructure) :
    pvNTHistogram(pvStructure),
    pvValue(pvNTHistogram->getSubField<PVScalarArray>("value"))
{}


}}
