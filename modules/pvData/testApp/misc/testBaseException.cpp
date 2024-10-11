/* testBaseException.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Matej Sekoranja Date: 2010.10.18 */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/epicsException.h>

using namespace epics::pvData;

struct Unroller
{
    template <int N>
    void unroll(double d) {
        unroll<N-1>(d);
    }
};

template<>
void Unroller::unroll<0>(double /*d*/) {
    THROW_BASE_EXCEPTION("the root cause");
}

void testBaseExceptionTest() {
    printf("testBaseException... ");

    try {
        THROW_BASE_EXCEPTION("all is OK");
    } catch (BaseException& be) {
        printf("\n\n%s\n\n", be.what());
    }

    testPass("testBaseException");
}

void testLogicException() {
    try {
        THROW_EXCEPTION(std::logic_error("There is a logic_error"));
    } catch (std::logic_error& be) {
        printf("\n\n%s\n\n", be.what());
        PRINT_EXCEPTION2(be, stdout);
    }

    try {
        THROW_EXCEPTION2(std::logic_error, "There is another logic_error");
    } catch (std::logic_error& be) {
        printf("\n\n%s\n\n", be.what());
        printf("%s\n", SHOW_EXCEPTION(be).c_str());
    }
    testPass("testLogicException");
}

MAIN(testBaseException)
{
    testPlan(2);
    testDiag("Tests base exception");
    testLogicException();
    testBaseExceptionTest();
    return testDone();
}
