/* testStandardField.cpp */
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

#include <pv/pvUnitTest.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/standardField.h>

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::string;

static FieldCreatePtr fieldCreate = getFieldCreate();
static StandardFieldPtr standardField = getStandardField();

static void print(const string& name, FieldConstPtr const & f)
{
    testShow()<<name<<'\n'<<format::indent_level(1)<<f;
}

MAIN(testStandardField)
{
    testPlan(1);
    StructureConstPtr doubleValue = standardField->scalar(pvDouble,
        "alarm,timeStamp,display,control,valueAlarm");
    print("doubleValue", doubleValue);
    StructureConstPtr stringArrayValue = standardField->scalarArray(pvString,
        "alarm,timeStamp");
    print("stringArrayValue", stringArrayValue);
    StructureConstPtr enumeratedValue = standardField->enumerated(
        "alarm,timeStamp,valueAlarm");
    print("enumeratedValue", enumeratedValue);
    FieldConstPtrArray fields;
    fields.reserve(2);
    fields.push_back(fieldCreate->createScalar(pvDouble));
    fields.push_back(fieldCreate->createScalarArray(pvDouble));
    StringArray fieldNames;
    fieldNames.reserve(2);
    fieldNames.push_back("scalarValue");
    fieldNames.push_back("arrayValue");
    StructureConstPtr structure = fieldCreate->createStructure(fieldNames, fields);
    StructureConstPtr structureArrayValue = standardField->structureArray(structure, "alarm,timeStamp");
    print("structureArrayValue", structureArrayValue);

    StructureConstPtr punion = standardField->regUnion(
        fieldCreate->createFieldBuilder()->
        add("doubleValue", pvDouble)->
        add("intValue", pvInt)->
        add("timeStamp",standardField->timeStamp())->
        createUnion(),
        "alarm,timeStamp");
    print("unionValue", punion);
    testPass("testStandardField");
    return testDone();
}
