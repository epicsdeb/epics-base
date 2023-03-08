/* pvIntrospect.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk and Michael Davidsaver
 */
#ifndef PVINTROSPECT_H
#define PVINTROSPECT_H

#include <string>
#include <stdexcept>
#include <iostream>
#include <map>

#include <epicsAssert.h>

#include <pv/lock.h>
#include <pv/noDefaultMethods.h>
#include <pv/pvType.h>
#include <pv/byteBuffer.h>
#include <pv/serialize.h>
#include <pv/pvdVersion.h>

#include <shareLib.h>
#define PVD_DEPRECATED_52 PVD_DEPRECATED("See https://github.com/epics-base/pvDataCPP/issues/52")

/* C++11 keywords
 @code
 struct Base {
   virtual void foo();
 };
 struct Class : public Base {
   virtual void foo() OVERRIDE FINAL;
 };
 @endcode
 */
#ifndef FINAL
#  if __cplusplus>=201103L
#    define FINAL final
#  else
#    define FINAL
#  endif
#endif
#ifndef OVERRIDE
#  if __cplusplus>=201103L
#    define OVERRIDE override
#  else
#    define OVERRIDE
#  endif
#endif

namespace epics { namespace pvData { 

namespace format {

struct indent_level
{
    long level;

    indent_level(long l) : level(l) {}
};

epicsShareExtern long& indent_value(std::ios_base& ios);

epicsShareExtern std::ostream& operator<<(std::ostream& os, indent_level const& indent);

struct indent_scope
{
    long saved_level;
    std::ios_base& stream;

    indent_scope(std::ios_base& ios) :
        stream(ios)
    {
        long& l = indent_value(ios);
        saved_level = l;
        l = saved_level + 1;
    }

    ~indent_scope()
    {
        indent_value(stream) = saved_level;
    }
};

struct indent
{
};

epicsShareExtern std::ostream& operator<<(std::ostream& os, indent const&);

struct array_at
{
    std::size_t index;

    array_at(std::size_t ix) : index(ix) {}
};

struct array_at_internal
{
    std::size_t index;
    std::ostream& stream;

    array_at_internal(std::size_t ix, std::ostream& str) : index(ix), stream(str) {}
};

epicsShareExtern array_at_internal operator<<(std::ostream& str, array_at const& manip);

};

class Field;
class Scalar;
class Array;
class ScalarArray;
class Structure;
class StructureArray;
class Union;
class UnionArray;

class BoundedString;

class PVField;
class PVScalar;
class PVScalarArray;
class PVStructure;
class PVUnion;
template<typename T> class PVValueArray;

/**
 * typedef for a shared pointer to an immutable Field.
 */
typedef std::tr1::shared_ptr<const Field> FieldConstPtr;
/**
 * typedef for an array of shared pointer to an immutable Field.
 */
typedef std::vector<FieldConstPtr> FieldConstPtrArray;
/**
 * typedef for a shared pointer to an immutable Scalar.
 */
typedef std::tr1::shared_ptr<const Scalar> ScalarConstPtr;
/**
 * typedef for a shared pointer to an immutable Array.
 */
typedef std::tr1::shared_ptr<const Array> ArrayConstPtr;
/**
 * typedef for a shared pointer to an immutable ScalarArray.
 */
typedef std::tr1::shared_ptr<const ScalarArray> ScalarArrayConstPtr;
/**
 * typedef for a shared pointer to an immutable Structure.
 */
typedef std::tr1::shared_ptr<const Structure> StructureConstPtr;
/**
 * typedef for a shared pointer to an immutable StructureArray.
 */
typedef std::tr1::shared_ptr<const StructureArray> StructureArrayConstPtr;
/**
 * typedef for a shared pointer to an immutable Union.
 */
typedef std::tr1::shared_ptr<const Union> UnionConstPtr;
/**
 * typedef for a shared pointer to an immutable UnionArray.
 */
typedef std::tr1::shared_ptr<const UnionArray> UnionArrayConstPtr;
/**
 * typedef for a shared pointer to an immutable BoundedString.
 */
typedef std::tr1::shared_ptr<const BoundedString> BoundedStringConstPtr;

/**
 * Definition of support field types.
 */
enum Type {
    /**
     * The type is scalar. It has a scalarType
     */
    scalar,
    /**
     * The type is scalarArray. Each element is a scalar of the same scalarType.
     */
    scalarArray,
    /**
     * The type is structure.
     */
    structure,
    /**
     * The type is structureArray. Each element is a structure.
     */
    structureArray,
    /**
     * The type is an union.
     */
    union_,
    /**
     * The type is an array of unions.
     */
    unionArray
};

/**
 * @brief Convenience functions for Type.
 *
 */
namespace TypeFunc {
    /**
     * Get a name for the type.
     * @param  type The type.
     * @return The name for the type.
     */
    epicsShareExtern const char* name(Type type);
};

epicsShareExtern std::ostream& operator<<(std::ostream& o, const Type& type);


/**
 * Definition of support scalar types.
 */
enum ScalarType {
    /**
     * The type is boolean, i.e. value can be @c false or @c true
     */
    pvBoolean,
    /**
     * The type is byte, i.e. a 8 bit signed integer.
     */
    pvByte,
    /**
     * The type is short, i.e. a 16 bit signed integer.
     */
    pvShort,
    /**
     * The type is int, i.e. a 32 bit signed integer.
     */
    pvInt,
    /**
     * The type is long, i.e. a 64 bit signed integer.
     */
    pvLong,
    /**
     * The type is unsigned byte, i.e. a 8 bit unsigned integer.
     */
    pvUByte,
    /**
     * The type is unsigned short, i.e. a 16 bit unsigned integer.
     */
    pvUShort,
    /**
     * The type is unsigned int, i.e. a 32 bit unsigned integer.
     */
    pvUInt,
    /**
     * The type is unsigned long, i.e. a 64 bit unsigned integer.
     */
    pvULong,
    /**
     * The type is float, i.e. 32 bit IEEE floating point,
     */
    pvFloat,
    /**
     * The type is float, i.e. 64 bit IEEE floating point,
     */
    pvDouble,
    /**
     * The type is string, i.e. a UTF8 character string.
     */
    pvString
};

#define MAX_SCALAR_TYPE pvString

/**
 * @brief Convenience functions for ScalarType.
 *
 */
namespace ScalarTypeFunc {
    /**
     * Is the type an integer, i.e. is it one of byte,...ulong
     * @param  scalarType The type.
     * @return (false,true) if the scalarType is an integer.
     */
    epicsShareExtern bool isInteger(ScalarType scalarType);
    /**
     * Is the type an unsigned integer, i.e. is it one of ubyte,...ulong
     * @param  scalarType The type.
     * @return (false,true) if the scalarType is an integer.
     */
    epicsShareExtern bool isUInteger(ScalarType scalarType);
    /**
     * Is the type numeric, i.e. is it one of byte,...,double
     * @param  scalarType The type.
     * @return (false,true) if the scalarType is a numeric
     */
    epicsShareExtern bool isNumeric(ScalarType scalarType);
    /**
     * Is the type primitive, i.e. not string
     * @param  scalarType The type.
     * @return (false,true) if the scalarType is primitive.
     */
    epicsShareExtern bool isPrimitive(ScalarType scalarType);
    /**
     * Get the scalarType for value.
     * @param  value The name of the scalar type.
     * @return The scalarType.
     * An exception is thrown if the name is not the name of a scalar type.
     */
    epicsShareExtern ScalarType getScalarType(std::string const &value);
    /**
     * Get a name for the scalarType.
     * @param  scalarType The type.
     * @return The name for the scalarType.
     */
    epicsShareExtern const char* name(ScalarType scalarType);

