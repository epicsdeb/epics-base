/*************************************************************************\
* Copyright (c) 2006 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/* epicsUnitTestTest.c,v 1.1.2.1 2006/03/30 20:46:16 anj Exp
 * Author: Andrew Johnson
 *
 * Test for the unit test module...
 */

#include "epicsUnitTest.h"

#define testOk1_success 1
#define testOk1_failure 0

int main () {
    testPlan(11);
    testOk(1, "testOk(1)");
    testOk(0, "testOk(0)");
    testPass("testPass()");
    testFail("testFail()");
    testSkip(2, "Skipping two");
    testTodoBegin("Testing Todo");
    testOk(1, "Todo pass");
    testOk(0, "Todo fail");
    testSkip(1, "Todo skip");
    testTodoEnd();
    testOk1(testOk1_success);
    testOk1(testOk1_failure);
    testDiag("Diagnostic");
/*    testAbort("testAbort"); */
    return testDone();
}
