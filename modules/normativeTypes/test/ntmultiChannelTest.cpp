/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/*
 * ntmutiChannelTest.cpp
 *
 *  Created on: 2014.08
 *      Author: Marty Kraimer
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <iostream>

#include <epicsUnitTest.h>
#include <testMain.h>


#include <pv/nt.h>
#include <pv/sharedVector.h>
#include <pv/ntmultiChannel.h>

using namespace epics::pvData;
using namespace epics::nt;
using std::string;
using std::cout;
using std::endl;
using std::vector;

static bool debug = false;

static  FieldCreatePtr fieldCreate = getFieldCreate();
static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static NTFieldPtr ntField = NTField::get();
static PVNTFieldPtr pvntField = PVNTField::get();

static void test()
{
    NTMultiChannelBuilderPtr builder = NTMultiChannel::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTMultiChannelPtr multiChannel = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addSeverity() ->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            create();
    testOk1(multiChannel.get() != 0);

    testOk1(NTMultiChannel::is_a(multiChannel->getPVStructure()));
    testOk1(NTMultiChannel::isCompatible(multiChannel->getPVStructure()));

    PVStructurePtr pvStructure = multiChannel->getPVStructure();
    testOk1(pvStructure.get()!=NULL);
    testOk1(NTMultiChannel::is_a(pvStructure->getStructure()));
    size_t nchan = 3;
    shared_vector<string> names(nchan);
    names[0] = "channel 0";
    names[1] = "channel 1";
    names[2] = "channel 2";
    shared_vector<const string> channelNames(freeze(names));
    PVStringArrayPtr pvChannelName = multiChannel->getChannelName();
    pvChannelName->replace(channelNames);
    if(debug) {cout << *pvStructure << endl;}
    UnionConstPtr unionPtr =
       fieldCreate->createFieldBuilder()->
           add("doubleValue", pvDouble)->
           add("intValue", pvInt)->
           createUnion();
    multiChannel = builder->
            value(unionPtr) ->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addSeverity() ->
            addIsConnected() ->
            create();
    testOk1(multiChannel.get() != 0);
    pvStructure = multiChannel->getPVStructure();
    if(debug) {cout << *pvStructure << endl;}
    pvChannelName = multiChannel->getChannelName();
    pvChannelName->replace(channelNames);
    PVUnionArrayPtr pvValue = multiChannel->getValue();
    shared_vector<PVUnionPtr> unions(nchan);
    unions[0] = pvDataCreate->createPVUnion(unionPtr);
    unions[1] = pvDataCreate->createPVUnion(unionPtr);
    unions[2] = pvDataCreate->createPVUnion(unionPtr);
    unions[0]->select("doubleValue");
    unions[1]->select("intValue");
    unions[2]->select("intValue");
    PVDoublePtr pvDouble = unions[0]->get<PVDouble>();
    pvDouble->put(1.235);
    PVIntPtr pvInt = unions[1]->get<PVInt>();
    pvInt->put(5);
    pvInt = unions[2]->get<PVInt>();
    pvInt->put(7);
    pvValue->replace(freeze(unions));
    shared_vector<int32> severities(nchan);
    severities[0] = 0;
    severities[1] = 1;
    severities[2] = 2;
    PVIntArrayPtr pvSeverity = multiChannel->getSeverity();
    pvSeverity->replace(freeze(severities));
    if(debug) {cout << *pvStructure << endl;}
    PVBooleanArrayPtr pvIsConnected = multiChannel->getIsConnected();
    shared_vector<const epics::pvData::boolean> isConnected = pvIsConnected->view();
    multiChannel = builder->
            value(unionPtr) ->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addSeverity() ->
            addStatus() ->
            addMessage() ->
            addSecondsPastEpoch() ->
            addNanoseconds() ->
            addUserTag() ->
            addIsConnected() ->
            create();
    testOk1(multiChannel.get() != 0);
    pvStructure = multiChannel->getPVStructure();
    if(debug) {cout << *pvStructure << endl;}
    testOk1(NTMultiChannel::isCompatible(pvStructure)==true);
    PVStructurePtr pvTimeStamp = multiChannel->getTimeStamp();
    testOk1(pvTimeStamp.get() !=0);
    PVStructurePtr pvAlarm = multiChannel->getAlarm();
    testOk1(pvAlarm.get() !=0);
    pvValue = multiChannel->getValue();
    testOk1(pvValue.get() !=0);
    pvChannelName = multiChannel->getChannelName();
    testOk1(pvChannelName.get() !=0);
    pvIsConnected = multiChannel->getIsConnected();
    testOk1(pvIsConnected.get() !=0);
    pvSeverity = multiChannel->getSeverity();
    testOk1(pvSeverity.get() !=0);
    PVIntArrayPtr pvStatus = multiChannel->getStatus();
    testOk1(pvStatus.get() !=0);
    PVStringArrayPtr pvMessage = multiChannel->getMessage();
    testOk1(pvMessage.get() !=0);
    PVLongArrayPtr pvSecondsPastEpoch = multiChannel->getSecondsPastEpoch();
    testOk1(pvSecondsPastEpoch.get() !=0);
    PVIntArrayPtr pvNanoseconds = multiChannel->getNanoseconds();
    testOk1(pvNanoseconds.get() !=0);
    PVIntArrayPtr pvUserTag = multiChannel->getUserTag();
    testOk1(pvUserTag.get() !=0);
    PVStringPtr pvDescriptor = multiChannel->getDescriptor();
    testOk1(pvDescriptor.get() !=0);
}


void test_wrap()
{
    testDiag("test_wrap");

    NTMultiChannelPtr nullPtr = NTMultiChannel::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTMultiChannel::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTMultiChannelBuilderPtr builder = NTMultiChannel::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    NTMultiChannelPtr ptr = NTMultiChannel::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    builder = NTMultiChannel::createBuilder();
    ptr = NTMultiChannel::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testCreateRequest)
{
    testPlan(27);
    test();
    test_wrap();
    return testDone();
}

