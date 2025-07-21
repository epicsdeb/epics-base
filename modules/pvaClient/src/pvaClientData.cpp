/* pvaClientData.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.04
 */

#include <typeinfo>
#include <sstream>
#include <istream>
#include <ostream>

#include <pv/createRequest.h>
#include <pv/convert.h>
#include <pv/pvEnumerated.h>

#if EPICS_VERSION_INT>=VERSION_INT(3,15,0,1)
#  include <pv/json.h>
#  define USE_JSON
#endif

#define epicsExportSharedSymbols

#include <pv/pvaClient.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvaClient {


typedef std::tr1::shared_ptr<PVArray> PVArrayPtr;
static ConvertPtr convert = getConvert();
static string noStructure("no pvStructure ");
static string noValue("no value field");
static string noScalar("value is not a scalar");
static string noArray("value is not an array");
static string noScalarArray("value is not a scalarArray");
static string noAlarm("no alarm");
static string noTimeStamp("no timeStamp");

PvaClientDataPtr PvaClientData::create(StructureConstPtr const & structure)
{
    if(PvaClient::getDebug()) cout << "PvaClientData::create\n";
    PvaClientDataPtr epv(new PvaClientData(structure));
    return epv;
}

PvaClientData::PvaClientData(StructureConstPtr const & structure)
: structure(structure)
{
}

PVFieldPtr PvaClientData::getSinglePVField()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getSinglePVField\n";
    PVStructurePtr pvStructure = getPVStructure();
    while(true) {
         const PVFieldPtrArray fieldPtrArray(pvStructure->getPVFields());
         if(fieldPtrArray.size()==0) {
              throw std::logic_error("PvaClientData::getSinglePVField() pvRequest for empty structure");
         }
         if(fieldPtrArray.size()!=1) {
              PVFieldPtr pvValue  = pvStructure->getSubField("value");
              if(pvValue) {
                  Type type = pvValue->getField()->getType();
                  if(type!=epics::pvData::structure) return pvValue;
              }
              throw std::logic_error("PvaClientData::getSinglePVField() pvRequest for multiple fields");
         }
         PVFieldPtr pvField(fieldPtrArray[0]);
         Type type = pvField->getField()->getType();
         if(type!=epics::pvData::structure) return pvField;
         pvStructure = static_pointer_cast<PVStructure>(pvField);
    }
}

void PvaClientData::checkValue()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::checkValue\n";
    if(pvValue) return;
    throw std::runtime_error(messagePrefix + noValue);
}

void PvaClientData::setMessagePrefix(std::string const & value)
{
    messagePrefix = value + " ";
}

StructureConstPtr PvaClientData::getStructure()
{
    return structure;
}

PVStructurePtr PvaClientData::getPVStructure()
{
    if(pvStructure) return pvStructure;
    throw std::runtime_error(messagePrefix + noStructure);
}

BitSetPtr PvaClientData::getChangedBitSet()
{
    if(bitSet)return bitSet;
    throw std::runtime_error(messagePrefix + noStructure);
}

std::ostream & PvaClientData::showChanged(std::ostream & out)
{
    if(!bitSet) throw std::runtime_error(messagePrefix + noStructure);
    size_t nextSet = bitSet->nextSetBit(0);
    PVFieldPtr pvField;
    while(nextSet!=string::npos) {
        if(nextSet==0) {
             pvField = pvStructure;
        } else {
              pvField = pvStructure->getSubField(nextSet);
        }
        string name = pvField->getFullName();
        out << name << " = " << pvField << endl;
        nextSet = bitSet->nextSetBit(nextSet+1);
    }
    return out;
}

void PvaClientData::setData(
    PVStructurePtr const & pvStructureFrom,
    BitSetPtr const & bitSetFrom)
{
   if(PvaClient::getDebug()) cout << "PvaClientData::setData\n";
   pvStructure = pvStructureFrom;
   bitSet = bitSetFrom;
   pvValue = pvStructure->getSubField("value");
}


bool PvaClientData::hasValue()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::hasValue\n";
    if(!pvValue) return false;
    return true;
}

bool PvaClientData::isValueScalar()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::isValueScalar\n";
    if(!pvValue) return false;
    if(pvValue->getField()->getType()==scalar) return true;
    return false;
}

bool PvaClientData::isValueScalarArray()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::isValueScalarArray\n";
    if(!pvValue) return false;
    if(pvValue->getField()->getType()==scalarArray) return true;
    return false;
}

PVFieldPtr  PvaClientData::getValue()
{
   if(PvaClient::getDebug()) cout << "PvaClientData::getValue\n";
   checkValue();
   return pvValue;
}

