/*PVStructureArray.cpp*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstdio>

#define epicsExportSharedSymbols
#include <pv/pvData.h>
#include <pv/factory.h>
#include <pv/serializeHelper.h>

using std::tr1::static_pointer_cast;
using std::size_t;

namespace epics { namespace pvData {

size_t PVStructureArray::append(size_t number)
{
    checkLength(value.size()+number);

    svector data(reuse());
    data.resize(data.size()+number);

    StructureConstPtr structure = structureArray->getStructure();

    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    for(svector::reverse_iterator it = data.rbegin(); number; ++it, --number)
        *it = pvDataCreate->createPVStructure(structure);

    size_t newLength = data.size();

    const_svector cdata(freeze(data));
    swap(cdata);

    return newLength;
}

bool PVStructureArray::remove(size_t offset,size_t number)
{
    if (number==0)
        return true;
    else if (offset+number>getLength())
        return false;
    else if (getArray()->getArraySizeType() == Array::fixed)
        return false;

    svector vec(reuse());

    size_t length = vec.size();

    for(size_t i = offset; i+number < length; i++) {
         vec[i].swap(vec[i + number]);
    }

    vec.resize(length - number);
    const_svector cdata(freeze(vec));
    swap(cdata);

    return true;
}

void PVStructureArray::compress() {
    if (getArray()->getArraySizeType() == Array::fixed)
            return;

    svector vec(reuse()); // TODO: check for first NULL before realloc

    size_t length = vec.size();
    size_t newLength = 0;

    for(size_t i=0; i<length; i++) {
        if(vec[i]) {
            newLength++;
            continue;
        }
        // find first non 0
        size_t notNull = 0;
        for(size_t j=i+1;j<length;j++) {
            if(vec[j]) {
                notNull = j;
                break;
            }
        }
        if(notNull!=0) {
            vec[i] = vec[notNull];
            vec[notNull].reset();
            newLength++;
            continue;
         }
         break;
    }

    vec.resize(newLength);
    const_svector cdata(freeze(vec));
    swap(cdata);
}

void PVStructureArray::setCapacity(size_t capacity)
{
    if (this->isCapacityMutable()) {
        checkLength(capacity);
        const_svector value;
        swap(value);
        if(value.capacity()<capacity) {
            svector mvalue(thaw(value));
            mvalue.reserve(capacity);
            value = freeze(mvalue);
        }
        swap(value);
    }
    else
        THROW_EXCEPTION2(std::logic_error, "capacity immutable");
}

void PVStructureArray::setLength(size_t length)
{
    if(this->isImmutable())
        THROW_EXCEPTION2(std::logic_error, "immutable");
    const_svector value;
    swap(value);

    if (length == value.size())
        return;

    checkLength(length);

    if (length < value.size()) {
        value.slice(0, length);
    } else {
        svector mvalue(thaw(value));
        mvalue.resize(length);
        value = freeze(mvalue);
    }
    swap(value);
}

void PVStructureArray::swap(const_svector &other)
{
    if (this->isImmutable())
        THROW_EXCEPTION2(std::logic_error, "immutable");

    // no checkLength call here

    value.swap(other);
}

void PVStructureArray::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher) const {
    serialize(pbuffer, pflusher, 0, getLength());
}

void PVStructureArray::deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pcontrol) {
    svector data(reuse());

    size_t size = this->getArray()->getArraySizeType() == Array::fixed ?
                this->getArray()->getMaximumCapacity() :
                SerializeHelper::readSize(pbuffer, pcontrol);

    data.resize(size);

    StructureConstPtr structure = structureArray->getStructure();

    PVDataCreatePtr pvDataCreate = getPVDataCreate();

    for(size_t i = 0; i<size; i++) {
        pcontrol->ensureData(1);
        size_t temp = pbuffer->getByte();
        if(temp==0) {
            data[i].reset();
        }
        else {
            if(data[i].get()==NULL || !data[i].unique()) {
                data[i] = pvDataCreate->createPVStructure(structure);
            }
            data[i]->deserialize(pbuffer, pcontrol);
        }
    }
    replace(freeze(data)); // calls postPut()
}

void PVStructureArray::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, size_t offset, size_t count) const {

    const_svector temp(view());
    temp.slice(offset, count);

    ArrayConstPtr array = this->getArray();
    if (array->getArraySizeType() != Array::fixed)
        SerializeHelper::writeSize(temp.size(), pbuffer, pflusher);
    else if (count != array->getMaximumCapacity())
        throw std::length_error("fixed array cannot be partially serialized");

    for(size_t i = 0; i<count; i++) {
        if(pbuffer->getRemaining()<1)
            pflusher->flushSerializeBuffer();

        if(temp[i].get()==NULL) {
            pbuffer->putByte(0);
        }
        else {
            pbuffer->putByte(1);
            temp[i]->serialize(pbuffer, pflusher);
        }
    }
}

std::ostream& PVStructureArray::dumpValue(std::ostream& o) const
{
    o << format::indent() << getStructureArray()->getID() << ' ' << getFieldName() << std::endl;
    size_t length = getLength();
    if (length > 0)
    {
        format::indent_scope s(o);

        for (size_t i = 0; i < length; i++)
            dumpValue(o, i);
    }

    return o;
}

std::ostream& PVStructureArray::dumpValue(std::ostream& o, std::size_t index) const
{
    const_svector temp(view());
    if (index<temp.size())
    {
        if (temp[index])
            o << *temp[index];
        else
            o << format::indent() << "(none)" << std::endl;
    }
    return o;
}

void PVStructureArray::copy(const PVStructureArray& from)
{
    if(isImmutable())
        throw std::invalid_argument("destination is immutable");

    if(*getStructureArray() != *from.getStructureArray())
        throw std::invalid_argument("structureArray definitions do not match");

    copyUnchecked(from);
}

void PVStructureArray::copyUnchecked(const PVStructureArray& from)
{
    if (this == &from)
        return;

    replace(from.view());
}

}}