    //! gives sizeof(T) where T depends on the scalar type id.
    epicsShareExtern size_t elementSize(ScalarType id);
};

epicsShareExtern std::ostream& operator<<(std::ostream& o, const ScalarType& scalarType);


/**
 * @brief This class implements introspection object for field.
 *
 */
class epicsShareClass Field : 
    virtual public Serializable,
    public std::tr1::enable_shared_from_this<Field> {
public:
   static size_t num_instances;

   POINTER_DEFINITIONS(Field);
   virtual ~Field();
    /**
     * Get the field type.
     * @return The type.
     */
   Type getType() const{return m_fieldType;}
   /**
    * Get the identification string.
    * @return The identification string, can be empty.
    */
   virtual std::string getID() const = 0;
   
    /**
     * Puts the string representation to the stream.
     * @param o output stream.
     * @return The output stream.
     */
    virtual std::ostream& dump(std::ostream& o) const = 0;

   //! Allocate a new instance
   //! @version Added after 7.0.0
    std::tr1::shared_ptr<PVField> build() const;

    enum {isField=1};

protected:
    /**
     * Constructor
     * @param  type The field type.
     */
   Field(Type type);
   void cacheCleanup();
private:
   const Type m_fieldType;
   unsigned int m_hash;
   struct Helper;
   friend struct Helper;

   friend class StructureArray;
   friend class Structure;
   friend class PVFieldPvt;
   friend class StandardField;
   friend class BasePVStructureArray;
   friend class FieldCreate;
   EPICS_NOT_COPYABLE(Field)
};

epicsShareExtern std::ostream& operator<<(std::ostream& o, const Field& field);


/**
 * @brief This class implements introspection object for Scalar.
 *
 */
class epicsShareClass Scalar : public Field{
public:
    POINTER_DEFINITIONS(Scalar);
    virtual ~Scalar();
    typedef Scalar& reference;
    typedef const Scalar& const_reference;
    /**
     * Get the scalarType
     * @return the scalarType
     */
    ScalarType getScalarType() const {return scalarType;}
    
    virtual std::string getID() const OVERRIDE;

    virtual std::ostream& dump(std::ostream& o) const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE;
    virtual void deserialize(ByteBuffer *buffer, DeserializableControl *control) OVERRIDE FINAL;

    //! Allocate a new instance
    //! @version Added after 7.0.0
    std::tr1::shared_ptr<PVScalar> build() const;
    
protected:
    Scalar(ScalarType scalarType);
private:
    static int8 getTypeCodeLUT(ScalarType scalarType);
    ScalarType scalarType;
    friend class FieldCreate;
    friend class ScalarArray;
    friend class BoundedScalarArray;
    friend class FixedScalarArray;
    friend class BoundedString;
    EPICS_NOT_COPYABLE(Scalar)
};

/**
 * @brief This class implements introspection object for BoundedString.
 *
 */
class epicsShareClass BoundedString : public Scalar{
public:
    POINTER_DEFINITIONS(BoundedString);
    virtual ~BoundedString();
    typedef BoundedString& reference;
    typedef const BoundedString& const_reference;

    virtual std::string getID() const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE FINAL;

    std::size_t getMaximumLength() const;

protected:
    BoundedString(std::size_t maxStringLength);
private:
    std::size_t maxLength;
    friend class FieldCreate;
    EPICS_NOT_COPYABLE(BoundedString)
};

/**
 * @brief This class implements introspection object for Array.
 *
 */
class epicsShareClass Array : public Field{
public:
    POINTER_DEFINITIONS(Array);
    virtual ~Array();
    typedef Array& reference;
    typedef const Array& const_reference;

