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
static StandardFieldPtr standardField = getStandardField();
void test_builder()
{
    testDiag("test_builder");

    NTScalarBuilderPtr builder = NTScalar::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            value(pvDouble)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addDisplay()->
            addControl()->
            add("valueAlarm",standardField->doubleAlarm()) ->
            add("extra",fieldCreate->createScalarArray(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTScalar::is_a(structure));
    testOk1(structure->getID() == NTScalar::URI);
    testOk1(structure->getNumberFields() == 8);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("display").get() != 0);
    testOk1(structure->getField("control").get() != 0);

    testOk(dynamic_pointer_cast<const Scalar>(structure->getField("value")).get() != 0 &&
            dynamic_pointer_cast<const Scalar>(structure->getField("value"))->getScalarType() == pvDouble, "value type");

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

void test_ntscalar()
{
    testDiag("test_ntscalar");

    NTScalarBuilderPtr builder = NTScalar::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTScalarPtr ntScalar = builder->
            value(pvInt)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            addDisplay()->
            addControl()->
            add("valueAlarm",standardField->intAlarm()) ->
            create();
    testOk1(ntScalar.get() != 0);

    testOk1(NTScalar::is_a(ntScalar->getPVStructure()));
    testOk1(NTScalar::isCompatible(ntScalar->getPVStructure()));

    testOk1(ntScalar->getPVStructure().get() != 0);
    testOk1(ntScalar->getValue().get() != 0);
    testOk1(ntScalar->getDescriptor().get() != 0);
    testOk1(ntScalar->getAlarm().get() != 0);
    testOk1(ntScalar->getTimeStamp().get() != 0);
    testOk1(ntScalar->getDisplay().get() != 0);
    testOk1(ntScalar->getControl().get() != 0);

    //
    // example how to set a value
    //
    ntScalar->getValue<PVInt>()->put(12);

    //
    // example how to get a value
    //
    int32 value = ntScalar->getValue<PVInt>()->get();
    testOk1(value == 12);

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntScalar->attachTimeStamp(pvTimeStamp))
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
    if (ntScalar->attachAlarm(pvAlarm))
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
    if (ntScalar->attachDisplay(pvDisplay))
    {
        testPass("display attach");

        // example how to set an display
        Display display;
        display.setLow(-15);
        display.setHigh(15);
        display.setDescription("This is a test scalar");
        display.setUnits("A");
        pvDisplay.set(display);
    }
    else
        testFail("display attach fail");

    //
    // control ops
    //
    PVControl pvControl;
    if (ntScalar->attachControl(pvControl))
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
    ntScalar->getDescriptor()->put("This is a test NTScalar");

    // dump ntScalar
    std::cout << *ntScalar->getPVStructure() << std::endl;

}

void test_wrap()
{
    testDiag("test_wrap");

    NTScalarPtr nullPtr = NTScalar::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTScalar::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTScalarBuilderPtr builder = NTScalar::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            value(pvDouble)->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTScalar::isCompatible(pvStructure)==true);
    NTScalarPtr ptr = NTScalar::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTScalar::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTScalar) {
    testPlan(37);
    test_builder();
    test_ntscalar();
    test_wrap();
    return testDone();
}


