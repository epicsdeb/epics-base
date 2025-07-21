/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/*
 * ntscalarMultiChannelTest.cpp
 *
 *  Created on: 2015.08
 *     Authors: Marty Kraimer, Dave Hickin
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
#include <pv/ntscalarMultiChannel.h>

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
    NTScalarMultiChannelBuilderPtr builder = NTScalarMultiChannel::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTScalarMultiChannelPtr multiChannel = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addSeverity() ->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            create();
    testOk1(multiChannel.get() != 0);

    testOk1(NTScalarMultiChannel::is_a(multiChannel->getPVStructure()));
    testOk1(NTScalarMultiChannel::isCompatible(multiChannel->getPVStructure()));

    PVStructurePtr pvStructure = multiChannel->getPVStructure();
    testOk1(pvStructure.get()!=NULL);
    testOk1(NTScalarMultiChannel::is_a(pvStructure->getStructure()));
    size_t nchan = 3;
    shared_vector<string> names(nchan);
    names[0] = "channel 0";
    names[1] = "channel 1";
    names[2] = "channel 2";
    shared_vector<const string> channelNames(freeze(names));
    PVStringArrayPtr pvChannelName = multiChannel->getChannelName();
    pvChannelName->replace(channelNames);
    if(debug) {cout << *pvStructure << endl;}
    multiChannel = builder->
            value(pvDouble) ->
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
    PVDoubleArrayPtr pvValue = multiChannel->getValue<PVDoubleArray>();
    PVDoubleArray::svector doubles(nchan);
    doubles.resize(nchan);
    doubles[0] = 3.14159;
    doubles[1] = 2.71828;
    doubles[2] = 137.036;
    pvValue->replace(freeze(doubles));
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
            value(pvDouble) ->
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
    testOk1(NTScalarMultiChannel::isCompatible(pvStructure)==true);
    PVStructurePtr pvTimeStamp = multiChannel->getTimeStamp();
    testOk1(pvTimeStamp.get() !=0);
    PVStructurePtr pvAlarm = multiChannel->getAlarm();
    testOk1(pvAlarm.get() !=0);
    pvValue = multiChannel->getValue<PVDoubleArray>();
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

    NTScalarMultiChannelPtr nullPtr = NTScalarMultiChannel::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTScalarMultiChannel::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTScalarMultiChannelBuilderPtr builder = NTScalarMultiChannel::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    NTScalarMultiChannelPtr ptr = NTScalarMultiChannel::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    builder = NTScalarMultiChannel::createBuilder();
    ptr = NTScalarMultiChannel::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testCreateRequest)
{
    testPlan(27);
    test();
    test_wrap();
    return testDone();
}

