/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <memory>

#define epicsExportSharedSymbols
#include <pv/pvData.h>
#include <pv/valueBuilder.h>

namespace epics{namespace pvData{

struct ValueBuilder::child {
    virtual ~child() {}
    Type type;
    child(Type t) : type(t) {}
    virtual void build(const std::string& name, FieldBuilderPtr& builder)=0;
    virtual void store(const PVFieldPtr& val)=0;

};

struct ValueBuilder::child_struct : public ValueBuilder::child
{
    virtual ~child_struct() {}
    child_struct(ValueBuilder *par, const std::string& id)
        :child(structure)
        ,builder(par, id)
    {}

    ValueBuilder builder;
    virtual void build(const std::string& name, FieldBuilderPtr& builder) OVERRIDE FINAL
    {
        FieldBuilderPtr nest(builder->addNestedStructure(name));
        buildStruct(this->builder, nest);
        builder = nest->endNested();
    }
    virtual void store(const PVFieldPtr& val) OVERRIDE FINAL
    {
        if(val->getField()->getType()!=structure)
            THROW_EXCEPTION2(std::logic_error, "Structure type mis-match");
        PVStructurePtr str(std::tr1::static_pointer_cast<PVStructure>(val));
        storeStruct(builder, str);
    }

    static
    void buildStruct(const ValueBuilder& self, FieldBuilderPtr& builder);
    static
    void storeStruct(const ValueBuilder& self, const PVStructurePtr& val);

    static
    void fillStruct(ValueBuilder& self, const PVStructure& val);
};

struct ValueBuilder::child_scalar_base : public ValueBuilder::child
{
    virtual ~child_scalar_base() {}
    ScalarType stype;
    child_scalar_base(ScalarType s) : child(scalar), stype(s) {}

    virtual void build(const std::string& name, FieldBuilderPtr& builder) OVERRIDE FINAL
    {
        builder->add(name, stype);
    }
};

struct ValueBuilder::child_scalar_array : public ValueBuilder::child
{
    virtual ~child_scalar_array() {}
    shared_vector<const void> array;
    child_scalar_array(const shared_vector<const void>& v) : child(scalarArray), array(v) {}

    virtual void build(const std::string& name, FieldBuilderPtr& builder) OVERRIDE FINAL
    {
        builder->addArray(name, array.original_type());
    }
    virtual void store(const PVFieldPtr& val) OVERRIDE FINAL
    {
        if(val->getField()->getType()!=scalarArray)
            THROW_EXCEPTION2(std::logic_error, "Scalar Array type mis-match");

        PVScalarArrayPtr arr(std::tr1::static_pointer_cast<PVScalarArray>(val));
        arr->putFrom(array);
    }
};

template <typename T>
struct ValueBuilder::child_scalar : public ValueBuilder::child_scalar_base
{
    virtual ~child_scalar() {}
    T value;
    child_scalar(const void* v) : child_scalar_base(static_cast<ScalarType>(ScalarTypeID<T>::value)), value(*static_cast<const T*>(v)) {}

