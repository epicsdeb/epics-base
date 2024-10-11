/* ntattribute.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntattribute.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt {

static NTFieldPtr ntField = NTField::get();

namespace detail {


StructureConstPtr NTAttributeBuilder::createStructure()
{
    FieldBuilderPtr builder =
            getFieldCreate()->createFieldBuilder()->
               setId(NTAttribute::URI)->
               add("name", pvString)->
               add("value", getFieldCreate()->createVariantUnion());

    if (tags)
        builder->addArray("tags", pvString);

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

NTAttributeBuilder::shared_pointer NTAttributeBuilder::addTags()
{
    tags = true;
    return shared_from_this();
}

NTAttributeBuilder::shared_pointer NTAttributeBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTAttributeBuilder::shared_pointer NTAttributeBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTAttributeBuilder::shared_pointer NTAttributeBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}

PVStructurePtr NTAttributeBuilder::createPVStructure()
{
    return getPVDataCreate()->createPVStructure(createStructure());
}

NTAttributePtr NTAttributeBuilder::create()
{
    return NTAttributePtr(new NTAttribute(createPVStructure()));
}

NTAttributeBuilder::NTAttributeBuilder()
{
    reset();
}

void NTAttributeBuilder::reset()
{
    descriptor = false;
    alarm = false;
    timeStamp = false;
    extraFieldNames.clear();
    extraFields.clear();
}

NTAttributeBuilder::shared_pointer NTAttributeBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}

}

const std::string NTAttribute::URI("epics:nt/NTAttribute:1.0");

NTAttribute::shared_pointer NTAttribute::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTAttribute::shared_pointer NTAttribute::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTAttribute(pvStructure));
}

bool NTAttribute::is_a(StructureConstPtr const & structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTAttribute::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}

bool NTAttribute::isCompatible(StructureConstPtr const & structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
       .is<Structure>()
       .has<Scalar>("name")
       .has<Union>("value")
       .maybeHas<ScalarArray>("tags")
       .maybeHas<Scalar>("descriptor")
       .maybeHas<&NTField::isAlarm, Structure>("alarm")
       .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
       .valid();
}

bool NTAttribute::isCompatible(PVStructurePtr const & pvStructure)
{
    if(!pvStructure) return false;

    return isCompatible(pvStructure->getStructure());
}

bool NTAttribute::isValid()
{
    return true;
}

NTAttributeBuilderPtr NTAttribute::createBuilder()
{
    return NTAttributeBuilderPtr(new detail::NTAttributeBuilder());
}

bool NTAttribute::attachTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTAttribute::attachAlarm(PVAlarm &pvAlarm) const
{
    PVStructurePtr al = getAlarm();
    if (al)
        return pvAlarm.attach(al);
    else
        return false;
}

PVStructurePtr NTAttribute::getPVStructure() const
{
    return pvNTAttribute;
}

PVStringPtr NTAttribute::getDescriptor() const
{
    return pvNTAttribute->getSubField<PVString>("descriptor");
}

PVStructurePtr NTAttribute::getTimeStamp() const
{
    return pvNTAttribute->getSubField<PVStructure>("timeStamp");
}

PVStructurePtr NTAttribute::getAlarm() const
{
    return pvNTAttribute->getSubField<PVStructure>("alarm");
}


PVStringPtr NTAttribute::getName() const
{
    return pvNTAttribute->getSubField<PVString>("name");
}

PVUnionPtr NTAttribute::getValue() const
{
    return pvValue;
}

PVStringArrayPtr NTAttribute::getTags() const
{
    return pvNTAttribute->getSubField<PVStringArray>("tags");
}

NTAttribute::NTAttribute(PVStructurePtr const & pvStructure) :
    pvNTAttribute(pvStructure), pvValue(pvNTAttribute->getSubField<PVUnion>("value"))
{
}


}}
