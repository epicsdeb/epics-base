/* ntmatrix.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntmatrix.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt {

static NTFieldPtr ntField = NTField::get();

namespace detail {

StructureConstPtr NTMatrixBuilder::createStructure()
{
    FieldBuilderPtr builder =
            getFieldCreate()->createFieldBuilder()->
               setId(NTMatrix::URI)->
               addArray("value", pvDouble);

    if (dim)
        builder->addArray("dim", pvInt);

    if (descriptor)
        builder->add("descriptor", pvString);

    if (alarm)
        builder->add("alarm", ntField->createAlarm());

    if (timeStamp)
        builder->add("timeStamp", ntField->createTimeStamp());

    if (display)
        builder->add("display", ntField->createDisplay());

    size_t extraCount = extraFieldNames.size();
    for (size_t i = 0; i< extraCount; i++)
        builder->add(extraFieldNames[i], extraFields[i]);


    StructureConstPtr s = builder->createStructure();

    reset();
    return s;
}

NTMatrixBuilder::shared_pointer NTMatrixBuilder::addDim()
{
    dim = true;
    return shared_from_this();
}

NTMatrixBuilder::shared_pointer NTMatrixBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTMatrixBuilder::shared_pointer NTMatrixBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTMatrixBuilder::shared_pointer NTMatrixBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}

NTMatrixBuilder::shared_pointer NTMatrixBuilder::addDisplay()
{
    display = true;
    return shared_from_this();
}

PVStructurePtr NTMatrixBuilder::createPVStructure()
{
    return getPVDataCreate()->createPVStructure(createStructure());
}

NTMatrixPtr NTMatrixBuilder::create()
{
    return NTMatrixPtr(new NTMatrix(createPVStructure()));
}

NTMatrixBuilder::NTMatrixBuilder()
{
    reset();
}

void NTMatrixBuilder::reset()
{
    dim = false;
    descriptor = false;
    alarm = false;
    timeStamp = false;
    display = false;
    extraFieldNames.clear();
    extraFields.clear();
}

NTMatrixBuilder::shared_pointer NTMatrixBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}

}

const std::string NTMatrix::URI("epics:nt/NTMatrix:1.0");

NTMatrix::shared_pointer NTMatrix::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTMatrix::shared_pointer NTMatrix::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTMatrix(pvStructure));
}

bool NTMatrix::is_a(StructureConstPtr const & structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTMatrix::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}

bool NTMatrix::isCompatible(StructureConstPtr const & structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
        .is<Structure>()
        .has<ScalarArray>("value")
        .maybeHas<ScalarArray>("dim")
        .maybeHas<Scalar>("descriptor")
        .maybeHas<&NTField::isAlarm, Structure>("alarm")
        .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
        .maybeHas<&NTField::isDisplay, Structure>("display")
        .valid();
}

bool NTMatrix::isCompatible(PVStructurePtr const & pvStructure)
{
    if(!pvStructure) return false;

    return isCompatible(pvStructure->getStructure());
}

bool NTMatrix::isValid()
{
    int valueLength = getValue()->getLength();
    if (valueLength == 0)
        return false;

    PVIntArrayPtr pvDim = getDim();
    if (pvDim.get())
    {
        int length = pvDim->getLength();
        if (length != 1 && length !=2)
            return false;

        PVIntArray::const_svector data = pvDim->view();
        int expectedLength = 1;
        for (PVIntArray::const_svector::const_iterator it = data.begin();
                 it != data.end(); ++it)
        {
             expectedLength *= *it;
        }
        if (expectedLength != valueLength)
        return false;
    }
    return true;
}

NTMatrixBuilderPtr NTMatrix::createBuilder()
{
    return NTMatrixBuilderPtr(new detail::NTMatrixBuilder());
}

bool NTMatrix::attachTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTMatrix::attachAlarm(PVAlarm &pvAlarm) const
{
    PVStructurePtr al = getAlarm();
    if (al)
        return pvAlarm.attach(al);
    else
        return false;
}

bool NTMatrix::attachDisplay(PVDisplay &pvDisplay) const
{
    PVStructurePtr dp = getDisplay();
    if (dp)
        return pvDisplay.attach(dp);
    else
        return false;
}

PVStructurePtr NTMatrix::getPVStructure() const
{
    return pvNTMatrix;
}

PVStringPtr NTMatrix::getDescriptor() const
{
    return pvNTMatrix->getSubField<PVString>("descriptor");
}

PVStructurePtr NTMatrix::getTimeStamp() const
{
    return pvNTMatrix->getSubField<PVStructure>("timeStamp");
}

PVStructurePtr NTMatrix::getAlarm() const
{
    return pvNTMatrix->getSubField<PVStructure>("alarm");
}

PVStructurePtr NTMatrix::getDisplay() const
{
    return pvNTMatrix->getSubField<PVStructure>("display");
}

PVDoubleArrayPtr NTMatrix::getValue() const
{
    return pvValue;
}

PVIntArrayPtr NTMatrix::getDim() const
{
    return pvNTMatrix->getSubField<PVIntArray>("dim");
}

NTMatrix::NTMatrix(PVStructurePtr const & pvStructure) :
    pvNTMatrix(pvStructure),
    pvValue(pvNTMatrix->getSubField<PVDoubleArray>("value"))
{}


}}
