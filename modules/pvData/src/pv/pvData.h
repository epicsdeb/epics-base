/* pvData.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef PVDATA_H
#define PVDATA_H

#include <string>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <iomanip>

#include <epicsAssert.h>

#include <pv/pvIntrospect.h>
#include <pv/typeCast.h>
#include <pv/anyscalar.h>
#include <pv/sharedVector.h>

#include <shareLib.h>
#include <compilerDependencies.h>

#if defined(vxWorks) && !defined(_WRS_VXWORKS_MAJOR)
typedef class std::ios std::ios_base;
#endif

namespace epics { namespace pvData { 

/** @defgroup pvcontainer Value containers
 *
 * The core of the pvDataCPP library are the typed, structured, data containers.
 */

class PostHandler;

class PVField;
class PVScalar;

class PVScalarArray;

class PVStructure;

class PVUnion;



template<typename T> class PVScalarValue;
template<typename T> class PVValueArray;


/**
 * typedef for a pointer to a PostHandler.
 */
typedef std::tr1::shared_ptr<PostHandler> PostHandlerPtr;

/**
 * typedef for a pointer to a PVField.
 */
typedef std::tr1::shared_ptr<PVField> PVFieldPtr;
/**
 * typedef for a pointer to a array of pointer to PVField.
 */
typedef std::vector<PVFieldPtr> PVFieldPtrArray;
typedef std::vector<PVFieldPtr>::iterator PVFieldPtrArray_iterator;
typedef std::vector<PVFieldPtr>::const_iterator PVFieldPtrArray_const__iterator;

/**
 * typedef for a pointer to a PVScalar.
 */
typedef std::tr1::shared_ptr<PVScalar> PVScalarPtr;

/**
 * typedef for a pointer to a PVScalarArray.
 */
typedef std::tr1::shared_ptr<PVScalarArray> PVScalarArrayPtr;

/**
 * typedef for a pointer to a PVStructure.
 */
typedef std::tr1::shared_ptr<PVStructure> PVStructurePtr;
/**
 * typedef for a pointer to a array of pointer to PVStructure.
 */
typedef std::vector<PVStructurePtr> PVStructurePtrArray;
typedef std::vector<PVStructurePtr>::iterator PVStructurePtrArray_iterator;
typedef std::vector<PVStructurePtr>::const_iterator PVStructurePtrArray_const__iterator;

/**
 * typedef for a pointer to a PVStructureArray.
 */

typedef PVValueArray<PVStructurePtr> PVStructureArray;
typedef std::tr1::shared_ptr<PVStructureArray> PVStructureArrayPtr;
typedef std::vector<PVStructureArrayPtr> PVStructureArrayPtrArray;
typedef std::tr1::shared_ptr<PVStructureArrayPtrArray> PVStructureArrayPtrArrayPtr;

/**
 * typedef for a pointer to a PVUnion.
 */
typedef std::tr1::shared_ptr<PVUnion> PVUnionPtr;
/**
 * typedef for a pointer to a array of pointer to PVUnion.
 */
typedef std::vector<PVUnionPtr> PVUnionPtrArray;
typedef std::vector<PVUnionPtr>::iterator PVUnionPtrArray_iterator;
typedef std::vector<PVUnionPtr>::const_iterator PVUnionPtrArray_const__iterator;

/**
 * typedef for a pointer to a PVUnionArray.
 */

typedef PVValueArray<PVUnionPtr> PVUnionArray;
typedef std::tr1::shared_ptr<PVUnionArray> PVUnionArrayPtr;
typedef std::vector<PVUnionArrayPtr> PVUnionArrayPtrArray;
typedef std::tr1::shared_ptr<PVUnionArrayPtrArray> PVUnionArrayPtrArrayPtr;

class PVDataCreate;
typedef std::tr1::shared_ptr<PVDataCreate> PVDataCreatePtr;

/**
 * @brief This class is implemented by code that calls setPostHander
 *
 */
class epicsShareClass PostHandler 
{
public:
    POINTER_DEFINITIONS(PostHandler);
    /**
     * Destructor
     */
    virtual ~PostHandler(){}
    /**
     * This is called every time postPut is called for this field.
     */
    virtual void postPut() = 0;
};

/**
 * @brief PVField is the base class for each PVData field.
 *
 * Each PVData field has an interface that extends PVField.
 *
 * @ingroup pvcontainer
 */
class epicsShareClass PVField
: virtual public Serializable,
  public std::tr1::enable_shared_from_this<PVField>
{
public:
    POINTER_DEFINITIONS(PVField);
    /**
     * Destructor
     */
    virtual ~PVField();
    /**
     * Get the fieldName for this field.
     * @return The name or empty string if top-level field.
     */
    inline const std::string& getFieldName() const {return fieldName;}
    /**
     * Fully expand the name of this field using the
     * names of its parent fields with a dot '.' separating
     * each name.
     */
    std::string getFullName() const;
    /**
     * Get offset of the PVField field within top-level structure.
     * Every field within the PVStructure has a unique offset.
     * The top-level structure has an offset of 0.
     * The first field within the structure has offset equal to 1.
     * The other offsets are determined by recursively traversing each structure of the tree.
     * @return The offset.
     */
    std::size_t getFieldOffset() const;
    /**
     * Get the next offset. If the field is a scalar or array field then this is just offset + 1.
     * If the field is a structure it is the offset of the next field after this structure.
     * Thus (nextOffset - offset) is always equal to the number of fields within the field.
     * @return The offset.
     */
    std::size_t getNextFieldOffset() const;
    /**
     * Get the total number of fields in this field.
     * This is equal to nextFieldOffset - fieldOffset.
     */
    std::size_t getNumberFields() const;
    /**
     * Is the field immutable, i.e. does it not allow changes.
     * @return (false,true) if it (is not, is) immutable.
     */
    inline bool isImmutable() const {return immutable;}
    /**
     * Set the field to be immutable, i.e. it can no longer be modified.
     * This is permanent, i.e. once done the field cannot be made mutable.
     */
    virtual void setImmutable();
    /**
     * Get the <i>Field</i> that describes the field.
     * @return Field, which is the reflection interface.
     */
    inline const FieldConstPtr & getField() const {return field;}
    /**
     * Get the parent of this field.
     * @return The parent interface or null if this is PVRecord
     */
    inline PVStructure * getParent() {return parent;}
    inline const PVStructure * getParent() const {return parent;}
    /**
     * postPut. Called when the field is updated by the implementation.
     */
    void postPut() ;
    /**
     * Set the handler for postPut.
     * At most one handler can be set.
     * @param postHandler The handler.
     */
    void setPostHandler(PostHandlerPtr const &postHandler);
    /**
     * Is this field equal to another field.
     * @param pv other field
     * @return (false,true) if (is not,is) equal.
     */
    virtual bool equals(PVField &pv);
    /**
     * Puts the PVField raw value to the stream.
     * @param o output stream.
     * @return The output stream.
     */
    virtual std::ostream& dumpValue(std::ostream& o) const = 0;

    void copy(const PVField& from);
    void copyUnchecked(const PVField& from);

    static size_t num_instances; // use atomic::get() or volatile* access
    enum {isPVField=1};
protected:
    PVField::shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    explicit PVField(FieldConstPtr field);
    void setParentAndName(PVStructure *parent, std::string const & fieldName);
private:
    static void computeOffset(const PVField *pvField);
    static void computeOffset(const PVField *pvField,std::size_t offset);
    std::string fieldName;
    PVStructure *parent;
    const FieldConstPtr field;
    size_t fieldOffset;
    size_t nextFieldOffset;
    bool immutable;
    PostHandlerPtr postHandler;
    friend class PVDataCreate;
    friend class PVStructure;
    EPICS_NOT_COPYABLE(PVField)
};

