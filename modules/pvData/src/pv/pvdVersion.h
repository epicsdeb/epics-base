/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* pvdVersion.h did not exist prior to 7.0.0
 * from which time it is included by pv/pvIntrospect.h
 */
#ifndef PVDVERSION_H
#define PVDVERSION_H

#include <epicsVersion.h>
#include <shareLib.h>

#ifndef VERSION_INT
#  define VERSION_INT(V,R,M,P) ( ((V)<<24) | ((R)<<16) | ((M)<<8) | (P))
#endif

#ifndef EPICS_VERSION_INT
#  define EPICS_VERSION_INT VERSION_INT(EPICS_VERSION, EPICS_REVISION, EPICS_MODIFICATION, EPICS_PATCH_LEVEL)
#endif

/* include generated headers with:
 *   EPICS_PVD_MAJOR_VERSION
 *   EPICS_PVD_MINOR_VERSION
 *   EPICS_PVD_MAINTENANCE_VERSION
 *   EPICS_PVD_DEVELOPMENT_FLAG
 */
#include <pv/pvdVersionNum.h>

#define PVDATA_VERSION_INT VERSION_INT(EPICS_PVD_MAJOR_VERSION, EPICS_PVD_MINOR_VERSION, EPICS_PVD_MAINTENANCE_VERSION, 0)

namespace epics{namespace pvData{

struct PVDataVersion {
    unsigned major;
    unsigned minor;
    unsigned maint;
    unsigned devel;
};

epicsShareExtern void getVersion(PVDataVersion *ptr);

}} // namespace epics::pvData


#endif // PVDVERSION_H
