/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <stdlib.h>

#define epicsExportSharedSymbols
#include <pv/pvdVersion.h>

namespace epics{namespace pvData{

void getVersion(epics::pvData::PVDataVersion *ptr)
{
    ptr->major = EPICS_PVD_MAJOR_VERSION;
    ptr->minor = EPICS_PVD_MINOR_VERSION;
    ptr->maint = EPICS_PVD_MAINTENANCE_VERSION;
    ptr->devel = EPICS_PVD_DEVELOPMENT_FLAG;
}

}} // namespace epics::pvData

// perhaps helpful in a loadable module

extern "C" {
epicsShareExtern void getPVDataVersion(epics::pvData::PVDataVersion *ptr, size_t len);

void getPVDataVersion(epics::pvData::PVDataVersion *ptr, size_t len)
{
    if(len>=sizeof(*ptr)) {
        epics::pvData::getVersion(ptr);
    }
}
}