epicsShareExtern std::ostream& operator<<(std::ostream& o, const PVField& f);

/**
 * @brief PVScalar is the base class for each scalar field.
 *
 * @ingroup pvcontainer
 */
class epicsShareClass PVScalar : public PVField {
    // friend our child class(s) so that it
    // can call protected methods of other
    // PVScalar instances.
    template<typename E> friend class PVScalarValue;
public:
    POINTER_DEFINITIONS(PVScalar);
    /**
     * Destructor
     */
    virtual ~PVScalar();
    typedef PVScalar &reference;
    typedef const PVScalar& const_reference;
    /**
     * Get the Scalar introspection interface for the PVScalar.
     * @return the interface.
     */
    const ScalarConstPtr getScalar() const ;

    /**
     * Convert and return the scalar value in the requested type.
     * Result type is determined from the function template argument
     * which must be one of the ScalarType enums.
     * Uses castUnsafe<TO>() for value conversion.
     @code
      PVScalar* pv = ...;
      uint32 val = pv->getAs<pvInt>();
     @endcode
     */
    template<typename T>
    inline T getAs() const {
        T result;
        this->getAs((void*)&result, (ScalarType)ScalarTypeID<T>::value);
        return result;
    }
protected:
    virtual void getAs(void *, ScalarType) const = 0;
public:

    virtual void getAs(AnyScalar& v) const =0;

    /**
     * Convert and assign the provided value.
     * The value type is determined from the function template argument
     * which must be one of the ScalarType enums.
     * Uses castUnsafe<TO>() for value conversion.
     @code
      PVScalar* pv = ...;
      pv->putFrom<pvInt>((int32)42);
     @endcode
     */
    template<typename T>
    inline void putFrom(T val) {
        this->putFrom((const void*)&val, (ScalarType)ScalarTypeID<T>::value);
    }

    //! Convert and assign
    virtual void putFrom(const void *, ScalarType) = 0;

    inline void putFrom(const AnyScalar& v) {
        if(v)
            putFrom(v.unsafe(), v.type());
    }

    virtual void assign(const PVScalar&) = 0;

    virtual void copy(const PVScalar& from) = 0;
    virtual void copyUnchecked(const PVScalar& from) = 0;

protected:
    explicit PVScalar(ScalarConstPtr const & scalar);
    EPICS_NOT_COPYABLE(PVScalar)
};

namespace detail {
template<typename T>
struct ScalarStorageOps {
    T value;
    typedef T arg_type;
    inline void store(T v) {
        value = v;
    }
    ScalarStorageOps() :value(0) {}
};
template<>
struct ScalarStorageOps<std::string> {
    std::string value;
    size_t maxLength;
    typedef const std::string& arg_type;
    void store(const std::string& val) {
        if (maxLength > 0 && val.length() > maxLength)
            throw std::overflow_error("string too long");

        value = val;
    }

    ScalarStorageOps(): value(), maxLength(0) {} // initialized in PVString::PVString
};
} // namespace detail

/**
 * @brief Class that holds the data for each possible scalar type.
 *
 * @ingroup pvcontainer
 */
template<typename T>
class epicsShareClass PVScalarValue : public PVScalar {
    typedef detail::ScalarStorageOps<T> storage_t;
public:
    POINTER_DEFINITIONS(PVScalarValue);
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;

    static const ScalarType typeCode;

    /**
     * Destructor
     */
    virtual ~PVScalarValue();
    /**
     * Get the value.
     * @return The value.
     */
    typename storage_t::arg_type get() const { return storage.value; }
    /**
     * Put a new value into the PVScalar.
     * @param value The value.
     */
    inline void put(typename storage_t::arg_type v) {
        storage.store(v);
        PVField::postPut();
    }

    virtual std::ostream& dumpValue(std::ostream& o) const OVERRIDE;

    // get operator
    // double value; doubleField >>= value;
    // NOTE: virtual is needed for MS C++ compiler to get this operator exported
    virtual void operator>>=(T& value) const;

    // put operator
    // double value = 12.8; doubleField <<= value;
    // NOTE: virtual is needed for MS C++ compiler to get this operator exported
    virtual void operator<<=(typename storage_t::arg_type value);

    template<typename T1>
    inline T1 getAs() const {
        T1 result(castUnsafe<T1,T>(get()));
        return result;
    }

    template<typename T1>
    inline void putFrom(typename detail::ScalarStorageOps<T1>::arg_type val) {
        put(castUnsafe<T,T1>(val));
    }

    inline void putFrom(const AnyScalar& v) {
        // the template form of putFrom() hides the base class AnyScalar overload
        PVScalar::putFrom(v);
    }

    virtual void assign(const PVScalar& scalar) OVERRIDE FINAL;
    virtual void copy(const PVScalar& from) OVERRIDE FINAL;
    virtual void copyUnchecked(const PVScalar& from) OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher) const OVERRIDE;
    virtual void deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pflusher) OVERRIDE FINAL;

protected:
    explicit PVScalarValue(ScalarConstPtr const & scalar)
    : PVScalar(scalar), storage() {}
    virtual void getAs(void * result, ScalarType rtype) const OVERRIDE FINAL
    {
        const T src = get();
        castUnsafeV(1, rtype, result, typeCode, (const void*)&src);
    }
public:
    virtual void getAs(AnyScalar& v) const OVERRIDE FINAL
    {
        v = get();
    }
    virtual void putFrom(const void *src, ScalarType stype) OVERRIDE FINAL
    {
        T result;
        castUnsafeV(1, typeCode, (void*)&result, stype, src);
        put(result);
    }