    enum ArraySizeType { variable, fixed, bounded };

    /**
     * Get array size type (i.e. variable/fixed/bounded size array).
     * @return array size type enum.
     */
    virtual ArraySizeType getArraySizeType() const = 0;

    /**
     * Get maximum capacity of the array.
     * @return maximum capacity of the array, 0 indicates variable size array.
     */
    virtual std::size_t getMaximumCapacity() const = 0;

protected:
    /**
     * Constructor
     * @param type The field type.
     */
   Array(Type type);

   EPICS_NOT_COPYABLE(Array)
};

/**
 * @brief This class implements introspection object for scalar array.
 *
 */
class epicsShareClass ScalarArray : public Array{
public:
    POINTER_DEFINITIONS(ScalarArray);
    typedef ScalarArray& reference;
    typedef const ScalarArray& const_reference;

    /**
     * Constructor
     * @param scalarType The scalarType for the field.
     */
    ScalarArray(ScalarType scalarType);
    /**
     * Get the scalarType for the elements.
     * @return the scalarType
     */
    ScalarType getElementType() const {return elementType;}
    
    virtual ArraySizeType getArraySizeType() const OVERRIDE {return Array::variable;}

    virtual std::size_t getMaximumCapacity() const OVERRIDE {return 0;}

    virtual std::string getID() const OVERRIDE;

    virtual std::ostream& dump(std::ostream& o) const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE;
    virtual void deserialize(ByteBuffer *buffer, DeserializableControl *control) OVERRIDE FINAL;

    //! Allocate a new instance
    //! @version Added after 7.0.0
     std::tr1::shared_ptr<PVScalarArray> build() const;
    
    virtual ~ScalarArray();
private:
    const std::string getIDScalarArrayLUT() const;
    ScalarType elementType;
    friend class FieldCreate;
    EPICS_NOT_COPYABLE(ScalarArray)
};



/**
 * @brief This class implements introspection object for bounded scalar array.
 *
 */
class epicsShareClass BoundedScalarArray : public ScalarArray{
public:
    POINTER_DEFINITIONS(BoundedScalarArray);
    typedef BoundedScalarArray& reference;
    typedef const BoundedScalarArray& const_reference;

    /**
     * Constructor
     * @param scalarType The scalarType for the field.
     * @param size maximum (bound) capacity.
     */
    BoundedScalarArray(ScalarType scalarType, std::size_t size);

    virtual ArraySizeType getArraySizeType() const OVERRIDE FINAL {return Array::bounded;}

    virtual std::size_t getMaximumCapacity() const OVERRIDE FINAL {return size;}

    virtual std::string getID() const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE FINAL;

    virtual ~BoundedScalarArray();
private:
    std::size_t size;
    friend class FieldCreate;
    EPICS_NOT_COPYABLE(BoundedScalarArray)
};

/**
 * @brief This class implements introspection object for bounded scalar array.
 *
 */
class epicsShareClass FixedScalarArray : public ScalarArray{
public:
    POINTER_DEFINITIONS(FixedScalarArray);
    typedef FixedScalarArray& reference;
    typedef const FixedScalarArray& const_reference;

    /**
     * Constructor
     * @param scalarType The scalarType for the field.
     * @param size maximum (bound) capacity.
     */
    FixedScalarArray(ScalarType scalarType, std::size_t size);

    virtual ArraySizeType getArraySizeType() const OVERRIDE FINAL {return Array::fixed;}

    virtual std::size_t getMaximumCapacity() const OVERRIDE FINAL {return size;}

    virtual std::string getID() const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE FINAL;

    virtual ~FixedScalarArray();
private:
    std::size_t size;
    friend class FieldCreate;
    EPICS_NOT_COPYABLE(FixedScalarArray)
};

/**
 * @brief This class implements introspection object for a structureArray
 *
 */
class epicsShareClass StructureArray : public Array{
public:
    POINTER_DEFINITIONS(StructureArray);
    typedef StructureArray& reference;
    typedef const StructureArray& const_reference;

    /**
     * Get the introspection interface for the array elements.
     * @return The introspection interface.
     */
    const StructureConstPtr& getStructure() const {return pstructure;}

    virtual ArraySizeType getArraySizeType() const OVERRIDE FINAL {return Array::variable;}

    virtual std::size_t getMaximumCapacity() const OVERRIDE FINAL {return 0;}

    virtual std::string getID() const OVERRIDE FINAL;

    virtual std::ostream& dump(std::ostream& o) const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE FINAL;
    virtual void deserialize(ByteBuffer *buffer, DeserializableControl *control) OVERRIDE FINAL;

    //! Allocate a new instance
    //! @version Added after 7.0.0
     std::tr1::shared_ptr<PVValueArray<std::tr1::shared_ptr<PVStructure> > > build() const;

protected:
    /**
     * Constructor.
     * @param structure The introspection interface for the elements.
     */
    StructureArray(StructureConstPtr const & structure);
public:
    virtual ~StructureArray();
private:
    StructureConstPtr pstructure;
    friend class FieldCreate;
    EPICS_NOT_COPYABLE(StructureArray)
};

/**
 * @brief This class implements introspection object for a unionArray
 *
 */
class epicsShareClass UnionArray : public Array{
public:
    POINTER_DEFINITIONS(UnionArray);
    typedef UnionArray& reference;
    typedef const UnionArray& const_reference;

