/* pvaClientMonitorData.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.02
 */

#include <typeinfo>
#include <sstream>

#include <pv/createRequest.h>
#include <pv/convert.h>

#define epicsExportSharedSymbols

#include <pv/pvaClient.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvaClient {


typedef std::tr1::shared_ptr<PVArray> PVArrayPtr;

static ConvertPtr convert = getConvert();

static StructureConstPtr nullStructure;
static PVStructurePtr nullPVStructure;

static string noStructure("no pvStructure ");
static string noValue("no value field");
static string noScalar("value is not a scalar");
static string notCompatibleScalar("value is not a compatible scalar");
static string noArray("value is not an array");
static string noScalarArray("value is not a scalarArray");
static string notDoubleArray("value is not a doubleArray");
static string notStringArray("value is not a stringArray");
static string noAlarm("no alarm");
static string noTimeStamp("no timeStamp");


PvaClientMonitorDataPtr PvaClientMonitorData::create(StructureConstPtr const & structure)
{
    PvaClientMonitorDataPtr epv(new PvaClientMonitorData(structure));
    return epv;
}

PvaClientMonitorData::PvaClientMonitorData(StructureConstPtr const & structure)
: PvaClientData(structure)
{
}

void PvaClientMonitorData::setData(MonitorElementPtr const & monitorElement)
{
   PVStructurePtr pvStructure = monitorElement->pvStructurePtr;
   BitSetPtr changedBitSet = monitorElement->changedBitSet;
   PvaClientData::setData(pvStructure,changedBitSet);
   overrunBitSet = monitorElement->overrunBitSet;
}

BitSetPtr PvaClientMonitorData::getOverrunBitSet()
{
    if(!overrunBitSet) throw std::runtime_error(messagePrefix + noStructure);
    return overrunBitSet;
}

std::ostream & PvaClientMonitorData::showOverrun(std::ostream & out)
{
    if(!overrunBitSet) throw std::runtime_error(messagePrefix + noStructure);
    size_t nextSet = overrunBitSet->nextSetBit(0);
    PVFieldPtr pvField;
    while(nextSet!=string::npos) {
        if(nextSet==0) {
             pvField = getPVStructure();
        } else {
              pvField = getPVStructure()->getSubField(nextSet);
        }
        string name = pvField->getFullName();
        out << name << " = " << pvField << endl;
        nextSet = overrunBitSet->nextSetBit(nextSet+1);
    }
    return out;
}

}}
