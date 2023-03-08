/*testExampleRecordMain.cpp */
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
#define epicsExportSharedSymbols
#include "powerSupply.h"
#include "recordClient.h"
#include "listener.h"

#include <epicsExport.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

static bool debug = false;
static int traceLevel = 0;


static void test()
{
    StandardPVFieldPtr standardPVField = getStandardPVField();
    string properties;
    ScalarType scalarType;
    string recordName;
    properties = "alarm,timeStamp";
    scalarType = pvDouble;
    recordName = "exampleDouble";
    PVStructurePtr pvStructure;
    pvStructure = standardPVField->scalar(scalarType,properties);
    PVRecordPtr exampleRecord = PVRecord::create(recordName,pvStructure);
    exampleRecord->setTraceLevel(traceLevel);
    RecordClientPtr exampleRecordClient(RecordClient::create(exampleRecord));
    ListenerPtr exampleListener(Listener::create(exampleRecord));
    {
        exampleRecord->lock();
        exampleRecord->process();
        exampleRecord->unlock();
    }
    if(debug) {cout << "processed exampleDouble "  << endl; }
    recordName = "powerSupplyExample";
    PowerSupplyPtr psr;
    pvStructure = createPowerSupply();
    psr = PowerSupply::create("powerSupply",pvStructure);
    psr->setTraceLevel(traceLevel);
    RecordClientPtr psrRecordClient(RecordClient::create(psr));
    ListenerPtr psrListener(Listener::create(psr));
    testOk1(psr.get()!=0);
    if(!psr) {
        if(debug) {cout << "PowerSupplyRecordTest::create failed" << endl;}
        return;
    }
    double voltage,power,current;
    {
        psr->lock();
        voltage = psr->getVoltage();
        power = psr->getPower();
        current = psr->getCurrent();
        psr->unlock();
    }
    if(debug ) {
        cout << "initial ";
        cout << " voltage " << voltage ;
        cout << " power " << power;
        cout <<  " current " << current;
        cout << endl;
    }
    testOk1(psr->getVoltage()==0.0);
    testOk1(psr->getPower()==0.0);
    testOk1(psr->getCurrent()==0.0);
    voltage = 1.0;
    power = 1.0;
    if(debug) {
        cout << "before put ";
        cout << " voltage " << voltage ;
        cout << " power " << power;
        cout << endl;
    }
    {
        psr->lock();
        psr->put(power,voltage);
        psr->process();
        psr->unlock();
    }
    {
        psr->lock();
        if(debug) {
            cout << "after put ";
            cout << " voltage " << psr->getVoltage() ;
            cout << " power " << psr->getPower();
            cout <<  " current " << psr->getCurrent();
            cout << endl;
        }
        psr->unlock();
    }
    testOk1(psr->getVoltage()==1.0);
    testOk1(psr->getPower()==1.0);
    testOk1(psr->getCurrent()==1.0);
}

MAIN(testExampleRecord)
{
    testPlan(7);
    test();
    return 0;
}
