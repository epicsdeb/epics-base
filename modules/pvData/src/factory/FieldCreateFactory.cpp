/*FieldCreateFactory.cpp*/
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
#include <stdexcept>
#include <sstream>

#include <epicsString.h>
#include <epicsMutex.h>
#include <epicsThread.h>

#define epicsExportSharedSymbols
#include <pv/reftrack.h>
#include <pv/lock.h>
#include <pv/pvIntrospect.h>
#include <pv/factory.h>
#include <pv/serializeHelper.h>
#include <pv/thread.h>
#include <pv/pvData.h>

using std::tr1::static_pointer_cast;
using std::size_t;
using std::string;

namespace epics { namespace pvData {

size_t Field::num_instances;


struct Field::Helper {
    static unsigned hash(Field *fld) {
        std::ostringstream key;
        // hash the output of operator<<()
        // not efficient, but stable within this process.
        key<<(*fld);
        unsigned H = epicsStrHash(key.str().c_str(), 0xbadc0de1);
        fld->m_hash = H;
        return H;
    }
};

struct FieldCreate::Helper {
    template<typename FLD>
    static void cache(const FieldCreate *create, std::tr1::shared_ptr<FLD>& ent) {
        unsigned hash = Field::Helper::hash(ent.get());

        Lock G(create->mutex);
        // we examine raw pointers stored in create->cache, which is safe under create->mutex

        std::pair<cache_t::iterator, cache_t::iterator> itp(create->cache.equal_range(hash));
        for(; itp.first!=itp.second; ++itp.first) {
            Field* cent(itp.first->second);
            FLD* centx(dynamic_cast<FLD*>(cent));
            if(centx && compare(*centx, *ent)) {
                try{
                    ent = std::tr1::static_pointer_cast<FLD>(cent->shared_from_this());
                    return;
                }catch(std::tr1::bad_weak_ptr&){
                    // we're racing destruction.
                    // add a new entry.
                    // Field::~Field is in the process of removing this old one.
                    continue;
                }
            }
        }

        create->cache.insert(std::make_pair(hash, ent.get()));
        // cache cleaned from Field::~Field
    }
};

Field::Field(Type type)
    : m_fieldType(type)
    , m_hash(0)
{
    REFTRACE_INCREMENT(num_instances);
}

Field::~Field() {
    REFTRACE_DECREMENT(num_instances);
}

void Field::cacheCleanup()
{
    const FieldCreatePtr& create(getFieldCreate());

    Lock G(create->mutex);

    std::pair<FieldCreate::cache_t::iterator, FieldCreate::cache_t::iterator> itp(create->cache.equal_range(m_hash));
    for(; itp.first!=itp.second; ++itp.first) {
        Field* cent(itp.first->second);
        if(cent==this) {
            create->cache.erase(itp.first);
            return;
        }
    }
}

std::tr1::shared_ptr<PVField> Field::build() const
{
    FieldConstPtr self(shared_from_this());
    return getPVDataCreate()->createPVField(self);
}

std::ostream& operator<<(std::ostream& o, const Field& f)
{
    return f.dump(o);
};

Scalar::Scalar(ScalarType scalarType)
       : Field(scalar),scalarType(scalarType)
{
    if(scalarType<0 || scalarType>MAX_SCALAR_TYPE)
        THROW_EXCEPTION2(std::invalid_argument, "Can't construct Scalar from invalid ScalarType");
}

Scalar::~Scalar()
{
    cacheCleanup();
}

std::ostream& Scalar::dump(std::ostream& o) const
{
    return o << format::indent() << getID();
}

string Scalar::getID() const
{
    static const string idScalarLUT[] = {
        "boolean", // pvBoolean
        "byte",    // pvByte
        "short",   // pvShort
        "int",     // pvInt
        "long",    // pvLong
        "ubyte",   // pvUByte
        "ushort",  // pvUShort
        "uint",    // pvUInt
        "ulong",   // pvULong
        "float",   // pvFloat
        "double",  // pvDouble
        "string"   // pvString
    };
    return idScalarLUT[scalarType];
}

int8 Scalar::getTypeCodeLUT(ScalarType scalarType)
{
    static const int8 typeCodeLUT[] = {
        0x00, // pvBoolean
        0x20, // pvByte
        0x21, // pvShort
        0x22, // pvInt
        0x23, // pvLong
        0x24, // pvUByte
        0x25, // pvUShort
        0x26, // pvUInt
        0x27, // pvULong
        0x42, // pvFloat
        0x43, // pvDouble
        0x60  // pvString
    };
   return typeCodeLUT[scalarType];
}


void Scalar::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    buffer->putByte(getTypeCodeLUT(scalarType));
}

void Scalar::deserialize(ByteBuffer* /*buffer*/, DeserializableControl* /*control*/) {
    // must be done via FieldCreate
    throw std::runtime_error("not valid operation, use FieldCreate::deserialize instead");
}


std::tr1::shared_ptr<PVScalar> Scalar::build() const
{
    return getPVDataCreate()->createPVScalar(std::tr1::static_pointer_cast<const Scalar>(shared_from_this()));
}



std::string BoundedString::getID() const
{
    std::ostringstream id;
    id << Scalar::getID() << '(' << maxLength << ')';
    return id.str();
}

void BoundedString::serialize(ByteBuffer *buffer, SerializableControl *control) const
{
    control->ensureBuffer(1);
    buffer->putByte(0x83);
    SerializeHelper::writeSize(maxLength, buffer, control);
}

std::size_t BoundedString::getMaximumLength() const
{
    return maxLength;
}

BoundedString::BoundedString(std::size_t maxStringLength) :
    Scalar(pvString), maxLength(maxStringLength)
{
    if (maxLength == 0)
        THROW_EXCEPTION2(std::invalid_argument, "maxLength == 0");
}

BoundedString::~BoundedString()
{
    cacheCleanup();
}


static void serializeStructureField(const Structure* structure, ByteBuffer* buffer, SerializableControl* control)
{
    // to optimize default (non-empty) IDs optimization
    // empty IDs are not allowed
    string id = structure->getID();
    if (id == Structure::DEFAULT_ID)    // TODO slow comparison
        SerializeHelper::serializeString(string(), buffer, control);
    else
        SerializeHelper::serializeString(id, buffer, control);

    FieldConstPtrArray const & fields = structure->getFields();
    StringArray const & fieldNames = structure->getFieldNames();
    std::size_t len = fields.size();
    SerializeHelper::writeSize(len, buffer, control);
    for (std::size_t i = 0; i < len; i++)
    {
        SerializeHelper::serializeString(fieldNames[i], buffer, control);
        control->cachedSerialize(fields[i], buffer);
    }
}

static StructureConstPtr deserializeStructureField(const FieldCreate* fieldCreate, ByteBuffer* buffer, DeserializableControl* control)
{
    string id = SerializeHelper::deserializeString(buffer, control);
    const std::size_t size = SerializeHelper::readSize(buffer, control);
    FieldConstPtrArray fields; fields.reserve(size);
    StringArray fieldNames; fieldNames.reserve(size);
    for (std::size_t i = 0; i < size; i++)
    {
        fieldNames.push_back(SerializeHelper::deserializeString(buffer, control));
        fields.push_back(control->cachedDeserialize(buffer));
    }

    if (id.empty())
        return fieldCreate->createStructure(fieldNames, fields);
    else
        return fieldCreate->createStructure(id, fieldNames, fields);
}

static void serializeUnionField(const Union* punion, ByteBuffer* buffer, SerializableControl* control)
{
    // to optimize default (non-empty) IDs optimization
    // empty IDs are not allowed
    string id = punion->getID();
    if (id == Union::DEFAULT_ID)    // TODO slow comparison
        SerializeHelper::serializeString(string(), buffer, control);
    else
        SerializeHelper::serializeString(id, buffer, control);

    FieldConstPtrArray const & fields = punion->getFields();
    StringArray const & fieldNames = punion->getFieldNames();
    std::size_t len = fields.size();
    SerializeHelper::writeSize(len, buffer, control);
    for (std::size_t i = 0; i < len; i++)
    {
        SerializeHelper::serializeString(fieldNames[i], buffer, control);
        control->cachedSerialize(fields[i], buffer);
    }
}

static UnionConstPtr deserializeUnionField(const FieldCreate* fieldCreate, ByteBuffer* buffer, DeserializableControl* control)
{
    string id = SerializeHelper::deserializeString(buffer, control);
    const std::size_t size = SerializeHelper::readSize(buffer, control);
    FieldConstPtrArray fields; fields.reserve(size);
    StringArray fieldNames; fieldNames.reserve(size);
    for (std::size_t i = 0; i < size; i++)
    {
        fieldNames.push_back(SerializeHelper::deserializeString(buffer, control));
        fields.push_back(control->cachedDeserialize(buffer));
    }

    if (id.empty())
        return fieldCreate->createUnion(fieldNames, fields);
    else
        return fieldCreate->createUnion(id, fieldNames, fields);
}

Array::Array(Type type)
    : Field(type)
{
}

Array::~Array() {}

ScalarArray::ScalarArray(ScalarType elementType)
    : Array(scalarArray),
      elementType(elementType)
{
    if(elementType<0 || elementType>MAX_SCALAR_TYPE)
        throw std::invalid_argument("Can't construct ScalarArray from invalid ScalarType");
}

ScalarArray::~ScalarArray()
{
    cacheCleanup();
}

const string ScalarArray::getIDScalarArrayLUT() const
{
    static const string idScalarArrayLUT[] = {
        "boolean[]", // pvBoolean
        "byte[]",    // pvByte
        "short[]",   // pvShort
        "int[]",     // pvInt
        "long[]",    // pvLong
        "ubyte[]",   // pvUByte
        "ushort[]",  // pvUShort
        "uint[]",    // pvUInt
        "ulong[]",   // pvULong
        "float[]",   // pvFloat
        "double[]",  // pvDouble
        "string[]"   // pvString
    };
    return idScalarArrayLUT[elementType];
}

string ScalarArray::getID() const
{
    return getIDScalarArrayLUT();
}

std::ostream& ScalarArray::dump(std::ostream& o) const
{
    return o << format::indent() << getID();
}

void ScalarArray::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    buffer->putByte((int8)0x08 | Scalar::getTypeCodeLUT(elementType));
}

