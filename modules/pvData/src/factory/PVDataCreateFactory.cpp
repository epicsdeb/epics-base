/*PVDataCreateFactory.cpp*/
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

#include <epicsMutex.h>
#include <epicsThread.h>

#define epicsExportSharedSymbols
#include <pv/lock.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/factory.h>
#include <pv/serializeHelper.h>
#include <pv/reftrack.h>

using std::tr1::static_pointer_cast;
using std::size_t;
using std::string;
using std::min;

namespace epics { namespace pvData {


template<> const ScalarType PVBoolean::typeCode = pvBoolean;
template<> const ScalarType PVByte::typeCode = pvByte;
template<> const ScalarType PVShort::typeCode = pvShort;
template<> const ScalarType PVInt::typeCode = pvInt;
template<> const ScalarType PVLong::typeCode = pvLong;
template<> const ScalarType PVUByte::typeCode = pvUByte;
template<> const ScalarType PVUShort::typeCode = pvUShort;
template<> const ScalarType PVUInt::typeCode = pvUInt;
template<> const ScalarType PVULong::typeCode = pvULong;
template<> const ScalarType PVFloat::typeCode = pvFloat;
template<> const ScalarType PVDouble::typeCode = pvDouble;
template<> const ScalarType PVScalarValue<string>::typeCode = pvString;

template<> const ScalarType PVBooleanArray::typeCode = pvBoolean;
template<> const ScalarType PVByteArray::typeCode = pvByte;
template<> const ScalarType PVShortArray::typeCode = pvShort;
template<> const ScalarType PVIntArray::typeCode = pvInt;
template<> const ScalarType PVLongArray::typeCode = pvLong;
template<> const ScalarType PVUByteArray::typeCode = pvUByte;
template<> const ScalarType PVUShortArray::typeCode = pvUShort;
template<> const ScalarType PVUIntArray::typeCode = pvUInt;
template<> const ScalarType PVULongArray::typeCode = pvULong;
template<> const ScalarType PVFloatArray::typeCode = pvFloat;
template<> const ScalarType PVDoubleArray::typeCode = pvDouble;
template<> const ScalarType PVStringArray::typeCode = pvString;


template<typename T>
PVScalarValue<T>::~PVScalarValue() {}

template<typename T>
std::ostream& PVScalarValue<T>::dumpValue(std::ostream& o) const
{
    return o << get();
}

template<typename T>
void PVScalarValue<T>::operator>>=(T& value) const
{
    value = get();
}

template<typename T>
void PVScalarValue<T>::operator<<=(typename storage_t::arg_type value)
{
    put(value);
}

template<typename T>
void PVScalarValue<T>::assign(const PVScalar& scalar)
{
    if(isImmutable())
        throw std::invalid_argument("destination is immutable");
    copyUnchecked(scalar);
}

template<typename T>
void PVScalarValue<T>::copy(const PVScalar& from)
{
    assign(from);
}

template<typename T>
void PVScalarValue<T>::copyUnchecked(const PVScalar& from)
{
    if(this==&from)
        return;
    T result;
    from.getAs((void*)&result, typeCode);
    put(result);
}

template<typename T>
void PVScalarValue<T>::serialize(ByteBuffer *pbuffer,
    SerializableControl *pflusher) const {
    pflusher->ensureBuffer(sizeof(T));
    pbuffer->put(storage.value);
}

template<>
void PVScalarValue<std::string>::serialize(ByteBuffer *pbuffer,
    SerializableControl *pflusher) const {
    SerializeHelper::serializeString(storage.value, pbuffer, pflusher);
}

template<typename T>
void PVScalarValue<T>::deserialize(ByteBuffer *pbuffer,
    DeserializableControl *pflusher)
{
    pflusher->ensureData(sizeof(T));
    storage.value = pbuffer->GET(T);
}

template<>
void PVScalarValue<std::string>::deserialize(ByteBuffer *pbuffer,
    DeserializableControl *pflusher)
{
    storage.value = SerializeHelper::deserializeString(pbuffer, pflusher);
    // TODO: check for violations of maxLength?
}

PVString::PVString(ScalarConstPtr const & scalar)
    : PVScalarValue<std::string>(scalar)
{
    BoundedStringConstPtr boundedString = std::tr1::dynamic_pointer_cast<const BoundedString>(scalar);
    if (boundedString.get())
        storage.maxLength = boundedString->getMaximumLength();
    else
        storage.maxLength = 0;
}

std::ostream& PVString::dumpValue(std::ostream& o) const
{
    o<<maybeQuote(get());
    return o;
}

/* mixing overrides (virtual functions) and overloads (different argument lists) is fun...
 * we override all overloads to avoid the "hides overloaded virtual function" warning from clang.
 * In this case we don't need/want to, so just delegate to the base class.
 */
void PVString::serialize(ByteBuffer *pbuffer,
    SerializableControl *pflusher) const
{PVScalarValue<std::string>::serialize(pbuffer, pflusher);}

void PVString::serialize(ByteBuffer *pbuffer,
    SerializableControl *pflusher, size_t offset, size_t count) const
{
    // check bounds
    const size_t length = storage.value.length();
    /*if (offset < 0) offset = 0;
    else*/ if (offset > length) offset = length;
    //if (count < 0) count = length;

    const size_t maxCount = length - offset;
    if (count > maxCount)
        count = maxCount;

    // write
    SerializeHelper::serializeSubstring(storage.value, offset, count, pbuffer, pflusher);
}

void PVArray::checkLength(size_t len) const
{
    Array::ArraySizeType type = getArray()->getArraySizeType();
    if (type != Array::variable)
    {
        size_t size = getArray()->getMaximumCapacity();
        if (type == Array::fixed && len != size)
            throw std::invalid_argument("invalid length for a fixed size array");
        else if (type == Array::bounded && len > size)
            throw std::invalid_argument("new array capacity too large for a bounded size array");
    }
}

template<typename T>
PVValueArray<T>::PVValueArray(ScalarArrayConstPtr const & scalarArray)
    :base_t(scalarArray)
    ,value()

{}

PVValueArray<PVStructurePtr>::PVValueArray(StructureArrayConstPtr const & structureArray)
    :base_t(structureArray)
    ,structureArray(structureArray)

{}

PVValueArray<PVUnionPtr>::PVValueArray(UnionArrayConstPtr const & unionArray)
    :base_t(unionArray)
    ,unionArray(unionArray)
{}

template<typename T>
PVValueArray<T>::~PVValueArray() {}

template<typename T>
ArrayConstPtr PVValueArray<T>::getArray() const
{
    return std::tr1::static_pointer_cast<const Array>(this->getField());
}

template<typename T>
std::ostream& PVValueArray<T>::dumpValue(std::ostream& o) const
{
    const_svector v(this->view());
    typename const_svector::const_iterator it(v.begin()),
                                  end(v.end());
    o << '[';
    if(it!=end) {
        o << print_cast(*it++);
        for(; it!=end; ++it)
            o << ',' << print_cast(*it);

    }
    return o << ']';
}

template<>
std::ostream& PVValueArray<std::string>::dumpValue(std::ostream& o, size_t index) const
{
    return o << maybeQuote(this->view().at(index));
}

template<>
std::ostream& PVValueArray<std::string>::dumpValue(std::ostream& o) const
{
    const_svector v(this->view());
    const_svector::const_iterator it(v.begin()),
                                  end(v.end());
    o << '[';
    if(it!=end) {
        o << maybeQuote(*it++);
        for(; it!=end; ++it)
            o << ", " << maybeQuote(*it);

    }
    return o << ']';
}

template<typename T>
std::ostream& PVValueArray<T>::dumpValue(std::ostream& o, size_t index) const
{
    return o << print_cast(this->view().at(index));
}

template<typename T>
void PVValueArray<T>::setCapacity(size_t capacity)
{
    if(this->isCapacityMutable()) {
        this->checkLength(capacity);
        value.reserve(capacity);
    }
    else
        THROW_EXCEPTION2(std::logic_error, "capacity immutable");
}

template<typename T>
void PVValueArray<T>::setLength(size_t length)
{
    if(this->isImmutable())
        THROW_EXCEPTION2(std::logic_error, "immutable");

    if (length == value.size())
        return;

    this->checkLength(length);

    if (length < value.size())
        value.slice(0, length);
    else
        value.resize(length);
}

template<typename T>
void PVValueArray<T>::replace(const const_svector& next)
{
    this->checkLength(next.size());

    value = next;
    this->postPut();
}

template<typename T>
void PVValueArray<T>::swap(const_svector &other)
{
    if (this->isImmutable())
        THROW_EXCEPTION2(std::logic_error, "immutable");

    // no checkLength call here

    value.swap(other);
}


template<typename T>
void PVValueArray<T>::serialize(ByteBuffer *pbuffer,
            SerializableControl *pflusher) const {
    serialize(pbuffer, pflusher, 0, this->getLength());
}

template<typename T>
void PVValueArray<T>::deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pcontrol) {

    size_t size = this->getArray()->getArraySizeType() == Array::fixed ?
                this->getArray()->getMaximumCapacity() :
                SerializeHelper::readSize(pbuffer, pcontrol);

    svector nextvalue(thaw(value));
    nextvalue.resize(size); // TODO: avoid copy of stuff we will then overwrite

    T* cur = nextvalue.data();

    // try to avoid deserializing from the buffer
    // this is only possible if we do not need to do endian-swapping
    if (!pbuffer->reverse<T>())
        if (pcontrol->directDeserialize(pbuffer, (char*)cur, size, sizeof(T)))
        {
        // inform about the change?
        PVField::postPut();
        return;
    }

    // retrieve value from the buffer
    size_t remaining = size;
    while(remaining) {
        const size_t have_bytes = pbuffer->getRemaining();

        // correctly rounds down in an element is partially received
        const size_t available = have_bytes/sizeof(T);

        if(available == 0) {
            // get at least one element
            pcontrol->ensureData(sizeof(T));
            continue;
        }

        const size_t n2read = std::min(remaining, available);

        pbuffer->getArray(cur, n2read);
        cur += n2read;
        remaining -= n2read;
    }
    value = freeze(nextvalue);
    // TODO !!!
    // inform about the change?
    PVField::postPut();
}

template<typename T>
void PVValueArray<T>::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, size_t offset, size_t count) const
{
    //TODO: avoid incrementing the ref counter...
    const_svector temp(value);
    temp.slice(offset, count);
    count = temp.size();

    ArrayConstPtr array = this->getArray();
    if (array->getArraySizeType() != Array::fixed)
        SerializeHelper::writeSize(count, pbuffer, pflusher);
    else if (count != array->getMaximumCapacity())
        throw std::length_error("fixed array cannot be partially serialized");

    const T* cur = temp.data();

    // try to avoid copying into the buffer
    // this is only possible if we do not need to do endian-swapping
    if (!pbuffer->reverse<T>())
        if (pflusher->directSerialize(pbuffer, (const char*)cur, count, sizeof(T)))
            return;

    while(count) {
        const size_t empty = pbuffer->getRemaining();
        const size_t space_for = empty/sizeof(T);

        if(space_for==0) {
            pflusher->flushSerializeBuffer();
            // Can we be certain that more space is now free???
            // If not then we spinnnnnnnnn
            continue;
        }

        const size_t n2send = std::min(count, space_for);

        pbuffer->putArray(cur, n2send);
        cur += n2send;
        count -= n2send;
    }
}

