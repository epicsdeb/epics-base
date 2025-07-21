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

    NTUnionBuilderPtr builder = NTUnion::createBuilder();
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

    testOk1(NTUnion::is_a(structure));
    testOk1(structure->getID() == NTUnion::URI);
    testOk1(structure->getNumberFields() == 6);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);

    UnionConstPtr valueField = structure->getField<Union>("value");
    testOk(valueField.get() != 0, "value is enum");

    std::cout << *structure << std::endl;
}

void test_ntunion()
{
    testDiag("test_ntunion");

    NTUnionBuilderPtr builder = NTUnion::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTUnionPtr ntUnion = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            create();
    testOk1(ntUnion.get() != 0);

    testOk1(NTUnion::is_a(ntUnion->getPVStructure()));
    testOk1(NTUnion::isCompatible(ntUnion->getPVStructure()));

    testOk1(ntUnion->getPVStructure().get() != 0);
    testOk1(ntUnion->getValue().get() != 0);
    testOk1(ntUnion->getDescriptor().get() != 0);
    testOk1(ntUnion->getAlarm().get() != 0);
    testOk1(ntUnion->getTimeStamp().get() != 0);

    // TODO
    // 1. Variant union example.
    // 2. set the union value.

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntUnion->attachTimeStamp(pvTimeStamp))
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
    if (ntUnion->attachAlarm(pvAlarm))
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
    ntUnion->getDescriptor()->put("This is a test NTUnion");

    // dump ntUnion
    std::cout << *ntUnion->getPVStructure() << std::endl;

}

void test_wrap()
{
    testDiag("test_wrap");

    NTUnionPtr nullPtr = NTUnion::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTUnion::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTUnionBuilderPtr builder = NTUnion::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTUnion::isCompatible(pvStructure)==true);
    NTUnionPtr ptr = NTUnion::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTUnion::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}


void test_variant_union()
{
    StructureConstPtr structure = NTUnion::createBuilder()->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            createStructure();
    testOk1(structure->getField<Union>("value")->isVariant());
}

void test_regular_union()
{
    UnionConstPtr u = getFieldCreate()->createFieldBuilder()->
        add("x", pvDouble)->
        add("i", pvInt)->
        createUnion();

    StructureConstPtr structure = NTUnion::createBuilder()->
            value(u)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            createStructure();
    testOk1(!structure->getField<Union>("value")->isVariant());
    testOk1(structure->getField<Union>("value") == u);
}



MAIN(testNTUnion) {
    testPlan(32);
    test_builder();
    test_ntunion();
    test_wrap();
    test_variant_union();
    test_regular_union();
    return testDone();
}