void ScalarArray::deserialize(ByteBuffer* /*buffer*/, DeserializableControl* /*control*/) {
    throw std::runtime_error("not valid operation, use FieldCreate::deserialize instead");
}

std::tr1::shared_ptr<PVScalarArray> ScalarArray::build() const
{
    return getPVDataCreate()->createPVScalarArray(std::tr1::static_pointer_cast<const ScalarArray>(shared_from_this()));
}


BoundedScalarArray::~BoundedScalarArray()
{
    cacheCleanup();
}

BoundedScalarArray::BoundedScalarArray(ScalarType elementType, size_t size)
    : ScalarArray(elementType),
      size(size)
{
}

string BoundedScalarArray::getID() const
{
    char buffer[32];
    sprintf(buffer, "%s<%zu>", ScalarTypeFunc::name(getElementType()), size);
    return string(buffer);
}

void BoundedScalarArray::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    buffer->putByte((int8)0x10 | Scalar::getTypeCodeLUT(getElementType()));
    SerializeHelper::writeSize(size, buffer, control);
}


FixedScalarArray::~FixedScalarArray()
{
    cacheCleanup();
}

FixedScalarArray::FixedScalarArray(ScalarType elementType, size_t size)
    : ScalarArray(elementType),
      size(size)
{
}

string FixedScalarArray::getID() const
{
    char buffer[32];
    sprintf(buffer, "%s[%zu]", ScalarTypeFunc::name(getElementType()), size);
    return string(buffer);
}

void FixedScalarArray::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    buffer->putByte((int8)0x18 | Scalar::getTypeCodeLUT(getElementType()));
    SerializeHelper::writeSize(size, buffer, control);
}



StructureArray::StructureArray(StructureConstPtr const & structure)
: Array(structureArray),pstructure(structure)
{
}

StructureArray::~StructureArray()
{
    cacheCleanup();
}