// specializations for string

template<>
void PVValueArray<string>::deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pcontrol) {

    size_t size = this->getArray()->getArraySizeType() == Array::fixed ?
                this->getArray()->getMaximumCapacity() :
                SerializeHelper::readSize(pbuffer, pcontrol);

    svector nextvalue(thaw(value));

    // Decide if we must re-allocate
    if(size > nextvalue.size() || !nextvalue.unique())
        nextvalue.resize(size);
    else if(size < nextvalue.size())
        nextvalue.slice(0, size);


    string * pvalue = nextvalue.data();
    for(size_t i = 0; i<size; i++) {
        pvalue[i] = SerializeHelper::deserializeString(pbuffer,
                                                       pcontrol);
    }
    value = freeze(nextvalue);
    // inform about the change?
    postPut();
}

template<>
void PVValueArray<string>::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, size_t offset, size_t count) const {

    const_svector temp(value);
    temp.slice(offset, count);

    // TODO if fixed count == getArray()->getMaximumCapacity()
    if (this->getArray()->getArraySizeType() != Array::fixed)
        SerializeHelper::writeSize(temp.size(), pbuffer, pflusher);

    const string * pvalue = temp.data();
    for(size_t i = 0; i<temp.size(); i++) {
        SerializeHelper::serializeString(pvalue[i], pbuffer, pflusher);
    }
}

