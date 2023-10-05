/* testStandardPVField.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2012.08 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <sstream>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::string;

static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static StandardFieldPtr standardField = getStandardField();
static StandardPVFieldPtr standardPVField = getStandardPVField();

static void print(const string& name, PVFieldPtr const & f)
{
    std::ostringstream strm;
    strm << std::endl << name << std::endl << f << std::endl;
    testDiag("%s", strm.str().c_str());
}

MAIN(testStandardPVField)
{
    testPlan(1);
    PVStructurePtr pvStructure = standardPVField->scalar(pvDouble,
        "alarm,timeStamp,display,control,valueAlarm");
    PVDoublePtr pvValue = pvStructure->getSubField<PVDouble>("value");
    pvValue->put(10.0);
    PVIntPtr pvSeverity = pvStructure->getSubField<PVInt>("alarm.severity");
    pvSeverity->put(2);
    PVStringPtr pvMessage = pvStructure->getSubField<PVString>("alarm.message");
    pvMessage->put("test message");
    print("scalarTest", pvStructure);
    pvStructure = standardPVField->scalar(pvBoolean,"alarm,timeStamp,valueAlarm");
    print("booleanTest", pvStructure);
    StringArray choices;
    choices.reserve(3);
    choices.push_back("one");
    choices.push_back("two");
    choices.push_back("three");
    pvStructure = standardPVField->enumerated(choices, "alarm,timeStamp,valueAlarm");
    print("enumeratedTest", pvStructure);
    pvStructure = standardPVField->scalarArray(pvBoolean,"alarm,timeStamp");
    print("scalarArrayTest", pvStructure);
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
    print("structureArrayTest", pvStructure);
    testPass("testStandardPVField");
    return testDone();
}
