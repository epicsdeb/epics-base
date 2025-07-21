/* ntmultiChannel.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <algorithm>

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntmultiChannel.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt { 


static FieldCreatePtr fieldCreate = getFieldCreate();
static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static NTFieldPtr ntField = NTField::get();

namespace detail {

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::value(UnionConstPtr valuePtr)
{
    valueType = valuePtr;
    return shared_from_this();
}


NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addSeverity()
{
    severity = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addStatus()
{
    status = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addMessage()
{
    message = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addSecondsPastEpoch()
{
    secondsPastEpoch = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addNanoseconds()
{
    nanoseconds = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addUserTag()
{
    userTag = true;
    return shared_from_this();
}

NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::addIsConnected()
{
    isConnected = true;
    return shared_from_this();
}

StructureConstPtr NTMultiChannelBuilder::createStructure()
{
    StandardFieldPtr standardField = getStandardField();
    size_t nfields = 2;
    size_t extraCount = extraFieldNames.size();
    nfields += extraCount;
    if(descriptor) ++nfields;
    if(alarm) ++nfields;
    if(timeStamp) ++nfields;
    if(severity) ++nfields;
    if(status) ++nfields;
    if(message) ++nfields;
    if(secondsPastEpoch) ++nfields;
    if(nanoseconds) ++nfields;
    if(userTag) ++nfields;
    if(isConnected) ++nfields;
    FieldConstPtrArray fields(nfields);
    StringArray names(nfields);
    size_t ind = 0;
    names[ind] = "value";
    if(valueType) {
        fields[ind++] =  fieldCreate->createUnionArray(valueType);
    } else {
        fields[ind++] =  fieldCreate->createVariantUnionArray();
    }
    names[ind] = "channelName";
    fields[ind++] =  fieldCreate->createScalarArray(pvString);
    if(descriptor) {
        names[ind] = "descriptor";
        fields[ind++] = fieldCreate->createScalar(pvString);
    }
    if(alarm) {
        names[ind] = "alarm";
        fields[ind++] = standardField->alarm();
    }
    if(timeStamp) {
        names[ind] = "timeStamp";
        fields[ind++] = standardField->timeStamp();
    }
    if(severity) {
        names[ind] = "severity";
        fields[ind++] = fieldCreate->createScalarArray(pvInt);
    }
    if(status) {
        names[ind] = "status";
        fields[ind++] = fieldCreate->createScalarArray(pvInt);
    }
    if(message) {
        names[ind] = "message";
        fields[ind++] = fieldCreate->createScalarArray(pvString);
    }
    if(secondsPastEpoch) {
        names[ind] = "secondsPastEpoch";
        fields[ind++] = fieldCreate->createScalarArray(pvLong);
    }
    if(nanoseconds) {
        names[ind] = "nanoseconds";
        fields[ind++] = fieldCreate->createScalarArray(pvInt);
    }
    if(userTag) {
        names[ind] = "userTag";
        fields[ind++] = fieldCreate->createScalarArray(pvInt);
    }
    if(isConnected) {
        names[ind] = "isConnected";
        fields[ind++] =  fieldCreate->createScalarArray(pvBoolean);
    }
    for (size_t i = 0; i< extraCount; i++) {
        names[ind] = extraFieldNames[i];
        fields[ind++] = extraFields[i];
    }

    StructureConstPtr st = fieldCreate->createStructure(NTMultiChannel::URI,names,fields);
    reset();
    return st;
}

PVStructurePtr NTMultiChannelBuilder::createPVStructure()
{
    return pvDataCreate->createPVStructure(createStructure());
}

NTMultiChannelPtr NTMultiChannelBuilder::create()
{
    return NTMultiChannelPtr(new NTMultiChannel(createPVStructure()));
}

NTMultiChannelBuilder::NTMultiChannelBuilder()
{
    reset();
}

void NTMultiChannelBuilder::reset()
{
    valueType.reset();
    extraFieldNames.clear();
    extraFields.clear();
    descriptor = false;
    alarm = false;
    timeStamp = false;
    severity = false;
    status = false;
    message = false;
    secondsPastEpoch = false;
    nanoseconds = false;
    userTag = false;
    isConnected = false;
}


NTMultiChannelBuilder::shared_pointer NTMultiChannelBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}

}

const std::string NTMultiChannel::URI("epics:nt/NTMultiChannel:1.0");

NTMultiChannel::shared_pointer NTMultiChannel::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTMultiChannel::shared_pointer NTMultiChannel::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTMultiChannel(pvStructure));
}

bool NTMultiChannel::is_a(StructureConstPtr const &structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTMultiChannel::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}

bool NTMultiChannel::isCompatible(StructureConstPtr const & structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
        .is<Structure>()
        .has<UnionArray>("value")
        .has<ScalarArray>("channelName")
        .maybeHas<ScalarArray>("severity")
        .maybeHas<ScalarArray>("status")
        .maybeHas<ScalarArray>("message")
        .maybeHas<ScalarArray>("secondsPastEpoch")
        .maybeHas<ScalarArray>("nanoseconds")
        .maybeHas<ScalarArray>("userTag")
        .maybeHas<Scalar>("descriptor")
        .maybeHas<&NTField::isAlarm, Structure>("alarm")
        .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
        .valid();
}

bool NTMultiChannel::isCompatible(PVStructurePtr const &pvStructure)
{
    if(!pvStructure.get()) return false;

    return isCompatible(pvStructure->getStructure());
}


bool NTMultiChannel::isValid()
{
    size_t valueLength = getValue()->getLength();
    if (getChannelName()->getLength() != valueLength) return false;

    PVScalarArrayPtr arrayFields[] = {
          getSeverity(), getStatus(), getMessage(), 
          getSecondsPastEpoch(), getNanoseconds(), getUserTag()
    };
    size_t N = sizeof(arrayFields)/sizeof(arrayFields[0]);

    PVScalarArrayPtr arrayField;
    for (PVScalarArrayPtr * pa = arrayFields; pa != arrayFields+N; ++pa)
    {
        arrayField = *pa;
        if (arrayField.get() && arrayField->getLength() != valueLength)
            return false;
    }
    return true; 
}


NTMultiChannelBuilderPtr NTMultiChannel::createBuilder()
{
    return NTMultiChannelBuilderPtr(new detail::NTMultiChannelBuilder());
}

NTMultiChannel::NTMultiChannel(PVStructurePtr const & pvStructure)
: pvNTMultiChannel(pvStructure),
  pvTimeStamp(pvStructure->getSubField<PVStructure>("timeStamp")),
  pvAlarm(pvStructure->getSubField<PVStructure>("alarm")),
  pvValue(pvStructure->getSubField<PVUnionArray>("value")),
  pvChannelName(pvStructure->getSubField<PVStringArray>("channelName")),
  pvIsConnected(pvStructure->getSubField<PVBooleanArray>("isConnected")),
  pvSeverity(pvStructure->getSubField<PVIntArray>("severity")),
  pvStatus(pvStructure->getSubField<PVIntArray>("status")),
  pvMessage(pvStructure->getSubField<PVStringArray>("message")),
  pvSecondsPastEpoch(pvStructure->getSubField<PVLongArray>("secondsPastEpoch")),
  pvNanoseconds(pvStructure->getSubField<PVIntArray>("nanoseconds")),
  pvUserTag(pvStructure->getSubField<PVIntArray>("userTag")),
  pvDescriptor(pvStructure->getSubField<PVString>("descriptor"))
{
}


bool  NTMultiChannel::attachTimeStamp(PVTimeStamp &pv) const
{
    if (pvTimeStamp)
        return pv.attach(pvTimeStamp);
    else
        return false;
}

bool  NTMultiChannel::attachAlarm(PVAlarm &pv) const
{
    if (pvAlarm)
        return pv.attach(pvAlarm);
    else
        return false;
}

}}
