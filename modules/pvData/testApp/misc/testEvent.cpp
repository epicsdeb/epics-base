/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/event.h>

using namespace epics::pvData;

static void testBasicEvent()
{
    testDiag("testBasicEvent");

    Event e;

    // 0 signals, 2 waits
    testOk1(!e.tryWait());
    testOk1(!e.tryWait());

    // signal, wait, signal, wait
    e.signal();
    testOk1(e.tryWait());
    e.signal();
    testOk1(e.tryWait());

    // 1 signal, 2 waits
    e.signal();
    testOk1(e.tryWait());
    testOk1(!e.tryWait());

    // 2 signals, 2 waits
    e.signal();
    e.signal();
    testOk1(e.tryWait());
    testOk1(!e.tryWait());

    // 0 signals, 1 wait
    testOk1(!e.tryWait());
}

MAIN(testEvent)
{
    testPlan(9);
    testBasicEvent();
    return testDone();
}
