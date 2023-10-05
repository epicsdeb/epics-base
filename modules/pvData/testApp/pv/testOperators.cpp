/* testOperators.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Matej Sekoranja Date: 2013.02 */

#include <iostream>

#include <pv/pvData.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/status.h>

#include <epicsUnitTest.h>
#include <testMain.h>

using namespace epics::pvData;
using std::string;

static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static StandardFieldPtr standardField = getStandardField();
static StandardPVFieldPtr standardPVField = getStandardPVField();

MAIN(testOperators)
{
    testPlan(2);
    PVStructurePtr pvStructure = standardPVField->scalar(pvDouble,
        "alarm,timeStamp,display,control,valueAlarm");

    const double testDV = 12.8;

    PVDoublePtr pvValue = pvStructure->getSubField<PVDouble>("value");
    *pvValue <<= testDV;

    double dv;
    *pvValue >>= dv;
    testOk1(testDV == dv);


    const string testSV = "test message";

    PVStringPtr pvMessage = pvStructure->getSubField<PVString>("alarm.message");
    *pvMessage <<= testSV;

    string sv;
    *pvMessage >>= sv;
    testOk1(testSV == sv);

    //
    // to stream tests
    //

    std::cout << *pvValue << std::endl;
    std::cout << *pvMessage << std::endl;
    std::cout << *pvStructure << std::endl;

    std::cout << *pvStructure->getStructure() << std::endl;

    std::cout << Status::Ok << std::endl;
    std::cout << Status::STATUSTYPE_OK << std::endl;

    StringArray choices;
    choices.reserve(3);
    choices.push_back("one");
    choices.push_back("two");
    choices.push_back("three");
    pvStructure = standardPVField->enumerated(choices, "alarm,timeStamp,valueAlarm");
    std::cout << *pvStructure << std::endl;



    pvStructure = standardPVField->scalarArray(pvDouble,"alarm,timeStamp");
    std::cout << *pvStructure << std::endl;

    PVDoubleArray::svector values(3);
    values[0] = 1.1; values[1] = 2.2; values[2] = 3.3;
    PVDoubleArrayPtr darray = pvStructure->getSubField<PVDoubleArray>("value");
    darray->replace(freeze(values));
    std::cout << *darray << std::endl;
    std::cout << format::array_at(1) << *darray << std::endl;


    StructureConstPtr structure = standardField->scalar(pvDouble, "alarm,timeStamp");
    pvStructure = standardPVField->structureArray(structure,"alarm,timeStamp");
    size_t num = 2;
    PVStructureArray::svector pvStructures(num);
    for(size_t i=0; i<num; i++) {
        pvStructures[i]=
            pvDataCreate->createPVStructure(structure);
    }
    PVStructureArrayPtr pvStructureArray = pvStructure->getSubField<PVStructureArray>("value");
    pvStructureArray->replace(freeze(pvStructures));
    std::cout << *pvStructure << std::endl;

   return testDone();
}