protected:

    friend class PVDataCreate;
    storage_t storage;
    EPICS_NOT_COPYABLE(PVScalarValue)
};

/**
 * @brief Some explicit specializations exist
 */
template<> inline
std::ostream& PVScalarValue<int8>::dumpValue(std::ostream& o) const
{
    return o << static_cast<int>(get());
}

template<> inline
std::ostream& PVScalarValue<uint8>::dumpValue(std::ostream& o) const
{
    return o << static_cast<unsigned int>(get());
}

template<> inline
std::ostream& PVScalarValue<boolean>::dumpValue(std::ostream& o) const
{
    return o << std::boolalpha << static_cast<bool>(get());
}

/*
 * typedefs for the various possible scalar types.
 */
typedef PVScalarValue<boolean> PVBoolean;
typedef PVScalarValue<int8> PVByte;
typedef PVScalarValue<int16> PVShort;
typedef PVScalarValue<int32> PVInt;
typedef PVScalarValue<int64> PVLong;
typedef PVScalarValue<uint8> PVUByte;
typedef PVScalarValue<uint16> PVUShort;
typedef PVScalarValue<uint32> PVUInt;
typedef PVScalarValue<uint64> PVULong;
typedef PVScalarValue<float> PVFloat;
typedef PVScalarValue<double> PVDouble;
typedef std::tr1::shared_ptr<PVBoolean> PVBooleanPtr;
typedef std::tr1::shared_ptr<PVByte> PVBytePtr;
typedef std::tr1::shared_ptr<PVShort> PVShortPtr;
typedef std::tr1::shared_ptr<PVInt> PVIntPtr;
typedef std::tr1::shared_ptr<PVLong> PVLongPtr;
typedef std::tr1::shared_ptr<PVUByte> PVUBytePtr;
typedef std::tr1::shared_ptr<PVUShort> PVUShortPtr;
typedef std::tr1::shared_ptr<PVUInt> PVUIntPtr;
typedef std::tr1::shared_ptr<PVULong> PVULongPtr;
typedef std::tr1::shared_ptr<PVFloat> PVFloatPtr;
typedef std::tr1::shared_ptr<PVDouble> PVDoublePtr;

/**
 * @brief PVString is special case, since it implements SerializableArray
 *
 * @ingroup pvcontainer
 */
class epicsShareClass PVString : public PVScalarValue<std::string>, SerializableArray {
public:
    /**
     * Destructor
     */
    virtual ~PVString() {}

    virtual std::ostream& dumpValue(std::ostream& o) const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher) const OVERRIDE FINAL;
    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, size_t offset, size_t count) const OVERRIDE FINAL;
protected:
    explicit PVString(ScalarConstPtr const & scalar);

    friend class PVDataCreate;
    EPICS_NOT_COPYABLE(PVString)
};
typedef std::tr1::shared_ptr<PVString> PVStringPtr;


/**
 * @brief PVArray is the base class for all array types.
 *
 * The array types are unionArray, strucrtureArray and scalarArray.
 * There is a scalarArray type for each scalarType.
 *
 * @ingroup pvcontainer
 */
class epicsShareClass PVArray : public PVField, public SerializableArray {
public:
    POINTER_DEFINITIONS(PVArray);
    /**
     * Destructor
     */
    virtual ~PVArray(){};
    /**
     * Get the introspection interface
     * @return The interface.
     */
    virtual ArrayConstPtr getArray() const = 0;
    /**
     * Set the field to be immutable, i.e. it can no longer be modified.
     * This is permanent, i.e. once done the field cannot be made mutable.
     */
    virtual void setImmutable() OVERRIDE;
    /**
     * Get the array length.
     * @return The length.
     */
    virtual std::size_t getLength() const = 0;
    /**
     * Set the array length.
     * @param length The length.
     */
    virtual void setLength(std::size_t length) = 0;
    /**
     * Get the array capacity.
     * @return The capacity.
     */
    virtual std::size_t getCapacity() const = 0;
    /**
     * Can the capacity be changed.
     * @return (false,true) if (can not, can) be changed.
     */
    bool isCapacityMutable() const;
    /**
     * Set the mutability of the array capacity.
     * @return false or true
     */
    void setCapacityMutable(bool isMutable);
    /**
     * Set the array capacity.
     * @param capacity The capacity.
     */
    virtual void setCapacity(std::size_t capacity) = 0;

    using PVField::dumpValue;
    virtual std::ostream& dumpValue(std::ostream& o, std::size_t index) const = 0;

protected:
    explicit PVArray(FieldConstPtr const & field);
    void checkLength(size_t length) const;
private:
    bool capacityMutable;
    friend class PVDataCreate;
    EPICS_NOT_COPYABLE(PVArray)
};

epicsShareExtern std::ostream& operator<<(format::array_at_internal const& manip, const PVArray& array);

/**
 * @brief Base class for a scalarArray.
 *
 * @ingroup pvcontainer
 */
class epicsShareClass PVScalarArray : public PVArray {
public:
    POINTER_DEFINITIONS(PVScalarArray);
    /**
     * Destructor
     */
    virtual ~PVScalarArray();
    typedef PVScalarArray &reference;
    typedef const PVScalarArray& const_reference;
    /**
     * Get the introspection interface
     * @return The interface.
     */
    const ScalarArrayConstPtr getScalarArray() const ;

protected:
    virtual void _getAsVoid(shared_vector<const void>&) const = 0;
    virtual void _putFromVoid(const shared_vector<const void>&) = 0;
public:

    /**
     * Fetch the current value and convert to the requested type.
     *
     * A copy is made if the requested type does not match
     * the element type.  If the types do match then
     * no copy is made.
     */
    template<typename T>
    void
    getAs(shared_vector<const T>& out) const
    {
        shared_vector<const void> temp;
        _getAsVoid(temp);
        out = shared_vector_convert<const T>(temp);
    }

    /**
     * Assign the given value after conversion.
     *
     * A copy and element-wise conversion is performed unless
     * the element type of the PVScalarArray matches the
     * type of the provided data.
     * If the types do match then a new reference to the provided
     * data is kept.
     *
     * Calls postPut()
     */
    template<typename T>
    inline void putFrom(const shared_vector<const T>& inp)
    {
        shared_vector<const void> temp(static_shared_vector_cast<const void>(inp));
        _putFromVoid(temp);
    }

    /**
     * Assign the given PVScalarArray's value.
     *
     * A copy and element-wise conversion is performed unless
     * the element type of the PVScalarArray matches the
     * type of the provided data.
     * If the types do match then a new reference to the provided
     * data is kept.
     */
    void assign(const PVScalarArray& pv) {
        if (isImmutable())
            throw std::invalid_argument("destination is immutable");
        copyUnchecked(pv);
    }

