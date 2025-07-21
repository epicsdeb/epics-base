
#include <initHooks.h>
#include <epicsExit.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsStdio.h>

#include <dbAccess.h>
#include <dbChannel.h>
#include <dbStaticLib.h>
#include <dbLock.h>
#include <dbEvent.h>
#include <epicsVersion.h>
#include <dbNotify.h>

#include <pv/reftrack.h>
#include <pv/pvAccess.h>
#include <pv/serverContext.h>
#include <pv/iocshelper.h>

#include "pv/qsrv.h"
#include "pvahelper.h"
#include "pvif.h"
#include "pdb.h"
#include "pdbsingle.h"
#ifdef USE_MULTILOCK
#  include "pdbgroup.h"
#endif

#include <epicsExport.h>

namespace pva = epics::pvAccess;

void QSRVRegistrar_counters()
{
    epics::registerRefCounter("PDBSinglePV", &PDBSinglePV::num_instances);
    epics::registerRefCounter("PDBSingleChannel", &PDBSingleChannel::num_instances);
    epics::registerRefCounter("PDBSinglePut", &PDBSinglePut::num_instances);
    epics::registerRefCounter("PDBSingleMonitor", &PDBSingleMonitor::num_instances);
#ifdef USE_MULTILOCK
    epics::registerRefCounter("PDBGroupPV", &PDBGroupPV::num_instances);
    epics::registerRefCounter("PDBGroupChannel", &PDBGroupChannel::num_instances);
    epics::registerRefCounter("PDBGroupPut", &PDBGroupPut::num_instances);
    epics::registerRefCounter("PDBGroupMonitor", &PDBGroupMonitor::num_instances);
#endif // USE_MULTILOCK
    epics::registerRefCounter("PDBProvider", &PDBProvider::num_instances);
}

long dbLoadGroup(const char* fname)
{
    try {
        if(!fname) {
            printf("dbLoadGroup(\"file.json\")\n"
                   "\n"
                   "Load additional DB group definitions from file.\n");
            return 1;
        }
#ifndef USE_MULTILOCK
            static bool warned;
            if(!warned) {
                warned = true;
                fprintf(stderr, "ignoring %s\n", fname);
            }
#endif

        if(fname[0]=='-') {
            fname++;
            if(fname[0]=='*' && fname[1]=='\0') {
                PDBProvider::group_files.clear();
            } else {
                PDBProvider::group_files.remove(fname);
            }
        } else {
            PDBProvider::group_files.remove(fname);
            PDBProvider::group_files.push_back(fname);
        }

        return 0;
    }catch(std::exception& e){
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
}

namespace {

void dbLoadGroupWrap(const char* fname)
{
    (void)dbLoadGroup(fname);
}

void dbgl(int lvl, const char *pattern)
{
    if(!pattern)
        pattern = "";

    try {
        PDBProvider::shared_pointer prov(
                    std::tr1::dynamic_pointer_cast<PDBProvider>(
                        pva::ChannelProviderRegistry::servers()->getProvider("QSRV")));
        if(!prov)
            throw std::runtime_error("No Provider (PVA server not running?)");

        PDBProvider::persist_pv_map_t pvs;
        {
            epicsGuard<epicsMutex> G(prov->transient_pv_map.mutex());
            pvs = prov->persist_pv_map; // copy map
        }

        for(PDBProvider::persist_pv_map_t::const_iterator it(pvs.begin()), end(pvs.end());
            it != end; ++it)
        {
            if(pattern[0] && epicsStrGlobMatch(it->first.c_str(), pattern)==0)
                continue;

            printf("%s\n", it->first.c_str());
            if(lvl<=0)
                continue;
            it->second->show(lvl);
        }

    }catch(std::exception& e){
        fprintf(stderr, "Error: %s\n", e.what());
    }
}

void QSRVRegistrar()
{
    QSRVRegistrar_counters();
    pva::ChannelProviderRegistry::servers()->addSingleton<PDBProvider>("QSRV");
    epics::iocshRegister<int, const char*, &dbgl>("dbgl", "level", "pattern");
    epics::iocshRegister<const char*, &dbLoadGroupWrap>("dbLoadGroup", "jsonfile");
}

} // namespace

unsigned qsrvVersion(void)
{
    return QSRV_VERSION_INT;
}

unsigned qsrvABIVersion(void)
{
    return QSRV_ABI_VERSION_INT;
}

extern "C" {
    epicsExportRegistrar(QSRVRegistrar);
}