template<typename T>
void PVValueArray<T>::_getAsVoid(epics::pvData::shared_vector<const void>& out) const
{
    out = static_shared_vector_cast<const void>(this->view());
}

template<typename T>
void PVValueArray<T>::_putFromVoid(const epics::pvData::shared_vector<const void>& in)
{
    this->replace(shared_vector_convert<const T>(in));
}

// Factory

PVDataCreate::PVDataCreate()
: fieldCreate(getFieldCreate())
{ }

PVFieldPtr PVDataCreate::createPVField(FieldConstPtr const & field)
{
     switch(field->getType()) {
     case scalar: {
         ScalarConstPtr xx = static_pointer_cast<const Scalar>(field);
         return createPVScalar(xx);
     }
     case scalarArray: {
         ScalarArrayConstPtr xx = static_pointer_cast<const ScalarArray>(field);
         return createPVScalarArray(xx);
     }
     case structure: {
         StructureConstPtr xx = static_pointer_cast<const Structure>(field);
         return createPVStructure(xx);
     }
     case structureArray: {
         StructureArrayConstPtr xx = static_pointer_cast<const StructureArray>(field);
         return createPVStructureArray(xx);
     }
     case union_: {
         UnionConstPtr xx = static_pointer_cast<const Union>(field);
         return createPVUnion(xx);
     }
     case unionArray: {
         UnionArrayConstPtr xx = static_pointer_cast<const UnionArray>(field);
         return createPVUnionArray(xx);
     }
     }
     throw std::logic_error("PVDataCreate::createPVField should never get here");
}

