/*testLocalProviderMain.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <epicsUnitTest.h>
#include <testMain.h>

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/channelProviderLocal.h>
#include <pv/serverContext.h>
#include "recordClient.h"
#include "listener.h"

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

static bool debug = false;


static void test()
{
    PVDatabasePtr master = PVDatabase::getMaster();
    testOk1(master.get()!=0);
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();
    testOk1(channelProvider.get()!=0);
    StandardPVFieldPtr standardPVField = getStandardPVField();
    string properties;
    ScalarType scalarType;
    string recordName;
    properties = "alarm,timeStamp";
    scalarType = pvDouble;
    recordName = "exampleDouble";
    PVStructurePtr pvStructure(standardPVField->scalar(scalarType,properties));
    PVRecordPtr pvRecord(PVRecord::create(recordName,pvStructure));
    RecordClientPtr exampleRecordClient(RecordClient::create(pvRecord));
    ListenerPtr exampleListener(Listener::create(pvRecord));
    if(debug) pvRecord->setTraceLevel(3);
    master->addRecord(pvRecord);
    pvRecord = master->findRecord("exampleDouble");
    testOk1(pvRecord.get()!=0);
    {
        pvRecord->lock();
        pvRecord->process();
        pvRecord->unlock();
    }
    if(debug) {cout << "processed exampleDouble "  << endl; }
}

MAIN(testLocalProvider)
{
    testPlan(3);
    test();
    return 0;
}