string StructureArray::getID() const
{
    return pstructure->getID() + "[]";
}

std::ostream& StructureArray::dump(std::ostream& o) const
{
    o << format::indent() << getID() << std::endl;
    {
        format::indent_scope s(o);
        o << *pstructure;
    }
    return o;
}

void StructureArray::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    buffer->putByte((int8)0x88);
    control->cachedSerialize(pstructure, buffer);
}

void StructureArray::deserialize(ByteBuffer* /*buffer*/, DeserializableControl* /*control*/) {
    throw std::runtime_error("not valid operation, use FieldCreate::deserialize instead");
}

std::tr1::shared_ptr<PVValueArray<std::tr1::shared_ptr<PVStructure> > > StructureArray::build() const
{
    return getPVDataCreate()->createPVStructureArray(std::tr1::static_pointer_cast<const StructureArray>(shared_from_this()));
}

UnionArray::UnionArray(UnionConstPtr const & _punion)
: Array(unionArray),punion(_punion)
{
}

UnionArray::~UnionArray()
{
    cacheCleanup();
}

string UnionArray::getID() const
{
    return punion->getID() + "[]";
}

std::ostream& UnionArray::dump(std::ostream& o) const
{
    o << format::indent() << getID() << std::endl;
    {
        format::indent_scope s(o);
        o << *punion;
    }
    return o;
}

void UnionArray::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    if (punion->isVariant())
    {
        // unrestricted/variant union
        buffer->putByte((int8)0x8A);
    }
    else
    {
        buffer->putByte((int8)0x89);
        control->cachedSerialize(punion, buffer);
    }
}

void UnionArray::deserialize(ByteBuffer* /*buffer*/, DeserializableControl* /*control*/) {
    throw std::runtime_error("not valid operation, use FieldCreate::deserialize instead");
}

std::tr1::shared_ptr<PVValueArray<std::tr1::shared_ptr<PVUnion> > > UnionArray::build() const
{
    return getPVDataCreate()->createPVUnionArray(std::tr1::static_pointer_cast<const UnionArray>(shared_from_this()));
}

const string Structure::DEFAULT_ID = Structure::defaultId();

const string & Structure::defaultId()
{
    static const string id = "structure";
    return id;
}

Structure::Structure (
    StringArray const & fieldNames,
    FieldConstPtrArray const & infields,
    string const & inid)
: Field(structure),
      fieldNames(fieldNames),
      fields(infields),
      id(inid)
{
    if(inid.empty()) {
        THROW_EXCEPTION2(std::invalid_argument, "Can't construct Structure, id is empty string");
    }
    if(fieldNames.size()!=fields.size()) {
        THROW_EXCEPTION2(std::invalid_argument, "Can't construct Structure, fieldNames.size()!=fields.size()");
    }
    size_t number = fields.size();
    for(size_t i=0; i<number; i++) {
        const string& name = fieldNames[i];
        if(name.empty()) {
            THROW_EXCEPTION2(std::invalid_argument, "Can't construct Structure, empty string in fieldNames");
        }
        if(fields[i].get()==NULL)
            THROW_EXCEPTION2(std::invalid_argument, "Can't construct Structure, NULL in fields");
        // look for duplicates
        for(size_t j=i+1; j<number; j++) {
            string otherName = fieldNames[j];
            int result = name.compare(otherName);
            if(result==0) {
                string  message("Can't construct Structure, duplicate fieldName ");
                message += name;
                THROW_EXCEPTION2(std::invalid_argument, message);
            }
        }
    }
}

Structure::~Structure()
{
    cacheCleanup();
}


string Structure::getID() const
{
    return id;
}

FieldConstPtr  Structure::getField(string const & fieldName) const {
    for(size_t i=0, N=fields.size(); i<N; i++) {
        if(fieldName==fieldNames[i]) {
            return fields[i];
        }
    }
    return FieldConstPtr();
}

size_t Structure::getFieldIndex(string const &fieldName) const {
    size_t numberFields = fields.size();
    for(size_t i=0; i<numberFields; i++) {
        FieldConstPtr pfield = fields[i];
        int result = fieldName.compare(fieldNames[i]);
        if(result==0) return i;
    }
    return -1;
}

FieldConstPtr Structure::getFieldImpl(string const & fieldName, bool throws) const {
    for(size_t i=0, N=fields.size(); i<N; i++)
        if(fieldName==fieldNames[i])
            return fields[i];

    if (throws) {
        std::stringstream ss;
        ss << "Failed to get field: "
           << fieldName << " (not found)";
        throw std::runtime_error(ss.str());
    } else {
        return FieldConstPtr();
    }
}

std::ostream& Structure::dump(std::ostream& o) const
{
    o << format::indent() << getID() << std::endl;
    {
        format::indent_scope s(o);
        dumpFields(o);
    }
    return o;
}

void Structure::dumpFields(std::ostream& o) const
{
    size_t numberFields = fields.size();
    for(size_t i=0; i<numberFields; i++) {
        FieldConstPtr pfield = fields[i];
        o << format::indent() << pfield->getID() << ' ' << fieldNames[i] << std::endl;
        switch(pfield->getType()) {
            case scalar:
            case scalarArray:
                break;
            case structure:
            {
                Field const *xxx = pfield.get();
                Structure const *pstruct = static_cast<Structure const*>(xxx);
                format::indent_scope s(o);
                pstruct->dumpFields(o);
                break;
            }
            case structureArray:
            {
                format::indent_scope s(o);
                Field const *xxx = pfield.get();
                StructureArray const *pstructureArray = static_cast<StructureArray const*>(xxx);
                o << *pstructureArray->getStructure();
                break;
            }
            case union_:
            {
                Field const *xxx = pfield.get();
                Union const *punion = static_cast<Union const*>(xxx);
                format::indent_scope s(o);
                punion->dumpFields(o);
                break;
            }
            case unionArray:
            {
                format::indent_scope s(o);
                Field const *xxx = pfield.get();
                UnionArray const *punionArray = static_cast<UnionArray const*>(xxx);
                o << *punionArray->getUnion();
                break;
            }
        }
    }
}

void Structure::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    buffer->putByte((int8)0x80);
    serializeStructureField(this, buffer, control);
}

