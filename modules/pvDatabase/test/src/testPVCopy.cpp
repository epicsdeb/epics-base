/*testPVCopyMain.cpp */
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
#include <pv/channelProviderLocal.h>
#include <pv/convert.h>
#define epicsExportSharedSymbols
#include "powerSupply.h"


using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvCopy;
using namespace epics::pvDatabase;

static bool debug = false;

class MyRequester;
typedef std::tr1::shared_ptr<MyRequester> MyRequesterPtr;

class MyRequester : public Requester {
public:
    POINTER_DEFINITIONS(MyRequester);
    MyRequester(string const &requesterName)
    : requesterName(requesterName)
    {}
    virtual ~MyRequester() {}
    virtual string getRequesterName() { return requesterName;}
    virtual void message(string const & message,MessageType messageType)
    {
         cout << message << endl;
    }
private:
    string requesterName;
};

static PVRecordPtr createScalar(
    string const & recordName,
    ScalarType scalarType,
    string const & properties)
{
    PVStructurePtr pvStructure = getStandardPVField()->scalar(scalarType,properties);
    return PVRecord::create(recordName,pvStructure);
}

static PVRecordPtr createScalarArray(
    string const & recordName,
    ScalarType scalarType,
    string const & properties)
{
    PVStructurePtr pvStructure = getStandardPVField()->scalarArray(scalarType,properties);
    return PVRecord::create(recordName,pvStructure);
}

