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
static NTFieldPtr ntField = NTField::get();

void test_builder()
{
    testDiag("test_builder");

    NTAggregateBuilderPtr builder = NTAggregate::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("valueAlarm",standardField->doubleAlarm()) ->
            add("extra",fieldCreate->createScalarArray(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTAggregate::is_a(structure));
    testOk1(structure->getID() == NTAggregate::URI);
    testOk1(structure->getNumberFields() == 7);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);

    ScalarConstPtr valueField = structure->getField<Scalar>("value");
    testOk(valueField.get() != 0, "value is scalar");

    std::cout << *structure << std::endl;

}

void test_ntaggregate()
{
    testDiag("test_ntaggregate");

    NTAggregateBuilderPtr builder = NTAggregate::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTAggregatePtr ntAggregate = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("valueAlarm",standardField->intAlarm()) ->
            create();
    testOk1(ntAggregate.get() != 0);

    testOk1(NTAggregate::is_a(ntAggregate->getPVStructure()));
    testOk1(NTAggregate::isCompatible(ntAggregate->getPVStructure()));

    testOk1(ntAggregate->getPVStructure().get() != 0);
    testOk1(ntAggregate->getValue().get() != 0);
    testOk1(ntAggregate->getDescriptor().get() != 0);
    testOk1(ntAggregate->getAlarm().get() != 0);
    testOk1(ntAggregate->getTimeStamp().get() != 0);

    //
    // example how to set a value
    //
    ntAggregate->getValue()->put(1.0);
    
    //
    // example how to get a value
    //
    double value = ntAggregate->getValue()->get();
    testOk1(value == 1.0);

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntAggregate->attachTimeStamp(pvTimeStamp))
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
    if (ntAggregate->attachAlarm(pvAlarm))
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
    ntAggregate->getDescriptor()->put("This is a test NTAggregate");

    // dump ntAggregate
    std::cout << *ntAggregate->getPVStructure() << std::endl;

}

void test_wrap()
{
    testDiag("test_wrap");

    NTAggregatePtr nullPtr = NTAggregate::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTAggregate::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTAggregateBuilderPtr builder = NTAggregate::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTAggregate::isCompatible(pvStructure)==true);
    NTAggregatePtr ptr = NTAggregate::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTAggregate::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTAggregate) {
    testPlan(30);
    test_builder();
    test_ntaggregate();
    test_wrap();
    return testDone();
}