void Structure::deserialize(ByteBuffer* /*buffer*/, DeserializableControl* /*control*/) {
    throw std::runtime_error("not valid operation, use FieldCreate::deserialize instead");
}

std::tr1::shared_ptr<PVStructure> Structure::build() const
{
    return getPVDataCreate()->createPVStructure(std::tr1::static_pointer_cast<const Structure>(shared_from_this()));
}

const string Union::DEFAULT_ID = Union::defaultId();

const string & Union::defaultId()
{
    static const string id = "union";
    return id;
}

const string Union::ANY_ID = Union::anyId();

const string & Union::anyId()
{
    static const string id = "any";
    return id;
}

Union::Union ()
: Field(union_),
      fieldNames(),
      fields(),
      id(anyId())
{
}


Union::Union (
    StringArray const & fieldNames,
    FieldConstPtrArray const & infields,
    string const & inid)
: Field(union_),
      fieldNames(fieldNames),
      fields(infields),
      id(inid)
{
    if(inid.empty()) {
        THROW_EXCEPTION2(std::invalid_argument, "Can't construct Union, id is empty string");
    }
    if(fieldNames.size()!=fields.size()) {
        THROW_EXCEPTION2(std::invalid_argument, "Can't construct Union, fieldNames.size()!=fields.size()");
    }
    if(fields.size()==0 && inid!=ANY_ID) {
        THROW_EXCEPTION2(std::invalid_argument, "Can't construct Union, no fields only allowed when id = " + ANY_ID);
    }

    size_t number = fields.size();
    for(size_t i=0; i<number; i++) {
        const string& name = fieldNames[i];
        if(name.empty()) {
            THROW_EXCEPTION2(std::invalid_argument, "Can't construct Union, empty string in fieldNames");
        }
        if(fields[i].get()==NULL)
            THROW_EXCEPTION2(std::invalid_argument, "Can't construct Union, NULL in fields");
        // look for duplicates
        for(size_t j=i+1; j<number; j++) {
            string otherName = fieldNames[j];
            int result = name.compare(otherName);
            if(result==0) {
                string  message("Can't construct Union, duplicate fieldName ");
                message += name;
                THROW_EXCEPTION2(std::invalid_argument, message);
            }
        }
    }
}

Union::~Union()
{
    cacheCleanup();
}

int32 Union::guess(Type t, ScalarType s) const
{
    if(t!=scalar && t!=scalarArray)
        THROW_EXCEPTION2(std::logic_error, "PVUnion::guess() only support scalar and scalarArray");

    int32 ret = -1;
    for(size_t i=0, N=fields.size(); i<N; i++)
    {
        if(fields[i]->getType()!=t)
            continue;

        ScalarType type;
        switch(fields[i]->getType()) {
        case scalar:
            type = static_cast<const Scalar*>(fields[i].get())->getScalarType();
            break;
        case scalarArray:
            type = static_cast<const ScalarArray*>(fields[i].get())->getElementType();
            break;
        default:
            continue;
        }

        if(type==s) {
            // exact match
            ret = i;
            break; // we're done

        } else if(ret==-1) {
            // first partial match
            ret = i;
        }
    }
    return ret;
}

string Union::getID() const
{
    return id;
}

FieldConstPtr  Union::getField(string const & fieldName) const {
    size_t numberFields = fields.size();
    for(size_t i=0; i<numberFields; i++) {
        FieldConstPtr pfield = fields[i];
        int result = fieldName.compare(fieldNames[i]);
        if(result==0) return pfield;
    }
    return FieldConstPtr();
}

size_t Union::getFieldIndex(string const &fieldName) const {
    size_t numberFields = fields.size();
    for(size_t i=0; i<numberFields; i++) {
        FieldConstPtr pfield = fields[i];
        int result = fieldName.compare(fieldNames[i]);
        if(result==0) return i;
    }
    return -1;
}

FieldConstPtr Union::getFieldImpl(string const & fieldName, bool throws) const {
    for(size_t i=0, N=fields.size(); i<N; i++)
        if(fieldName==fieldNames[i])
            return fields[i];

    if (throws) {
        std::stringstream ss;
        ss << "Failed to get field: "
           << fieldName << " (not found)";
        throw std::runtime_error(ss.str());
    } else {
        return FieldConstPtr();
    }
}

std::ostream& Union::dump(std::ostream& o) const
{
    o << format::indent() << getID() << std::endl;
    {
        format::indent_scope s(o);
        dumpFields(o);
    }
    return o;
}

void Union::dumpFields(std::ostream& o) const
{
    size_t numberFields = fields.size();
    for(size_t i=0; i<numberFields; i++) {
        FieldConstPtr pfield = fields[i];
        o << format::indent() << pfield->getID() << ' ' << fieldNames[i] << std::endl;
        switch(pfield->getType()) {
            case scalar:
            case scalarArray:
                break;
            case structure:
            {
                Field const *xxx = pfield.get();
                Structure const *pstruct = static_cast<Structure const*>(xxx);
                format::indent_scope s(o);
                pstruct->dumpFields(o);
                break;
            }
            case structureArray:
            {
                format::indent_scope s(o);
                o << *pfield;
                break;
            }
            case union_:
            {
                Field const *xxx = pfield.get();
                Union const *punion = static_cast<Union const*>(xxx);
                format::indent_scope s(o);
                punion->dumpFields(o);
                break;
            }
            case unionArray:
            {
                format::indent_scope s(o);
                o << *pfield;
                break;
            }
        }
    }
}

void Union::serialize(ByteBuffer *buffer, SerializableControl *control) const {
    control->ensureBuffer(1);
    if (fields.size() == 0)
    {
        // unrestricted/variant union
        buffer->putByte((int8)0x82);
    }
    else
    {
        buffer->putByte((int8)0x81);
        serializeUnionField(this, buffer, control);
    }
}

void Union::deserialize(ByteBuffer* /*buffer*/, DeserializableControl* /*control*/) {
    throw std::runtime_error("not valid operation, use FieldCreate::deserialize instead");
}

std::tr1::shared_ptr<PVUnion> Union::build() const
{
    return getPVDataCreate()->createPVUnion(std::tr1::static_pointer_cast<const Union>(shared_from_this()));
}

