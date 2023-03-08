/* Convert.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>
#include <typeinfo>

#include <epicsMutex.h>

#define epicsExportSharedSymbols
#include <pv/lock.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>

using std::tr1::static_pointer_cast;
using std::size_t;
using std::string;

namespace epics { namespace pvData {


static std::vector<string> split(string commaSeparatedList) {
    string::size_type numValues = 1;
    string::size_type index=0;
    while(true) {
        string::size_type pos = commaSeparatedList.find(',',index);
        if(pos==string::npos) break;
        numValues++;
        index = pos +1;
    }
    std::vector<string> valueList(numValues,"");
    index=0;
    for(size_t i=0; i<numValues; i++) {
        size_t pos = commaSeparatedList.find(',',index);
        string value = commaSeparatedList.substr(index,pos);
        valueList[i] = value;
        index = pos +1;
    }
    return valueList;
}

void Convert::getString(string *buf,PVField const *pvField,int /*indentLevel*/)
{
    // TODO indextLevel ignored
    std::ostringstream strm;
    pvField->dumpValue(strm);
    strm << std::endl;
    strm.str().swap(*buf);
}


size_t Convert::fromString(PVStructurePtr const &pvStructure, StringArray const & from, size_t fromStartIndex)
{
    size_t processed = 0;

    PVFieldPtrArray const & fieldsData = pvStructure->getPVFields();
    if (fieldsData.size() != 0) {
        size_t length = pvStructure->getStructure()->getNumberFields();
        for(size_t i=0; i<length; i++) {
            PVFieldPtr fieldField = fieldsData[i];

            Type type = fieldField->getField()->getType();
            if(type==structure) {
                PVStructurePtr pv = static_pointer_cast<PVStructure>(fieldField);
                size_t count = fromString(pv, from, fromStartIndex);
                processed += count;
                fromStartIndex += count;
            }
            else if(type==scalarArray) {
                PVScalarArrayPtr pv = static_pointer_cast<PVScalarArray>(fieldField);
                size_t count = fromString(pv, from[fromStartIndex]);
                processed += count;
                fromStartIndex += count;
            }
            else if(type==scalar) {
                PVScalarPtr pv = static_pointer_cast<PVScalar>(fieldField);
                fromString(pv, from[fromStartIndex++]);
                processed++;
            }
            else {
                // union, structureArray, unionArray not supported
                std::ostringstream oss;
                oss << "Convert::fromString unsupported fieldType " << type;
                throw std::logic_error(oss.str());
            }
        }
    }

    return processed;
}

size_t Convert::fromString(PVScalarArrayPtr const &pv, string from)
{
   if(from[0]=='[' && from[from.length()]==']') {
        size_t offset = from.rfind(']');
        from = from.substr(1, offset);
    }
    std::vector<string> valueList(split(from));
    size_t length = valueList.size();
    size_t num = fromStringArray(pv,0,length,valueList,0);
    if(num<length) length = num;
    pv->setLength(length);
    return length;
}

size_t Convert::fromStringArray(PVScalarArrayPtr const &pv,
                                size_t offset, size_t length,
                                StringArray const & from,
                                size_t fromOffset)
{
    size_t alen = pv->getLength();

    if(offset==0 && length>=alen) {
        // replace all existing elements
        assert(from.size()>=fromOffset+length);

        PVStringArray::svector data(length);
        std::copy(from.begin()+fromOffset,
                  from.begin()+fromOffset+length,
                  data.begin());

        PVStringArray::const_svector temp(freeze(data));
        pv->putFrom<string>(temp);
        return length;

    } else {
        // partial update.
        throw std::runtime_error("fromStringArray: partial update not implemented");
    }
}

size_t Convert::toStringArray(PVScalarArrayPtr const & pv,
                              size_t offset, size_t length,
                              StringArray  &to, size_t toOffset)
{
    PVStringArray::const_svector data;
    pv->getAs<string>(data);
    data.slice(offset, length);
    if(toOffset+data.size() > to.size())
        to.resize(toOffset+data.size());
    std::copy(data.begin()+toOffset, data.end(), to.begin());
    return data.size();
}

ConvertPtr Convert::getConvert()
{
    static ConvertPtr convert;
    static Mutex mutex;
    Lock xx(mutex);

    if(convert.get()==0) {
        convert = ConvertPtr(new Convert());
    }
    return convert;
}

}}
