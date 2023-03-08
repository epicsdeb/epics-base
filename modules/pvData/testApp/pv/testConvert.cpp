/* testConvert.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2010.11 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>

#include <epicsAssert.h>
#include <epicsExit.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>

using namespace epics::pvData;

static void testFromString()
{
    StringArray inp(2);

    inp[0] = "0";
    inp[1] = "1";

    PVScalarArrayPtr A(getPVDataCreate()->createPVScalarArray(pvInt));
    PVScalarArrayPtr B(getPVDataCreate()->createPVScalarArray(pvString));

    testOk1(2==getConvert()->fromStringArray(A, 0, inp.size(), inp, 0));
    testOk1(2==getConvert()->fromStringArray(B, 0, inp.size(), inp, 0));

    PVIntArrayPtr Ax(std::tr1::static_pointer_cast<PVIntArray>(A));
    PVStringArrayPtr Bx(std::tr1::static_pointer_cast<PVStringArray>(B));

    PVIntArray::const_svector Adata(Ax->view());
    PVStringArray::const_svector Bdata(Bx->view());

    testOk1(inp.size()==Adata.size());
    if(inp.size()==Adata.size())
        testOk1(Adata[0]==0 && Adata[1]==1);
    else
        testFail("Can't compare");

    testOk1(inp.size()==Bdata.size());
    if(inp.size()==Bdata.size())
        testOk1(Bdata[0]=="0" && Bdata[1]=="1");
    else
        testFail("Can't compare");
}

MAIN(testConvert)
{
    testPlan(6);
    testFromString();
    return testDone();
}
