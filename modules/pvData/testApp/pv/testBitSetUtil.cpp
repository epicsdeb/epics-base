/* testBitSetUtil.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2013.05 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <sstream>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/bitSetUtil.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>

using namespace epics::pvData;
using std::string;
using std::tr1::static_pointer_cast;

static FieldCreatePtr fieldCreate;
static PVDataCreatePtr pvDataCreate;
static StandardFieldPtr standardField;
static StandardPVFieldPtr standardPVField;

static void test()
{
    std::ostringstream oss;

    testDiag("\ntestBitSetUtil\n");
    StringArray fieldNames;
    PVFieldPtrArray pvFields;
    fieldNames.reserve(5);
    pvFields.reserve(5);
    fieldNames.push_back("timeStamp");
    fieldNames.push_back("alarm");
    fieldNames.push_back("voltage");
    fieldNames.push_back("power");
    fieldNames.push_back("current");
    pvFields.push_back(
        pvDataCreate->createPVStructure(standardField->timeStamp()));
    pvFields.push_back(
        pvDataCreate->createPVStructure(standardField->alarm()));
    pvFields.push_back(
        pvDataCreate->createPVStructure(
            standardField->scalar(pvDouble,"alarm")));
    pvFields.push_back(
        pvDataCreate->createPVStructure(
            standardField->scalar(pvDouble,"alarm")));
    pvFields.push_back(
        pvDataCreate->createPVStructure(
            standardField->scalar(pvDouble,"alarm")));
    PVStructurePtr pvs =  pvDataCreate->createPVStructure(
         fieldNames,pvFields);

    int32 nfields = (int32)pvs->getNumberFields();
    BitSetPtr bitSet = BitSet::create(nfields);
    for(int32 i=0; i<nfields; i++) bitSet->set(i);

    BitSetUtil::compress(bitSet,pvs);

    bitSet->clear();
    PVFieldPtr pvField = pvs->getSubField<PVStructure>("timeStamp");
    int32 offsetTimeStamp = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVLong>("timeStamp.secondsPastEpoch");
    int32 offsetSeconds = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVInt>("timeStamp.nanoseconds");
    int32 offsetNano = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVInt>("timeStamp.userTag");
    int32 offsetUserTag = (int32)pvField->getFieldOffset();
    bitSet->set(offsetSeconds);
    BitSetUtil::compress(bitSet,pvs);
    testOk1(bitSet->get(offsetSeconds)==true);
    bitSet->set(offsetNano);
    bitSet->set(offsetUserTag);

    BitSetUtil::compress(bitSet,pvs);
    testOk1(bitSet->get(offsetSeconds)==false);
    testOk1(bitSet->get(offsetTimeStamp)==true);

    bitSet->clear();

    pvField = pvs->getSubField<PVStructure>("current");
    int32 offsetCurrent = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVDouble>("current.value");
    int32 offsetValue = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVStructure>("current.alarm");
    int32 offsetAlarm = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVInt>("current.alarm.severity");
    int32 offsetSeverity = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVInt>("current.alarm.status");
    int32 offsetStatus = (int32)pvField->getFieldOffset();
    pvField = pvs->getSubField<PVString>("current.alarm.message");
    int32 offsetMessage = (int32)pvField->getFieldOffset();
    bitSet->set(offsetValue);
    bitSet->set(offsetSeverity);
    bitSet->set(offsetStatus);
    bitSet->set(offsetMessage);

    BitSetUtil::compress(bitSet,pvs);

    testOk1(bitSet->get(offsetCurrent)==true);
    bitSet->clear();
    bitSet->set(offsetSeverity);
    bitSet->set(offsetStatus);
    bitSet->set(offsetMessage);

    BitSetUtil::compress(bitSet,pvs);

    testOk1(bitSet->get(offsetAlarm)==true);
    bitSet->clear();
    printf("testBitSetUtil PASSED\n");
}

MAIN(testBitSetUtil)
{
    testPlan(5);
    fieldCreate = getFieldCreate();
    pvDataCreate = getPVDataCreate();
    standardField = getStandardField();
    standardPVField = getStandardPVField();
    test();
    return testDone();
}
