/*
 * Run pvDatabase tests as a batch.
 *
 * Do *not* include performance measurements here, they don't help to
 * prove functionality (which is the point of this convenience routine).
 */

#include <stdio.h>
#include <epicsThread.h>
#include <epicsUnitTest.h>
#include <epicsExit.h>

/* src */
int testExampleRecord(void);
int testPVCopy(void);
int testPlugin(void);
int testPVRecord(void);
int testLocalProvider(void);
int testPVAServer(void);

void pvDatabaseAllTests(void)
{
    testHarness();

    /* src */
    runTest(testExampleRecord);
    runTest(testPVCopy);
    runTest(testPlugin);
    runTest(testPVRecord);
    runTest(testLocalProvider);
    runTest(testPVAServer);

    epicsExit(0);   /* Trigger test harness */
}
