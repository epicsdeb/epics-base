/* testPVStructureArray.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2010.10 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>

#include <epicsAssert.h>
#include <epicsExit.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>

#include <epicsUnitTest.h>
#include <testMain.h>

using namespace epics::pvData;

static FieldCreatePtr fieldCreate;
static PVDataCreatePtr pvDataCreate;
static StandardFieldPtr standardField;
static StandardPVFieldPtr standardPVField;
static ConvertPtr convert;

static void testBasic()
{
    testDiag("Basic structure array ops");

    StructureArrayConstPtr alarmtype(
        fieldCreate->createStructureArray(standardField->alarm()));

    PVStructureArrayPtr alarmarr(alarmtype->build());

    testOk1(alarmarr->getLength()==0);

    alarmarr->setLength(5);

    testOk1(alarmarr->getLength()==5);

    PVStructureArray::const_svector aview = alarmarr->view();

    testOk1(aview.size()==5);
    testOk1(aview[4].get()==NULL);

    alarmarr->append(2);

    testOk1(alarmarr->getLength()==7);

    aview = alarmarr->view();

    testOk1(aview[4].get()==NULL);
    testOk1(aview[5].get()!=NULL);
    testOk1(aview[6].get()!=NULL);
}

static void testCompress()
{
    testDiag("Test structure array compress");

    StructureArrayConstPtr alarmtype(
        fieldCreate->createStructureArray(standardField->alarm()));

    PVStructureArrayPtr alarmarr(alarmtype->build());

    alarmarr->setLength(5);

    testOk1(alarmarr->getLength()==5);

    alarmarr->compress();

    testOk1(alarmarr->getLength()==0);

    alarmarr->setLength(4);

    testOk1(alarmarr->getLength()==4);

    PVStructureArray::svector contents(10);

    contents[2] = standardField->alarm()->build();
    contents[4] = standardField->alarm()->build();
    contents[5] = standardField->alarm()->build();
    contents[8] = standardField->alarm()->build();

    PVStructureArray::const_svector scont(freeze(contents));

    alarmarr->replace(scont);

    testOk1(!scont.unique());
    testOk1(alarmarr->getLength()==10);

    alarmarr->compress();

    testOk1(scont.unique()); // a realloc happened
    testOk1(alarmarr->getLength()==4);

    PVStructureArray::svector compressed(alarmarr->reuse());

    testOk1(scont[2]==compressed[0]);
    testOk1(scont[4]==compressed[1]);
    testOk1(scont[5]==compressed[2]);
    testOk1(scont[8]==compressed[3]);
}

static void testRemove()
{
    testDiag("Test structure array remove");

    PVStructureArray::svector contents(10);

    for(size_t i=0; i<contents.size(); i++)
        contents[i] = standardField->alarm()->build();

    StructureArrayConstPtr alarmtype(
        fieldCreate->createStructureArray(standardField->alarm()));
    PVStructureArrayPtr alarmarr(alarmtype->build());

    PVStructureArray::const_svector scont(freeze(contents));

    alarmarr->replace(scont);

    alarmarr->remove(0, 10); // all

    testOk1(alarmarr->getLength()==0);

    alarmarr->replace(scont);

    alarmarr->remove(1, 1);

    PVStructureArray::const_svector check(alarmarr->view());

    testOk1(scont[0]==check[0]);
    testOk1(scont[2]==check[1]);
    testOk1(scont[3]==check[2]);
}

static void testFromRaw()
{
    testDiag("Test structure array external allocation for shared_vector");

    PVStructurePtr* raw = new PVStructurePtr[4];
    raw[0] = standardField->alarm()->build();
    raw[1] = standardField->alarm()->build();
    raw[2] = standardField->alarm()->build();
    raw[3] = standardField->alarm()->build();

    PVStructureArray::svector cont(raw, 1, 2);
}

MAIN(testPVStructureArray)
{
    testPlan(23);
    testDiag("Testing structure array handling");
    fieldCreate = getFieldCreate();
    pvDataCreate = getPVDataCreate();
    standardField = getStandardField();
    standardPVField = getStandardPVField();
    testBasic();
    testCompress();
    testRemove();
    testFromRaw();
    return testDone();
}