    /**
     * Get the introspection interface for the array elements.
     * @return The introspection interface.
     */
    UnionConstPtr getUnion() const {return punion;}

    virtual ArraySizeType getArraySizeType() const OVERRIDE FINAL {return Array::variable;}

    virtual std::size_t getMaximumCapacity() const OVERRIDE FINAL {return 0;}

    virtual std::string getID() const OVERRIDE FINAL;

    virtual std::ostream& dump(std::ostream& o) const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE FINAL;
    virtual void deserialize(ByteBuffer *buffer, DeserializableControl *control) OVERRIDE FINAL;

    //! Allocate a new instance
    //! @version Added after 7.0.0
     std::tr1::shared_ptr<PVValueArray<std::tr1::shared_ptr<PVUnion> > > build() const;

protected:
    /**
     * Constructor.
     * @param _punion The introspection interface for the elements.
     */
    UnionArray(UnionConstPtr const & _punion);
public:
    virtual ~UnionArray();
private:
    UnionConstPtr punion;
    friend class FieldCreate;
    EPICS_NOT_COPYABLE(UnionArray)
};

/**
 * @brief This class implements introspection object for a structure.
 *
 */
class epicsShareClass Structure : public Field {
public:
    POINTER_DEFINITIONS(Structure);

    /**
     * Default structure ID.
     */
    static const std::string DEFAULT_ID;

    /**
     * Get the default structure ID.
     * @return The default structure ID.
     */
    static const std::string & defaultId();

    virtual ~Structure();
    typedef Structure& reference;
    typedef const Structure& const_reference;

    /**
     * Get the number of immediate subfields in the structure.
     * @return The number of fields.
     */
    std::size_t getNumberFields() const {return fieldNames.size();}

    /**
     * Lookup Field by name
     * @param fieldName Member field name. May not contain '.'
     * @return NULL if no member by this name.
     */
    FieldConstPtr getField(std::string const &fieldName) const;

    /** Lookup Field by name and cast to Field sub-class.
     * @param fieldName Member field name. May not contain '.'
     * @return NULL If no member by this name, or member exists, but has type other than FT.
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getField(std::string const &fieldName) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        return std::tr1::dynamic_pointer_cast<const FT>(getField(fieldName));
    }

    /**
     * Lookup Field by name
     * @param fieldName Member field name. May not contain '.'
     * @return Field pointer (never NULL)
     * @throws std::runtime_error If no member by this name
     */
    FieldConstPtr getFieldT(std::string const &fieldName) const {return getFieldImpl(fieldName, true);};

    /** Lookup Field by name and cast to Field sub-class.
     * @param fieldName Member field name. May not contain '.'
     * @return Field pointer (never NULL)
     * @throws std::runtime_error If no member by this name, or member exists, but has type other than FT.
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getFieldT(std::string const &fieldName) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        std::tr1::shared_ptr<const FT> result(
            std::tr1::dynamic_pointer_cast<const FT>(getFieldT(fieldName))
        );

        if (!result)
            throw std::runtime_error("Wrong Field type");

        return result;
    }

    /** Lookup Field by index, within this Structure.
     * @param index Index of member in this structure.  @code index>=0 && index<getNumberFields() @endcode
     * @return Field pointer (never NULL)
     * @throws std::out_of_range If index >= getNumberFields()
     */
    const FieldConstPtr& getField(std::size_t index) const {return fields.at(index);}

    /** Lookup Field by index, within this Structure.
     * @param index Index of member in this structure.  @code index>=0 && index<getNumberFields() @endcode
     * @return NULL if member is not a sub-class of FT
     * @throws std::out_of_range If index >= getNumberFields()
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getField(std::size_t index) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        return std::tr1::dynamic_pointer_cast<const FT>(getField(index));
    }

    /** Lookup Field by index, within this Structure.
     * @param index Index of member in this structure.  @code index>=0 && index<getNumberFields() @endcode
     * @return Field pointer (never NULL)
     * @throws std::out_of_range If index >= getNumberFields()
     */
    FieldConstPtr getFieldT(std::size_t index) const {return fields.at(index);}

    /** Lookup Field by index, within this Structure.
     * @param index Index of member in this structure.  @code index>=0 && index<getNumberFields() @endcode
     * @return Field pointer (never NULL)
     * @throws std::out_of_range If index >= getNumberFields()
     * @throws std::runtime_error If member is not a sub-class of FT
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getFieldT(std::size_t index) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        std::tr1::shared_ptr<const FT> result(
            std::tr1::dynamic_pointer_cast<const FT>(getFieldT(index))
        );

        if (!result)
            throw std::runtime_error("Wrong Field type");

        return result;
    }

    /**
     * Get the field index for the specified fieldName.
     * @return The introspection interface.
     * This will be -1 if the field is not in the structure.
     */
    std::size_t getFieldIndex(std::string const &fieldName) const;
    /**
     * Get the fields in the structure.
     * @return The array of fields.
     */
    FieldConstPtrArray const & getFields() const {return fields;}
    /**
     * Get the names of the fields in the structure.
     * @return The array of fieldNames.
     */
    StringArray const & getFieldNames() const {return fieldNames;}
    /**
     * Get the name of the field with the specified index;
     * @param fieldIndex The index of the desired field.
     * @return The fieldName.
     */
    const std::string& getFieldName(std::size_t fieldIndex) const {return fieldNames.at(fieldIndex);}

    virtual std::string getID() const OVERRIDE FINAL;

    virtual std::ostream& dump(std::ostream& o) const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE FINAL;
    virtual void deserialize(ByteBuffer *buffer, DeserializableControl *control) OVERRIDE FINAL;

