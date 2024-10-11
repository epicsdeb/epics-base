/*************************************************************************\
* Copyright (c) 2012 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 * Run filter tests as a batch.
 */

#include "epicsUnitTest.h"
#include "epicsExit.h"
#include "dbmf.h"

int tsTest(void);
int dbndTest(void);
int syncTest(void);
int arrTest(void);
int decTest(void);

void epicsRunFilterTests(void)
{
    testHarness();

    runTest(tsTest);
    runTest(dbndTest);
    runTest(syncTest);
    runTest(arrTest);
    runTest(decTest);

    dbmfFreeChunks();

    epicsExit(0);   /* Trigger test harness */
}