static void testPVScalar(
    string const & valueNameRecord,
    string const & valueNameCopy,
    PVRecordPtr const & pvRecord,
    PVCopyPtr const & pvCopy)
{
    PVStructurePtr pvStructureRecord;
    PVStructurePtr pvStructureCopy;
    PVFieldPtr pvField;
    PVScalarPtr pvValueRecord;
    PVScalarPtr pvValueCopy;
    BitSetPtr bitSet;
    size_t offset;
    ConvertPtr convert = getConvert();

    if(debug)  cout << endl;
    pvStructureRecord = pvRecord->getPVRecordStructure()->getPVStructure();
    pvField = pvStructureRecord->getSubField(valueNameRecord);
    pvValueRecord = static_pointer_cast<PVScalar>(pvField);
    convert->fromDouble(pvValueRecord,.04);
    StructureConstPtr structure = pvCopy->getStructure();
    if(debug)  cout << "structure from copy" << endl << *structure << endl;
    pvStructureCopy = pvCopy->createPVStructure();
    pvField = pvStructureCopy->getSubField(valueNameCopy);
    pvValueCopy = static_pointer_cast<PVScalar>(pvField);
    bitSet = BitSetPtr(new BitSet(pvStructureCopy->getNumberFields()));
    pvCopy->initCopy(pvStructureCopy, bitSet);
    if(debug) {
        cout << "after initCopy pvValueCopy " << convert->toDouble(pvValueCopy);
        cout << endl;
    }
    convert->fromDouble(pvValueRecord,.06);
    testOk1(convert->toDouble(pvValueCopy)==.04);
    pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    testOk1(convert->toDouble(pvValueCopy)==.06);
    testOk1(bitSet->get(pvValueCopy->getFieldOffset()));
    if(debug) {
        cout << "after put(.06) pvValueCopy " << convert->toDouble(pvValueCopy);
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    offset = pvCopy->getCopyOffset(pvValueRecord);
    if(debug) {
        cout << "getCopyOffset() " << offset;
        cout << " pvValueCopy->getOffset() " << pvValueCopy->getFieldOffset();
        cout << " pvValueRecord->getOffset() " << pvValueRecord->getFieldOffset();
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    bitSet->clear();
    convert->fromDouble(pvValueRecord,1.0);
    if(debug) {
        cout << "before updateCopyFromBitSet";
        cout << " recordValue " << convert->toDouble(pvValueRecord);
        cout << " copyValue " << convert->toDouble(pvValueCopy);
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    bitSet->set(0);
    testOk1(convert->toDouble(pvValueCopy)==0.06);
    pvCopy->updateCopyFromBitSet(pvStructureCopy,bitSet);
    testOk1(convert->toDouble(pvValueCopy)==1.0);
    if(debug) {
        cout << "after updateCopyFromBitSet";
        cout << " recordValue " << convert->toDouble(pvValueRecord);
        cout << " copyValue " << convert->toDouble(pvValueCopy);
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    convert->fromDouble(pvValueCopy,2.0);
    bitSet->set(0);
    if(debug) {
        cout << "before updateMaster";
        cout << " recordValue " << convert->toDouble(pvValueRecord);
        cout << " copyValue " << convert->toDouble(pvValueCopy);
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    testOk1(convert->toDouble(pvValueRecord)==1.0);
    pvCopy->updateMaster(pvStructureCopy,bitSet);
    testOk1(convert->toDouble(pvValueRecord)==2.0);
    if(debug) {
        cout << "after updateMaster";
        cout << " recordValue " << convert->toDouble(pvValueRecord);
        cout << " copyValue " << convert->toDouble(pvValueCopy);
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
}

static void testPVScalarArray(
    string const & valueNameRecord,
    string const & valueNameCopy,
    PVRecordPtr const & pvRecord,
    PVCopyPtr const & pvCopy)
{
    PVStructurePtr pvStructureRecord;
    PVStructurePtr pvStructureCopy;
    PVScalarArrayPtr pvValueRecord;
    PVScalarArrayPtr pvValueCopy;
    BitSetPtr bitSet;
    size_t offset;
    size_t n = 5;
    shared_vector<double> values(n);
    shared_vector<const double> cvalues;

    if(debug) {cout << endl;}
    pvStructureRecord = pvRecord->getPVRecordStructure()->getPVStructure();
    pvValueRecord = pvStructureRecord->getSubField<PVScalarArray>(valueNameRecord);
    for(size_t i=0; i<n; i++) values[i] = i;
    const shared_vector<const double> xxx(freeze(values));
    pvValueRecord->putFrom(xxx);
    StructureConstPtr structure = pvCopy->getStructure();
    if(debug) { cout << "structure from copy" << endl << *structure << endl;}
    pvStructureCopy = pvCopy->createPVStructure();
    pvValueCopy = pvStructureCopy->getSubField<PVScalarArray>(valueNameCopy);
    bitSet = BitSetPtr(new BitSet(pvStructureCopy->getNumberFields()));
    pvCopy->initCopy(pvStructureCopy, bitSet);
    if(debug) {
        cout << "after initCopy pvValueCopy " << *pvValueCopy << endl;
        cout << endl;
    }
    values.resize(n);
    for(size_t i=0; i<n; i++) values[i] = i + .06;
    const shared_vector<const double> yyy(freeze(values));
    pvValueRecord->putFrom(yyy);
    pvValueCopy->getAs(cvalues);
    testOk1(cvalues[0]==0.0);
    pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    pvValueCopy->getAs(cvalues);
    testOk1(cvalues[0]==0.06);
    if(debug) {
        cout << "after put(i+ .06) pvValueCopy " << *pvValueCopy << endl;
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    offset = pvCopy->getCopyOffset(pvValueRecord);
    if(debug) {
        cout << "getCopyOffset() " << offset;
        cout << " pvValueCopy->getOffset() " << pvValueCopy->getFieldOffset();
        cout << " pvValueRecord->getOffset() " << pvValueRecord->getFieldOffset();
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    bitSet->clear();
    values.resize(n);
    for(size_t i=0; i<n; i++) values[i] = i + 1.0;
    const shared_vector<const double> zzz(freeze(values));
    pvValueRecord->putFrom(zzz);
    if(debug) {
        cout << "before updateCopyFromBitSet";
        cout << " recordValue " << *pvValueRecord << endl;
        cout << " copyValue " << *pvValueCopy << endl;
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    bitSet->set(0);
    pvValueCopy->getAs(cvalues);
    testOk1(cvalues[0]==0.06);
    pvCopy->updateCopyFromBitSet(pvStructureCopy,bitSet);
    pvValueCopy->getAs(cvalues);
    testOk1(cvalues[0]==1.0);
    if(debug) {
        cout << "after updateCopyFromBitSet";
        cout << " recordValue " << *pvValueRecord << endl;
        cout << " copyValue " << *pvValueCopy << endl;
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    values.resize(n);
    for(size_t i=0; i<n; i++) values[i] = i + 2.0;
    const shared_vector<const double> ttt(freeze(values));
    pvValueRecord->putFrom(ttt);
    bitSet->set(0);
    if(debug) {
        cout << "before updateMaster";
        cout << " recordValue " << *pvValueRecord << endl;
        cout << " copyValue " << *pvValueCopy << endl;
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
    pvValueRecord->getAs(cvalues);
    testOk1(cvalues[0]==2.0);
    pvCopy->updateMaster(pvStructureCopy,bitSet);
    pvValueRecord->getAs(cvalues);
    testOk1(cvalues[0]==1.0);
    if(debug) {
        cout << "after updateMaster";
        cout << " recordValue " << *pvValueRecord << endl;
        cout << " copyValue " << *pvValueRecord << endl;
        cout << " bitSet " << *bitSet;
        cout << endl;
    }
}

static void testMasterField(PVRecordPtr const& pvRecord)
{
    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    PVStructurePtr pvRequest = createRequest->createRequest("field(_)");
    if(debug) {
        cout << "pvRequest" << *pvRequest << endl ;
    }
    PVStructurePtr pvStructureRecord = pvRecord->getPVRecordStructure()->getPVStructure();
    PVCopyPtr pvCopy = PVCopy::create(pvStructureRecord,pvRequest,"");
    PVStructurePtr pvMasterField = pvCopy->getPVMaster();
    if(debug) {
        cout << "PV structure from record" << endl << *pvStructureRecord << endl;
        cout << "Master PV structure from copy" << endl << *pvMasterField << endl;
        cout << "Master PV structure from copy offset " << pvMasterField->getFieldOffset() << endl;
    }
    testOk1(pvMasterField->getNumberFields() == pvStructureRecord->getNumberFields());
    testOk1(pvMasterField->getFieldOffset() == 0);
    PVStructurePtr pvStructureCopy = pvCopy->createPVStructure();
    BitSetPtr bitSet = BitSetPtr(new BitSet(pvStructureCopy->getNumberFields()));
    pvCopy->initCopy(pvStructureCopy, bitSet);
    if(debug) {
        cout << "PV structure from copy" << endl << *pvStructureCopy << endl;
        cout << "PV structure from copy offset " << pvStructureCopy->getFieldOffset() << endl;
    }
    testOk1(pvMasterField->getNumberFields() == pvStructureCopy->getNumberFields());
}

static void scalarTest()
{
    if(debug) {cout << endl << endl << "****scalarTest****" << endl;}
    RequesterPtr requester(new MyRequester("exampleTest"));
    PVRecordPtr pvRecord;
    string request;
    PVStructurePtr pvRequest;
    PVCopyPtr pvCopy;
    string valueNameRecord;
    string valueNameCopy;

    pvRecord = createScalar("doubleRecord",pvDouble,"alarm,timeStamp,display");
    valueNameRecord = request = "value";
    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    pvRequest = createRequest->createRequest(request);
    if(debug) {
        cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;
    }
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {
        cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;
    }
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,value";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {
        cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;
    }
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
}

static void arrayTest()
{
    if(debug) {cout << endl << endl << "****arrayTest****" << endl;}
    RequesterPtr requester(new MyRequester("exampleTest"));
    PVRecordPtr pvRecord;
    string request;
    PVStructurePtr pvRequest;
    PVCopyPtr pvCopy;
    string valueNameRecord;
    string valueNameCopy;

    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    pvRecord = createScalarArray("doubleArrayRecord",pvDouble,"alarm,timeStamp");
    valueNameRecord = request = "value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;}
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;}
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,value";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;}
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
}

static void powerSupplyTest()
{
    if(debug) {cout << endl << endl << "****powerSupplyTest****" << endl;}
    RequesterPtr requester(new MyRequester("exampleTest"));
    PowerSupplyPtr pvRecord;
    string request;
    PVStructurePtr pvRequest;
    PVCopyPtr pvCopy;
    string valueNameRecord;
    string valueNameCopy;

    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    PVStructurePtr pv = createPowerSupply();
    pvRecord = PowerSupply::create("powerSupply",pv);
    valueNameRecord = request = "power.value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;}
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "power.value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "power.value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;}
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "power.value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,voltage.value,power.value,current.value";
    valueNameRecord = "power.value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;}
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "power.value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,voltage{value,alarm},power{value,alarm,display},current.value";
    valueNameRecord = "power.value";
    pvRequest = createRequest->createRequest(request);
    if(debug) {cout << "request " << request << endl << "pvRequest" << *pvRequest << endl ;}
    pvCopy = PVCopy::create(pvRecord->getPVRecordStructure()->getPVStructure(),pvRequest,"");
    valueNameCopy = "power.value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
}

static void masterFieldTest()
{
    if(debug) {
        cout << endl << endl << "****masterFieldTest****" << endl;
    }
    PVRecordPtr pvRecord = createScalar("doubleRecord",pvDouble,"alarm,timeStamp,display");
    testMasterField(pvRecord);
}

MAIN(testPVCopy)
{
    testPlan(70);
    scalarTest();
    arrayTest();
    powerSupplyTest();
    masterFieldTest();
    return 0;
}
