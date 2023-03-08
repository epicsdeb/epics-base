/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/*
 * Run pvData tests as a batch.
 *
 * Do *not* include performance measurements here, they don't help to
 * prove functionality (which is the point of this convenience routine).
 */

#include <stdio.h>
#include <epicsThread.h>
#include <epicsUnitTest.h>
#include <epicsExit.h>

/* copy */
int testCreateRequest(void);

/* misc */
int testBaseException(void);
int testBitSet(void);
int testByteBuffer(void);
int testOverrunBitSet(void);
int testSerialization(void);
int testSharedVector(void);
int testThread(void);
int testEvent(void);
int testTimeStamp(void);
int testTimer(void);
int testTypeCast(void);

/* property */
int testCreateRequest(void);

/* pv */
int testBitSetUtil(void);
int testConvert(void);
int testFieldBuilder(void);
int testIntrospect(void);
int testOperators(void);
int testPVData(void);
int testPVScalarArray(void);
int testPVStructureArray(void);
int testPVType(void);
int testPVUnion(void);
int testStandardField(void);
int testStandardPVField(void);

void pvDataAllTests(void)
{
    testHarness();

    /* pv */
    runTest(testBitSetUtil);
    runTest(testConvert);
    runTest(testFieldBuilder);
    runTest(testIntrospect);
    runTest(testOperators);
    runTest(testPVData);
    runTest(testPVScalarArray);
    runTest(testPVStructureArray);
    runTest(testPVType);
    runTest(testPVUnion);
    runTest(testStandardField);
    runTest(testStandardPVField);

    /* misc */
    runTest(testBaseException);
    runTest(testBitSet);
    runTest(testByteBuffer);
    runTest(testOverrunBitSet);
    runTest(testSerialization);
    runTest(testSharedVector);
    runTest(testThread);
    runTest(testEvent);
    runTest(testTimeStamp);
    runTest(testTimer);
    runTest(testTypeCast);

    /* copy */
    runTest(testCreateRequest);

    /* property */
    runTest(testCreateRequest);

    epicsExit(0);   /* Trigger test harness */
}