FieldBuilder::FieldBuilder()
    :fieldCreate(getFieldCreate())
    ,idSet(false)
    ,nestedClassToBuild(structure)
    ,nestedArray(false)
    ,createNested(true)
{}

FieldBuilder::FieldBuilder(const Structure* S)
    :fieldCreate(getFieldCreate())
    ,id(S->getID())
    ,idSet(!id.empty())
    ,fieldNames(S->getFieldNames())
    ,fields(S->getFields())
    ,parentBuilder()
    ,nestedClassToBuild(structure)
    ,nestedName()
    ,nestedArray(false)
    ,createNested(false)
{}

FieldBuilder::FieldBuilder(const FieldBuilderPtr & _parentBuilder,
                           const std::string& name,
                           const Structure* S)
    :fieldCreate(_parentBuilder->fieldCreate)
    ,id(S->getID())
    ,idSet(!id.empty())
    ,fieldNames(S->getFieldNames())
    ,fields(S->getFields())
    ,parentBuilder(_parentBuilder)
    ,nestedClassToBuild(structure)
    ,nestedName(name)
    ,nestedArray(false)
    ,createNested(false)
{}

FieldBuilder::FieldBuilder(const FieldBuilderPtr & _parentBuilder,
                           const std::string& name,
                           const StructureArray* S)
    :fieldCreate(getFieldCreate())
    ,id(S->getStructure()->getID())
    ,idSet(!id.empty())
    ,fieldNames(S->getStructure()->getFieldNames())
    ,fields(S->getStructure()->getFields())
    ,parentBuilder(_parentBuilder)
    ,nestedClassToBuild(structure)
    ,nestedName(name)
    ,nestedArray(true)
    ,createNested(false)
{}

FieldBuilder::FieldBuilder(const FieldBuilderPtr & _parentBuilder,
                           const std::string& name,
                           const Union* S)
    :fieldCreate(getFieldCreate())
    ,id(S->getID())
    ,idSet(!id.empty())
    ,fieldNames(S->getFieldNames())
    ,fields(S->getFields())
    ,parentBuilder(_parentBuilder)
    ,nestedClassToBuild(union_)
    ,nestedName(name)
    ,nestedArray(false)
    ,createNested(false)
{}

FieldBuilder::FieldBuilder(const FieldBuilderPtr & _parentBuilder,
                           const std::string& name,
                           const UnionArray* S)
    :fieldCreate(getFieldCreate())
    ,id(S->getUnion()->getID())
    ,idSet(!id.empty())
    ,fieldNames(S->getUnion()->getFieldNames())
    ,fields(S->getUnion()->getFields())
    ,parentBuilder(_parentBuilder)
    ,nestedClassToBuild(union_)
    ,nestedName(name)
    ,nestedArray(true)
    ,createNested(false)
{}

FieldBuilder::FieldBuilder(FieldBuilderPtr const & _parentBuilder,
            string const & _nestedName,
            Type _nestedClassToBuild, bool _nestedArray)
    :fieldCreate(_parentBuilder->fieldCreate)
    ,idSet(false)
    ,parentBuilder(_parentBuilder)
    ,nestedClassToBuild(_nestedClassToBuild)
    ,nestedName(_nestedName)
    ,nestedArray(_nestedArray)
    ,createNested(true)
{}

void FieldBuilder::reset()
{
    id.erase();
    idSet = false;
    fieldNames.clear();
    fields.clear();
}

FieldBuilderPtr FieldBuilder::begin()
{
    FieldBuilderPtr ret(new FieldBuilder);
    return ret;
}

FieldBuilderPtr FieldBuilder::begin(StructureConstPtr S)
{
    FieldBuilderPtr ret(new FieldBuilder(S.get()));
    return ret;
}


FieldBuilderPtr FieldBuilder::setId(string const & id)
{
    this->id = id;
    idSet = true;
    return shared_from_this();
}

FieldBuilderPtr FieldBuilder::add(string const & name, ScalarType scalarType)
{
    return add(name, fieldCreate->createScalar(scalarType));
}

FieldBuilderPtr FieldBuilder::addBoundedString(std::string const & name, std::size_t maxLength)
{
    return add(name, fieldCreate->createBoundedString(maxLength));
}

FieldBuilderPtr FieldBuilder::add(string const & name, FieldConstPtr const & field)
{
    const Field* cur = findField(name, field->getType());
    if(!cur) {
        fields.push_back(field); fieldNames.push_back(name);
    } else if(*cur!=*field) {
        THROW_EXCEPTION2(std::runtime_error, "duplicate field name w/ different type : "+name);
    } // else exact duplicate is silently ignored
    return shared_from_this();
}

FieldBuilderPtr FieldBuilder::addArray(string const & name, ScalarType scalarType)
{
    return add(name, fieldCreate->createScalarArray(scalarType));
}

FieldBuilderPtr FieldBuilder::addFixedArray(string const & name, ScalarType scalarType, size_t size)
{
    return add(name, fieldCreate->createFixedScalarArray(scalarType, size));
}

FieldBuilderPtr FieldBuilder::addBoundedArray(string const & name, ScalarType scalarType, size_t size)
{
    return add(name, fieldCreate->createBoundedScalarArray(scalarType, size));
}

FieldBuilderPtr FieldBuilder::addArray(string const & name, FieldConstPtr const & element)
{
    FieldConstPtr fld;
    switch (element->getType())
    {
        case structure:
            fld = fieldCreate->createStructureArray(static_pointer_cast<const Structure>(element));
            break;
        case union_:
            fld = fieldCreate->createUnionArray(static_pointer_cast<const Union>(element));
            break;
        case scalar:

            if (std::tr1::dynamic_pointer_cast<const BoundedString>(element).get())
                THROW_EXCEPTION2(std::invalid_argument, "bounded string arrays are not supported");

            fld = fieldCreate->createScalarArray(static_pointer_cast<const Scalar>(element)->getScalarType());
            break;
        // scalarArray?
        default:
            std::ostringstream msg("unsupported array element type: ");
            msg << element->getType();
            THROW_EXCEPTION2(std::invalid_argument, msg.str());
    }

    return add(name, fld);
}