    void copy(const PVScalarArray& from) {
        assign(from);
    }

    void copyUnchecked(const PVScalarArray& from) {
        if (this==&from)
            return;
        shared_vector<const void> temp;
        from._getAsVoid(temp);
        _putFromVoid(temp);
    }

protected:
    explicit PVScalarArray(ScalarArrayConstPtr const & scalarArray);
private:
    friend class PVDataCreate;
    EPICS_NOT_COPYABLE(PVScalarArray)
};


/**
 * @brief Data interface for a structure,
 *
 * @ingroup pvcontainer
 */
class epicsShareClass PVStructure : public PVField, public BitSetSerializable
{
public:
    POINTER_DEFINITIONS(PVStructure);
    /**
     * Destructor
     */
    virtual ~PVStructure();
    typedef PVStructure & reference;
    typedef const PVStructure & const_reference;
    /**
     * Set the field to be immutable, i.e. it can no longer be modified.
     * This is permanent, i.e. once done the field cannot be made mutable.
     */
    virtual void setImmutable() OVERRIDE FINAL;
    /**
     * Get the introspection interface
     * @return The interface.
     */
    inline const StructureConstPtr &getStructure() const { return structurePtr; }
    /**
     * Get the array of pointers to the subfields in the structure.
     * @return The array.
     */
    inline const PVFieldPtrArray & getPVFields() const { return pvFields; }

    /**
     * Get the subfield with the specified offset.
     * @param a A sub-field name or index
     * @return Pointer to the field or NULL if field does not exist.
     */
    template<typename A>
    FORCE_INLINE std::tr1::shared_ptr<PVField> getSubField(A a)
    {
        return getSubFieldImpl(a, false);
    }

    template<typename A>
    FORCE_INLINE std::tr1::shared_ptr<const PVField> getSubField(A a) const
    {
        return getSubFieldImpl(a, false);
    }

    /**
     * Get a subfield with the specified name.
     * @param a A sub-field name or index
     * @returns A pointer to the sub-field or null if field does not exist or has a different type
     * @code
     *   PVIntPtr ptr = pvStruct->getSubField<PVInt>("substruct.leaffield");
     * @endcode
     *
     *  A field name is a '.' delimited list of child field names (no whitespace allowed)
     */
    template<typename PVD, typename A>
    inline std::tr1::shared_ptr<PVD> getSubField(A a)
    {
        STATIC_ASSERT(PVD::isPVField); // only allow cast from PVField sub-class
        return std::tr1::dynamic_pointer_cast<PVD>(getSubFieldImpl(a, false));
    }

    template<typename PVD, typename A>
    inline std::tr1::shared_ptr<const PVD> getSubField(A a) const
    {
        STATIC_ASSERT(PVD::isPVField); // only allow cast from PVField sub-class
        return std::tr1::dynamic_pointer_cast<const PVD>(getSubFieldImpl(a, false));
    }

    /**
     * Get the subfield with the specified offset.
     * @param a A sub-field name or index
     * @throws std::runtime_error if the requested sub-field doesn't exist, or has a different type
     * @return Pointer to the field
     */
    template<typename A>
    FORCE_INLINE std::tr1::shared_ptr<PVField> getSubFieldT(A a)
    {
        return getSubFieldImpl(a, true);
    }

    template<typename A>
    FORCE_INLINE std::tr1::shared_ptr<const PVField> getSubFieldT(A a) const
    {
        return getSubFieldImpl(a, true);
    }

private:
    static void throwBadFieldType(const char *name);
    static void throwBadFieldType(std::size_t fieldOffset);
    static FORCE_INLINE void throwBadFieldType(const std::string& name) {
        throwBadFieldType(name.c_str());
    }
public:

    template<typename PVD, typename A>
    inline std::tr1::shared_ptr<PVD> getSubFieldT(A a)
    {
        STATIC_ASSERT(PVD::isPVField); // only allow cast from PVField sub-class
        std::tr1::shared_ptr<PVD> ret(std::tr1::dynamic_pointer_cast<PVD>(getSubFieldImpl(a, true)));
        if(!ret)
            throwBadFieldType(a);
        return ret;
    }

    template<typename PVD, typename A>
    inline std::tr1::shared_ptr<const PVD> getSubFieldT(A a) const
    {
        STATIC_ASSERT(PVD::isPVField); // only allow cast from PVField sub-class
        std::tr1::shared_ptr<const PVD> ret(std::tr1::dynamic_pointer_cast<const PVD>(getSubFieldImpl(a, true)));
        if(!ret)
            throwBadFieldType(a);
        return ret;
    }

    /**
     * Serialize.
     * @param pbuffer The byte buffer.
     * @param pflusher Interface to call when buffer is full.
     */
    virtual void serialize(
        ByteBuffer *pbuffer,SerializableControl *pflusher) const OVERRIDE FINAL;
    /**
     * Deserialize
     * @param pbuffer The byte buffer.
     * @param pflusher Interface to call when buffer is empty.
     */
    virtual void deserialize(
        ByteBuffer *pbuffer,DeserializableControl *pflusher) OVERRIDE FINAL;
    /**
     * Serialize.
     * @param pbuffer The byte buffer.
     * @param pflusher Interface to call when buffer is full.
     * @param pbitSet A bitset the specifies which fields to serialize.
    */
    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher,BitSet *pbitSet) const OVERRIDE FINAL;
    /**
     * Deserialize
     * @param pbuffer The byte buffer.
     * @param pflusher Interface to call when buffer is empty.
     * @param pbitSet A bitset the specifies which fields to deserialize.
     */
    virtual void deserialize(ByteBuffer *pbuffer,
        DeserializableControl*pflusher,BitSet *pbitSet) OVERRIDE FINAL;
    /**
     * Constructor
     * @param structure The introspection interface.
     */
    explicit PVStructure(StructureConstPtr const & structure);
    /**
     * Constructor
     * @param structure The introspection interface.
     * @param pvFields The array of fields for the structure.
     */
    PVStructure(StructureConstPtr const & structure,PVFieldPtrArray const & pvFields);

    virtual std::ostream& dumpValue(std::ostream& o) const OVERRIDE FINAL;

    void copy(const PVStructure& from);

    void copyUnchecked(const PVStructure& from);
    void copyUnchecked(const PVStructure& from, const BitSet& maskBitSet, bool inverse = false);

    struct Formatter {
        enum mode_t {
            Auto,
            Plain,
            ANSI,
        };
        enum format_t {
            Raw,
            NT,
            JSON,
        };
    private:
        const PVStructure& xtop;
        const BitSet* xshow;
        const BitSet* xhighlight;
        mode_t xmode;
        format_t xfmt;
    public:
        explicit Formatter(const PVStructure& top)
            :xtop(top)
            ,xshow(0)
            ,xhighlight(0)
            ,xmode(Auto)
            ,xfmt(NT)
        {}

        // those fields (and their parents) to be printed.  non-NT mode.
        FORCE_INLINE Formatter& show(const BitSet& set) { xshow = &set; return *this; }
        // those fields (and not their parents) to be specially highlighted.  non-NT mode.
        FORCE_INLINE Formatter& highlight(const BitSet& set) { xhighlight = &set; return *this; }

        FORCE_INLINE Formatter& mode(mode_t m) { xmode = m; return *this; }

        FORCE_INLINE Formatter& format(format_t f) { xfmt = f; return *this; }

        friend epicsShareFunc std::ostream& operator<<(std::ostream& strm, const Formatter& format);
        friend void printRaw(std::ostream& strm, const PVStructure::Formatter& format, const PVStructure& cur);
    };

    FORCE_INLINE Formatter stream() const { return Formatter(*this); }

