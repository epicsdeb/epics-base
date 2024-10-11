#ifndef PV_QSRV_H
#define PV_QSRV_H

#include <epicsVersion.h>

#ifndef VERSION_INT
#  define VERSION_INT(V,R,M,P) ( ((V)<<24) | ((R)<<16) | ((M)<<8) | (P))
#endif

/* generated header with EPICS_QSRV_*_VERSION macros */
#  include <pv/qsrvVersionNum.h>

#define QSRV_VERSION_INT VERSION_INT(EPICS_QSRV_MAJOR_VERSION, EPICS_QSRV_MINOR_VERSION, EPICS_QSRV_MAINTENANCE_VERSION, !(EPICS_QSRV_DEVELOPMENT_FLAG))

#define QSRV_ABI_VERSION_INT VERSION_INT(EPICS_QSRV_ABI_MAJOR_VERSION, EPICS_QSRV_ABI_MINOR_VERSION, 0, 0)

#if defined(QSRV_API_BUILDING) && defined(epicsExportSharedSymbols)
#  error Use QSRV_API or shareLib.h not both
#endif

#if defined(_WIN32) || defined(__CYGWIN__)

#  if defined(QSRV_API_BUILDING) && defined(EPICS_BUILD_DLL)
/* building library as dll */
#    define QSRV_API __declspec(dllexport)
#  elif !defined(QSRV_API_BUILDING) && defined(EPICS_CALL_DLL)
/* calling library in dll form */
#    define QSRV_API __declspec(dllimport)
#  endif

#elif __GNUC__ >= 4
#  define QSRV_API __attribute__ ((visibility("default")))
#endif

#ifndef QSRV_API
#  define QSRV_API
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct link; /* aka. DBLINK from link.h */

/** returns QSRV_VERSION_INT captured at compilation time */
QSRV_API unsigned qsrvVersion(void);

/** returns QSRV_ABI_VERSION_INT captured at compilation time */
QSRV_API unsigned qsrvABIVersion(void);

QSRV_API
long dbLoadGroup(const char* fname);

QSRV_API void testqsrvWaitForLinkEvent(struct link *plink);

/** Call before testIocShutdownOk()
 @code
   testdbPrepare();
   ...
   testIocInitOk();
   ...
   testqsrvShutdownOk();
   testIocShutdownOk();
   testqsrvCleanup();
   testdbCleanup();
 @endcode
 */
QSRV_API void testqsrvShutdownOk(void);

/** Call after testIocShutdownOk() and before testdbCleanup()
 @code
   testdbPrepare();
   ...
   testIocInitOk();
   ...
   testqsrvShutdownOk();
   testIocShutdownOk();
   testqsrvCleanup();
   testdbCleanup();
 @endcode
 */
QSRV_API void testqsrvCleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* PV_QSRV_H */
