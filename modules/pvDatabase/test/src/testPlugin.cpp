/*testPluginMain.cpp */
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
#include <pv/pvStructureCopy.h>
#include <pv/pvDatabase.h>
#define epicsExportSharedSymbols
#include "powerSupply.h"


using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvCopy;
using namespace epics::pvDatabase;

static bool debug = true;

static void deadbandTest()
{
    if(debug) {cout << endl << endl << "****deadbandTest****" << endl;}
    bool result = false;
    uint32 nset = 0;

    PVStructurePtr pvRecordStructure(getStandardPVField()->scalar(pvDouble,""));
    PVRecordPtr pvRecord(PVRecord::create("doubleRecord",pvRecordStructure));
    PVStructurePtr pvRequest(CreateRequest::create()->createRequest("value[deadband=abs:1.0]"));
    PVCopyPtr pvCopy(PVCopy::create(pvRecordStructure,pvRequest,""));
    PVStructurePtr pvStructureCopy(pvCopy->createPVStructure());
    BitSetPtr bitSet(new BitSet(pvStructureCopy->getNumberFields()));
    PVDoublePtr pvValue(pvRecordStructure->getSubField<PVDouble>("value"));
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "initial"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==1);
    pvValue->put(.1);
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after pvValue"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }
    testOk1(result==false);
    testOk1(nset==0);
    pvValue->put(1.0);
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after pvValue"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==1);
}

static void arrayTest()
{
    if(debug) {cout << endl << endl << "****arrayTest****" << endl;}
    bool result = false;
    uint32 nset = 0;
    size_t n = 10;
    shared_vector<double> values(n);

    PVStructurePtr pvRecordStructure(getStandardPVField()->scalarArray(pvDouble,""));
    PVRecordPtr pvRecord(PVRecord::create("doubleArrayRecord",pvRecordStructure));
    PVStructurePtr pvRequest(CreateRequest::create()->createRequest("value[array=1:3]"));
    PVCopyPtr pvCopy(PVCopy::create(pvRecordStructure,pvRequest,""));
    PVStructurePtr pvStructureCopy(pvCopy->createPVStructure());
    BitSetPtr bitSet(new BitSet(pvStructureCopy->getNumberFields()));
    PVDoubleArrayPtr pvValue(pvRecordStructure->getSubField<PVDoubleArray>("value"));
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "initial"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
    testOk1(result==false);
    testOk1(nset==0);
    for(size_t i=0; i<n; i++) values[i] = i + .06;
    const shared_vector<const double> yyy(freeze(values));
    pvValue->putFrom(yyy);
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after pvValue"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==1);
}

static void unionArrayTest()
{
    if(debug) {cout << endl << endl << "****unionArrayTest****" << endl;}
    bool result = false;
    uint32 nset = 0;
    size_t n = 10;
    shared_vector<double> values(n);
    for(size_t i=0; i<n; i++) values[i] = i + .06;
    PVDoubleArrayPtr pvDoubleArray =
        static_pointer_cast<PVDoubleArray>(PVDataCreate::getPVDataCreate()->createPVScalarArray(pvDouble));
    const shared_vector<const double> yyy(freeze(values));
    pvDoubleArray->putFrom(yyy);

    StandardFieldPtr standardField = getStandardField();
    FieldCreatePtr fieldCreate = getFieldCreate();
    StructureConstPtr top = fieldCreate->createFieldBuilder()->
        add("value",fieldCreate->createVariantUnion()) ->
        add("timeStamp", standardField->timeStamp()) ->
        addNestedStructure("subfield") ->
           add("value",fieldCreate->createVariantUnion()) ->
           endNested()->
        createStructure();
    PVStructurePtr pvRecordStructure(PVDataCreate::getPVDataCreate()->createPVStructure(top));
    PVRecordPtr pvRecord(PVRecord::create("unionArrayRecord",pvRecordStructure));
    PVUnionPtr pvUnion = pvRecord->getPVStructure()->getSubField<PVUnion>("value");
    pvUnion->set(pvDoubleArray);
    pvUnion = pvRecord->getPVStructure()->getSubField<PVUnion>("subfield.value");
    pvUnion->set(pvDoubleArray);
    if(debug) { cout << "initial\n" << pvRecordStructure << "\n";}

    PVStructurePtr pvRequest(CreateRequest::create()->createRequest("value[array=1:3]"));
    PVCopyPtr pvCopy(PVCopy::create(pvRecordStructure,pvRequest,""));
    PVStructurePtr pvStructureCopy(pvCopy->createPVStructure());
    BitSetPtr bitSet(new BitSet(pvStructureCopy->getNumberFields()));
    PVDoubleArrayPtr pvValue(pvRecordStructure->getSubField<PVDoubleArray>("value"));
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after get value"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==1);

    pvRequest = CreateRequest::create()->createRequest("subfield.value[array=1:3]");
    pvCopy = PVCopy::create(pvRecordStructure,pvRequest,"");
    pvStructureCopy = pvCopy->createPVStructure();
    bitSet = BitSetPtr(new BitSet(pvStructureCopy->getNumberFields()));
    pvValue = pvRecordStructure->getSubField<PVDoubleArray>("subfield.value");
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after get subfield.value"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==1);
}

