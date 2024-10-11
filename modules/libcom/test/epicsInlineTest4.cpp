/*************************************************************************\
* Copyright (c) 2015 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include "compilerSpecific.h"
#include "epicsUnitTest.h"

static EPICS_ALWAYS_INLINE int epicsInlineTestFn1(void)
{
    return 4;
}

inline int epicsInlineTestFn2(void)
{
    return 42;
}

extern "C"
void epicsInlineTest4(void)
{
    testDiag("epicsInlineTest4()");
    testOk1(epicsInlineTestFn1()==4);
    testOk1(epicsInlineTestFn2()==42);
}