    //! Allocate a new instance
    //! @version Added after 7.0.0
    std::tr1::shared_ptr<PVStructure> build() const;

protected:
    Structure(StringArray const & fieldNames, FieldConstPtrArray const & fields, std::string const & id = defaultId());
private:
    StringArray fieldNames;
    FieldConstPtrArray fields;
    std::string id;

    FieldConstPtr getFieldImpl(const std::string& fieldName, bool throws) const;
    void dumpFields(std::ostream& o) const;
    
    friend class FieldCreate;
    friend class Union;
    EPICS_NOT_COPYABLE(Structure)
};

/**
 * @brief This class implements introspection object for a union.
 *
 */
class epicsShareClass Union : public Field {
public:
    POINTER_DEFINITIONS(Union);

    /**
     * Default union ID.
     */
    static const std::string DEFAULT_ID;

    /**
     * Get the default union ID.
     * @return The default union ID.
     */
    static const std::string & defaultId();

    /**
     * Default variant union ID.
     */
    static const std::string ANY_ID;

    /**
     * Get the default variant union ID.
     * @return The default variant union ID.
     */
    static const std::string & anyId();

    virtual ~Union();
    typedef Union& reference;
    typedef const Union& const_reference;

    /**
     * Get the number of immediate subfields in the union.
     * @return The number of fields.
     */
    std::size_t getNumberFields() const {return fieldNames.size();}

    /**
     * Lookup Field by name
     * @param fieldName Member field name. May not contain '.'
     * @return NULL if no member by this name.
     */
    FieldConstPtr getField(std::string const &fieldName) const;

    /** Lookup Field by name and cast to Field sub-class.
     * @param fieldName Member field name. May not contain '.'
     * @return NULL If no member by this name, or member exists, but has type other than FT.
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getField(std::string const &fieldName) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        return std::tr1::dynamic_pointer_cast<const FT>(getField(fieldName));
    }

    /**
     * Lookup Field by name
     * @param fieldName Member field name. May not contain '.'
     * @return Field pointer (never NULL)
     * @throws std::runtime_error If no member by this name
     */
    FieldConstPtr getFieldT(std::string const &fieldName) const {return getFieldImpl(fieldName, true);};

    /** Lookup Field by name and cast to Field sub-class.
     * @param fieldName Member field name. May not contain '.'
     * @return Field pointer (never NULL)
     * @throws std::runtime_error If no member by this name, or member exists, but has type other than FT.
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getFieldT(std::string const &fieldName) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        std::tr1::shared_ptr<const FT> result(
            std::tr1::dynamic_pointer_cast<const FT>(getFieldT(fieldName))
        );

        if (!result)
            throw std::runtime_error("Wrong Field type");

        return result;
    }

    /** Lookup Field by index, within this Union.
     * @param index Index of member in this union.  @code index>=0 && index<getNumberFields() @endcode
     * @return Field pointer (never NULL)
     * @throws std::out_of_range If index >= getNumberFields()
     */
    FieldConstPtr getField(std::size_t index) const {return fields.at(index);}

    /** Lookup Field by index, within this Union.
     * @param index Index of member in this union.  @code index>=0 && index<getNumberFields() @endcode
     * @return NULL if member is not a sub-class of FT
     * @throws std::out_of_range If index >= getNumberFields()
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getField(std::size_t index) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        return std::tr1::dynamic_pointer_cast<const FT>(getField(index));
    }

    /** Lookup Field by index, within this Union.
     * @param index Index of member in this union.  @code index>=0 && index<getNumberFields() @endcode
     * @return Field pointer (never NULL)
     * @throws std::out_of_range If index >= getNumberFields()
     */
    FieldConstPtr getFieldT(std::size_t index) const {return fields.at(index);}

    /** Lookup Field by index, within this Structure.
     * @param index Index of member in this structure.  @code index>=0 && index<getNumberFields() @endcode
     * @return Field pointer (never NULL)
     * @throws std::out_of_range If index >= getNumberFields()
     * @throws std::runtime_error If member is not a sub-class of FT
     */
    template<typename FT>
    std::tr1::shared_ptr<const FT> getFieldT(std::size_t index) const
    {
        STATIC_ASSERT(FT::isField); // only allow cast from Field sub-class
        std::tr1::shared_ptr<const FT> result(
            std::tr1::dynamic_pointer_cast<const FT>(getFieldT(index))
        );

        if (!result)
            throw std::runtime_error("Wrong Field type");

        return result;
    }

    /**
     * Get the field index for the specified fieldName.
     * @return The introspection interface.
     * This will be -1 if the field is not in the union.
     */
    std::size_t getFieldIndex(std::string const &fieldName) const;
    /**
     * Get the fields in the union.
     * @return The array of fields.
     */
    FieldConstPtrArray const & getFields() const {return fields;}
    /**
     * Get the names of the fields in the union.
     * @return The array of fieldNames.
     */
    StringArray const & getFieldNames() const {return fieldNames;}
    /**
     * Get the name of the field with the specified index;
     * @param fieldIndex The index of the desired field.
     * @return The fieldName.
     */
    std::string getFieldName(std::size_t fieldIndex) const {return fieldNames.at(fieldIndex);}
    /**
     * Check if this union is variant union (aka any type).
     * @return <code>true</code> if this union is variant union, otherwise <code>false</code>.
     */
    bool isVariant() const {return (fieldNames.size() == 0);}

