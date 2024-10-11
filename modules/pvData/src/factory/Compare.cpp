/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author mes
 */

#include <algorithm>
#include <iterator>
#include <sstream>

#define epicsExportSharedSymbols
#include <pv/pvData.h>

using std::string;

namespace epics { namespace pvData {

// Introspection object comparison

/** Field equality conditions:
 * 1) same instance
 * 2) same type (field and scalar/element), same name, same subfields (if any)
 */
bool compare(const Field& a, const Field& b)
{
    if(&a==&b)
        return true;
    if(a.getType()!=b.getType())
        return false;
    switch(a.getType()) {
    case scalar: {
        const Scalar &A=static_cast<const Scalar&>(a);
        const Scalar &B=static_cast<const Scalar&>(b);
        return compare(A, B);
    }
    case scalarArray: {
            const ScalarArray &A=static_cast<const ScalarArray&>(a);
            const ScalarArray &B=static_cast<const ScalarArray&>(b);
            return compare(A, B);
        }
    case structure: {
            const Structure &A=static_cast<const Structure&>(a);
            const Structure &B=static_cast<const Structure&>(b);
            return compare(A, B);
        }
    case structureArray: {
            const StructureArray &A=static_cast<const StructureArray&>(a);
            const StructureArray &B=static_cast<const StructureArray&>(b);
            return compare(A, B);
        }
    case union_: {
            const Union &A=static_cast<const Union&>(a);
            const Union &B=static_cast<const Union&>(b);
            return compare(A, B);
        }
    case unionArray: {
            const UnionArray &A=static_cast<const UnionArray&>(a);
            const UnionArray &B=static_cast<const UnionArray&>(b);
            return compare(A, B);
        }
    default:
        throw std::logic_error("Invalid Field type in comparison");
    }
}

bool compare(const Scalar& a, const Scalar& b)
{
    if(&a==&b)
        return true;
    return a.getScalarType()==b.getScalarType();
}

bool compare(const ScalarArray& a, const ScalarArray& b)
{
    if(&a==&b)
        return true;
    return a.getElementType()==b.getElementType();
}

bool compare(const Structure& a, const Structure& b)
{
    if(&a==&b)
        return true;
    if (a.getID()!=b.getID())
        return false;
    size_t nflds=a.getNumberFields();
    if (b.getNumberFields()!=nflds)
        return false;

    // std::equals does not work, since FieldConstPtrArray is an array of shared_pointers
    FieldConstPtrArray const & af = a.getFields();
    FieldConstPtrArray const & bf = b.getFields();
    for (size_t i = 0; i < nflds; i++)
        if (*(af[i].get()) != *(bf[i].get()))
            return false;

    StringArray const & an = a.getFieldNames();
    StringArray const & bn = b.getFieldNames();
    return std::equal( an.begin(), an.end(), bn.begin() );
}

bool compare(const StructureArray& a, const StructureArray& b)
{
    return *(a.getStructure().get())==*(b.getStructure().get());
}

bool compare(const Union& a, const Union& b)
{
    if(&a==&b)
        return true;
    if (a.getID()!=b.getID())
        return false;
    size_t nflds=a.getNumberFields();
    if (b.getNumberFields()!=nflds)
        return false;

    // std::equals does not work, since FieldConstPtrArray is an array of shared_pointers
    FieldConstPtrArray const & af = a.getFields();
    FieldConstPtrArray const & bf = b.getFields();
    for (size_t i = 0; i < nflds; i++)
        if (*(af[i].get()) != *(bf[i].get()))
            return false;

    StringArray const & an = a.getFieldNames();
    StringArray const & bn = b.getFieldNames();
    return std::equal( an.begin(), an.end(), bn.begin() );
}

bool compare(const UnionArray& a, const UnionArray& b)
{
    return *(a.getUnion().get())==*(b.getUnion().get());
}

bool compare(const BoundedString& a, const BoundedString& b)
{
    if(&a==&b)
        return true;
    return a.getMaximumLength()==b.getMaximumLength();
}

// PVXXX object comparison

namespace {

// fully typed comparisons

template<typename T>
bool compareScalar(const PVScalarValue<T>* left, const PVScalarValue<T>* right)
{
    return left->get()==right->get();
}

template<typename T>
bool compareArray(const PVValueArray<T>* left, const PVValueArray<T>* right)
{
    typename PVValueArray<T>::const_svector lhs(left->view()), rhs(right->view());
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// partially typed comparisons

bool compareField(const PVScalar* left, const PVScalar* right)
{
    ScalarType lht = left->getScalar()->getScalarType();
    if(lht != right->getScalar()->getScalarType())
        return false;
    switch(lht) {
#define OP(ENUM, TYPE) case ENUM: return compareScalar(static_cast<const PVScalarValue<TYPE>*>(left), static_cast<const PVScalarValue<TYPE>*>(right))
    OP(pvBoolean, uint8);
    OP(pvUByte, uint8);
    OP(pvByte, int8);
    OP(pvUShort, uint16);
    OP(pvShort, int16);
    OP(pvUInt, uint32);
    OP(pvInt, int32);
    OP(pvULong, uint64);
    OP(pvLong, int64);
    OP(pvFloat, float);
    OP(pvDouble, double);
#undef OP
    case pvString: {
            const PVString *a=static_cast<const PVString*>(left), *b=static_cast<const PVString*>(right);
            return a->get()==b->get();
        }
    }
    throw std::logic_error("PVScalar with invalid scalar type!");
}

bool compareField(const PVScalarArray* left, const PVScalarArray* right)
{
    ScalarType lht = left->getScalarArray()->getElementType();
    if(lht != right->getScalarArray()->getElementType())
        return false;

    if(left->getLength()!=right->getLength())
        return false;

    switch(lht) {
#define OP(ENUM, TYPE) case ENUM: return compareArray(static_cast<const PVValueArray<TYPE>*>(left), static_cast<const PVValueArray<TYPE>*>(right))
    OP(pvBoolean, uint8);
    OP(pvUByte, uint8);
    OP(pvByte, int8);
    OP(pvUShort, uint16);
    OP(pvShort, int16);
    OP(pvUInt, uint32);
    OP(pvInt, int32);
    OP(pvULong, uint64);
    OP(pvLong, int64);
    OP(pvFloat, float);
    OP(pvDouble, double);
    OP(pvString, string);
#undef OP
    }
    throw std::logic_error("PVScalarArray with invalid element type!");
}

bool compareField(const PVStructure* left, const PVStructure* right)
{
    StructureConstPtr ls = left->getStructure();

    if(*ls!=*right->getStructure())
        return false;

    const PVFieldPtrArray& lf = left->getPVFields();
    const PVFieldPtrArray& rf = right->getPVFields();

    for(size_t i=0, nfld=ls->getNumberFields(); i<nfld; i++) {
        if(*lf[i]!=*rf[i])
            return false;
    }
    return true;
}

bool compareField(const PVStructureArray* left, const PVStructureArray* right)
{
    if(*left->getStructureArray()->getStructure()
        != *right->getStructureArray()->getStructure())
        return false;

    PVStructureArray::const_svector ld=left->view(), rd=right->view();

    if(ld.size()!=rd.size())
        return false;

    PVStructureArray::const_svector::const_iterator lit, lend, rit;

    for(lit=ld.begin(), lend=ld.end(), rit=rd.begin();
        lit!=lend;
        ++lit, ++rit)
    {
        // element can be null
        if (!(*lit) || !(*rit))
        {
            if (*lit || *rit)
                return false;
        }
        else if (**lit != **rit)
            return false;
    }
    return true;
}

bool compareField(const PVUnion* left, const PVUnion* right)
{
    UnionConstPtr ls = left->getUnion();

    if(*ls!=*right->getUnion())
        return false;

    if (ls->isVariant())
    {
        const PVField::const_shared_pointer& lval = left->get();
        if (lval.get() == 0)
            return right->get().get() == 0;
        else
            return *(lval.get()) == *(right->get().get());
    }
    else
    {
        int32 lix = left->getSelectedIndex();
        if (lix == right->getSelectedIndex())
        {
            if (lix == PVUnion::UNDEFINED_INDEX || *(left->get()) == *(right->get().get()))
                return true;
            else
                return false;
        }
        else
            return false;
    }
}

bool compareField(const PVUnionArray* left, const PVUnionArray* right)
{
    if(*left->getUnionArray()->getUnion()
        != *right->getUnionArray()->getUnion())
        return false;

    PVUnionArray::const_svector ld=left->view(), rd=right->view();

    if(ld.size()!=rd.size())
        return false;

    PVUnionArray::const_svector::const_iterator lit, lend, rit;

    for(lit=ld.begin(), lend=ld.end(), rit=rd.begin();
        lit!=lend;
        ++lit, ++rit)
    {
        // element can be null
        if (!(*lit) || !(*rit))
        {
            if (*lit || *rit)
                return false;
        }
        else if (**lit != **rit)
            return false;
    }
    return true;
}

} // end namespace

// untyped comparison

bool operator==(const PVField& left, const PVField& right)
{
    if(&left == &right)
        return true;

    Type lht = left.getField()->getType();
    if(lht != right.getField()->getType())
        return false;

    switch(lht) {
    case scalar: return compareField(static_cast<const PVScalar*>(&left), static_cast<const PVScalar*>(&right));
    case scalarArray: return compareField(static_cast<const PVScalarArray*>(&left), static_cast<const PVScalarArray*>(&right));
    case structure: return compareField(static_cast<const PVStructure*>(&left), static_cast<const PVStructure*>(&right));
    case structureArray: return compareField(static_cast<const PVStructureArray*>(&left), static_cast<const PVStructureArray*>(&right));
    case union_: return compareField(static_cast<const PVUnion*>(&left), static_cast<const PVUnion*>(&right));
    case unionArray: return compareField(static_cast<const PVUnionArray*>(&left), static_cast<const PVUnionArray*>(&right));
    }
    throw std::logic_error("PVField with invalid type!");
}

}} // namespace epics::pvData
