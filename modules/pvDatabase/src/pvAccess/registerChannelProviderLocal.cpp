/*registerChannelProviderLocal.cpp*/
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2014.07.03
 */

/* Author: Marty Kraimer */

#include <iocsh.h>
#include <pv/pvAccess.h>
#include <pv/serverContext.h>
#include <pv/syncChannelFind.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/timeStamp.h>

// The following must be the last include for code pvDatabase uses
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvDatabase.h"
#include "pv/channelProviderLocal.h"

using std::cout;
using std::endl;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

static const iocshFuncDef pvdblFuncDef = {
    "pvdbl", 0, 0
};
extern "C" void pvdbl(const iocshArgBuf *args)
{
    PVDatabasePtr master = PVDatabase::getMaster();
    PVStringArrayPtr pvNames = master->getRecordNames();
    PVStringArray::const_svector xxx = pvNames->view();
    for(size_t i=0; i<xxx.size(); ++i) cout<< xxx[i] << endl;
}


static void registerChannelProviderLocal(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&pvdblFuncDef, pvdbl);
        getChannelProviderLocal();
    }
}

extern "C" {
    epicsExportRegistrar(registerChannelProviderLocal);
}