FieldConstPtr FieldBuilder::createFieldInternal(Type type)
{
    // minor optimization
    if (fieldNames.size() == 0 && type == union_)
        return fieldCreate->createVariantUnion();

    if (type == structure)
    {
        return (idSet) ?
            fieldCreate->createStructure(id, fieldNames, fields) :
            fieldCreate->createStructure(fieldNames, fields);
    }
    else if (type == union_)
    {
        return (idSet) ?
            fieldCreate->createUnion(id, fieldNames, fields) :
            fieldCreate->createUnion(fieldNames, fields);
    }
    else
    {
        std::ostringstream msg("unsupported type: ");
        msg << type;
        THROW_EXCEPTION2(std::invalid_argument, msg.str());
    }
}


StructureConstPtr FieldBuilder::createStructure()
{
    if (parentBuilder.get())
        THROW_EXCEPTION2(std::runtime_error, "createStructure() called in nested FieldBuilder");

    StructureConstPtr field(static_pointer_cast<const Structure>(createFieldInternal(structure)));
    reset();
    return field;
}

UnionConstPtr FieldBuilder::createUnion()
{
    if (parentBuilder.get())
        THROW_EXCEPTION2(std::runtime_error, "createUnion() called in nested FieldBuilder");

    UnionConstPtr field(static_pointer_cast<const Union>(createFieldInternal(union_)));
    reset();
    return field;
}

const Field* FieldBuilder::findField(const std::string& name, Type ftype)
{
    // linear search on the theory that the number of fields is small
    for(size_t i=0; i<fieldNames.size(); i++)
    {
        if(name!=fieldNames[i])
            continue;

        if(fields[i]->getType()!=ftype)
            THROW_EXCEPTION2(std::invalid_argument, "nested field not required type: "+name);

        return fields[i].get();
    }
    return 0;
}

FieldBuilderPtr FieldBuilder::addNestedStructure(string const & name)
{
    const Field *cur = findField(name, structure);
    if(cur) {
        return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name,
                                                static_cast<const Structure*>(cur)));
    }
    return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name, structure, false));
}


FieldBuilderPtr FieldBuilder::addNestedUnion(string const & name)
{
    const Field *cur = findField(name, union_);
    if(cur) {
        return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name,
                                                static_cast<const Union*>(cur)));
    }
    return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name, union_, false));
}


FieldBuilderPtr FieldBuilder::addNestedStructureArray(string const & name)
{
    const Field *cur = findField(name, structureArray);
    if(cur) {
        return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name,
                                                static_cast<const StructureArray*>(cur)));
    }
    return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name, structure, true));
}

FieldBuilderPtr FieldBuilder::addNestedUnionArray(string const & name)
{
    const Field *cur = findField(name, unionArray);
    if(cur) {
        return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name,
                                                static_cast<const UnionArray*>(cur)));
    }
    return FieldBuilderPtr(new FieldBuilder(shared_from_this(), name, union_, true));
}

FieldBuilderPtr FieldBuilder::endNested()
{
    if (!parentBuilder)
        THROW_EXCEPTION2(std::runtime_error, "FieldBuilder::endNested() can only be called to create nested fields");

    FieldConstPtr nestedField = createFieldInternal(nestedClassToBuild);

    if(createNested) {
        if (nestedArray)
            parentBuilder->addArray(nestedName, nestedField);
        else
            parentBuilder->add(nestedName, nestedField);
        return parentBuilder;
    } else {
        for(size_t i=0, N = parentBuilder->fieldNames.size(); i<N; i++)
        {
            if(nestedName!=parentBuilder->fieldNames[i])
                continue;

            if(nestedArray) {
                if(nestedClassToBuild==structure)
                    parentBuilder->fields[i] = fieldCreate->createStructureArray(std::tr1::static_pointer_cast<const Structure>(nestedField));
                else if(nestedClassToBuild==union_)
                    parentBuilder->fields[i] = fieldCreate->createUnionArray(std::tr1::static_pointer_cast<const Union>(nestedField));
                else
                    throw std::logic_error("bad nested class");
            } else {
                parentBuilder->fields[i] = nestedField;
            }
            return parentBuilder;
        }
        // this only reached if bug in ctor
        THROW_EXCEPTION2(std::logic_error, "no nested field field?");
    }
}

FieldBuilderPtr FieldCreate::createFieldBuilder() const
{
    return FieldBuilderPtr(new FieldBuilder());
}

FieldBuilderPtr FieldCreate::createFieldBuilder(StructureConstPtr S) const
{
    FieldBuilderPtr ret(new FieldBuilder(S.get()));
    return ret;
}

ScalarConstPtr FieldCreate::createScalar(ScalarType scalarType) const
{
    if(scalarType<0 || scalarType>MAX_SCALAR_TYPE) {
        std::ostringstream strm("Can't construct Scalar from invalid ScalarType ");
        strm << scalarType;
        THROW_EXCEPTION2(std::invalid_argument, strm.str());
    }

    return scalars[scalarType];
}

BoundedStringConstPtr FieldCreate::createBoundedString(std::size_t maxLength) const
{
    std::tr1::shared_ptr<BoundedString> s(new BoundedString(maxLength));
    Helper::cache(this, s);
    return s;
}

ScalarArrayConstPtr FieldCreate::createScalarArray(ScalarType elementType) const
{
    if(elementType<0 || elementType>MAX_SCALAR_TYPE) {
        std::ostringstream strm("Can't construct ScalarArray from invalid ScalarType ");
        strm << elementType;
        THROW_EXCEPTION2(std::invalid_argument, strm.str());
    }

    return scalarArrays[elementType];
}

ScalarArrayConstPtr FieldCreate::createFixedScalarArray(ScalarType elementType, size_t size) const
{
    if(elementType<0 || elementType>MAX_SCALAR_TYPE) {
        std::ostringstream strm("Can't construct fixed ScalarArray from invalid ScalarType ");
        strm << elementType;
        THROW_EXCEPTION2(std::invalid_argument, strm.str());
    }

    std::tr1::shared_ptr<ScalarArray> s(new FixedScalarArray(elementType, size));
    Helper::cache(this, s);
    return s;
}

