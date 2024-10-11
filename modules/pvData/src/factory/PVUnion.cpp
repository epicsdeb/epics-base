/*PVUnion.cpp*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mse
 */
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <vector>

#define epicsExportSharedSymbols
#include <pv/pvData.h>
#include <pv/pvIntrospect.h>
#include <pv/factory.h>
#include <pv/serializeHelper.h>

using std::tr1::static_pointer_cast;
using std::size_t;
using std::string;

namespace epics { namespace pvData {

#define PVUNION_UNDEFINED_INDEX -1
const int32 PVUnion::UNDEFINED_INDEX = PVUNION_UNDEFINED_INDEX;

PVDataCreatePtr PVUnion::pvDataCreate(getPVDataCreate());

PVUnion::PVUnion(UnionConstPtr const & unionPtr)
: PVField(unionPtr),
  unionPtr(unionPtr),
  selector(PVUNION_UNDEFINED_INDEX),    // to allow out-of-order static initialization
  value(),
  variant(unionPtr->isVariant())
{
}

#undef PVUNION_UNDEFINED_INDEX

PVUnion::~PVUnion() {}

string PVUnion::getSelectedFieldName() const
{
    // no name for undefined and for variant unions
    if (selector == UNDEFINED_INDEX)
        return string();
    else
        return unionPtr->getFieldName(selector);
}

PVFieldPtr PVUnion::select(int32 index)
{
    if (variant && index != UNDEFINED_INDEX)
        throw std::invalid_argument("index out of bounds");

    // no change
    if (selector == index && !variant)
        return value;

    if (index == UNDEFINED_INDEX)
    {
        selector = UNDEFINED_INDEX;
        value.reset();
        return value;
    }
    else if (index < 0 || size_t(index) >= unionPtr->getFields().size())
        throw std::invalid_argument("index out of bounds");

    FieldConstPtr field = unionPtr->getField(index);
    selector = index;
    value = pvDataCreate->createPVField(field);

    return value;
}

PVFieldPtr PVUnion::select(string const & fieldName)
{
    int32 index = variant ? -1 : static_cast<int32>(unionPtr->getFieldIndex(fieldName));
    if (index == -1)
        throw std::invalid_argument("no such fieldName");
    return select(index);
}

void PVUnion::set(int32 index, PVFieldPtr const & value)
{
    if (variant && index != UNDEFINED_INDEX)
        throw std::invalid_argument("index out of bounds");
    else if (!variant)
    {
        if (index == UNDEFINED_INDEX)
        {
            // for undefined index we accept only null values
            if (value)
                throw std::invalid_argument("non-null value for index == UNDEFINED_INDEX");
        }
        else if (index < 0 || size_t(index) >= unionPtr->getFields().size())
            throw std::invalid_argument("index out of bounds");
        else if (!value)
            throw std::invalid_argument("Can't set defined index w/ NULL");
        else if (value->getField() != unionPtr->getField(index))
            throw std::invalid_argument("selected field and its introspection data do not match");
    }

    selector = index;
    this->value = value;
    postPut();
}

void PVUnion::set(string const & fieldName, PVFieldPtr const & value)
{
    int32 index = variant ? -1 : static_cast<int32>(unionPtr->getFieldIndex(fieldName));
    if (index == -1)
        throw std::invalid_argument("no such fieldName");

    set(index, value);
}

void PVUnion::serialize(ByteBuffer *pbuffer, SerializableControl *pflusher) const
{
    if (variant)
    {
        // write introspection data
        if (value.get() == 0) {
            pflusher->ensureBuffer(1);
            pbuffer->put((int8)-1);
        }else {
            pflusher->cachedSerialize(value->getField(), pbuffer);
            value->serialize(pbuffer, pflusher);
        }
    }
    else
    {
        // write selector value
        SerializeHelper::writeSize(selector, pbuffer, pflusher);
        // write value, no value for UNDEFINED_INDEX
        if (selector != UNDEFINED_INDEX)
            value->serialize(pbuffer, pflusher);
    }
}

void PVUnion::deserialize(ByteBuffer *pbuffer, DeserializableControl *pcontrol)
{
    if (variant)
    {
        FieldConstPtr field = pcontrol->cachedDeserialize(pbuffer);
        if (field.get())
        {
            // try to reuse existing field instance
            if (!value.get() || *value->getField() != *field)
                value = pvDataCreate->createPVField(field);
            value->deserialize(pbuffer, pcontrol);
        }
        else
            value.reset();
    }
    else
    {
        int32 previousSelector = selector;
        selector = static_cast<int32>(SerializeHelper::readSize(pbuffer, pcontrol));
        if (selector != UNDEFINED_INDEX)
        {
            if (selector != previousSelector)
            {
                FieldConstPtr field = unionPtr->getField(selector);
                // try to reuse existing field instance
                if (!value.get() || *value->getField() != *field)
                    value = pvDataCreate->createPVField(field);
            }
            value->deserialize(pbuffer, pcontrol);
        }
        else
            value.reset();
    }
}

std::ostream& PVUnion::dumpValue(std::ostream& o) const
{
    o << format::indent() << getUnion()->getID() << ' ' << getFieldName() << std::endl;
    {
        format::indent_scope s(o);
        const PVField::const_shared_pointer& fieldField = get();
        if (fieldField.get() == NULL)
          o << format::indent() << "(none)" << std::endl;
        else
        {
            Type type = fieldField->getField()->getType();
            if (type == scalar || type == scalarArray)
                o << format::indent() << fieldField->getField()->getID() << ' ' << fieldField->getFieldName() << ' ' << *(fieldField.get()) << std::endl;
            else
                o << *(fieldField.get());
       }
    }
    return o;
}

void PVUnion::copy(const PVUnion& from)
{
    if(isImmutable())
        throw std::invalid_argument("destination is immutable");

    if(*getUnion() != *from.getUnion())
        throw std::invalid_argument("union definitions do not match");

    copyUnchecked(from);
}

void PVUnion::copyUnchecked(const PVUnion& from)
{

    const PVField::const_shared_pointer& fromValue = from.get();
    if (from.getUnion()->isVariant())
    {
        if (fromValue.get() == 0)
        {
            set(PVField::shared_pointer());
        }
        else
        {
            PVFieldPtr toValue = get();
            if (toValue.get() == 0 || *toValue->getField() != *fromValue->getField())
            {
                toValue = pvDataCreate->createPVField(fromValue->getField());
                toValue->copyUnchecked(*fromValue);
                set(toValue);
            }
            else
            {
                toValue->copyUnchecked(*fromValue);
                postPut();
            }
        }
    }
    else
    {
        if (fromValue.get() == 0)
        {
            select(PVUnion::UNDEFINED_INDEX);
        }
        else
        {
            select(from.getSelectedIndex())->copyUnchecked(*fromValue);
        }
        postPut();
    }

}


}}