PVFieldPtr PVDataCreate::createPVField(PVFieldPtr const & fieldToClone)
{
     switch(fieldToClone->getField()->getType()) {
     case scalar:
        {
            PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(fieldToClone);
            return createPVScalar(pvScalar);
        }
     case scalarArray:
        {
             PVScalarArrayPtr pvScalarArray
                 = static_pointer_cast<PVScalarArray>(fieldToClone);
             return createPVScalarArray(pvScalarArray);
        }
     case structure:
         {
             PVStructurePtr pvStructure
                   = static_pointer_cast<PVStructure>(fieldToClone);
             StringArray const & fieldNames = pvStructure->getStructure()->getFieldNames();
             PVFieldPtrArray const & pvFieldPtrArray = pvStructure->getPVFields();
             return createPVStructure(fieldNames,pvFieldPtrArray);
         }
     case structureArray:
         {
             PVStructureArrayPtr from
                 = static_pointer_cast<PVStructureArray>(fieldToClone);
             StructureArrayConstPtr structureArray = from->getStructureArray();
             PVStructureArrayPtr to = createPVStructureArray(
                 structureArray);
             to->copyUnchecked(*from);
             return to;
         }
     case union_:
         {
             PVUnionPtr pvUnion
                   = static_pointer_cast<PVUnion>(fieldToClone);
             return createPVUnion(pvUnion);
         }
     case unionArray:
         {
             PVUnionArrayPtr from
                 = static_pointer_cast<PVUnionArray>(fieldToClone);
             UnionArrayConstPtr unionArray = from->getUnionArray();
             PVUnionArrayPtr to = createPVUnionArray(unionArray);
             to->copyUnchecked(*from);
             return to;
         }
     }
     throw std::logic_error("PVDataCreate::createPVField should never get here");
}