ScalarArrayConstPtr FieldCreate::createBoundedScalarArray(ScalarType elementType, size_t size) const
{
    if(elementType<0 || elementType>MAX_SCALAR_TYPE) {
        std::ostringstream strm("Can't construct bounded ScalarArray from invalid ScalarType ");
        strm << elementType;
        THROW_EXCEPTION2(std::invalid_argument, strm.str());
    }

    std::tr1::shared_ptr<ScalarArray> s(new BoundedScalarArray(elementType, size));
    Helper::cache(this, s);
    return s;
}

StructureConstPtr FieldCreate::createStructure () const
{
      StringArray fieldNames;
      FieldConstPtrArray fields;
      return createStructure(fieldNames,fields);
}

namespace {
bool xisalnum(char c)
{
    return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9');
}

void validateFieldName(const std::string& n)
{
    // enforce [A-Za-z_][A-Za-z0-9_]*
    if(n.size()==0)
        throw std::invalid_argument("zero length field names not allowed");
    if(n[0]>='0' && n[0]<='9') {
        std::ostringstream msg;
        msg<<"Field name \""<<n<<"\" must begin with A-Z, a-z, or '_'";
        throw std::invalid_argument(msg.str());
    }
    for(size_t i=0, N=n.size(); i<N; i++)
    {
        char c = n[i];
        if(xisalnum(c)) {}
        else {
            switch(c){
            case '_':
                break;
            default:
            {
                std::ostringstream msg;
                msg<<"Invalid charactor '"<<c<<"' ("<<(int)c<<") in field name \""<<n<<"\" "
                     "must be A-Z, a-z, 0-9, or '_'";
                throw std::invalid_argument(msg.str());
            }
            }
        }
    }
}

void validateFieldNames(const StringArray& l)
{
    for(StringArray::const_iterator it=l.begin(), end=l.end(); it!=end; ++it)
        validateFieldName(*it);
}
}

StructureConstPtr FieldCreate::createStructure (
    StringArray const & fieldNames,FieldConstPtrArray const & fields) const
{
      validateFieldNames(fieldNames);
      std::tr1::shared_ptr<Structure> sp(new Structure(fieldNames,fields));
      Helper::cache(this, sp);
      return sp;
}

StructureConstPtr FieldCreate::createStructure (
    string const & id,
    StringArray const & fieldNames,
    FieldConstPtrArray const & fields) const
{
      validateFieldNames(fieldNames);
      std::tr1::shared_ptr<Structure> sp(new Structure(fieldNames,fields,id));
      Helper::cache(this, sp);
      return sp;
}

StructureArrayConstPtr FieldCreate::createStructureArray(
    StructureConstPtr const & structure) const
{
     std::tr1::shared_ptr<StructureArray> sp(new StructureArray(structure));
     Helper::cache(this, sp);
     return sp;
}

UnionConstPtr FieldCreate::createUnion (
    StringArray const & fieldNames,FieldConstPtrArray const & fields) const
{
      validateFieldNames(fieldNames);
      std::tr1::shared_ptr<Union> sp(new Union(fieldNames,fields));
      Helper::cache(this, sp);
      return sp;
}

UnionConstPtr FieldCreate::createUnion (
    string const & id,
    StringArray const & fieldNames,
    FieldConstPtrArray const & fields) const
{
      validateFieldNames(fieldNames);
      std::tr1::shared_ptr<Union> sp(new Union(fieldNames,fields,id));
      Helper::cache(this, sp);
      return sp;
}

UnionConstPtr FieldCreate::createVariantUnion () const
{
    return variantUnion;
}

UnionArrayConstPtr FieldCreate::createUnionArray(
    UnionConstPtr const & punion) const
{
     std::tr1::shared_ptr<UnionArray> sp(new UnionArray(punion));
     Helper::cache(this, sp);
     return sp;
}

UnionArrayConstPtr FieldCreate::createVariantUnionArray () const
{
    return variantUnionArray;
}

StructureConstPtr FieldCreate::appendField(
    StructureConstPtr const & structure,
    string const & fieldName,
    FieldConstPtr const & field) const
{
    StringArray const & oldNames = structure->getFieldNames();
    FieldConstPtrArray const & oldFields = structure->getFields();
    size_t oldLen = oldNames.size();
    StringArray newNames(oldLen+1);
    FieldConstPtrArray newFields(oldLen+1);
    for(size_t i = 0; i<oldLen; i++) {
        newNames[i] = oldNames[i];
        newFields[i] = oldFields[i];
    }
    newNames[oldLen] = fieldName;
    newFields[oldLen] = field;
    return createStructure(structure->getID(),newNames,newFields);
}

StructureConstPtr FieldCreate::appendFields(
    StructureConstPtr const & structure,
    StringArray const & fieldNames,
    FieldConstPtrArray const & fields) const
{
    validateFieldNames(fieldNames);
    StringArray const & oldNames = structure->getFieldNames();
    FieldConstPtrArray const & oldFields = structure->getFields();
    size_t oldLen = oldNames.size();
    size_t extra = fieldNames.size();
    StringArray newNames(oldLen+extra);
    FieldConstPtrArray newFields(oldLen+extra);
    for(size_t i = 0; i<oldLen; i++) {
        newNames[i] = oldNames[i];
        newFields[i] = oldFields[i];
    }
    for(size_t i = 0; i<extra; i++) {
        newNames[oldLen +i] = fieldNames[i];
        newFields[oldLen +i] = fields[i];
    }
    return createStructure(structure->getID(),newNames,newFields);
}


static int decodeScalar(int8 code)
{
    static const int integerLUT[] =
    {
        pvByte,   // 8-bits
        pvShort,  // 16-bits
        pvInt,    // 32-bits
        pvLong,   // 64-bits
        pvUByte,  // unsigned 8-bits
        pvUShort, // unsigned 16-bits
        pvUInt,   // unsigned 32-bits
        pvULong   // unsigned 64-bits
    };

    static const int floatLUT[] =
    {
        -1, // reserved
        -1, // 16-bits
        pvFloat,   // 32-bits
        pvDouble,  // 64-bits
        -1,
        -1,
        -1,
        -1
    };
    // bits 7-5
    switch (code >> 5)
    {
    case 0: return pvBoolean;
    case 1: return integerLUT[code & 0x07];
    case 2: return floatLUT[code & 0x07];
    case 3: return pvString;
    default: return -1;
    }
}