private:

    inline PVFieldPtr getSubFieldImpl(const std::string& name, bool throws) const {
        return getSubFieldImpl(name.c_str(), throws);
    }
    PVFieldPtr getSubFieldImpl(const char *name, bool throws) const;
    PVFieldPtr getSubFieldImpl(std::size_t fieldOffset, bool throws) const;

    PVFieldPtrArray pvFields;
    StructureConstPtr structurePtr;
    std::string extendsStructureName;
    friend class PVDataCreate;
    EPICS_NOT_COPYABLE(PVStructure)
};

epicsShareFunc
std::ostream& operator<<(std::ostream& strm, const PVStructure::Formatter& format);

/**
 * @brief PVUnion has a single subfield.
 *
 * The type for the subfield is specified by a union introspection interface.
 *
 * @ingroup pvcontainer
 */
class epicsShareClass PVUnion : public PVField
{
public:
    POINTER_DEFINITIONS(PVUnion);
    /**
     * Destructor
     */
    virtual ~PVUnion();
    typedef PVUnion & reference;
    typedef const PVUnion & const_reference;

        /**
         * Undefined index.
         * Default value upon PVUnion construction. Can be set by the user.
         * Corresponds to @c null value.
         */
    static const int32 UNDEFINED_INDEX;

    /**
     * Get the introspection interface
     * @return The interface.
     */
    inline const UnionConstPtr& getUnion() const { return unionPtr; }

    /**
     * Get the @c PVField value stored in the field.
     * @return @c PVField value of field, @c null if {@code getSelectedIndex() == UNDEFINED_INDEX}.
     */
    inline const PVFieldPtr& get() { return value; }
    inline PVField::const_shared_pointer get() const { return value; }
   
    template<typename PVT>
    inline std::tr1::shared_ptr<PVT> get() {
        STATIC_ASSERT(PVT::isPVField); // only allow cast from PVField sub-class
        return std::tr1::dynamic_pointer_cast<PVT>(get());
    }

    template<typename PVT>
    inline std::tr1::shared_ptr<const PVT> get() const {
        STATIC_ASSERT(PVT::isPVField); // only allow cast from PVField sub-class
        return std::tr1::dynamic_pointer_cast<const PVT>(get());
    }

    /**
     * Select field (set index) and get the field at the index.
     * @param index index of the field to select.
     * @return corresponding PVField (of undetermined value), @c null if {@code index == UNDEFINED_INDEX}.
     * @throws std::invalid_argument if index is invalid (out of range).
     */
    PVFieldPtr select(int32 index);

    template<typename PVT>
    inline std::tr1::shared_ptr<PVT> select(int32 index) {
        STATIC_ASSERT(PVT::isPVField); // only allow cast from PVField sub-class
        return std::tr1::dynamic_pointer_cast<PVT>(select(index));
    }

    /**
     * Select field (set index) and get the field by given name.
     * @param fieldName the name of the field to select.
     * @return corresponding PVField (of undetermined value).
     * @throws std::invalid_argument if field does not exist.
     */
    PVFieldPtr select(std::string const & fieldName);

    template<typename PVT>
    inline std::tr1::shared_ptr<PVT> select(std::string const & fieldName) {
        return std::tr1::dynamic_pointer_cast<PVT>(select(fieldName));
    }

    /**
     * Get selected field index.
     * @return selected field index.
     */
    inline int32 getSelectedIndex() const { return selector; }
    
    /**
     * Get selected field name.
     * @return selected field name, empty string if field does not exist.
     */
    std::string getSelectedFieldName() const;

    /**
     * Set the @c PVField (by reference!) as selected field.
     * If a value is not a valid union field an @c std::invalid_argument
     * exception is thrown.
     * @param value the field to set.
     */
    inline void set(PVFieldPtr const & value) {
        set(selector, value);
    }
    /**
     * Set the @c PVField (by reference!) as field at given index.
     * If a value is not a valid union field an @c std::invalid_argument
     * exception is thrown.
     * Use @c select(int32) to put by value.
     * @param index index of a field to put.
     * @param value the field to set.
     * @see #select(int32)
     */
    void set(int32 index, PVFieldPtr const & value);

    /**
     * Set the @c PVField (by reference!) as field by given name.
     * If a value is not a valid union field an @c std::invalid_argument
     * exception is thrown.
     * Use @c select(std::string const &) to put by value.
     * @param fieldName Name of the field to put.
     * @param value the field to set.
     * @see #select(std::string const &)
     */
    void set(std::string const & fieldName, PVFieldPtr const & value);

    /**
     * Serialize.
     * @param pbuffer The byte buffer.
     * @param pflusher Interface to call when buffer is full.
     */
    virtual void serialize(
        ByteBuffer *pbuffer,SerializableControl *pflusher) const OVERRIDE FINAL;
    /**
     * Deserialize
     * @param pbuffer The byte buffer.
     * @param pflusher Interface to call when buffer is empty.
     */
    virtual void deserialize(
        ByteBuffer *pbuffer,DeserializableControl *pflusher) OVERRIDE FINAL;
    /**
     * Constructor
     * @param punion The introspection interface.
     */
    explicit PVUnion(UnionConstPtr const & punion);

    virtual std::ostream& dumpValue(std::ostream& o) const OVERRIDE FINAL;

    void copy(const PVUnion& from);
    void copyUnchecked(const PVUnion& from);

private:
    static PVDataCreatePtr pvDataCreate;

