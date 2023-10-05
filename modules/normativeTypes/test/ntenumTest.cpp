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

    NTEnumBuilderPtr builder = NTEnum::createBuilder();
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

    testOk1(NTEnum::is_a(structure));
    testOk1(structure->getID() == NTEnum::URI);
    testOk1(structure->getNumberFields() == 6);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);

    FieldConstPtr valueField = structure->getField("value");
    testOk(valueField.get() != 0 &&
           ntField->isEnumerated(valueField), "value is enum");

    std::cout << *structure << std::endl;

}

void test_ntenum()
{
    testDiag("test_ntenum");

    NTEnumBuilderPtr builder = NTEnum::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTEnumPtr ntEnum = builder->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("valueAlarm",standardField->intAlarm()) ->
            create();
    testOk1(ntEnum.get() != 0);

    testOk1(NTEnum::is_a(ntEnum->getPVStructure()));
    testOk1(NTEnum::isCompatible(ntEnum->getPVStructure()));

    testOk1(ntEnum->getPVStructure().get() != 0);
    testOk1(ntEnum->getValue().get() != 0);
    testOk1(ntEnum->getDescriptor().get() != 0);
    testOk1(ntEnum->getAlarm().get() != 0);
    testOk1(ntEnum->getTimeStamp().get() != 0);

    //
    // example how to set a value
    //
    PVStructurePtr pvValue = ntEnum->getValue();
    //PVStringArray pvChoices = pvValue->getSubField<PVStringArray>("choices");
    PVStringArray::svector choices(2);
    choices[0] = "Off";
    choices[1] = "On";
    pvValue->getSubField<PVStringArray>("choices")->replace(freeze(choices));
    pvValue->getSubField<PVInt>("index")->put(1);
    
    //
    // example how to get a value
    //
    int32 value = ntEnum->getValue()->getSubField<PVInt>("index")->get();
    testOk1(value == 1);

    PVStringArrayPtr pvChoices = ntEnum->getValue()->getSubField<PVStringArray>("choices");
    std::string choice0 = pvChoices->view()[0];
    std::string choice1 = pvChoices->view()[1];
    testOk1(choice0 == "Off");
    testOk1(choice1 == "On");

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntEnum->attachTimeStamp(pvTimeStamp))
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
    if (ntEnum->attachAlarm(pvAlarm))
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
    ntEnum->getDescriptor()->put("This is a test NTEnum");

    // dump ntEnum
    std::cout << *ntEnum->getPVStructure() << std::endl;

}

void test_wrap()
{
    testDiag("test_wrap");

    NTEnumPtr nullPtr = NTEnum::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTEnum::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTEnumBuilderPtr builder = NTEnum::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTEnum::isCompatible(pvStructure)==true);
    NTEnumPtr ptr = NTEnum::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTEnum::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTEnum) {
    testPlan(32);
    test_builder();
    test_ntenum();
    test_wrap();
    return testDone();
}