PVScalarPtr PVDataCreate::createPVScalar(ScalarConstPtr const & scalar)
{
     ScalarType scalarType = scalar->getScalarType();
     switch(scalarType) {
     case pvBoolean:
         return PVScalarPtr(new PVBoolean(scalar));
     case pvByte:
         return PVScalarPtr(new PVByte(scalar));
     case pvShort:
         return PVScalarPtr(new PVShort(scalar));
     case pvInt:
         return PVScalarPtr(new PVInt(scalar));
     case pvLong:
         return PVScalarPtr(new PVLong(scalar));
     case pvUByte:
         return PVScalarPtr(new PVUByte(scalar));
     case pvUShort:
         return PVScalarPtr(new PVUShort(scalar));
     case pvUInt:
         return PVScalarPtr(new PVUInt(scalar));
     case pvULong:
         return PVScalarPtr(new PVULong(scalar));
     case pvFloat:
         return PVScalarPtr(new PVFloat(scalar));
     case pvDouble:
         return PVScalarPtr(new PVDouble(scalar));
     case pvString:
         return PVScalarPtr(new PVString(scalar));
     }
     throw std::logic_error("PVDataCreate::createPVScalar should never get here");
}

PVScalarPtr PVDataCreate::createPVScalar(ScalarType scalarType)
{
     ScalarConstPtr scalar = fieldCreate->createScalar(scalarType);
     return createPVScalar(scalar);
}


PVScalarPtr PVDataCreate::createPVScalar(PVScalarPtr const & scalarToClone)
{
     ScalarType scalarType = scalarToClone->getScalar()->getScalarType();
     PVScalarPtr pvScalar = createPVScalar(scalarType);
     pvScalar->copyUnchecked(*scalarToClone);
     return pvScalar;
}

PVScalarArrayPtr PVDataCreate::createPVScalarArray(
        ScalarArrayConstPtr const & scalarArray)
{
     switch(scalarArray->getElementType()) {
     case pvBoolean:
           return PVScalarArrayPtr(new PVBooleanArray(scalarArray));
     case pvByte:
           return PVScalarArrayPtr(new PVByteArray(scalarArray));
     case pvShort:
           return PVScalarArrayPtr(new PVShortArray(scalarArray));
     case pvInt:
           return PVScalarArrayPtr(new PVIntArray(scalarArray));
     case pvLong:
           return PVScalarArrayPtr(new PVLongArray(scalarArray));
     case pvUByte:
           return PVScalarArrayPtr(new PVUByteArray(scalarArray));
     case pvUShort:
           return PVScalarArrayPtr(new PVUShortArray(scalarArray));
     case pvUInt:
           return PVScalarArrayPtr(new PVUIntArray(scalarArray));
     case pvULong:
           return PVScalarArrayPtr(new PVULongArray(scalarArray));
     case pvFloat:
           return PVScalarArrayPtr(new PVFloatArray(scalarArray));
     case pvDouble:
           return PVScalarArrayPtr(new PVDoubleArray(scalarArray));
     case pvString:
           return PVScalarArrayPtr(new PVStringArray(scalarArray));
     }
     throw std::logic_error("PVDataCreate::createPVScalarArray should never get here");

}

PVScalarArrayPtr PVDataCreate::createPVScalarArray(
        ScalarType elementType)
{
     ScalarArrayConstPtr scalarArray = fieldCreate->createScalarArray(elementType);
     return createPVScalarArray(scalarArray);
}

PVScalarArrayPtr PVDataCreate::createPVScalarArray(
        PVScalarArrayPtr const & arrayToClone)
{
     PVScalarArrayPtr pvArray = createPVScalarArray(
          arrayToClone->getScalarArray()->getElementType());
     pvArray->assign(*arrayToClone.get());
    return pvArray;
}

PVStructureArrayPtr PVDataCreate::createPVStructureArray(
        StructureArrayConstPtr const & structureArray)
{
     return PVStructureArrayPtr(new PVStructureArray(structureArray));
}

PVStructurePtr PVDataCreate::createPVStructure(
        StructureConstPtr const & structure)
{
     return PVStructurePtr(new PVStructure(structure));
}

PVUnionArrayPtr PVDataCreate::createPVUnionArray(
        UnionArrayConstPtr const & unionArray)
{
     return PVUnionArrayPtr(new PVUnionArray(unionArray));
}

