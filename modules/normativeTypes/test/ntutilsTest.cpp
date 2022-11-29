/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/ntutils.h>


using namespace epics::nt;

void test_is_a()
{
    testDiag("test_is_a");

    testOk1(NTUtils::is_a("epics:nt/NTTable:1.0", "epics:nt/NTTable:1.0"));
    testOk1(NTUtils::is_a("epics:nt/NTTable:2.0", "epics:nt/NTTable:2.0"));
    testOk1(NTUtils::is_a("epics:nt/NTTable:1.0", "epics:nt/NTTable:1.1"));
    testOk1(NTUtils::is_a("epics:nt/NTTable:1.1", "epics:nt/NTTable:1.0"));

    testOk1(!NTUtils::is_a("epics:nt/NTTable:1.0", "epics:nt/NTTable:2.0"));
    testOk1(!NTUtils::is_a("epics:nt/NTTable:2.0", "epics:nt/NTTable:1.0"));
    testOk1(!NTUtils::is_a("epics:nt/NTTable:1.3", "epics:nt/NTTable:2.3"));
    testOk1(!NTUtils::is_a("epics:nt/NTTable:1.0", "epics:nt/NTTable:11.0"));
    testOk1(!NTUtils::is_a("epics:nt/NTTable:11.0", "epics:nt/NTTable:1.0"));
    testOk1(!NTUtils::is_a("epics:nt/NTTable:1.0", "epics:nt/NTMatrix:1.0"));
}

MAIN(testNTUtils) {
    testPlan(10);
    test_is_a();
    return testDone();
}