static void timeStampTest()
{
    if(debug) {cout << endl << endl << "****timeStampTest****" << endl;}
    bool result = false;
    uint32 nset = 0;

    PVStructurePtr pvRecordStructure(getStandardPVField()->scalar(pvDouble,"timeStamp"));
    PVRecordPtr pvRecord(PVRecord::create("doubleRecord",pvRecordStructure));
    PVStructurePtr pvRequest(CreateRequest::create()->createRequest("value,timeStamp[timestamp=current]"));
    PVCopyPtr pvCopy(PVCopy::create(pvRecordStructure,pvRequest,""));
    PVStructurePtr pvStructureCopy(pvCopy->createPVStructure());
    BitSetPtr bitSet(new BitSet(pvStructureCopy->getNumberFields()));
    PVDoublePtr pvValue(pvRecordStructure->getSubField<PVDouble>("value"));
    if(debug) {
        cout << "initial"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after update"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==1);
    pvRecord->process();
    pvValue->put(1.0);
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after pvValue"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==2);
}

static void ignoreTest()
{
    if(debug) {cout << endl << endl << "****ignoreTest****" << endl;}
    bool result = false;
    uint32 nset = 0;

    PVStructurePtr pvRecordStructure(getStandardPVField()->scalar(pvDouble,"alarm,timeStamp"));
    PVRecordPtr pvRecord(PVRecord::create("doubleRecord",pvRecordStructure));
    PVStructurePtr pvRequest(CreateRequest::create()->createRequest("value,alarm[ignore=true],timeStamp[ignore=true]"));
    PVCopyPtr pvCopy(PVCopy::create(pvRecordStructure,pvRequest,""));
    PVStructurePtr pvStructureCopy(pvCopy->createPVStructure());
    BitSetPtr bitSet(new BitSet(pvStructureCopy->getNumberFields()));
    PVDoublePtr pvValue(pvRecordStructure->getSubField<PVDouble>("value"));
    PVStringPtr pvMessage(pvRecordStructure->getSubField<PVString>("alarm.message"));
    PVIntPtr pvUserTag(pvRecordStructure->getSubField<PVInt>("timeStamp.userTag"));
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "initial"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
    testOk1(result==false);
    testOk1(nset==0);
    pvMessage->put("test message");
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after pvMessage"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
    testOk1(result==false);
    testOk1(nset==1);
    pvUserTag->put(50);
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after pvMessage"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
    testOk1(result==false);
    testOk1(nset==2);
    pvValue->put(1.0);
    result = pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    nset = bitSet->cardinality();
    if(debug) {
        cout << "after pvValue"
             << " result " << (result ? "true" : "false")
             << " nset " << nset
             << " bitSet " << *bitSet
             << " pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
    testOk1(result==true);
    testOk1(nset==3);
}

static void debugOutput(const string& what, bool result, uint32 nSet, BitSetPtr bitSet, PVStructurePtr pvStructureCopy)
{
    if(debug) {
        cout << what
             << " result " << (result ? "true" : "false")
             << " nSet " << nSet
             << " bitSet " << *bitSet
             << "\n pvStructureCopy\n" << pvStructureCopy
             << "\n";
    }
}

static void dataDistributorTest()
{
    if(debug) {cout << endl << endl << "****dataDistributorTest****" << endl;}
    bool result = false;
    uint32 nSet = 0;

    // Create test structure
    PVStructurePtr pvRecordStructure(getStandardPVField()->scalar(pvInt,""));
    PVIntPtr pvValue(pvRecordStructure->getSubField<PVInt>("value"));
    PVRecordPtr pvRecord(PVRecord::create("intRecord",pvRecordStructure));
    if(debug) {
        cout << " pvRecordStructure\n" << pvRecordStructure
             << "\n";
    }

    // Request distributor plugin with trigger field value
    PVStructurePtr pvRequest(CreateRequest::create()->createRequest("_[distributor=trigger:value]"));

    // Create clients
    PVCopyPtr pvCopy1(PVCopy::create(pvRecordStructure,pvRequest,""));
    PVStructurePtr pvStructureCopy1(pvCopy1->createPVStructure());
    BitSetPtr bitSet1(new BitSet(pvStructureCopy1->getNumberFields()));

    PVCopyPtr pvCopy2(PVCopy::create(pvRecordStructure,pvRequest,""));
    PVStructurePtr pvStructureCopy2(pvCopy2->createPVStructure());
    BitSetPtr bitSet2(new BitSet(pvStructureCopy2->getNumberFields()));

    // Update 0: both clients get it
    result = pvCopy1->updateCopySetBitSet(pvStructureCopy1,bitSet1);
    nSet = bitSet1->cardinality();
    debugOutput("client 1: update 0", result, nSet, bitSet1, pvStructureCopy1);
    testOk1(result==true);
    testOk1(nSet==1);

    result = pvCopy2->updateCopySetBitSet(pvStructureCopy2,bitSet2);
    nSet = bitSet2->cardinality();
    debugOutput("client 2: update 0", result, nSet, bitSet2, pvStructureCopy2);
    testOk1(result==true);
    testOk1(nSet==1);

    // Update 1: only client 1 gets it
    pvValue->put(1);

    result = pvCopy1->updateCopySetBitSet(pvStructureCopy1,bitSet1);
    nSet = bitSet1->cardinality();
    debugOutput("client 1: update 1", result, nSet, bitSet1, pvStructureCopy1);
    testOk1(result==true);
    testOk1(nSet==1);

    result = pvCopy2->updateCopySetBitSet(pvStructureCopy2,bitSet2);
    nSet = bitSet2->cardinality();
    debugOutput("client 2: update 1", result, nSet, bitSet2, pvStructureCopy2);
    testOk1(result==false);
    testOk1(nSet==0);

    // Update 2: only client 2 gets it
    pvValue->put(2);

    result = pvCopy1->updateCopySetBitSet(pvStructureCopy1,bitSet1);
    nSet = bitSet1->cardinality();
    debugOutput("client 1: update 2", result, nSet, bitSet1, pvStructureCopy1);
    testOk1(result==false);
    testOk1(nSet==0);

    result = pvCopy2->updateCopySetBitSet(pvStructureCopy2,bitSet2);
    nSet = bitSet2->cardinality();
    debugOutput("client 2: update 2", result, nSet, bitSet2, pvStructureCopy2);
    testOk1(result==true);
    testOk1(nSet==1);

    // Update 3: only client 1 gets it
    pvValue->put(3);

    result = pvCopy1->updateCopySetBitSet(pvStructureCopy1,bitSet1);
    nSet = bitSet1->cardinality();
    debugOutput("client 1: update 3", result, nSet, bitSet1, pvStructureCopy1);
    testOk1(result==true);
    testOk1(nSet==1);

    result = pvCopy2->updateCopySetBitSet(pvStructureCopy2,bitSet2);
    nSet = bitSet2->cardinality();
    debugOutput("client 2: update 3", result, nSet, bitSet2, pvStructureCopy2);
    testOk1(result==false);
    testOk1(nSet==0);

    // Update 4: only client 2 gets it
    pvValue->put(4);

    result = pvCopy1->updateCopySetBitSet(pvStructureCopy1,bitSet1);
    nSet = bitSet1->cardinality();
    debugOutput("client 1: update 4", result, nSet, bitSet1, pvStructureCopy1);
    testOk1(result==false);
    testOk1(nSet==0);

    result = pvCopy2->updateCopySetBitSet(pvStructureCopy2,bitSet2);
    nSet = bitSet2->cardinality();
    debugOutput("client 2: update 4", result, nSet, bitSet2, pvStructureCopy2);
    testOk1(result==true);
    testOk1(nSet==1);
}

MAIN(testPlugin)
{
    testPlan(46);
    PVDatabasePtr pvDatabase(PVDatabase::getMaster());
    deadbandTest();
    arrayTest();
    unionArrayTest();
    timeStampTest();
    ignoreTest();
    dataDistributorTest();
    return 0;
}
