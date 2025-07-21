/* ntenum.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntenum.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt {

static NTFieldPtr ntField = NTField::get();

namespace detail {


StructureConstPtr NTEnumBuilder::createStructure()
{
    FieldBuilderPtr builder =
            getFieldCreate()->createFieldBuilder()->
               setId(NTEnum::URI)->
               add("value", ntField->createEnumerated());

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

NTEnumBuilder::shared_pointer NTEnumBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTEnumBuilder::shared_pointer NTEnumBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTEnumBuilder::shared_pointer NTEnumBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}

PVStructurePtr NTEnumBuilder::createPVStructure()
{
    return getPVDataCreate()->createPVStructure(createStructure());
}

NTEnumPtr NTEnumBuilder::create()
{
    return NTEnumPtr(new NTEnum(createPVStructure()));
}

NTEnumBuilder::NTEnumBuilder()
{
    reset();
}

void NTEnumBuilder::reset()
{
    descriptor = false;
    alarm = false;
    timeStamp = false;
    extraFieldNames.clear();
    extraFields.clear();
}

NTEnumBuilder::shared_pointer NTEnumBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}


}

const std::string NTEnum::URI("epics:nt/NTEnum:1.0");

NTEnum::shared_pointer NTEnum::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTEnum::shared_pointer NTEnum::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTEnum(pvStructure));
}

bool NTEnum::is_a(StructureConstPtr const & structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTEnum::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}

bool NTEnum::isCompatible(StructureConstPtr const &structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
        .is<Structure>()
        .has<&NTField::isEnumerated, Structure>("value")
        .maybeHas<Scalar>("descriptor")
        .maybeHas<&NTField::isAlarm, Structure>("alarm")
        .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
        .valid();
}

bool NTEnum::isCompatible(PVStructurePtr const & pvStructure)
{
    if(!pvStructure) return false;

    return isCompatible(pvStructure->getStructure());
}

bool NTEnum::isValid()
{
    return true;
}

NTEnumBuilderPtr NTEnum::createBuilder()
{
    return NTEnumBuilderPtr(new detail::NTEnumBuilder());
}

bool NTEnum::attachTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTEnum::attachAlarm(PVAlarm &pvAlarm) const
{
    PVStructurePtr al = getAlarm();
    if (al)
        return pvAlarm.attach(al);
    else
        return false;
}

PVStructurePtr NTEnum::getPVStructure() const
{
    return pvNTEnum;
}

PVStringPtr NTEnum::getDescriptor() const
{
    return pvNTEnum->getSubField<PVString>("descriptor");
}

PVStructurePtr NTEnum::getTimeStamp() const
{
    return pvNTEnum->getSubField<PVStructure>("timeStamp");
}

PVStructurePtr NTEnum::getAlarm() const
{
    return pvNTEnum->getSubField<PVStructure>("alarm");
}

PVStructurePtr NTEnum::getValue() const
{
    return pvValue;
}

NTEnum::NTEnum(PVStructurePtr const & pvStructure) :
    pvNTEnum(pvStructure), pvValue(pvNTEnum->getSubField<PVStructure>("value"))
{}


}}
