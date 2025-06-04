/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <stdexcept>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/epicsException.h>
#include <pv/reftrack.h>

namespace {

void testReg()
{
    testDiag("testReg()");

    static size_t cnt1 = 1;

    epics::registerRefCounter("cnt1", &cnt1);

    testOk1(epics::readRefCounter("cnt1")==1);

    cnt1 = 2;

    testOk1(epics::readRefCounter("cnt1")==2);

    epics::unregisterRefCounter("cnt1", &cnt1);

    testOk1(epics::readRefCounter("cnt1")==0);
}

void testSnap()
{
    testDiag("testSnap()");

    static size_t cnt1 = 1,
                  cnt2 = 10;

    epics::registerRefCounter("cnt1", &cnt1);
    epics::registerRefCounter("cnt2", &cnt2);

    epics::RefSnapshot snap1;
    snap1.update();

    cnt1 = 2;
    cnt2 = 11;
    static size_t cnt3 = 21;
    epics::registerRefCounter("cnt3", &cnt3);

    epics::RefSnapshot snap2;
    snap2.update();

    cnt1 = 4;
    cnt2 = 13;
    cnt3 = 23;
    epics::unregisterRefCounter("cnt1", &cnt1);

    epics::RefSnapshot snap3;
    snap3.update();

    testOk1(snap1.size()>=2);
    testOk1(snap2.size()>=3);
    testOk1(snap3.size()>=2);

    epics::RefSnapshot delta12(snap2-snap1);
    epics::RefSnapshot delta13(snap3-snap1);
    epics::RefSnapshot delta23(snap3-snap2);

    testOk1(delta12.size()>=3);
    testOk1(delta13.size()>=3);
    testOk1(delta23.size()>=3);

    testOk1(delta12["cnt1"]==epics::RefSnapshot::Count(2, 1));
    testOk1(delta12["cnt2"]==epics::RefSnapshot::Count(11, 1));
    testOk1(delta12["cnt3"]==epics::RefSnapshot::Count(21, 21));

    testOk1(delta23["cnt1"]==epics::RefSnapshot::Count(0, -2));
    testOk1(delta23["cnt2"]==epics::RefSnapshot::Count(13, 2));
    testOk1(delta23["cnt3"]==epics::RefSnapshot::Count(23, 2));

    testOk1(delta13["cnt1"]==epics::RefSnapshot::Count(0, -1));
    testOk1(delta13["cnt2"]==epics::RefSnapshot::Count(13, 3));
    testOk1(delta13["cnt3"]==epics::RefSnapshot::Count(23, 23));
}

} // namespace

MAIN(test_reftrack)
{
    testPlan(18);
    try {
        testReg();
        testSnap();
    }catch(std::exception& e){
        PRINT_EXCEPTION(e);
        testAbort("Unexpected exception: %s", e.what());
    }
    return testDone();
}