    virtual void store(const PVFieldPtr& val) OVERRIDE FINAL
    {
        if(val->getField()->getType()!=scalar)
            THROW_EXCEPTION2(std::logic_error, "Scalar type mis-match");

        PVScalarPtr scalar(std::tr1::static_pointer_cast<PVScalar>(val));
        scalar->putFrom(value);
    }
};

ValueBuilder::ValueBuilder(const std::string &id) :parent(0),id(id) {}

void ValueBuilder::child_struct::fillStruct(ValueBuilder& self, const PVStructure& val)
{
    StructureConstPtr type(val.getStructure());
    const StringArray& field = type->getFieldNames();
    for(StringArray::const_iterator it=field.begin(), end=field.end(); it!=end; ++it)
    {
        PVField::const_shared_pointer sub(val.getSubField(*it));
        assert(sub);
        FieldConstPtr subtype(sub->getField());
        switch(subtype->getType()) {
        case scalar:
        {
            const PVScalar* subs(static_cast<const PVScalar*>(sub.get()));
            ScalarType stype = subs->getScalar()->getScalarType();
            switch(stype) {
#define STYPE(stype) case pv##stype: { const PV ##stype* ptr(static_cast<const PV##stype*>(subs)); PV##stype::value_type temp(ptr->get()); self._add(*it, pv##stype, &temp); } break
            STYPE(Boolean);
            STYPE(Byte);
            STYPE(Short);
            STYPE(Int);
            STYPE(Long);
            STYPE(UByte);
            STYPE(UShort);
            STYPE(UInt);
            STYPE(ULong);
            STYPE(Float);
            STYPE(Double);
            STYPE(String);
#undef STYPE
            }
        }
            break;
        case structure:
            self._add(*it, *static_cast<const PVStructure*>(sub.get()));
            break;
        default:
            THROW_EXCEPTION2(std::runtime_error, "ValueBuilder can only clone scalar and structure");
        }
    }
}

ValueBuilder::ValueBuilder(const PVStructure& clone) :parent(0)
{
    StructureConstPtr ctype(clone.getStructure());
    id = ctype->getID();
    child_struct::fillStruct(*this, clone);
}


ValueBuilder::ValueBuilder(ValueBuilder* par, const std::string &id)
    :parent(par)
    ,id(id)
{}

ValueBuilder::~ValueBuilder()
{
    for(children_t::const_iterator it=children.begin(), end=children.end(); it!=end; ++it)
        delete it->second;
    children.clear();
}

void ValueBuilder::_add(const std::string& name, const PVStructure& V)
{
    StructureConstPtr T(V.getStructure());
    ValueBuilder& self = addNested(name, structure, T->getID());
    child_struct::fillStruct(self, V);
    self.endNested();
}

void ValueBuilder::_add(const std::string& name, ScalarType stype, const void *V)
{
    const children_t::iterator it(children.find(name));
    if(it!=children.end()) {
        if(it->second->type!=scalar && it->second->type!=scalarArray)
            THROW_EXCEPTION2(std::logic_error, "Not allowed to replace field.  wrong type");
    }

    epics::auto_ptr<child> store;
    switch(stype) {
#define STYPE(stype) case stype: store.reset(new child_scalar<ScalarTypeTraits<stype>::type>(V)); break
    STYPE(pvBoolean);
    STYPE(pvByte);
    STYPE(pvShort);
    STYPE(pvInt);
    STYPE(pvLong);
    STYPE(pvUByte);
    STYPE(pvUShort);
    STYPE(pvUInt);
    STYPE(pvULong);
    STYPE(pvFloat);
    STYPE(pvDouble);
    STYPE(pvString);
#undef STYPE
    }
    if(!store.get())
        THROW_EXCEPTION2(std::logic_error, "Unhandled ScalarType");

    if(it!=children.end()) {
        delete it->second;
        children.erase(it);
    }
    children[name] = store.get();
    store.release();
}

void ValueBuilder::_add(const std::string& name, const shared_vector<const void>& V)
{
    const children_t::iterator it(children.find(name));
    if(it!=children.end()) {
        if(it->second->type!=scalar && it->second->type!=scalarArray)
            THROW_EXCEPTION2(std::logic_error, "Not allowed to replace field.  wrong type");
    }

    epics::auto_ptr<child> store(new child_scalar_array(V));

    children[name] = store.get();
    store.release();
}

ValueBuilder& ValueBuilder::addNested(const std::string& name, Type type, const std::string &id)
{
    if(type!=structure)
        THROW_EXCEPTION2(std::invalid_argument, "addNested() only supports structure");
    child_struct *sub;
    children_t::const_iterator it(children.find(name));
    if(it==children.end()) {
        epics::auto_ptr<child_struct> store(new child_struct(this, id));
        sub = store.get();
        children[name] = store.get();
        store.release();
    } else if(it->second->type==structure) {
        sub = static_cast<child_struct*>(it->second);
    } else {
        std::ostringstream msg;
        msg<<"Can't replace non-struct field '"<<name<<"' with struct";
        THROW_EXCEPTION2(std::invalid_argument, msg.str());
    }
    sub->builder.id = id;
    return sub->builder;
}

ValueBuilder& ValueBuilder::endNested()
{
    if(!parent) {
        THROW_EXCEPTION2(std::logic_error, "Can't end top of structure");
    }
    return *parent;
}

void ValueBuilder::child_struct::buildStruct(const ValueBuilder& self, FieldBuilderPtr& builder)
{
    if(!self.id.empty())
        builder->setId(self.id);

    for(children_t::const_iterator it=self.children.begin(), end=self.children.end(); it!=end; ++it)
    {
        it->second->build(it->first, builder);
    }
}

void ValueBuilder::child_struct::storeStruct(const ValueBuilder& self, const PVStructurePtr& val)
{
    for(children_t::const_iterator it=self.children.begin(), end=self.children.end(); it!=end; ++it)
    {
        it->second->store(val->getSubFieldT(it->first));
    }
}

PVStructure::shared_pointer ValueBuilder::buildPVStructure() const
{
    if(parent)
        THROW_EXCEPTION2(std::logic_error, "Only top level structure may be built.  Missing endNested() ?");

    StructureConstPtr type;
    {
        FieldBuilderPtr tbuild(getFieldCreate()->createFieldBuilder());

        child_struct::buildStruct(*this, tbuild);

        type = tbuild->createStructure();
    }

    PVStructure::shared_pointer root(type->build());

    child_struct::storeStruct(*this, root);

    return root;
}

}}// namespace epics::pvData
