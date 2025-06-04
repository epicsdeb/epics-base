/* pvDeadbandPlugin.cpp */
/*
 * The License for this software can be found in the file LICENSE that is included with the distribution.
 */
#include <stdlib.h>

#include <string>
#include <map>
#include <pv/lock.h>
#include <pv/pvData.h>
#include <pv/bitSet.h>
#include <pv/convert.h>

#define epicsExportSharedSymbols
#include "pv/pvDeadbandPlugin.h"

using std::string;
using std::size_t;
using std::cout;
using std::endl;
using std::tr1::static_pointer_cast;
using std::vector;
using namespace epics::pvData;

namespace epics { namespace pvCopy{

static ConvertPtr convert = getConvert();
static std::string name("deadband");

PVDeadbandPlugin::PVDeadbandPlugin()
{
}

PVDeadbandPlugin::~PVDeadbandPlugin()
{
}

void PVDeadbandPlugin::create()
{
     static bool firstTime = true;
     if(firstTime) {
         firstTime = false;
         PVDeadbandPluginPtr pvPlugin = PVDeadbandPluginPtr(new PVDeadbandPlugin());
         PVPluginRegistry::registerPlugin(name,pvPlugin);
     }
}

PVFilterPtr PVDeadbandPlugin::create(
     const std::string & requestValue,
     const PVCopyPtr & pvCopy,
     const PVFieldPtr & master)
{
    return PVDeadbandFilter::create(requestValue,master);
}

PVDeadbandFilter::~PVDeadbandFilter()
{
}

PVDeadbandFilterPtr PVDeadbandFilter::create(
     const std::string & requestValue,
     const PVFieldPtr & master)
{
    FieldConstPtr field =master->getField();
    Type type = field->getType();
    if(type!=scalar) return PVDeadbandFilterPtr();
    ScalarConstPtr scalar = static_pointer_cast<const Scalar>(field);
    if(!ScalarTypeFunc::isNumeric(scalar->getScalarType())) return PVDeadbandFilterPtr();
    bool absolute = false;
    if(requestValue.find("abs")==0) {
        absolute = true;
    } else if(requestValue.find("rel")==0) {
        absolute = false;
    } else {
        return PVDeadbandFilterPtr();
    }
    size_t ind = requestValue.find(':');
    if(ind==string::npos) return PVDeadbandFilterPtr();
    string svalue = requestValue.substr(ind+1);
    double deadband = atof(svalue.c_str());
    if(deadband==0.0) return PVDeadbandFilterPtr();
    PVDeadbandFilterPtr filter =
         PVDeadbandFilterPtr(
             new PVDeadbandFilter(
                 absolute,deadband,static_pointer_cast<PVScalar>(master)));
    return filter;
}

PVDeadbandFilter::PVDeadbandFilter(bool absolute,double deadband,PVScalarPtr const & master)
: absolute(absolute),
  deadband(deadband),
  master(master),
  firstTime(true),
  lastReportedValue(0.0)
{
}


bool PVDeadbandFilter::filter(const PVFieldPtr & pvCopy,const BitSetPtr & bitSet,bool toCopy)
{
    if(!toCopy) return false;
    double value = convert->toDouble(master);
    double diff = value - lastReportedValue;
    if(diff<0.0) diff = - diff;
    bool report = true;
    if(firstTime) {
        firstTime = false;
    } else if(absolute) {
        if(diff<deadband) report = false;
    } else {
        double last = lastReportedValue;
        if(last<0.0) last = -last;
        if(last>1e-20) {
            double percent = (diff/last)*100.0;
            if(percent<deadband) report = false;
         }
     }
     PVScalarPtr copy = static_pointer_cast<PVScalar>(pvCopy);
     convert->fromDouble(copy,value);
     if(report) {
         lastReportedValue = value;
         bitSet->set(pvCopy->getFieldOffset());
     } else {
         bitSet->clear(pvCopy->getFieldOffset());
     }
     return true;
}

string PVDeadbandFilter::getName()
{
    return name;
}

}}
