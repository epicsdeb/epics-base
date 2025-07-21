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

    NTScalarArrayBuilderPtr builder = NTScalarArray::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            value(pvDouble)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addDisplay()->
            addControl()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTScalarArray::is_a(structure));
    testOk1(structure->getID() == NTScalarArray::URI);
    testOk1(structure->getNumberFields() == 8);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("display").get() != 0);
    testOk1(structure->getField("control").get() != 0);

    testOk(dynamic_pointer_cast<const ScalarArray>(structure->getField("value")).get() != 0 &&
            dynamic_pointer_cast<const ScalarArray>(structure->getField("value"))->getElementType() == pvDouble, "value type");

    std::cout << *structure << std::endl;

    // no value set
    try
    {
        structure = builder->
                addDescriptor()->
                addAlarm()->
                addTimeStamp()->
                addDisplay()->
                addControl()->
                createStructure();
        testFail("no value type set");
    } catch (std::runtime_error &) {
        testPass("no value type set");
    }
}

void test_ntscalarArray()
{
    testDiag("test_ntscalarArray");

    NTScalarArrayBuilderPtr builder = NTScalarArray::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTScalarArrayPtr ntScalarArray = builder->
            value(pvInt)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addDisplay()->
            addControl()->
            create();
    testOk1(ntScalarArray.get() != 0);

    testOk1(NTScalarArray::is_a(ntScalarArray->getPVStructure()));
    testOk1(NTScalarArray::isCompatible(ntScalarArray->getPVStructure()));

    testOk1(ntScalarArray->getPVStructure().get() != 0);
    testOk1(ntScalarArray->getValue().get() != 0);
    testOk1(ntScalarArray->getDescriptor().get() != 0);
    testOk1(ntScalarArray->getAlarm().get() != 0);
    testOk1(ntScalarArray->getTimeStamp().get() != 0);
    testOk1(ntScalarArray->getDisplay().get() != 0);
    testOk1(ntScalarArray->getControl().get() != 0);

    //
    // example how to set values
    //
    PVIntArray::svector newValues;
    newValues.push_back(1);
    newValues.push_back(2);
    newValues.push_back(8);

    PVIntArrayPtr pvValueField = ntScalarArray->getValue<PVIntArray>();
    pvValueField->replace(freeze(newValues));

    //
    // example how to get values
    //
    PVIntArray::const_svector values(pvValueField->view());

    testOk1(values.size() == 3);
    testOk1(values[0] == 1);
    testOk1(values[1] == 2);
    testOk1(values[2] == 8);

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntScalarArray->attachTimeStamp(pvTimeStamp))
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
    if (ntScalarArray->attachAlarm(pvAlarm))
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
    // display ops
    //
    PVDisplay pvDisplay;
    if (ntScalarArray->attachDisplay(pvDisplay))
    {
        testPass("display attach");

        // example how to set an display
        Display display;
        display.setLow(-15);
        display.setHigh(15);
        display.setDescription("This is a test scalar array");
        display.setUnits("A");
        pvDisplay.set(display);
    }
    else
        testFail("display attach fail");

    //
    // control ops
    //
    PVControl pvControl;
    if (ntScalarArray->attachControl(pvControl))
    {
        testPass("control attach");

        // example how to set an control
        Control control;
        control.setLow(-10);
        control.setHigh(10);
        control.setMinStep(1);
        pvControl.set(control);
    }
    else
        testFail("control attach fail");

    //
    // set descriptor
    //
    ntScalarArray->getDescriptor()->put("This is a test NTScalarArray");

    // dump ntScalarArray
    std::cout << *ntScalarArray->getPVStructure() << std::endl;

}


void test_wrap()
{
    testDiag("test_wrap");

    NTScalarArrayPtr nullPtr = NTScalarArray::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTScalarArray::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTScalarArrayBuilderPtr builder = NTScalarArray::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            value(pvDouble)->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTScalarArray::isCompatible(pvStructure)==true);
    NTScalarArrayPtr ptr = NTScalarArray::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTScalarArray::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTScalarArray) {
    testPlan(40);
    test_builder();
    test_ntscalarArray();
    test_wrap();
    return testDone();
}


