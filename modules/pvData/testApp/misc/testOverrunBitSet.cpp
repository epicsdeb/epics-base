/* testOverrunBitSet.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2013.09.16 */

#include <iostream>
#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/bitSetUtil.h>
#include <pv/standardPVField.h>

#include <epicsAssert.h>

using namespace epics::pvData;
using std::string;
using std::cout;
using std::endl;

static StandardPVFieldPtr standardPVField = getStandardPVField();

void test()
{
     string buffer;
     string properties("alarm,timeStamp,display");
     PVStructurePtr pvStructure = standardPVField->scalar(pvDouble,properties);
     PVDoublePtr pvValue = pvStructure->getSubField<PVDouble>("value");
     uint32 valueOffset = (uint32) pvValue->getFieldOffset();
     PVStructurePtr pvAlarm = pvStructure->getSubField<PVStructure>("alarm");
     PVIntPtr pvSeverity = pvAlarm->getSubField<PVInt>("severity");
     PVStringPtr pvMessage = pvAlarm->getSubField<PVString>("message");
     uint32 severityOffset = (uint32) pvSeverity->getFieldOffset();
     uint32 messageOffset = (uint32) pvMessage->getFieldOffset();
     PVStructurePtr pvTimeStamp = pvStructure->getSubField<PVStructure>("timeStamp");
     PVLongPtr pvSeconds = pvTimeStamp->getSubField<PVLong>("secondsPastEpoch");
     PVIntPtr pvNanoseconds = pvTimeStamp->getSubField<PVInt>("nanoseconds");
     PVIntPtr pvUserTag = pvTimeStamp->getSubField<PVInt>("userTag");
     uint32 timeStampOffset = (uint32) pvTimeStamp->getFieldOffset();
     uint32 secondsOffset = (uint32) pvSeconds->getFieldOffset();
     uint32 nanosecondsOffset = (uint32) pvNanoseconds->getFieldOffset();
     uint32 userTagOffset = (uint32) pvUserTag->getFieldOffset();
     uint32 nfields = (uint32) pvStructure->getNumberFields();
     BitSetPtr changeBitSet = BitSet::create(nfields);
     BitSetPtr userChangeBitSet = BitSet::create(nfields);
     BitSetPtr userOverrunBitSet = BitSet::create(nfields);
     pvValue->put(1.0); changeBitSet->set(valueOffset);
     pvSeverity->put(2); changeBitSet->set(severityOffset);
     pvMessage->put("error"); changeBitSet->set(messageOffset);
     pvSeconds->put(1); changeBitSet->set(secondsOffset);
     pvNanoseconds->put(1000000); changeBitSet->set(nanosecondsOffset);
     pvUserTag->put(1); changeBitSet->set(userTagOffset);
     userOverrunBitSet->or_and(*changeBitSet.get(),*userChangeBitSet.get());
     (*userChangeBitSet)|=(*changeBitSet.get());
     changeBitSet->clear();
     pvValue->put(2.0); changeBitSet->set(valueOffset);
     pvSeverity->put(0); changeBitSet->set(severityOffset);
     pvMessage->put(""); changeBitSet->set(messageOffset);
     pvSeconds->put(2); changeBitSet->set(secondsOffset);
     pvNanoseconds->put(0); changeBitSet->set(nanosecondsOffset);
     pvUserTag->put(0); changeBitSet->set(userTagOffset);
     userOverrunBitSet->or_and(*changeBitSet.get(),*userChangeBitSet.get());
     (*userChangeBitSet)|=(*changeBitSet.get());
     testOk1(userChangeBitSet->cardinality()==6);
     testOk1(userChangeBitSet->get(valueOffset));
     testOk1(userChangeBitSet->get(severityOffset));
     testOk1(userChangeBitSet->get(messageOffset));
     testOk1(userChangeBitSet->get(secondsOffset));
     testOk1(userChangeBitSet->get(nanosecondsOffset));
     testOk1(userChangeBitSet->get(userTagOffset));
     testOk1(userOverrunBitSet->cardinality()==6);
     testOk1(userOverrunBitSet->get(valueOffset));
     testOk1(userOverrunBitSet->get(severityOffset));
     testOk1(userOverrunBitSet->get(messageOffset));
     testOk1(userOverrunBitSet->get(secondsOffset));
     testOk1(userOverrunBitSet->get(nanosecondsOffset));
     testOk1(userOverrunBitSet->get(userTagOffset));

     BitSetUtil::compress(userChangeBitSet,pvStructure);
     BitSetUtil::compress(userOverrunBitSet,pvStructure);
     testOk1(userChangeBitSet->cardinality()==4);
     testOk1(userChangeBitSet->get(valueOffset));
     testOk1(userChangeBitSet->get(severityOffset));
     testOk1(userChangeBitSet->get(messageOffset));
     testOk1(userChangeBitSet->get(timeStampOffset));
     testOk1(userOverrunBitSet->cardinality()==4);
     testOk1(userOverrunBitSet->get(valueOffset));
     testOk1(userOverrunBitSet->get(severityOffset));
     testOk1(userOverrunBitSet->get(messageOffset));
     testOk1(userOverrunBitSet->get(timeStampOffset));

     changeBitSet->clear();
     userChangeBitSet->clear();
     userOverrunBitSet->clear();
     pvValue->put(1.0); changeBitSet->set(valueOffset);
     pvSeconds->put(3); changeBitSet->set(secondsOffset);
     pvNanoseconds->put(0); changeBitSet->set(nanosecondsOffset);
     userOverrunBitSet->or_and(*changeBitSet.get(),*userChangeBitSet.get());
     (*userChangeBitSet)|=(*changeBitSet.get());
     testOk1(userChangeBitSet->cardinality()==3);
     testOk1(userChangeBitSet->get(valueOffset));
     testOk1(userChangeBitSet->get(secondsOffset));
     testOk1(userChangeBitSet->get(nanosecondsOffset));
     testOk1(userOverrunBitSet->cardinality()==0);

     changeBitSet->clear();
     pvValue->put(2.0); changeBitSet->set(valueOffset);
     userOverrunBitSet->or_and(*changeBitSet.get(),*userChangeBitSet.get());
     (*userChangeBitSet)|=(*changeBitSet.get());
     testOk1(userChangeBitSet->cardinality()==3);
     testOk1(userChangeBitSet->get(valueOffset));
     testOk1(userChangeBitSet->get(secondsOffset));
     testOk1(userChangeBitSet->get(nanosecondsOffset));
     testOk1(userOverrunBitSet->cardinality()==1);
     testOk1(userOverrunBitSet->get(valueOffset));

     BitSetUtil::compress(userChangeBitSet,pvStructure);
     BitSetUtil::compress(userOverrunBitSet,pvStructure);
     testOk1(userChangeBitSet->cardinality()==3);
     testOk1(userChangeBitSet->get(valueOffset));
     testOk1(userChangeBitSet->get(secondsOffset));
     testOk1(userChangeBitSet->get(nanosecondsOffset));
     testOk1(userOverrunBitSet->cardinality()==1);
     testOk1(userOverrunBitSet->get(valueOffset));
}


MAIN(testOverrunBitSet)
{
    testPlan(41);
    testDiag("Tests for changeBitSet and overrunBitSet");
    test();
    return testDone();
}
