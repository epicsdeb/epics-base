/* ntndarray.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <algorithm>

#include <pv/lock.h>
#include <pv/sharedPtr.h>

#include "validator.h"

#define epicsExportSharedSymbols
#include <pv/ntndarray.h>
#include <pv/ntndarrayAttribute.h>
#include <pv/ntutils.h>

using namespace std;
using namespace epics::pvData;

namespace epics { namespace nt {

namespace detail {

static FieldCreatePtr fieldCreate = getFieldCreate();
static PVDataCreatePtr pvDataCreate = getPVDataCreate();

static Mutex mutex;

StructureConstPtr NTNDArrayBuilder::createStructure()
{
    enum
    {
        DISCRIPTOR_INDEX,
        TIMESTAMP_INDEX,
        ALARM_INDEX,
        DISPLAY_INDEX
    };

    const size_t NUMBER_OF_INDICES = DISPLAY_INDEX+1;
    const size_t NUMBER_OF_STRUCTURES = 1 << NUMBER_OF_INDICES;

    Lock xx(mutex);

    static StructureConstPtr ntndarrayStruc[NUMBER_OF_STRUCTURES];
    static UnionConstPtr valueType;
    static StructureConstPtr codecStruc;
    static StructureConstPtr dimensionStruc;
    static StructureConstPtr attributeStruc;

    StructureConstPtr returnedStruc;

    size_t index = 0;
    if (descriptor) index  |= 1 << DISCRIPTOR_INDEX;
    if (timeStamp)  index  |= 1 << TIMESTAMP_INDEX;
    if (alarm)      index  |= 1 << ALARM_INDEX;
    if (display)    index  |= 1 << DISPLAY_INDEX;

    bool isExtended = !extraFieldNames.empty();

    if (isExtended || !ntndarrayStruc[index])
    {
        StandardFieldPtr standardField = getStandardField();
        FieldBuilderPtr fb = fieldCreate->createFieldBuilder();

        if (!valueType)
        {
            for (int i = pvBoolean; i < pvString; ++i)
            {
                ScalarType st = static_cast<ScalarType>(i);
                fb->addArray(std::string(ScalarTypeFunc::name(st)) + "Value", st);
            }
            valueType = fb->createUnion();                
        }

        if (!codecStruc)
        {
            codecStruc = fb->setId("codec_t")->
                add("name", pvString)->
                add("parameters", fieldCreate->createVariantUnion())->
                createStructure();
        }

        if (!dimensionStruc)
        {
            dimensionStruc = fb->setId("dimension_t")->
                add("size", pvInt)->
                add("offset",  pvInt)->
                add("fullSize",  pvInt)->
                add("binning",  pvInt)->
                add("reverse",  pvBoolean)->
                createStructure();
        }

        if (!attributeStruc)
        {
            attributeStruc = NTNDArrayAttribute::createBuilder()->createStructure();
        }

        fb->setId(NTNDArray::URI)->
            add("value", valueType)->
            add("codec", codecStruc)->
            add("compressedSize", pvLong)->
            add("uncompressedSize", pvLong)->
            addArray("dimension", dimensionStruc)->
            add("uniqueId", pvInt)->
            add("dataTimeStamp", standardField->timeStamp())->
            addArray("attribute", attributeStruc);

        if (descriptor)
            fb->add("descriptor", pvString);

        if (alarm)
            fb->add("alarm", standardField->alarm());

        if (timeStamp)
            fb->add("timeStamp", standardField->timeStamp());

        if (display)
            fb->add("display", standardField->display());

        size_t extraCount = extraFieldNames.size();
        for (size_t i = 0; i< extraCount; i++)
            fb->add(extraFieldNames[i], extraFields[i]);

        returnedStruc = fb->createStructure();

        if (!isExtended)
            ntndarrayStruc[index] = returnedStruc; 
    }
    else
    {
        return ntndarrayStruc[index];
    }

    return returnedStruc;
}

NTNDArrayBuilder::shared_pointer NTNDArrayBuilder::addDescriptor()
{
    descriptor = true;
    return shared_from_this();
}

NTNDArrayBuilder::shared_pointer NTNDArrayBuilder::addAlarm()
{
    alarm = true;
    return shared_from_this();
}

NTNDArrayBuilder::shared_pointer NTNDArrayBuilder::addTimeStamp()
{
    timeStamp = true;
    return shared_from_this();
}

NTNDArrayBuilder::shared_pointer NTNDArrayBuilder::addDisplay()
{
    display = true;
    return shared_from_this();
}

PVStructurePtr NTNDArrayBuilder::createPVStructure()
{
    return getPVDataCreate()->createPVStructure(createStructure());
}

NTNDArrayPtr NTNDArrayBuilder::create()
{
    return NTNDArrayPtr(new NTNDArray(createPVStructure()));
}

NTNDArrayBuilder::NTNDArrayBuilder()
{
    reset();
}

void NTNDArrayBuilder::reset()
{
    descriptor = false;
    timeStamp = false;
    alarm = false;
    display = false;
    extraFieldNames.clear();
    extraFields.clear();
}

NTNDArrayBuilder::shared_pointer NTNDArrayBuilder::add(string const & name, FieldConstPtr const & field)
{
    extraFields.push_back(field); extraFieldNames.push_back(name);
    return shared_from_this();
}

}

const std::string NTNDArray::URI("epics:nt/NTNDArray:1.0");
const std::string ntAttrStr("epics:nt/NTAttribute:1.0");

NTNDArray::shared_pointer NTNDArray::wrap(PVStructurePtr const & pvStructure)
{
    if(!isCompatible(pvStructure)) return shared_pointer();
    return wrapUnsafe(pvStructure);
}

NTNDArray::shared_pointer NTNDArray::wrapUnsafe(PVStructurePtr const & pvStructure)
{
    return shared_pointer(new NTNDArray(pvStructure));
}

bool NTNDArray::is_a(StructureConstPtr const & structure)
{
    return NTUtils::is_a(structure->getID(), URI);
}

bool NTNDArray::is_a(PVStructurePtr const & pvStructure)
{
    return is_a(pvStructure->getStructure());
}

namespace {
    Result& isValue(Result& result)
    {
        result.is<Union>(Union::defaultId());

        for (int i = pvBoolean; i < pvString; ++i) {
            ScalarType type = static_cast<ScalarType>(i);
            string name(ScalarTypeFunc::name(type));
            result.has<ScalarArray>(name + "Value");
        }

        return result;
    }

    Result& isCodec(Result& result)
    {
        return result
            .is<Structure>("codec_t")
            .has<Scalar>("name")
            .has<Union>("parameters");
    }

    Result& isDimension(Result& result)
    {
        return result
            .is<StructureArray>("dimension_t[]")
            .has<Scalar>("size")
            .has<Scalar>("offset")
            .has<Scalar>("fullSize")
            .has<Scalar>("binning")
            .has<Scalar>("reverse");
    }
}

bool NTNDArray::isCompatible(StructureConstPtr const &structure)
{
    if (!structure)
        return false;

    Result result(structure);

    return result
        .is<Structure>()
        .has<&isValue>("value")
        .has<&isCodec>("codec")
        .has<Scalar>("compressedSize")
        .has<Scalar>("uncompressedSize")
        .has<&isDimension>("dimension")
        .has<Scalar>("uniqueId")
        .has<&NTField::isTimeStamp, Structure>("dataTimeStamp")
        .has<&NTNDArrayAttribute::isAttribute, StructureArray>("attribute")
        .maybeHas<Scalar>("descriptor")
        .maybeHas<&NTField::isAlarm, Structure>("alarm")
        .maybeHas<&NTField::isTimeStamp, Structure>("timeStamp")
        .maybeHas<&NTField::isDisplay, Structure>("display")
        .valid();
}


bool NTNDArray::isCompatible(PVStructurePtr const & pvStructure)
{
    if(!pvStructure.get()) return false;

    return isCompatible(pvStructure->getStructure());
}

bool NTNDArray::isValid()
{
    int64 valueSize = getValueSize();
    int64 compressedSize = getCompressedDataSize()->get();
    if (valueSize != compressedSize)
        return false;

    long expectedUncompressed = getExpectedUncompressedSize();
    long uncompressedSize = getUncompressedDataSize()->get();
    if (uncompressedSize != expectedUncompressed)
        return false;

    std::string codecName = getCodec()->getSubField<PVString>("name")->get();
    if (codecName == "" && valueSize < uncompressedSize)
        return false;

    return true;
}

int64 NTNDArray::getExpectedUncompressedSize()
{
    int64 size = 0;
    PVStructureArrayPtr pvDim = getDimension();

    if (pvDim->getLength() != 0)
    {
        PVStructureArray::const_svector data = pvDim->view();
        size = getValueTypeSize();
        for (PVStructureArray::const_svector::const_iterator it = data.begin();
        it != data.end(); ++it )
        {
            PVStructurePtr dim = *it;
            size *= dim->getSubField<PVInt>("size")->get();
        }
    }

    return size;
}

int64 NTNDArray::getValueSize()
{
    int64 size = 0;
    PVScalarArrayPtr storedValue = getValue()->get<PVScalarArray>();
    if (!storedValue.get())
    {
        size = storedValue->getLength()*getValueTypeSize();
    }
    return size;
}

int64 NTNDArray::getValueTypeSize()
{
    int64 typeSize = 0;
    PVScalarArrayPtr storedValue = getValue()->get<PVScalarArray>();
    if (storedValue.get())
    {
        switch (storedValue->getScalarArray()->getElementType())
        {
        case pvBoolean:
        case pvByte:
        case pvUByte:
            typeSize = 1;
            break;

        case pvShort:
        case pvUShort:
            typeSize = 2;
            break;

        case pvInt:
        case pvUInt:
        case pvFloat:
            typeSize = 4;
            break;

        case pvLong:
        case pvULong:
        case pvDouble:
            typeSize = 8;
            break;

        default:
            break;
        }
    }
    return typeSize;
}

NTNDArrayBuilderPtr NTNDArray::createBuilder()
{
    return NTNDArrayBuilderPtr(new detail::NTNDArrayBuilder());
}


bool NTNDArray::attachTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTNDArray::attachDataTimeStamp(PVTimeStamp &pvTimeStamp) const
{
    PVStructurePtr ts = getDataTimeStamp();
    if (ts)
        return pvTimeStamp.attach(ts);
    else
        return false;
}

bool NTNDArray::attachAlarm(PVAlarm &pvAlarm) const
{
    PVStructurePtr al = getAlarm();
    if (al)
        return pvAlarm.attach(al);
    else
        return false;
}

bool NTNDArray::attachDisplay(PVDisplay &pvDisplay) const
{
    PVStructurePtr dp = getDisplay();
    if (dp)
        return pvDisplay.attach(dp);
    else
        return false;
}

PVStructurePtr NTNDArray::getPVStructure() const
{
    return pvNTNDArray;
}

PVUnionPtr NTNDArray::getValue() const
{
    return pvNTNDArray->getSubField<PVUnion>("value");
}

PVStructurePtr NTNDArray::getCodec() const
{
    return pvNTNDArray->getSubField<PVStructure>("codec");
}

PVLongPtr NTNDArray::getCompressedDataSize() const
{
    return pvNTNDArray->getSubField<PVLong>("compressedSize");
}

PVLongPtr NTNDArray::getUncompressedDataSize() const
{
    return pvNTNDArray->getSubField<PVLong>("uncompressedSize");
}

PVStructureArrayPtr NTNDArray::getDimension() const
{
    return pvNTNDArray->getSubField<PVStructureArray>("dimension");
}

PVIntPtr NTNDArray::getUniqueId() const
{
    return pvNTNDArray->getSubField<PVInt>("uniqueId");
}

PVStructurePtr NTNDArray::getDataTimeStamp() const
{
    return pvNTNDArray->getSubField<PVStructure>("dataTimeStamp");
}

PVStructureArrayPtr NTNDArray::getAttribute() const
{
    return pvNTNDArray->getSubField<PVStructureArray>("attribute");
}

PVStringPtr NTNDArray::getDescriptor() const
{
    return pvNTNDArray->getSubField<PVString>("descriptor");
}

PVStructurePtr NTNDArray::getTimeStamp() const
{
    return pvNTNDArray->getSubField<PVStructure>("timeStamp");
}

PVStructurePtr NTNDArray::getAlarm() const
{
    return pvNTNDArray->getSubField<PVStructure>("alarm");
}

PVStructurePtr NTNDArray::getDisplay() const
{
    return pvNTNDArray->getSubField<PVStructure>("display");
}


NTNDArray::NTNDArray(PVStructurePtr const & pvStructure) :
    pvNTNDArray(pvStructure)
{}


}}
