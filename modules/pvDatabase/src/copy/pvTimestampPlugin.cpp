/* pvTimestampPlugin.cpp */
/*
 * The License for this software can be found in the file LICENSE that is included with the distribution.
 */

#include <string>
#include <map>
#include <pv/lock.h>
#include <pv/pvData.h>
#include <pv/bitSet.h>
#include <pv/convert.h>
#include <pv/pvTimeStamp.h>
#define epicsExportSharedSymbols
#include "pv/pvPlugin.h"
#include "pv/pvTimestampPlugin.h"


using std::string;
using std::size_t;
using std::cout;
using std::endl;
using std::tr1::static_pointer_cast;
using std::vector;
using namespace epics::pvData;

namespace epics { namespace pvCopy{

static ConvertPtr convert = getConvert();
static std::string name("timestamp");

PVTimestampPlugin::PVTimestampPlugin()
{
}

PVTimestampPlugin::~PVTimestampPlugin()
{
}

void PVTimestampPlugin::create()
{
     static bool firstTime = true;
     if(firstTime) {
         firstTime = false;
         PVTimestampPluginPtr pvPlugin = PVTimestampPluginPtr(new PVTimestampPlugin());
         PVPluginRegistry::registerPlugin(name,pvPlugin);
     }
}

PVFilterPtr PVTimestampPlugin::create(
     const std::string & requestValue,
     const PVCopyPtr & pvCopy,
     const PVFieldPtr & master)
{
    return PVTimestampFilter::create(requestValue,master);
}

PVTimestampFilter::~PVTimestampFilter()
{
}


PVTimestampFilterPtr PVTimestampFilter::create(
     const std::string & requestValue,
     const PVFieldPtr & master)
{
    PVTimeStamp pvTimeStamp;
    if(!pvTimeStamp.attach(master)) return PVTimestampFilterPtr();
    bool current = false;
    bool copy = false;
    if(requestValue.compare("current")==0) {
        current = true;
    } else if(requestValue.compare("copy")==0){
        copy = true;
    } else {
        return  PVTimestampFilterPtr();
    }
    PVTimestampFilterPtr filter = PVTimestampFilterPtr(
             new PVTimestampFilter(current,copy,master));
    return filter;
}

PVTimestampFilter::PVTimestampFilter(bool current,bool copy,PVFieldPtr const & master)
: current(current),
  copy(copy),
  master(master)
{
}


bool PVTimestampFilter::filter(const PVFieldPtr & pvCopy,const BitSetPtr & bitSet,bool toCopy)
{
    if(current) {
        timeStamp.getCurrent();
        if(toCopy) {
            if(!pvTimeStamp.attach(pvCopy)) return false;
        } else {
            if(!pvTimeStamp.attach(master)) return false;
        }
        pvTimeStamp.set(timeStamp);
        bitSet->set(pvCopy->getFieldOffset());
        return true;
     }
     if(copy) {
        if(toCopy) {
            if(!pvTimeStamp.attach(master)) return false;
            pvTimeStamp.get(timeStamp);
            if(!pvTimeStamp.attach(pvCopy)) return false;
            pvTimeStamp.set(timeStamp);
            bitSet->set(pvCopy->getFieldOffset());
        } else {
            if(!pvTimeStamp.attach(pvCopy)) return false;
            pvTimeStamp.get(timeStamp);
            if(!pvTimeStamp.attach(master)) return false;
            pvTimeStamp.set(timeStamp);
        }
        return true;
     }
     return false;
}

string PVTimestampFilter::getName()
{
    return name;
}

}}