PVScalarPtr  PvaClientData::getScalarValue()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getScalarValue\n";
    checkValue();
    if(pvValue->getField()->getType()!=scalar) {
       throw std::runtime_error(messagePrefix + noScalar);
    }
    return pvStructure->getSubField<PVScalar>("value");
}

PVArrayPtr  PvaClientData::getArrayValue()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getArrayValue\n";
    checkValue();
    Type type = pvValue->getField()->getType();
    if(type!=scalarArray && type!=structureArray && type!=unionArray) {
        throw std::runtime_error(messagePrefix + noArray);
    }
    return pvStructure->getSubField<PVArray>("value");
}

PVScalarArrayPtr  PvaClientData::getScalarArrayValue()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getScalarArrayValue\n";
    checkValue();
    Type type = pvValue->getField()->getType();
    if(type!=scalarArray) {
        throw std::runtime_error(messagePrefix + noScalarArray);
    }
    return pvStructure->getSubField<PVScalarArray>("value");
}

double PvaClientData::getDouble()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getDouble\n";
    PVFieldPtr pvField = getSinglePVField();
    Type type = pvField->getField()->getType();
    if(type!=scalar) {
        throw std::logic_error("PvaClientData::getDouble() did not find a scalar field");
    }
    PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvField);
    ScalarType scalarType = pvScalar->getScalar()->getScalarType();
    if(scalarType==pvDouble) {
        PVDoublePtr pvDouble = static_pointer_cast<PVDouble>(pvScalar);
        return pvDouble->get();
    }
    if(!ScalarTypeFunc::isNumeric(scalarType)) {
        throw std::logic_error(
            "PvaClientData::getDouble() did not find a numeric scalar field");
    }
    return convert->toDouble(pvScalar);
}

string PvaClientData::getString()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getString\n";
    PVFieldPtr pvField = getSinglePVField();
    Type type = pvField->getField()->getType();
    if(type!=scalar) {
        throw std::logic_error("PvaClientData::getString() did not find a scalar field");
    }
    PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvField);
    return convert->toString(pvScalar);
}

shared_vector<const double> PvaClientData::getDoubleArray()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getDoubleArray\n";
    PVFieldPtr pvField = getSinglePVField();
    Type type = pvField->getField()->getType();
    if(type!=scalarArray) {
        throw std::logic_error("PvaClientData::getDoubleArray() did not find a scalarArray field");
    }
    PVScalarArrayPtr pvScalarArray = static_pointer_cast<PVScalarArray>(pvField);
    ScalarType scalarType = pvScalarArray->getScalarArray()->getElementType();
    if(!ScalarTypeFunc::isNumeric(scalarType)) {
        throw std::logic_error(
            "PvaClientData::getDoubleArray() did not find a numeric scalarArray field");
    }
    shared_vector<const double> retValue;
    pvScalarArray->getAs<const double>(retValue);
    return retValue;
}

shared_vector<const string> PvaClientData::getStringArray()
{
    if(PvaClient::getDebug()) cout << "PvaClientData::getStringArray\n";
    PVFieldPtr pvField = getSinglePVField();
    Type type = pvField->getField()->getType();
    if(type!=scalarArray) {
        throw std::logic_error("PvaClientData::getStringArray() did not find a scalarArray field");
    }
    PVScalarArrayPtr pvScalarArray = static_pointer_cast<PVScalarArray>(pvField);
    shared_vector<const string> retValue;
    pvScalarArray->getAs<const string>(retValue);
    return retValue;
}

Alarm PvaClientData::getAlarm()
{
   if(PvaClient::getDebug()) cout << "PvaClientData::getAlarm\n";
   if(!pvStructure) throw new std::runtime_error(messagePrefix + noStructure);
   PVStructurePtr pvs = pvStructure->getSubField<PVStructure>("alarm");
   if(!pvs) throw std::runtime_error(messagePrefix + noAlarm);
   pvAlarm.attach(pvs);
   if(pvAlarm.isAttached()) {
       Alarm alarm;
       pvAlarm.get(alarm);
       pvAlarm.detach();
       return alarm;
   }
   throw std::runtime_error(messagePrefix + noAlarm);
}

TimeStamp PvaClientData::getTimeStamp()
{
   if(PvaClient::getDebug()) cout << "PvaClientData::getTimeStamp\n";
   if(!pvStructure) throw new std::runtime_error(messagePrefix + noStructure);
   PVStructurePtr pvs = pvStructure->getSubField<PVStructure>("timeStamp");
   if(!pvs) throw std::runtime_error(messagePrefix + noTimeStamp);
   pvTimeStamp.attach(pvs);
   if(pvTimeStamp.isAttached()) {
       TimeStamp timeStamp;
       pvTimeStamp.get(timeStamp);
       pvTimeStamp.detach();
       return timeStamp;
   }
   throw std::runtime_error(messagePrefix + noTimeStamp);
}

