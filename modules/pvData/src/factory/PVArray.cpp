/*PVArray.cpp*/
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

#define epicsExportSharedSymbols
#include <pv/pvData.h>
#include <pv/factory.h>

using std::size_t;

namespace epics { namespace pvData {

PVArray::PVArray(FieldConstPtr const & field)
: PVField(field),capacityMutable(true)
{ }

 void PVArray::setImmutable()
 {
     capacityMutable = false;
     PVField::setImmutable();
 }

 bool PVArray::isCapacityMutable() const
 {
      if(PVField::isImmutable()) {
          return false;
      }
      return capacityMutable;
 }

 void PVArray::setCapacityMutable(bool isMutable)
 {
    if(isMutable && PVField::isImmutable()) {
       throw std::runtime_error("field is immutable");
    }
    capacityMutable = isMutable;
 }


std::ostream& operator<<(format::array_at_internal const& manip, const PVArray& array)
{
    return array.dumpValue(manip.stream, manip.index);
}

}}
