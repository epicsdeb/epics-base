/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/nt.h>

using namespace epics::nt;
using namespace epics::pvData;
using std::tr1::dynamic_pointer_cast;

static FieldCreatePtr fieldCreate = getFieldCreate();

void test_builder()
{
    testDiag("test_builder");

    NTNameValueBuilderPtr builder = NTNameValue::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            value(pvDouble)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTNameValue::is_a(structure));
    testOk1(structure->getID() == NTNameValue::URI);
    testOk1(structure->getNumberFields() == 7);
    testOk1(structure->getField("name").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);

    testOk(dynamic_pointer_cast<const ScalarArray>(structure->getField("value")).get() != 0 &&
            dynamic_pointer_cast<const ScalarArray>(structure->getField("value"))->getElementType() == pvDouble, "value array element type");

    std::cout << *structure << std::endl;

    // no value set
    try
    {
        structure = builder->
                addDescriptor()->
                addAlarm()->
                addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
                createStructure();
        testFail("no value type set");
    } catch (std::runtime_error &) {
        testPass("no value type set");
    }
}

void test_ntnameValue()
{
    testDiag("test_ntnameValue");

    NTNameValueBuilderPtr builder = NTNameValue::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTNameValuePtr ntNameValue = builder->
            value(pvInt)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            create();
    testOk1(ntNameValue.get() != 0);

    testOk1(NTNameValue::is_a(ntNameValue->getPVStructure()));
    testOk1(NTNameValue::isCompatible(ntNameValue->getPVStructure()));

    testOk1(ntNameValue->getPVStructure().get() != 0);
    testOk1(ntNameValue->getDescriptor().get() != 0);
    testOk1(ntNameValue->getAlarm().get() != 0);
    testOk1(ntNameValue->getTimeStamp().get() != 0);
    testOk1(ntNameValue->getName().get() != 0);
    testOk1(ntNameValue->getValue().get() != 0);

    //
    // example how to set name
    //
    PVStringArray::svector newName;
    newName.push_back("name1");
    newName.push_back("name2");
    newName.push_back("name3");

    PVStringArrayPtr pvNameField = ntNameValue->getName();
    pvNameField->replace(freeze(newName));

    //
    // example how to get name
    //
    PVStringArray::const_svector name(pvNameField->view());

    testOk1(name.size() == 3);
    testOk1(name[0] == "name1");
    testOk1(name[1] == "name2");
    testOk1(name[2] == "name3");

    //
    // example how to set value
    //
    PVIntArray::svector newValue;
    newValue.push_back(1);
    newValue.push_back(2);
    newValue.push_back(8);

    PVIntArrayPtr pvValueField = ntNameValue->getValue<PVIntArray>();
    pvValueField->replace(freeze(newValue));

    //
    // example how to get column value
    //
    PVIntArray::const_svector value(pvValueField->view());

    testOk1(value.size() == 3);
    testOk1(value[0] == 1);
    testOk1(value[1] == 2);
    testOk1(value[2] == 8);

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntNameValue->attachTimeStamp(pvTimeStamp))
    {
        testPass("timeStamp attach");

        // example how to set current time
        TimeStamp ts;
        ts.getCurrent();
        pvTimeStamp.set(ts);

        // example how to get EPICS time
        TimeStamp ts2;
        pvTimeStamp.get(ts2);
        testOk1(ts2.getEpicsSecondsPastEpoch() != 0);
    }
    else
        testFail("timeStamp attach fail");

    //
    // alarm ops
    //
    PVAlarm pvAlarm;
    if (ntNameValue->attachAlarm(pvAlarm))
    {
        testPass("alarm attach");

        // example how to set an alarm
        Alarm alarm;
        alarm.setStatus(deviceStatus);
        alarm.setSeverity(minorAlarm);
        alarm.setMessage("simulation alarm");
        pvAlarm.set(alarm);
    }
    else
        testFail("alarm attach fail");

    //
    // set descriptor
    //
    ntNameValue->getDescriptor()->put("This is a test NTNameValue");

    // dump NTNameValue
    std::cout << *ntNameValue->getPVStructure() << std::endl;

}


void test_wrap()
{
    testDiag("test_wrap");

    NTNameValuePtr nullPtr = NTNameValue::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTNameValue::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTNameValueBuilderPtr builder = NTNameValue::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            value(pvString)->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTNameValue::isCompatible(pvStructure)==true);
    NTNameValuePtr ptr = NTNameValue::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTNameValue::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

void test_extra()
{
    testDiag("test_extra");

    NTNameValueBuilderPtr builder = NTNameValue::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            value(pvDouble)->
            addTimeStamp()->
            add("function", getFieldCreate()->createScalar(pvString))->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTNameValue::is_a(structure));
    testOk1(structure->getID() == NTNameValue::URI);
    testOk1(structure->getNumberFields() == 4);
    testOk1(structure->getField("name").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("function").get() != 0);

    testOk(dynamic_pointer_cast<const Scalar>(structure->getField("function")).get() != 0 &&
            dynamic_pointer_cast<const Scalar>(structure->getField("function"))->getScalarType() == pvString, "function type");

    std::cout << *structure << std::endl;
}


MAIN(testNTNameValue) {
    testPlan(50);
    test_builder();
    test_ntnameValue();
    test_wrap();
    test_extra();
    return testDone();
}


