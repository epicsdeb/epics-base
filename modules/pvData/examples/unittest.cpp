/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* c++ unittest skeleton */

#include <stdexcept>

#include <testMain.h>

#include <pv/pvUnitTest.h>
#include <pv/epicsException.h>

namespace {
void testCase1() {
    testEqual(1+1, 2);
}
} // namespace

MAIN(testUnitTest)
{
    testPlan(1);
    try {
        testCase1();
    }catch(std::exception& e){
        PRINT_EXCEPTION(e); // print stack trace if thrown with THROW_EXCEPTION()
        testAbort("Unhandled exception: %s", e.what());
    }
    return testDone();
}