    friend class PVDataCreate;
    UnionConstPtr unionPtr; // same as PVField::getField()

        int32 selector;
        PVFieldPtr value;
        bool variant;  
    EPICS_NOT_COPYABLE(PVUnion)
};


namespace detail {
    // adaptor to allow epics::pvData::shared_vector to hold a reference
    // to a shared_ptr<std::vector<> >
    template<typename T>
    struct shared_ptr_vector_deletor {
        typedef std::tr1::shared_ptr<std::vector<T> > shared_vector;
        shared_vector vec;
        shared_ptr_vector_deletor(const shared_vector& v)
            :vec(v) {}
        void operator()(T*){vec.reset();}
    };

    //! Common code for PV*Array
    template<typename T, class Base>
    class PVVectorStorage : public Base
    {
    public:
        typedef T  value_type;
        typedef T* pointer;
        typedef const T* const_pointer;

        //TODO: full namespace can be removed along with local typedef 'shared_vector'
        typedef ::epics::pvData::shared_vector<T> svector;
        typedef ::epics::pvData::shared_vector<const T> const_svector;


    protected:
        template<typename A>
        explicit PVVectorStorage(A a) : Base(a) {}
    public:
        virtual ~PVVectorStorage(){}

        // Primitive array manipulations

        //! Fetch a read-only view of the current array data
        virtual const_svector view() const = 0;

        /** Exchange our contents for the provided.
         *
         @throws std::logic_error for Immutable arrays.
         *
         *  Callers must ensure that postPut() is called
         *  after the last swap() operation.
         *
         *  Before you call this directly, consider using
         *  the reuse(), or replace() methods.
         */
        virtual void swap(const_svector& other) = 0;

        //! Discard current contents and replaced with the provided.
        //! Fails for Immutable arrays
        //! calls postPut()
        virtual void replace(const const_svector& next) = 0;

        // Derived operations

        /** Remove and return the current array data
         * or an unique copy if shared.
         *
         * Does @b not (and should not) call postPut()
         *
         * The returned shared_vector<T> will
         * have unique()==true.
         */
        inline svector reuse()
        {
            const_svector result;
            this->swap(result);
            return thaw(result);
        }

        EPICS_NOT_COPYABLE(PVVectorStorage)
    };
} // namespace detail

/**
 * @brief template class for all extensions of PVArray.
 *
 * The direct extensions are pvBooleanArray, pvByteArray, ..., pvStringArray.
 * There are specializations for PVStringArray, PVStructureArray, and PVUnionArray.
 *
 * @ingroup pvcontainer
 */
template<typename T>
class epicsShareClass PVValueArray : public detail::PVVectorStorage<T,PVScalarArray> {
    typedef detail::PVVectorStorage<T,PVScalarArray> base_t;
public:
    POINTER_DEFINITIONS(PVValueArray);
    typedef T  value_type;
    typedef T* pointer;
    typedef const T* const_pointer;

    //TODO: full namespace can be removed along with local typedef 'shared_vector'
    typedef ::epics::pvData::shared_vector<T> svector;
    typedef ::epics::pvData::shared_vector<const T> const_svector;


    static const ScalarType typeCode;

    /**
     * Destructor
     */
    virtual ~PVValueArray();

    /**
     * Get introspection interface.
     */
    virtual ArrayConstPtr getArray() const OVERRIDE FINAL;

    virtual std::ostream& dumpValue(std::ostream& o) const OVERRIDE FINAL;
    virtual std::ostream& dumpValue(std::ostream& o, size_t index) const OVERRIDE FINAL;

    virtual size_t getLength() const OVERRIDE FINAL {return value.size();}
    virtual size_t getCapacity() const OVERRIDE FINAL {return value.capacity();}

    virtual void setCapacity(size_t capacity) OVERRIDE FINAL;
    virtual void setLength(size_t length) OVERRIDE FINAL;

    virtual const_svector view() const OVERRIDE FINAL {return value;}
    virtual void swap(const_svector &other) OVERRIDE FINAL;
    virtual void replace(const const_svector& next) OVERRIDE FINAL;

    // from Serializable
    virtual void serialize(ByteBuffer *pbuffer,SerializableControl *pflusher) const OVERRIDE FINAL;
    virtual void deserialize(ByteBuffer *pbuffer,DeserializableControl *pflusher) OVERRIDE FINAL;
    virtual void serialize(ByteBuffer *pbuffer,
                           SerializableControl *pflusher, size_t offset, size_t count) const OVERRIDE FINAL;

protected:
    virtual void _getAsVoid(epics::pvData::shared_vector<const void>& out) const OVERRIDE FINAL;
    virtual void _putFromVoid(const epics::pvData::shared_vector<const void>& in) OVERRIDE FINAL;

    explicit PVValueArray(ScalarArrayConstPtr const & scalar);
    const_svector value;
    friend class PVDataCreate;
    EPICS_NOT_COPYABLE(PVValueArray)
};


/**
 * @brief Data class for a structureArray
 *
 * @ingroup pvcontainer
 */
template<>
class epicsShareClass PVValueArray<PVStructurePtr> : public detail::PVVectorStorage<PVStructurePtr,PVArray>
{
    typedef detail::PVVectorStorage<PVStructurePtr,PVArray> base_t;
public:
    POINTER_DEFINITIONS(PVStructureArray);
    typedef PVStructurePtr  value_type;
    typedef PVStructurePtr* pointer;
    typedef const PVStructurePtr* const_pointer;
    typedef PVStructureArray &reference;
    typedef const PVStructureArray& const_reference;

    //TODO: full namespace can be removed along with local typedef 'shared_vector'
    typedef ::epics::pvData::shared_vector<PVStructurePtr> svector;
    typedef ::epics::pvData::shared_vector<const PVStructurePtr> const_svector;
    /**
     * Destructor
     */
    virtual ~PVValueArray() {}

    virtual ArrayConstPtr getArray() const OVERRIDE FINAL
    {
        return std::tr1::static_pointer_cast<const Array>(structureArray);
    }

    virtual size_t getLength() const OVERRIDE FINAL {return value.size();}
    virtual size_t getCapacity() const OVERRIDE FINAL {return value.capacity();}

    /**
     * Set the array capacity.
     * @param capacity The length.
     */
    virtual void setCapacity(size_t capacity) OVERRIDE FINAL;
    /**
     * Set the array length.
     * @param length The length.
     */
    virtual void setLength(std::size_t length) OVERRIDE FINAL;

