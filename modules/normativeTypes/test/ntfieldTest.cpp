/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/*
 * ntfieldTest.cpp
 *
 *  Created on: 2011.11
 *      Author: Marty Kraimer
 */

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/nt.h>

using namespace epics::nt;
using namespace epics::pvData;
using std::string;
using std::cout;
using std::endl;

static FieldCreatePtr fieldCreate = getFieldCreate();
static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static StandardFieldPtr standardField = getStandardField();
static StandardPVFieldPtr standardPVField = getStandardPVField();
static NTFieldPtr ntField = NTField::get();
static PVNTFieldPtr pvntField = PVNTField::get();

void testNTField()
{
    testDiag("testNTField");

    StructureConstPtr structureConstPtr = ntField->createEnumerated();
    cout << *structureConstPtr << endl;
    testOk1(ntField->isEnumerated(structureConstPtr));

    structureConstPtr = ntField->createTimeStamp();
    cout << *structureConstPtr << endl;
    testOk1(ntField->isTimeStamp(structureConstPtr));

    structureConstPtr = ntField->createAlarm();
    cout << *structureConstPtr << endl;
    testOk1(ntField->isAlarm(structureConstPtr));

    structureConstPtr = ntField->createDisplay();
    cout << *structureConstPtr << endl;
    testOk1(ntField->isDisplay(structureConstPtr));

    structureConstPtr = standardField->doubleAlarm();
    cout << *structureConstPtr << endl;
    testOk1(ntField->isAlarmLimit(structureConstPtr));

    structureConstPtr = ntField->createControl();
    cout << *structureConstPtr << endl;
    testOk1(ntField->isControl(structureConstPtr));

    StructureArrayConstPtr structureArrayConstPtr
        = ntField->createEnumeratedArray();
    cout << *structureConstPtr << endl;

    structureArrayConstPtr = ntField->createTimeStampArray();
    cout << *structureConstPtr << endl;

    structureArrayConstPtr = ntField->createAlarmArray();
    cout << *structureConstPtr << endl;
}

void testPVNTField()
{
    testDiag("testPVNTField");

    StringArray choices;
    choices.resize(3);
    choices[0] = "one";
    choices[1] = "two";
    choices[2] = "three";
    PVStructurePtr pvStructure = PVStructurePtr(
        pvntField->createEnumerated(choices));
    cout << *pvStructure << endl;
    testOk1(ntField->isEnumerated(pvStructure->getStructure()));

    pvStructure = PVStructurePtr(pvntField->createTimeStamp());
    cout << *pvStructure << endl;
    testOk1(ntField->isTimeStamp(pvStructure->getStructure()));

    pvStructure = PVStructurePtr(pvntField->createAlarm());
    cout << *pvStructure << endl;
    testOk1(ntField->isAlarm(pvStructure->getStructure()));

    pvStructure = PVStructurePtr(pvntField->createDisplay());
    cout << *pvStructure << endl;
    testOk1(ntField->isDisplay(pvStructure->getStructure()));

    pvStructure = PVStructurePtr(pvDataCreate->createPVStructure(standardField->doubleAlarm()));
    cout << *pvStructure << endl;
    testOk1(ntField->isAlarmLimit(pvStructure->getStructure()));

    PVStructureArrayPtr pvStructureArray = PVStructureArrayPtr(
        pvntField->createEnumeratedArray());
    cout << *pvStructure << endl;
    cout << *pvStructureArray->getStructureArray()->getStructure();

    pvStructureArray = PVStructureArrayPtr(
        pvntField->createTimeStampArray());
    cout << *pvStructure << endl;
    cout << *pvStructureArray->getStructureArray()->getStructure();

    pvStructureArray = PVStructureArrayPtr(
        pvntField->createAlarmArray());
    cout << *pvStructure << endl;
    cout << *pvStructureArray->getStructureArray()->getStructure();
}

MAIN(testNTField) {
    testPlan(11);
    testNTField();
    testPVNTField();
    return testDone();
}