    /** Attempt to find an suitable member to stored the specified type.
     *
     * Returned index is guerenteed to by of specified Type (either scalar or scalarArray).
     * Provided ScalarType is taken as a hint.
     *
     @param t Must be either scalar or scalarArray
     @param s The preferred ScalarType
     @returns A valid index or -1
     */
    int32 guess(Type t, ScalarType s) const;

    virtual std::string getID() const OVERRIDE FINAL;

    virtual std::ostream& dump(std::ostream& o) const OVERRIDE FINAL;

    virtual void serialize(ByteBuffer *buffer, SerializableControl *control) const OVERRIDE FINAL;
    virtual void deserialize(ByteBuffer *buffer, DeserializableControl *control) OVERRIDE FINAL;

    //! Allocate a new instance
    //! @version Added after 7.0.0
    std::tr1::shared_ptr<PVUnion> build() const;
    
protected:
   Union();
   Union(StringArray const & fieldNames, FieldConstPtrArray const & fields, std::string const & id = defaultId());
private:
   StringArray fieldNames;
   FieldConstPtrArray fields;
   std::string id;

   FieldConstPtr getFieldImpl(const std::string& fieldName, bool throws) const;
   void dumpFields(std::ostream& o) const;

   friend class FieldCreate;
   friend class Structure;
   EPICS_NOT_COPYABLE(Union)
};

class FieldCreate;
typedef std::tr1::shared_ptr<FieldCreate> FieldCreatePtr;

class FieldBuilder;
typedef std::tr1::shared_ptr<FieldBuilder> FieldBuilderPtr;

/**
 * @brief Interface for in-line creating of introspection interfaces.
 *
 * One instance can be used to create multiple @c Field instances.
 * An instance of this object must not be used concurrently (an object has a state).
 * @author mse
 */
class epicsShareClass FieldBuilder :
    public std::tr1::enable_shared_from_this<FieldBuilder>
{
public:
    //! Create a new instance of in-line @c Field builder.
    //! @version Added after 7.0.0
    static FieldBuilderPtr begin();
    //! Create a new instance of in-line @c Field builder pre-initialized with and existing Structure
    static FieldBuilderPtr begin(StructureConstPtr S);

        /**
         * Set ID of an object to be created.
         * @param id id to be set.
     * @return this instance of a @c FieldBuilder.
         */
        FieldBuilderPtr setId(std::string const & id);

    /**
     * Add a @c Scalar.
     * @param name name of the array.
     * @param scalarType type of a scalar to add.
     * @return this instance of a @c FieldBuilder.
     */
    FieldBuilderPtr add(std::string const & name, ScalarType scalarType);

    /**
     * Add a @c BoundedString.
     * @param name name of the array.
     * @param maxLength a string maximum length.
     * @return this instance of a @c FieldBuilder.
     */
    FieldBuilderPtr addBoundedString(std::string const & name, std::size_t maxLength) PVD_DEPRECATED_52;

    /**
     * Add a @c Field (e.g. @c Structure, @c Union).
     * @param name name of the array.
     * @param field a field to add.
     * @return this instance of a @c FieldBuilder.
     */
    FieldBuilderPtr add(std::string const & name, FieldConstPtr const & field);

    /**
     * Add variable size array of @c Scalar elements.
     * @param name name of the array.
     * @param scalarType type of a scalar element.
     * @return this instance of a @c FieldBuilder.
     */
    FieldBuilderPtr addArray(std::string const & name, ScalarType scalarType);
    
    /**
     * Add fixed-size array of @c Scalar elements.
     * @param name name of the array.
     * @param scalarType type of a scalar element.
     * @param size Array fixed size.
     * @return this instance of a @c FieldBuilder.
     */
    FieldBuilderPtr addFixedArray(std::string const & name, ScalarType scalarType, std::size_t size) PVD_DEPRECATED_52;

    /**
     * Add bounded-size array of @c Scalar elements.
     * @param name name of the array.
     * @param scalarType type of a scalar element.
     * @param bound Array maximum capacity (size).
     * @return this instance of a @c FieldBuilder.
     */
    FieldBuilderPtr addBoundedArray(std::string const & name, ScalarType scalarType, std::size_t bound) PVD_DEPRECATED_52;

    /**
     * Add array of @c Field elements.
     * @param name name of the array.
     * @param element a type of an array element.
     * @return this instance of a @c FieldBuilder.
     */
    FieldBuilderPtr addArray(std::string const & name, FieldConstPtr const & element);

    /**
     * Create a @c Structure.
     * This resets this instance state and allows new @c Field instance to be created.
     * @return a new instance of a @c Structure.
     */
    StructureConstPtr createStructure();
    
    /**
     * Create an @c Union.
     * This resets this instance state and allows new @c Field instance to be created.
     * @return a new instance of an @c Union.
     */
    UnionConstPtr createUnion();

    /**
     * Add new nested @c Structure.
     * endNested()  method must be called
     * to complete creation of the nested @c Structure.
     * @param name nested structure name.
     * @return a new instance of a @c FieldBuilder is returned.
     * @see #endNested()
     */
    FieldBuilderPtr addNestedStructure(std::string const & name); 
    
    /**
     * Add new nested @c Union.
     * endNested() method must be called
     * to complete creation of the nested @c Union.
     * @param name nested union name.
     * @return a new instance of a @c FieldBuilder is returned.
     * @see #endNested()
     */
    FieldBuilderPtr addNestedUnion(std::string const & name);
    
    /**
     * Add new nested @c Structure[].
     * endNested() method must be called
     * to complete creation of the nested @c Structure.
     * @param name nested structure name.
     * @return a new instance of a @c FieldBuilder is returned.
     * @see #endNested()
     */
    FieldBuilderPtr addNestedStructureArray(std::string const & name); 
    
    /**
     * Add new nested @c Union[].
     * endNested() method must be called
     * to complete creation of the nested @c Union.
     * @param name nested union name.
     * @return a new instance of a @c FieldBuilder is returned.
     * @see #endNested()
     */
    FieldBuilderPtr addNestedUnionArray(std::string const & name);

    /**
     * Complete the creation of a nested object.
     * @see #addNestedStructure(std::string const & name)
     * @see #addNestedUnion(std::string const & name)
     * @return a previous (parent) @c FieldBuilder.
     */
    FieldBuilderPtr endNested();

private:
    FieldBuilder();
    FieldBuilder(const Structure*);
    FieldBuilder(const FieldBuilderPtr & _parentBuilder, const std::string& name, const Structure*);
    FieldBuilder(const FieldBuilderPtr & _parentBuilder, const std::string& name, const StructureArray*);
    FieldBuilder(const FieldBuilderPtr & _parentBuilder, const std::string& name, const Union*);
    FieldBuilder(const FieldBuilderPtr & _parentBuilder, const std::string& name, const UnionArray*);
    FieldBuilder(FieldBuilderPtr const & parentBuilder,
                        std::string const & nestedName,
                        Type nestedClassToBuild, bool nestedArray);

    const Field *findField(const std::string& name, Type ftype);
                        
        void reset();
        FieldConstPtr createFieldInternal(Type type);

    friend class FieldCreate;
    
    const FieldCreatePtr fieldCreate;

        std::string id;
        bool idSet;

    StringArray fieldNames;
    FieldConstPtrArray fields;
    
    const FieldBuilderPtr parentBuilder;
    const Type nestedClassToBuild;
    const std::string nestedName;
    const bool nestedArray;
    const bool createNested; // true - endNested() creates in parent, false - endNested() appends to parent
};

namespace detail {
struct field_factory;
}

/**
 * @brief This is a singleton class for creating introspection interfaces.
 *
 */
class epicsShareClass FieldCreate {
    friend struct detail::field_factory;
public:
    static const FieldCreatePtr &getFieldCreate();
        /**
         * Create a new instance of in-line @c Field builder.
         * @return a new instance of a @c FieldBuilder.
         */
        FieldBuilderPtr createFieldBuilder() const;
    /**
     * Create a new instance of in-line @c Field builder pre-initialized with and existing Structure
     * @return a new instance of a @c FieldBuilder.
     */
    FieldBuilderPtr createFieldBuilder(StructureConstPtr S) const;
    /**
     * Create a @c ScalarField.
     * @param scalarType The scalar type.
     * @return a @c Scalar interface for the newly created object.
     * @throws IllegalArgumentException if an illegal type is specified.
     */
    ScalarConstPtr createScalar(ScalarType scalarType) const;
    /**
     * Create a @c BoundedString.
     * @param maxLength a string maximum length.
     * @return a @c BoundedString interface for the newly created object.
     * @throws IllegalArgumentException if maxLength == 0.
     */
    BoundedStringConstPtr createBoundedString(std::size_t maxLength) const PVD_DEPRECATED_52;
    /**
     * Create an @c Array field, variable size array.
     * @param elementType The @c ScalarType for array elements
     * @return An @c Array Interface for the newly created object.
     */
    ScalarArrayConstPtr createScalarArray(ScalarType elementType) const;
    /*
     * Create an @c Array field, fixed size array.
     * @param elementType The @c ScalarType for array elements
     * @param size Fixed array size.
     * @return An @c Array Interface for the newly created object.
     */
    ScalarArrayConstPtr createFixedScalarArray(ScalarType elementType, std::size_t size) const PVD_DEPRECATED_52;
    /**
     * Create an @c Array field, bounded size array.
     * @param elementType The @c ScalarType for array elements
     * @param bound Array maximum capacity.
     * @return An @c Array Interface for the newly created object.
     */
     ScalarArrayConstPtr createBoundedScalarArray(ScalarType elementType, std::size_t bound) const PVD_DEPRECATED_52;
     /**
      * Create an @c Array field that is has element type @c Structure
      * @param structure The @c Structure for each array element.
      * @return An @c Array Interface for the newly created object.
      */
    StructureArrayConstPtr createStructureArray(StructureConstPtr const & structure) const;
    /**
     * Create a @c Structure field.
     * @return a @c Structure interface for the newly created object.
     */
    StructureConstPtr createStructure () const;
    /**
     * Create a @c Structure field.
     * @param fieldNames the names of the fields for the structure.
     * @param fields The array of @c Field objects for the structure.
     * @return a @c Structure interface for the newly created object.
     */
    StructureConstPtr createStructure (
        StringArray const & fieldNames,
        FieldConstPtrArray const & fields) const;
    /**
     * Create a @c Structure field with identification string.
     * @param id The identification string for the structure.
     * @param fieldNames the names of the fields for the structure.
     * @param fields The array of @c Field objects for the structure.
     * @return a @c Structure interface for the newly created object.
     */
    StructureConstPtr createStructure (
        std::string const & id,
        StringArray const & fieldNames,
        FieldConstPtrArray const & fields) const;
     /**
      * Create an @c Array field that is has element type @c Union
      * @param punion The @c Union for each array element.
      * @return An @c Array Interface for the newly created object.
      */
    UnionArrayConstPtr createUnionArray(UnionConstPtr const & punion) const;
    /**
     * Create a variant @c UnionArray (aka any type) field.
     * @return a @c UnionArray interface for the newly created object.
     */
    UnionArrayConstPtr createVariantUnionArray() const;
    /**
     * Create a variant @c Union (aka any type) field.
     * @return a @c Union interface for the newly created object.
     */
    UnionConstPtr createVariantUnion() const;
    /**
     * Create a @c Union field.
     * @param fieldNames the names of the fields for the union.
     * @param fields The @c Field for each fields for the union.
     * @return a @c Union interface for the newly created object.
     */
    UnionConstPtr createUnion (
        StringArray const & fieldNames,
        FieldConstPtrArray const & fields) const;
    /**
     * Create a @c Union field with identification string.
     * @param id The identification string for the union.
     * @param fieldNames the names of the fields for the union.
     * @param fields The array of @c Field objects for the union.
     * @return a @c Union interface for the newly created object.
     */
    UnionConstPtr createUnion (
        std::string const & id,
        StringArray const & fieldNames,
        FieldConstPtrArray const & fields) const;
    /**
     * Append a field to a structure.
     * @param structure The structure to which the field is appended.
     * @param fieldName The name of the field.
     * @param field The field.
     * @return a @c Structure interface for the newly created object.
     */
    StructureConstPtr appendField(
        StructureConstPtr const & structure,
        std::string const & fieldName, FieldConstPtr const & field) const;
    /**
     * Append fields to a structure.
     * @param structure The structure to which the fields appended.
     * @param fieldNames The names of the fields.
     * @param fields The fields.
     * @return a @c Structure interface for the newly created object.
     */
    StructureConstPtr appendFields(
        StructureConstPtr const & structure,
        StringArray const & fieldNames,
        FieldConstPtrArray const & fields) const;
    /**
     * Deserialize @c Field instance from given byte buffer.
     * @param buffer Buffer containing serialized @c Field instance. 
     * @param control Deserialization control instance.
     * @return a deserialized @c Field instance.
     */
    FieldConstPtr deserialize(ByteBuffer* buffer, DeserializableControl* control) const;
        
private:
    FieldCreate();

