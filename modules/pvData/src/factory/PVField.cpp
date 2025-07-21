/*PVField.cpp*/
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

#define epicsExportSharedSymbols
#include <pv/lock.h>
#include <pv/pvData.h>
#include <pv/factory.h>
#include <pv/reftrack.h>

using std::tr1::const_pointer_cast;
using std::size_t;
using std::string;

namespace epics { namespace pvData {

size_t PVField::num_instances;

PVField::PVField(FieldConstPtr field)
: parent(NULL),field(field),
  fieldOffset(0), nextFieldOffset(0),
  immutable(false)
{
    REFTRACE_INCREMENT(num_instances);
}

PVField::~PVField()
{
    REFTRACE_DECREMENT(num_instances);
}


size_t PVField::getFieldOffset() const
{
    if(nextFieldOffset==0) computeOffset(this);
    return fieldOffset;
}

size_t PVField::getNextFieldOffset() const
{
    if(nextFieldOffset==0) computeOffset(this);
    return nextFieldOffset;
}

size_t PVField::getNumberFields() const
{
    if(nextFieldOffset==0) computeOffset(this);
    return (nextFieldOffset - fieldOffset);
}


void PVField::setImmutable() {immutable = true;}

void PVField::postPut()
{
   if(postHandler) postHandler->postPut();
}

void PVField::setPostHandler(PostHandlerPtr const &handler)
{
    if(postHandler) {
        if(postHandler.get()==handler.get()) return;
        throw std::logic_error(
            "PVField::setPostHandler a postHandler is already registered");

    }
    postHandler = handler;
}

void PVField::setParentAndName(PVStructure * xxx,string const & name)
{
    parent = xxx;
    fieldName = name;
}

bool PVField::equals(PVField &pv)
{
    return pv==*this;
}

std::ostream& operator<<(std::ostream& o, const PVField& f)
{
    return f.dumpValue(o);
};

string PVField::getFullName() const
{
    string ret(fieldName);
    for(const PVField *fld=getParent(); fld; fld=fld->getParent())
    {
        if(fld->getFieldName().size()==0) break;
        ret = fld->getFieldName() + '.' + ret;
    }
    return ret;
}

void PVField::computeOffset(const PVField   *  pvField) {
    const PVStructure * pvTop = pvField->getParent();
    if(pvTop==NULL) {
        if(pvField->getField()->getType()!=structure) {
           PVField *xxx = const_cast<PVField *>(pvField);
           xxx->fieldOffset = 0;
           xxx->nextFieldOffset = 1;
           return;
        }
        pvTop = static_cast<const PVStructure *>(pvField);
    } else {
        while(pvTop->getParent()!=NULL) pvTop = pvTop->getParent();
    }
    size_t offset = 0;
    size_t nextOffset = 1;
    const PVFieldPtrArray & pvFields = pvTop->getPVFields();
    for(size_t i=0; i < pvTop->getStructure()->getNumberFields(); i++) {
        offset = nextOffset;
        PVField *pvField = pvFields[i].get();
        FieldConstPtr field = pvField->getField();
        switch(field->getType()) {
        case scalar:
        case scalarArray:
        case structureArray:
        case union_:
        case unionArray: {
            nextOffset++;
            pvField->fieldOffset = offset;
            pvField->nextFieldOffset = nextOffset;
            break;
        }
        case structure: {
            pvField->computeOffset(pvField,offset);
            nextOffset = pvField->getNextFieldOffset();
        }
        }
    }
    PVField *top = (PVField *)pvTop;
    PVField *xxx = const_cast<PVField *>(top);
    xxx->fieldOffset = 0;
    xxx->nextFieldOffset = nextOffset;
}

void PVField::computeOffset(const PVField   *  pvField,size_t offset) {
    size_t beginOffset = offset;
    size_t nextOffset = offset + 1;
    const PVStructure *pvStructure = static_cast<const PVStructure *>(pvField);
    const PVFieldPtrArray & pvFields = pvStructure->getPVFields();
    for(size_t i=0; i < pvStructure->getStructure()->getNumberFields(); i++) {
        offset = nextOffset;
        PVField *pvSubField = pvFields[i].get();
        FieldConstPtr field = pvSubField->getField();
        switch(field->getType()) {
            case scalar:
            case scalarArray:
            case structureArray:
            case union_:
            case unionArray: {
                nextOffset++;
                pvSubField->fieldOffset = offset;
                pvSubField->nextFieldOffset = nextOffset;
                break;
            }
            case structure: {
                pvSubField->computeOffset(pvSubField,offset);
                nextOffset = pvSubField->getNextFieldOffset();
            }
        }
    }
    PVField *xxx = const_cast<PVField *>(pvField);
    xxx->fieldOffset = beginOffset;
    xxx->nextFieldOffset = nextOffset;
}

void PVField::copy(const PVField& from)
{
    if(isImmutable())
        throw std::invalid_argument("destination is immutable");

    if (getField() != from.getField())
        throw std::invalid_argument("field types do not match");

    copyUnchecked(from);
}

void PVField::copyUnchecked(const PVField& from)
{
    assert(getField()==from.getField());

    switch(getField()->getType())
    {
    case scalar:
        {
             const PVScalar* fromS = static_cast<const PVScalar*>(&from);
             PVScalar* toS = static_cast<PVScalar*>(this);
             toS->copyUnchecked(*fromS);
             break;
        }
    case scalarArray:
        {
             const PVScalarArray* fromS = static_cast<const PVScalarArray*>(&from);
             PVScalarArray* toS = static_cast<PVScalarArray*>(this);
             toS->copyUnchecked(*fromS);
             break;
        }
    case structure:
        {
             const PVStructure* fromS = static_cast<const PVStructure*>(&from);
             PVStructure* toS = static_cast<PVStructure*>(this);
             toS->copyUnchecked(*fromS);
             break;
        }
    case structureArray:
        {
             const PVStructureArray* fromS = static_cast<const PVStructureArray*>(&from);
             PVStructureArray* toS = static_cast<PVStructureArray*>(this);
             toS->copyUnchecked(*fromS);
             break;
        }
    case union_:
        {
             const PVUnion* fromS = static_cast<const PVUnion*>(&from);
             PVUnion* toS = static_cast<PVUnion*>(this);
             toS->copyUnchecked(*fromS);
             break;
        }
    case unionArray:
        {
             const PVUnionArray* fromS = static_cast<const PVUnionArray*>(&from);
             PVUnionArray* toS = static_cast<PVUnionArray*>(this);
             toS->copyUnchecked(*fromS);
             break;
        }
    default:
        {
            throw std::logic_error("PVField::copy unknown type");
        }
    }
}


}}