    /**
     * Get the introspection interface
     * @return The interface.
     */
    StructureArrayConstPtr getStructureArray() const {return structureArray;}
    /**
     * Append new elements to the end of the array.
     * @param number The number of elements to add.
     * @return the new length of the array.
     */
    std::size_t append(std::size_t number);
    /**
     * Remove elements from the array.
     * @param offset The offset of the first element to remove.
     * @param number The number of elements to remove.
     * @return (false,true) if the elements were removed.
     */
    bool remove(std::size_t offset,std::size_t number);
    /**
     * Compress. This removes all null elements from the array.
     */
    void compress();

    virtual const_svector view() const OVERRIDE FINAL { return value; }
    virtual void swap(const_svector &other) OVERRIDE FINAL;
    virtual void replace(const const_svector &other) OVERRIDE FINAL {
        checkLength(other.size());
        value = other;
        PVField::postPut();
    }

    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher) const OVERRIDE FINAL;
    virtual void deserialize(ByteBuffer *buffer,
        DeserializableControl *pflusher) OVERRIDE FINAL;
    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, std::size_t offset, std::size_t count) const OVERRIDE FINAL;

    virtual std::ostream& dumpValue(std::ostream& o) const OVERRIDE FINAL;
    virtual std::ostream& dumpValue(std::ostream& o, std::size_t index) const OVERRIDE FINAL;

    void copy(const PVStructureArray& from);
    void copyUnchecked(const PVStructureArray& from);

protected:
     PVValueArray(StructureArrayConstPtr const & structureArray);
private:
    StructureArrayConstPtr structureArray;
    const_svector value;
    friend class PVDataCreate;
    EPICS_NOT_COPYABLE(PVValueArray)
};



/**
 * @brief Data class for a unionArray
 *
 * @ingroup pvcontainer
 */
template<>
class epicsShareClass PVValueArray<PVUnionPtr> : public detail::PVVectorStorage<PVUnionPtr,PVArray>
{
    typedef detail::PVVectorStorage<PVUnionPtr,PVArray> base_t;
public:
    POINTER_DEFINITIONS(PVUnionArray);
    typedef PVUnionPtr  value_type;
    typedef PVUnionPtr* pointer;
    typedef const PVUnionPtr* const_pointer;
    typedef PVUnionArray &reference;
    typedef const PVUnionArray& const_reference;

    //TODO: full namespace can be removed along with local typedef 'shared_vector'
    typedef ::epics::pvData::shared_vector<PVUnionPtr> svector;
    typedef ::epics::pvData::shared_vector<const PVUnionPtr> const_svector;
    /**
     * Destructor
     */
    virtual ~PVValueArray() {}

    virtual ArrayConstPtr getArray() const OVERRIDE FINAL
    {
        return std::tr1::static_pointer_cast<const Array>(unionArray);
    }

    virtual size_t getLength() const OVERRIDE FINAL {return value.size();}
    virtual size_t getCapacity() const OVERRIDE FINAL {return value.capacity();}

    /**
     * Set the array capacity.
     * @param capacity The length.
     */
    virtual void setCapacity(size_t capacity) OVERRIDE FINAL;
    /**
     * Set the array length.
     * @param length The length.
     */
    virtual void setLength(std::size_t length) OVERRIDE FINAL;

    /**
     * Get the introspection interface
     * @return The interface.
     */
    UnionArrayConstPtr getUnionArray() const {return unionArray;}
    /**
     * Append new elements to the end of the array.
     * @param number The number of elements to add.
     * @return the new length of the array.
     */
    std::size_t append(std::size_t number);
    /**
     * Remove elements from the array.
     * @param offset The offset of the first element to remove.
     * @param number The number of elements to remove.
     * @return (false,true) if the elements were removed.
     */
    bool remove(std::size_t offset,std::size_t number);
    /**
     * Compress. This removes all null elements from the array.
     */
    void compress();

    virtual const_svector view() const OVERRIDE { return value; }
    virtual void swap(const_svector &other) OVERRIDE;
    virtual void replace(const const_svector &other) OVERRIDE FINAL {
        checkLength(other.size());
        value = other;
        PVField::postPut();
    }

    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher) const OVERRIDE FINAL;
    virtual void deserialize(ByteBuffer *buffer,
        DeserializableControl *pflusher) OVERRIDE FINAL;
    virtual void serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, std::size_t offset, std::size_t count) const OVERRIDE FINAL;

    virtual std::ostream& dumpValue(std::ostream& o) const OVERRIDE FINAL;
    virtual std::ostream& dumpValue(std::ostream& o, std::size_t index) const OVERRIDE FINAL;

    void copy(const PVUnionArray& from);
    void copyUnchecked(const PVUnionArray& from);

protected:
    explicit PVValueArray(UnionArrayConstPtr const & unionArray);
private:
    UnionArrayConstPtr unionArray;
    const_svector value;
    friend class PVDataCreate;
    EPICS_NOT_COPYABLE(PVValueArray)
};


/**
 * Definitions for the various scalarArray types.
 */
typedef PVValueArray<boolean> PVBooleanArray;
typedef std::tr1::shared_ptr<PVBooleanArray> PVBooleanArrayPtr;

typedef PVValueArray<int8> PVByteArray;
typedef std::tr1::shared_ptr<PVByteArray> PVByteArrayPtr;

typedef PVValueArray<int16> PVShortArray;
typedef std::tr1::shared_ptr<PVShortArray> PVShortArrayPtr;

typedef PVValueArray<int32> PVIntArray;
typedef std::tr1::shared_ptr<PVIntArray> PVIntArrayPtr;

typedef PVValueArray<int64> PVLongArray;
typedef std::tr1::shared_ptr<PVLongArray> PVLongArrayPtr;

typedef PVValueArray<uint8> PVUByteArray;
typedef std::tr1::shared_ptr<PVUByteArray> PVUByteArrayPtr;

typedef PVValueArray<uint16> PVUShortArray;
typedef std::tr1::shared_ptr<PVUShortArray> PVUShortArrayPtr;

typedef PVValueArray<uint32> PVUIntArray;
typedef std::tr1::shared_ptr<PVUIntArray> PVUIntArrayPtr;

typedef PVValueArray<uint64> PVULongArray;
typedef std::tr1::shared_ptr<PVULongArray> PVULongArrayPtr;

typedef PVValueArray<float> PVFloatArray;
typedef std::tr1::shared_ptr<PVFloatArray> PVFloatArrayPtr;

typedef PVValueArray<double> PVDoubleArray;
typedef std::tr1::shared_ptr<PVDoubleArray> PVDoubleArrayPtr;

typedef PVValueArray<std::string> PVStringArray;
typedef std::tr1::shared_ptr<PVStringArray> PVStringArrayPtr;

namespace detail {
struct pvfield_factory;
}

/**
 * @brief This is a singleton class for creating data instances.
 *
 */
