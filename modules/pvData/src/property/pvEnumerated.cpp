/* pvEnumerated.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <string>
#include <stdexcept>

#define epicsExportSharedSymbols
#include <pv/pvType.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvEnumerated.h>

using std::tr1::static_pointer_cast;
using std::string;

namespace epics { namespace pvData {

string PVEnumerated::notFound("No enumerated structure found");
string PVEnumerated::notAttached("Not attached to an enumerated structure");

bool PVEnumerated::attach(PVFieldPtr const & pvField)
{
    if(pvField->getField()->getType()!=structure) return false;
    PVStructurePtr pvStructure = static_pointer_cast<PVStructure>(pvField);
    pvIndex = pvStructure->getSubField<PVInt>("index");
    if(pvIndex.get()==NULL) return false;
    PVStringArrayPtr pvStringArray = pvStructure->getSubField<PVStringArray>("choices");
    if(pvStringArray.get()==NULL) {
        pvIndex.reset();
        return false;
    }
    pvChoices = pvStringArray;
    return true;
}

void PVEnumerated::detach()
{
    pvIndex.reset();
    pvChoices.reset();
}

bool PVEnumerated::isAttached() {
    if(pvIndex.get()==NULL) return false;
    return true;
}

bool PVEnumerated::setIndex(int32 index)
{
    if(pvIndex.get()==NULL ) {
         throw std::logic_error(notAttached);
    }
    if(pvIndex->isImmutable()) return false;
    pvIndex->put(index);
    return true;
}

int32 PVEnumerated::getIndex()
{
    if(pvIndex.get()==NULL ) {
         throw std::logic_error(notAttached);
    }
    return pvIndex->get();
}

string PVEnumerated::getChoice()
{
    if(pvIndex.get()==NULL ) {
         throw std::logic_error(notAttached);
    }
    size_t index = pvIndex->get();
    const PVStringArray::const_svector& data(pvChoices->view());
    if(/*index<0 ||*/ index>=data.size()) {
        string nullString;
        return nullString;
    }
    return data[index];
}

bool PVEnumerated::choicesMutable()
{
    if(pvIndex.get()==NULL ) {
         throw std::logic_error(notAttached);
    }
    return pvChoices->isImmutable();
}

int32 PVEnumerated::getNumberChoices()
{
    if(pvIndex.get()==NULL ) {
         throw std::logic_error(notAttached);
    }
    return static_cast<int32>(pvChoices->getLength());
}

bool PVEnumerated:: setChoices(const StringArray & choices)
{
    if(pvIndex.get()==NULL ) {
         throw std::logic_error(notAttached);
    }
    if(pvChoices->isImmutable()) return false;
    PVStringArray::svector data(choices.size());
    std::copy(choices.begin(), choices.end(), data.begin());
    pvChoices->replace(freeze(data));
    return true;
}


}}