PVUnionPtr PVDataCreate::createPVUnion(
        UnionConstPtr const & punion)
{
     return PVUnionPtr(new PVUnion(punion));
}

PVUnionPtr PVDataCreate::createPVVariantUnion()
{
     return PVUnionPtr(new PVUnion(fieldCreate->createVariantUnion()));
}

PVUnionArrayPtr PVDataCreate::createPVVariantUnionArray()
{
     return PVUnionArrayPtr(new PVUnionArray(fieldCreate->createVariantUnionArray()));
}

PVStructurePtr PVDataCreate::createPVStructure(
        StringArray const & fieldNames,PVFieldPtrArray const & pvFields)
{
     size_t num = fieldNames.size();
     FieldConstPtrArray fields(num);
     for (size_t i=0;i<num;i++) fields[i] = pvFields[i]->getField();
     StructureConstPtr structure = fieldCreate->createStructure(fieldNames,fields);
     PVStructurePtr pvStructure(new PVStructure(structure,pvFields));
     return pvStructure;
}

PVStructurePtr PVDataCreate::createPVStructure(PVStructurePtr const & structToClone)
{
    FieldConstPtrArray field;
    if(!structToClone) {
        // is this correct?!
        FieldConstPtrArray fields(0);
        StringArray fieldNames(0);
        StructureConstPtr structure = fieldCreate->createStructure(fieldNames,fields);
        return PVStructurePtr(new PVStructure(structure));
    }
    StructureConstPtr structure = structToClone->getStructure();
    PVStructurePtr pvStructure(new PVStructure(structure));
    pvStructure->copyUnchecked(*structToClone);
    return pvStructure;
}

PVUnionPtr PVDataCreate::createPVUnion(PVUnionPtr const & unionToClone)
{
    PVUnionPtr punion(new PVUnion(unionToClone->getUnion()));
    // set cloned value
    punion->set(unionToClone->getSelectedIndex(), createPVField(unionToClone->get()));
    return punion;
}

namespace detail {
struct pvfield_factory {
    PVDataCreatePtr pvDataCreate;
    pvfield_factory() :pvDataCreate(new PVDataCreate()) {
        registerRefCounter("PVField", &PVField::num_instances);
    }
};
}

static detail::pvfield_factory* pvfield_factory_s;
static epicsThreadOnceId pvfield_factory_once = EPICS_THREAD_ONCE_INIT;

static void pvfield_factory_init(void*)
{
    try {
        pvfield_factory_s = new detail::pvfield_factory;
    }catch(std::exception& e){
        std::cerr<<"Error initializing getFieldCreate() : "<<e.what()<<"\n";
    }
}

const PVDataCreatePtr& PVDataCreate::getPVDataCreate()
{
    epicsThreadOnce(&pvfield_factory_once, &pvfield_factory_init, 0);
    if(!pvfield_factory_s->pvDataCreate)
        throw std::logic_error("getPVDataCreate() not initialized");
    return pvfield_factory_s->pvDataCreate;
}

// explicitly instanciate to ensure that windows
// builds emit exported symbols for inline'd methods
template class PVScalarValue<boolean>;
template class PVScalarValue<int8>;
template class PVScalarValue<uint8>;
template class PVScalarValue<int16>;
template class PVScalarValue<uint16>;
template class PVScalarValue<int32>;
template class PVScalarValue<uint32>;
template class PVScalarValue<int64>;
template class PVScalarValue<uint64>;
template class PVScalarValue<float>;
template class PVScalarValue<double>;
template class PVScalarValue<std::string>;
template class PVValueArray<boolean>;
template class PVValueArray<int8>;
template class PVValueArray<uint8>;
template class PVValueArray<int16>;
template class PVValueArray<uint16>;
template class PVValueArray<int32>;
template class PVValueArray<uint32>;
template class PVValueArray<int64>;
template class PVValueArray<uint64>;
template class PVValueArray<float>;
template class PVValueArray<double>;
template class PVValueArray<std::string>;

}} // namespace epics::pvData

namespace std{
    std::ostream& operator<<(std::ostream& o, const epics::pvData::PVField *ptr)
    {
        if(ptr) return o << *ptr;
        return o << "nullptr";
    }
}