    // const after ctor
    std::vector<ScalarConstPtr> scalars;
    std::vector<ScalarArrayConstPtr> scalarArrays;
    UnionConstPtr variantUnion;
    UnionArrayConstPtr variantUnionArray;

    mutable Mutex mutex;
    typedef std::multimap<unsigned int, Field*> cache_t;
    mutable cache_t cache;

    struct Helper;
    friend class Field;
    EPICS_NOT_COPYABLE(FieldCreate)
};

/**
 * Get the single class that implements FieldCreate,
 * @return The fieldCreate factory.
 */
FORCE_INLINE const FieldCreatePtr& getFieldCreate() {
    return FieldCreate::getFieldCreate();
}

/** Define a compile time mapping from
 * type to enum value.
 @code
  ScalarType code = (ScalarType)ScalarTypeID<int8>::value;
  assert(code==pvByte);
 @endcode
 *
 * For unspecified types this evaluates to an invalid ScalarType
 * value (eg -1).
 */
template<typename T>
struct ScalarTypeID {};

/**
 * Static mapping from ScalarType enum to value type.
 @code
   typename ScalarTypeTraits<pvByte>::type value = 4;
 @endcode
 */
template<ScalarType ID>
struct ScalarTypeTraits {};

#define OP(ENUM, TYPE) \
template<> struct ScalarTypeTraits<ENUM> {typedef TYPE type;}; \
template<> struct ScalarTypeID<TYPE> { enum {value=ENUM}; }; \
template<> struct ScalarTypeID<const TYPE> { enum {value=ENUM}; };

OP(pvBoolean, boolean)
OP(pvByte, int8)
OP(pvShort, int16)
OP(pvInt, int32)
OP(pvLong, int64)
OP(pvUByte, uint8)
OP(pvUShort, uint16)
OP(pvUInt, uint32)
OP(pvULong, uint64)
OP(pvFloat, float)
OP(pvDouble, double)
OP(pvString, std::string)
#undef OP

bool epicsShareExtern compare(const Field&, const Field&);
bool epicsShareExtern compare(const Scalar&, const Scalar&);
bool epicsShareExtern compare(const ScalarArray&, const ScalarArray&);
bool epicsShareExtern compare(const Structure&, const Structure&);
bool epicsShareExtern compare(const StructureArray&, const StructureArray&);
bool epicsShareExtern compare(const Union&, const Union&);
bool epicsShareExtern compare(const UnionArray&, const UnionArray&);
bool epicsShareExtern compare(const BoundedString&, const BoundedString&);

/** Equality with other Field
 *
 * The creation process of class FieldCreate ensures that identical field definitions
 * will share the same instance.  So pointer equality is sufficient to show defintion
 * equality.  If in doubt, compare() will do an full test.
 */
#define MAKE_COMPARE(CLASS) \
static FORCE_INLINE bool operator==(const CLASS& a, const CLASS& b) {return (void*)&a==(void*)&b;} \
static FORCE_INLINE bool operator!=(const CLASS& a, const CLASS& b) {return !(a==b);}

MAKE_COMPARE(Field)
MAKE_COMPARE(Scalar)
MAKE_COMPARE(ScalarArray)
MAKE_COMPARE(Structure)
MAKE_COMPARE(StructureArray)
MAKE_COMPARE(Union)
MAKE_COMPARE(UnionArray)
MAKE_COMPARE(BoundedString)

#undef MAKE_COMPARE
}}

/**
 * stream support for Field
 */
namespace std{
    epicsShareExtern std::ostream& operator<<(std::ostream& o, const epics::pvData::Field *ptr);
}

#endif  /* PVINTROSPECT_H */
