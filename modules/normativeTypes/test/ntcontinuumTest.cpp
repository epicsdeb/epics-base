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

    NTContinuumBuilderPtr builder = NTContinuum::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTContinuum::is_a(structure));
    testOk1(structure->getID() == NTContinuum::URI);
    testOk1(structure->getNumberFields() == 8);
    testOk1(structure->getField("base").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("units").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);

    testOk(dynamic_pointer_cast<const ScalarArray>(structure->getField("base")).get() != 0 &&
            dynamic_pointer_cast<const ScalarArray>(structure->getField("base"))->getElementType() == pvDouble, "base array element type");

    testOk(dynamic_pointer_cast<const ScalarArray>(structure->getField("value")).get() != 0 &&
            dynamic_pointer_cast<const ScalarArray>(structure->getField("value"))->getElementType() == pvDouble, "value array element type");

    testOk(dynamic_pointer_cast<const ScalarArray>(structure->getField("units")).get() != 0 &&
            dynamic_pointer_cast<const ScalarArray>(structure->getField("units"))->getElementType() == pvString, "units array element type");


    std::cout << *structure << std::endl;
}

void test_ntcontinuum()
{
    testDiag("test_ntcontinuum");

    NTContinuumBuilderPtr builder = NTContinuum::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTContinuumPtr ntContinuum = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            create();
    testOk1(ntContinuum.get() != 0);

    testOk1(NTContinuum::is_a(ntContinuum->getPVStructure()));
    testOk1(NTContinuum::isCompatible(ntContinuum->getPVStructure()));

    testOk1(ntContinuum->getPVStructure().get() != 0);
    testOk1(ntContinuum->getDescriptor().get() != 0);
    testOk1(ntContinuum->getAlarm().get() != 0);
    testOk1(ntContinuum->getTimeStamp().get() != 0);
    testOk1(ntContinuum->getBase().get() != 0);
    testOk1(ntContinuum->getValue().get() != 0);
    //
    // example how to set base
    //
    PVDoubleArray::svector newBase;
    newBase.push_back(1.0);
    newBase.push_back(2.0);

    PVDoubleArrayPtr pvBaseField = ntContinuum->getBase();
    pvBaseField->replace(freeze(newBase));

    //
    // example how to get bases
    //
    PVDoubleArray::const_svector base(pvBaseField->view());

    testOk1(base.size() == 2);
    testOk1(base[0] == 1.0);
    testOk1(base[1] == 2.0);

    //
    // example how to set values
    //
    PVDoubleArray::svector newValue;
    newValue.push_back(1.0);
    newValue.push_back(2.0);
    newValue.push_back(10.0);
    newValue.push_back(20.0);
    newValue.push_back(100.0);
    newValue.push_back(200.0);

    PVDoubleArrayPtr pvValueField = ntContinuum->getValue();
    pvValueField->replace(freeze(newValue));

    //
    // example how to get values
    //
    PVDoubleArray::const_svector value(pvValueField->view());

    testOk1(value.size() == 6);
    testOk1(value[0] == 1.0);
    testOk1(value[1] == 2.0);
    testOk1(value[2] == 10.0);
    testOk1(value[3] == 20.0);
    testOk1(value[4] == 100.0);
    testOk1(value[5] == 200.0);

    //
    // example how to set units
    //
    PVStringArray::svector newUnits;
    newUnits.push_back("s");
    newUnits.push_back("ms");
    newUnits.push_back("us");
    newUnits.push_back("s");

    PVStringArrayPtr pvUnitsField = ntContinuum->getUnits();
    pvUnitsField->replace(freeze(newUnits));

    //
    // example how to get units
    //
    PVStringArray::const_svector units(pvUnitsField->view());

    testOk1(units.size() == 4);
    testOk1(units[0] == "s");
    testOk1(units[1] == "ms");
    testOk1(units[2] == "us");
    testOk1(units[3] == "s");

    //
    // test isValid
    //
    testOk1(ntContinuum->isValid());
    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntContinuum->attachTimeStamp(pvTimeStamp))
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
    if (ntContinuum->attachAlarm(pvAlarm))
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
    ntContinuum->getDescriptor()->put("This is a test NTContinuum");

    // dump NTContinuum
    std::cout << *ntContinuum->getPVStructure() << std::endl;

}


void test_wrap()
{
    testDiag("test_wrap");

    NTContinuumPtr nullPtr = NTContinuum::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTContinuum::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTContinuumBuilderPtr builder = NTContinuum::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTContinuum::isCompatible(pvStructure)==true);
    NTContinuumPtr ptr = NTContinuum::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTContinuum::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

void test_extra()
{
    testDiag("test_extra");

    NTContinuumBuilderPtr builder = NTContinuum::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            addTimeStamp()->
            add("function", getFieldCreate()->createScalar(pvString))->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTContinuum::is_a(structure));
    testOk1(structure->getID() == NTContinuum::URI);
    testOk1(structure->getNumberFields() == 5);
    testOk1(structure->getField("base").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("units").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("function").get() != 0);

    testOk(dynamic_pointer_cast<const Scalar>(structure->getField("function")).get() != 0 &&
            dynamic_pointer_cast<const Scalar>(structure->getField("function"))->getScalarType() == pvString, "function type");

    std::cout << *structure << std::endl;
}


MAIN(testNTContinuum) {
    testPlan(61);
    test_builder();
    test_ntcontinuum();
    test_wrap();
    test_extra();
    return testDone();
}


