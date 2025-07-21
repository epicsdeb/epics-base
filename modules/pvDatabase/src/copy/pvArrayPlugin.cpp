/* pvArrayPlugin.cpp */
/*
 * The License for this software can be found in the file LICENSE that is included with the distribution.
 */

#include <stdlib.h>
#include <pv/pvData.h>
#include <pv/bitSet.h>
#include <pv/convert.h>
#include <pv/pvSubArrayCopy.h>
#define epicsExportSharedSymbols
#include "pv/pvArrayPlugin.h"

using std::string;
using std::size_t;
using std::cout;
using std::endl;
using std::tr1::static_pointer_cast;
using std::vector;
using namespace epics::pvData;

namespace epics { namespace pvCopy{

static ConvertPtr convert = getConvert();
static std::string name("array");

PVArrayPlugin::PVArrayPlugin()
{
}

PVArrayPlugin::~PVArrayPlugin()
{
}

void PVArrayPlugin::create()
{
     static bool firstTime = true;
     if(firstTime) {
         firstTime = false;
         PVArrayPluginPtr pvPlugin = PVArrayPluginPtr(new PVArrayPlugin());
         PVPluginRegistry::registerPlugin(name,pvPlugin);
    }
}

PVFilterPtr PVArrayPlugin::create(
     const std::string & requestValue,
     const PVCopyPtr & pvCopy,
     const PVFieldPtr & master)
{
    return PVArrayFilter::create(requestValue,master);
}

PVArrayFilter::~PVArrayFilter()
{
}

static vector<string> split(string const & colonSeparatedList) {
    string::size_type numValues = 1;
    string::size_type index=0;
    while(true) {
        string::size_type pos = colonSeparatedList.find(':',index);
        if(pos==string::npos) break;
        numValues++;
        index = pos +1;
    }
    vector<string> valueList(numValues,"");
    index=0;
    for(size_t i=0; i<numValues; i++) {
        size_t pos = colonSeparatedList.find(':',index);
        string value = colonSeparatedList.substr(index,pos-index);
        valueList[i] = value;
        index = pos +1;
    }
    return valueList;
}

PVArrayFilterPtr PVArrayFilter::create(
     const std::string & requestValue,
     const PVFieldPtr & masterField)
{
    bool masterIsUnion = false;
    PVUnionPtr pvUnion;
    Type type = masterField->getField()->getType();
    if(type==epics::pvData::union_) {
        pvUnion = std::tr1::static_pointer_cast<PVUnion>(masterField);
        PVFieldPtr pvField = pvUnion->get();
        if(pvField) {
            masterIsUnion = true;
            type = pvField->getField()->getType();
        }
    }
    if(type!=scalarArray) {
        PVArrayFilterPtr filter = PVArrayFilterPtr();
        return filter;
    }
    long start =0;
    long increment =1;
    long end = -1;
    vector<string> values(split(requestValue));
    long num = values.size();
    bool ok = true;
    string value;
    if(num==1) {
        value = values[0];
        start = strtol(value.c_str(),0,10);
    } else if(num==2) {
        value = values[0];
        start = strtol(value.c_str(),0,10);
        value = values[1];
        end = strtol(value.c_str(),0,10);
    } else if(num==3) {
        value = values[0];
        start = strtol(value.c_str(),0,10);
        value = values[1];
        increment = strtol(value.c_str(),0,10);
        value = values[2];
        end = strtol(value.c_str(),0,10);
    } else {
        ok = false;
    }
    if(!ok) {
        PVArrayFilterPtr filter = PVArrayFilterPtr();
        return filter;
    }
    PVScalarArrayPtr masterArray;
    if(masterIsUnion) {
        masterArray = static_pointer_cast<PVScalarArray>(pvUnion->get());
    } else {
        masterArray = static_pointer_cast<PVScalarArray>(masterField);
    }
    PVArrayFilterPtr filter =
         PVArrayFilterPtr(
             new PVArrayFilter(start,increment,end,masterField,masterArray));
    return filter;
}

PVArrayFilter::PVArrayFilter(
    long start,long increment,long end,
    const PVFieldPtr & masterField,
    const epics::pvData::PVScalarArrayPtr masterArray)
: start(start),
  increment(increment),
  end(end),
  masterField(masterField),
  masterArray(masterArray)
{
}


bool PVArrayFilter::filter(const PVFieldPtr & pvField,const BitSetPtr & bitSet,bool toCopy)
{
    PVFieldPtr pvCopy = pvField;
    PVScalarArrayPtr copyArray;
    bool isUnion = false;
    Type type = masterField->getField()->getType();
    if(type==epics::pvData::union_) {
        isUnion = true;
        PVUnionPtr pvMasterUnion = std::tr1::static_pointer_cast<PVUnion>(masterField);
        PVUnionPtr pvCopyUnion = std::tr1::static_pointer_cast<PVUnion>(pvCopy);
        if(toCopy) pvCopyUnion->copy(*pvMasterUnion);
        PVFieldPtr pvField = pvCopyUnion->get();
        copyArray = static_pointer_cast<PVScalarArray>(pvField);
    } else {
       copyArray = static_pointer_cast<PVScalarArray>(pvCopy);
    }
    long len = 0;
    long start = this->start;
    long end = this->end;
    long no_elements = masterArray->getLength();
    if(start<0) {
        start = no_elements+start;
        if(start<0) start = 0;
    }
    if (end < 0) {
        end = no_elements + end;
        if (end < 0) end = 0;

    }
    if(toCopy) {
        if (end >= no_elements) end = no_elements - 1;
        if (end - start >= 0) len = 1 + (end - start) / increment;
        if(len<=0 || start>=no_elements) {
            copyArray->setLength(0);
            return true;
        }
        long indfrom = start;
        long indto = 0;
        copyArray->setCapacity(len);
        if(increment==1) {
            copy(*masterArray,indfrom,1,*copyArray,indto,1,len);
        } else {
            for(long i=0; i<len; ++i) {
                copy(*masterArray,indfrom,1,*copyArray,indto,1,1);
                indfrom += increment;
                 indto += 1;
            }
        }
        copyArray->setLength(len);
        bitSet->set(pvField->getFieldOffset());
        return true;
    }
    if (end - start >= 0) len = 1 + (end - start) / increment;
    if(len<=0) return true;
    if(no_elements<=end) masterArray->setLength(end+1);
    long indfrom = 0;
    long indto = start;
    if(increment==1) {
        copy(*copyArray,indfrom,1,*masterArray,indto,1,len);
    } else {
        for(long i=0; i<len; ++i) {
            copy(*copyArray,indfrom,1,*masterArray,indto,1,1);
            indfrom += 1;
             indto += increment;
        }
    }
    if(isUnion) masterField->postPut();
    return true;
}

string PVArrayFilter::getName()
{
    return name;
}

}}