class epicsShareClass PVDataCreate {
    friend struct detail::pvfield_factory;
public:
    /**
     * get the singleton
     * @return The PVDataCreate implementation
     */
    static const PVDataCreatePtr &getPVDataCreate();

    /**
     * Create a PVField using given Field introspection data.
     * @param field The introspection data to be used to create PVField.
     * @return The PVField implementation.
     */
    PVFieldPtr createPVField(FieldConstPtr const & field);
    /**
     * Create a PVField using given a PVField to clone.
     * This method calls the appropriate createPVScalar, createPVArray, or createPVStructure.
     * @param fieldToClone The field to clone.
     * @return The PVField implementation
     */
    PVFieldPtr createPVField(PVFieldPtr const & fieldToClone);
    /**
     * Create an implementation of a scalar field reusing the Scalar introspection interface.
     * @param scalar The introspection interface.
     * @return The PVScalar implementation.
     */
    PVScalarPtr createPVScalar(ScalarConstPtr const & scalar);
    /**
     * Create an implementation of a scalar field. A Scalar introspection interface is created.
     * @param scalarType The scalar type.
     * @return The PVScalar implementation.
     */
    PVScalarPtr createPVScalar(ScalarType scalarType);
    /**
     * Create an implementation of a scalar field by cloning an existing PVScalar.
     * The new PVScalar will have the same value and auxInfo as the original.
     * @param scalarToClone The PVScalar to clone.
     * @return The PVScalar implementation.
     */
    PVScalarPtr createPVScalar(PVScalarPtr const & scalarToClone);
    /**
     * template version
     * @tparam PVT must be a valid PVType
     * @return The PVScalar implementation.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> createPVScalar()
    {
        return std::tr1::static_pointer_cast<PVT>(createPVScalar(PVT::typeCode));   
    }

    /**
     * Create implementation for PVStructure.
     * @param structure The introspection interface.
     * @return The PVStructure implementation
     */
    PVStructurePtr createPVStructure(StructureConstPtr const & structure);
    /**
     * Create implementation for PVStructure.
     * @param fieldNames The field names.
     * @param pvFields Array of PVFields
     * @return The PVStructure implementation
     */
    PVStructurePtr createPVStructure(
        StringArray const & fieldNames,PVFieldPtrArray const & pvFields);
     /**
      * Create implementation for PVStructure.
      * @param structToClone A structure. Each subfield and any auxInfo is cloned and added to the newly created structure.
      * @return The PVStructure implementation.
      */
    PVStructurePtr createPVStructure(PVStructurePtr const & structToClone);

    /**
     * Create implementation for PVUnion.
     * @param punion The introspection interface.
     * @return The PVUnion implementation
     */
    PVUnionPtr createPVUnion(UnionConstPtr const & punion);
     /**
      * Create implementation for PVUnion.
      * @param unionToClone A union. Each subfield is cloned and added to the newly created union.
      * @return The PVUnion implementation.
      */
   PVUnionPtr createPVUnion(PVUnionPtr const & unionToClone);
    /**
     * Create variant union implementation.
     * @return The variant PVUnion implementation. 
     */
    PVUnionPtr createPVVariantUnion();

    /**
     * Create an implementation of an array field reusing the Array introspection interface.
     * @param scalarArray The introspection interface.
     * @return The PVScalarArray implementation.
     */
    PVScalarArrayPtr createPVScalarArray(ScalarArrayConstPtr const & scalarArray);
    /**
     * Create an implementation for an array field. An Array introspection interface is created.
     * @param elementType The element type.
     * @return The PVScalarArray implementation.
     */
    PVScalarArrayPtr createPVScalarArray(ScalarType elementType);
    /**
     * Create an implementation of an array field by cloning an existing PVArray.
     * The new PVArray will have the same value and auxInfo as the original.
     * @param scalarArrayToClone The PVScalarArray to clone.
     * @return The PVScalarArray implementation.
     */
    PVScalarArrayPtr createPVScalarArray(PVScalarArrayPtr const  & scalarArrayToClone);
    /**
     * template version
     * @tparam PVT must be a valid pvType
     * @return The PVScalarArray implementation.
     */
    template<typename PVAT>
    std::tr1::shared_ptr<PVAT> createPVScalarArray()
    {
        return std::tr1::static_pointer_cast<PVAT>(createPVScalarArray(PVAT::typeCode));   
    }

    /**
     * Create an implementation of an array with structure elements.
     * @param structureArray The introspection interface.
     * All elements share the same introspection interface.
     * @return The PVStructureArray implementation.
     */
    PVStructureArrayPtr createPVStructureArray(StructureArrayConstPtr const & structureArray);
    /**
     * Create an implementation of an array with structure elements.
     * @param structure The introspection interface that is used to create StructureArrayConstPtr.
     * All elements share the same introspection interface.
     * @return The PVStructureArray implementation.
     */
    PVStructureArrayPtr createPVStructureArray(StructureConstPtr const & structure)
    {
        return createPVStructureArray(fieldCreate->createStructureArray(structure));
    }

    /**
     * Create an implementation of an array with union elements.
     * @param unionArray The introspection interface.
     * All elements share the same introspection interface.
     * @return The PVUnionArray implementation.
     */
    PVUnionArrayPtr createPVUnionArray(UnionArrayConstPtr const & unionArray);
    /**
     * Create an implementation of an array with union elements.
     * @param punion The introspection interface that is used to create UnionArrayConstPtr.
     * All elements share the same introspection interface.
     * @return The PVUnionArray implementation.
     */
    PVUnionArrayPtr createPVUnionArray(UnionConstPtr const & punion)
    {
        return createPVUnionArray(fieldCreate->createUnionArray(punion));
    }
    /**
     * Create variant union array implementation.
     * @return The variant PVUnionArray implementation. 
     */
    PVUnionArrayPtr createPVVariantUnionArray();
    
private:
   PVDataCreate();
   FieldCreatePtr fieldCreate;
   EPICS_NOT_COPYABLE(PVDataCreate)
};

/**
 * Get the single class that implements PVDataCreate
 * @return The PVDataCreate factory.
 *
 * @ingroup pvcontainer
 */
FORCE_INLINE const PVDataCreatePtr& getPVDataCreate() {
    return PVDataCreate::getPVDataCreate();
}

bool epicsShareExtern operator==(const PVField&, const PVField&);

static inline bool operator!=(const PVField& a, const PVField& b)
{return !(a==b);}

}}

/**
 * stream support for pvField
 */
namespace std{
    epicsShareExtern std::ostream& operator<<(std::ostream& o, const epics::pvData::PVField *ptr);
}

#endif  /* PVDATA_H */