void PvaClientData::zeroArrayLength()
{
    if(!pvStructure) throw new std::runtime_error(messagePrefix + noStructure);
    zeroArrayLength(pvStructure);
}
void PvaClientData::parse(
    const std::string &arg,const PVFieldPtr &dest,BitSetPtr & bitSet)
{
#ifdef USE_JSON
    std::istringstream strm(arg);
    parseJSON(strm, dest,&(*bitSet));
#else
    throw std::runtime_error("JSON support not built");
#endif
}

void PvaClientData::parse(
    const std::string &arg,const PVUnionPtr &pvUnion)
{
    if(pvUnion->getUnion()->isVariant()) {
          throw std::runtime_error(messagePrefix + "varient union not implemented");
    }
    size_t iequals = arg.find_first_of('=');
    string field;
    string rest;
    if(iequals==std::string::npos) {
        string mess(arg);
        mess += " was expected to start with field=";
          throw std::runtime_error(messagePrefix + mess);
    }
    field = arg.substr(0,iequals);
    rest = arg.substr(iequals+1);
    PVFieldPtr pvField(pvUnion->select(field));
    if(pvField->getField()->getType()==epics::pvData::union_) {
        PVUnionPtr pvu = static_pointer_cast<PVUnion>(pvField);
        parse(rest,pvu);
        return;
    }
    BitSetPtr bs;
    parse(rest,pvField,bs);
    return;
}

void PvaClientData::parse(const std::vector<std::string> &args)
{
    if(!pvStructure) throw std::runtime_error(messagePrefix + noStructure);
    if(!bitSet) throw std::runtime_error(messagePrefix + noStructure);
    size_t num = args.size();
    if(num<1) throw std::runtime_error(messagePrefix + " no arguments");
    for(size_t i=0; i<num; ++i)
    {
        string val = args[i];
        size_t iequals = val.find_first_of('=');
        string field;
        string rest(val);
        if(iequals==std::string::npos) {
           parse(rest,pvStructure,bitSet);
           continue;
        }
        field = val.substr(0,iequals);
        rest = val.substr(iequals+1);
        if(field.size()==std::string::npos) {
           parse(rest,pvStructure,bitSet);
           continue;
        }
        PVFieldPtr pvField(pvStructure->getSubField(field));
        if(!pvField) throw std::runtime_error(messagePrefix + field +" does not exist");
        // look for enumerated structure
        PVEnumerated pvEnumerated;
        bool result = pvEnumerated.attach(pvField);
        if(result) {
             PVStringArray::const_svector choices(pvEnumerated.getChoices());
             for(size_t i=0; i<choices.size(); ++i) {
                  if(choices[i]==rest) {
                     pvEnumerated.setIndex(i);
                     return;
                  }
             }
        }
        // look for union
        PVUnionPtr pvUnion(pvStructure->getSubField<PVUnion>(field));
        if(pvUnion) {
            parse(rest,pvUnion);
            bitSet->set(pvUnion->getFieldOffset());
            return;
        }
        parse(rest,pvField,bitSet);
    }
}

void PvaClientData::streamJSON(
               std::ostream& strm,
               bool ignoreUnprintable,
               bool multiLine)
{
#ifdef USE_JSON
    JSONPrintOptions opts;
    opts.ignoreUnprintable = ignoreUnprintable;
    opts.multiLine = multiLine;
    printJSON(strm,*pvStructure,*bitSet,opts);
#else
    throw std::runtime_error("JSON support not built");
#endif
}


void PvaClientData::zeroArrayLength(const PVStructurePtr &pvStructure)
{
const PVFieldPtrArray pvFields(pvStructure->getPVFields());
    for(size_t i=0; i<pvFields.size(); ++i) {
        PVFieldPtr pvField = pvFields[i];
        Type type(pvField->getField()->getType());
        switch(type) {
        case scalarArray:
            {
                PVScalarArrayPtr pvScalarArray = static_pointer_cast<PVScalarArray>(pvField);
                pvScalarArray->setLength(0);
            }
            break;
        case structureArray:
            {
                PVStructureArrayPtr pvStructureArray = static_pointer_cast<PVStructureArray>(pvField);
                pvStructureArray->setLength(0);
            }
            break;
        case epics::pvData::structure:
            {
                PVStructurePtr pvStructure = static_pointer_cast<PVStructure>(pvField);
                zeroArrayLength(pvStructure);
            }
            break;
        default:
               break;
        }
    }
}


}}