FieldConstPtr FieldCreate::deserialize(ByteBuffer* buffer, DeserializableControl* control) const
{
    control->ensureData(1);
    int8 code = buffer->getByte();
    if (code == -1)
        return FieldConstPtr();

    int typeCode = code & 0xE7;
    int scalarOrArray = code & 0x18;
    bool notArray = (scalarOrArray == 0);
    if (notArray)
    {
        if (typeCode < 0x80)
        {
            // Type type = Type.scalar;
            int scalarType = decodeScalar(code);
            if (scalarType == -1)
                throw std::invalid_argument("invalid scalar type encoding");
            return scalars[scalarType];
        }
        else if (typeCode == 0x80)
        {
            // Type type = Type.structure;
            return deserializeStructureField(this, buffer, control);
        }
        else if (typeCode == 0x81)
        {
            // Type type = Type.union;
            return deserializeUnionField(this, buffer, control);
        }
        else if (typeCode == 0x82)
        {
            // Type type = Type.union; variant union (aka any type)
            return variantUnion;
        }
        else if (typeCode == 0x83)
        {
            // TODO cache?
            // bounded string

            size_t size = SerializeHelper::readSize(buffer, control);

            std::tr1::shared_ptr<Field> sp(
                        new BoundedString(size));
            Helper::cache(this, sp);
            return sp;
        }
        else
            throw std::invalid_argument("invalid type encoding");
    }
    else // array
    {
        bool isVariable = (scalarOrArray == 0x08);
        // bounded == 0x10;
        bool isFixed = (scalarOrArray == 0x18);

        size_t size = (isVariable ? 0 : SerializeHelper::readSize(buffer, control));

        if (typeCode < 0x80)
        {
            // Type type = Type.scalarArray;
            int scalarType = decodeScalar(code);
            if (scalarType == -1)
                throw std::invalid_argument("invalid scalarArray type encoding");
            if (isVariable)
                return scalarArrays[scalarType];
            else if (isFixed)
            {
                // TODO use std::make_shared
                std::tr1::shared_ptr<Field> sp(
                            new FixedScalarArray(static_cast<epics::pvData::ScalarType>(scalarType), size));
                Helper::cache(this, sp);
                return sp;
            }
            else
            {
                // TODO use std::make_shared
                std::tr1::shared_ptr<Field> sp(
                            new BoundedScalarArray(static_cast<epics::pvData::ScalarType>(scalarType), size));
                Helper::cache(this, sp);
                return sp;
            }
        }
        else if (typeCode == 0x80)
        {
            // TODO fixed and bounded array support
            if (!isVariable)
                throw std::invalid_argument("fixed and bounded structure array not supported");

            // Type type = Type.structureArray;
            StructureConstPtr elementStructure = std::tr1::static_pointer_cast<const Structure>(control->cachedDeserialize(buffer));
            // TODO use std::make_shared
            std::tr1::shared_ptr<Field> sp(new StructureArray(elementStructure));
            Helper::cache(this, sp);
            return sp;
        }
        else if (typeCode == 0x81)
        {
            // TODO fixed and bounded array support
            if (!isVariable)
                throw std::invalid_argument("fixed and bounded structure array not supported");

            // Type type = Type.unionArray;
            UnionConstPtr elementUnion = std::tr1::static_pointer_cast<const Union>(control->cachedDeserialize(buffer));
            // TODO use std::make_shared
            std::tr1::shared_ptr<Field> sp(new UnionArray(elementUnion));
            Helper::cache(this, sp);
            return sp;
        }
        else if (typeCode == 0x82)
        {
            // TODO fixed and bounded array support
            if (!isVariable)
                throw std::invalid_argument("fixed and bounded structure array not supported");

            // Type type = Type.unionArray; variant union (aka any type)
            return variantUnionArray;
        }
        else
            throw std::invalid_argument("invalid type encoding");
    }
}

namespace detail {
struct field_factory {
    FieldCreatePtr fieldCreate;
    field_factory() :fieldCreate(new FieldCreate()) {
        registerRefCounter("Field", &Field::num_instances);
        registerRefCounter("Thread", &Thread::num_instances);
    }
};
}

static detail::field_factory* field_factory_s;
static epicsThreadOnceId field_factory_once = EPICS_THREAD_ONCE_INIT;

static void field_factory_init(void*)
{
    try {
        field_factory_s = new detail::field_factory;
    }catch(std::exception& e){
        std::cerr<<"Error initializing getFieldCreate() : "<<e.what()<<"\n";
    }
}

const FieldCreatePtr& FieldCreate::getFieldCreate()
{
    epicsThreadOnce(&field_factory_once, &field_factory_init, 0);
    if(!field_factory_s->fieldCreate)
        throw std::logic_error("getFieldCreate() not initialized");
    return field_factory_s->fieldCreate;
}

FieldCreate::FieldCreate()
{
    for (int i = 0; i <= MAX_SCALAR_TYPE; i++)
    {
        std::tr1::shared_ptr<Scalar> sp(new Scalar(static_cast<ScalarType>(i)));
        Helper::cache(this, sp);
        scalars.push_back(sp);

        std::tr1::shared_ptr<ScalarArray> spa(new ScalarArray(static_cast<ScalarType>(i)));
        Helper::cache(this, spa);
        scalarArrays.push_back(spa);
    }

    std::tr1::shared_ptr<Union> su(new Union());
    Helper::cache(this, su);
    variantUnion = su;

    std::tr1::shared_ptr<UnionArray> sua(new UnionArray(variantUnion));
    Helper::cache(this, sua);
    variantUnionArray = sua;
}

}}

namespace std{
    std::ostream& operator<<(std::ostream& o, const epics::pvData::Field *ptr)
    {
        if(ptr) return o << *ptr;
        return o << "nullptr";
    }
}
