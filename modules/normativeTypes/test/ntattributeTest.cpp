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

    NTAttributeBuilderPtr builder = NTAttribute::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            addTags()->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra",fieldCreate->createScalar(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTAttribute::is_a(structure));
    testOk1(structure->getID() == NTAttribute::URI);
    testOk1(structure->getNumberFields() == 7);
    testOk1(structure->getField("name").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("tags").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);

    ScalarConstPtr nameField = structure->getField<Scalar>("name");
    testOk(nameField.get() != 0 && nameField->getScalarType() == pvString,
        "name is string");

    UnionConstPtr valueField = structure->getField<Union>("value");
    testOk(valueField.get() != 0, "value is enum");

    ScalarArrayConstPtr tagsField = structure->getField<ScalarArray>("tags");
    testOk(tagsField.get() != 0 && tagsField->getElementType() == pvString,
        "tags is string[]");

    std::cout << *structure << std::endl;

}

void test_ntattribute()
{
    testDiag("test_ntattribute");

    NTAttributeBuilderPtr builder = NTAttribute::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTAttributePtr ntAttribute = builder->
            addTags()->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            create();
    testOk1(ntAttribute.get() != 0);

    testOk1(NTAttribute::is_a(ntAttribute->getPVStructure()));
    testOk1(NTAttribute::isCompatible(ntAttribute->getPVStructure()));

    testOk1(ntAttribute->getPVStructure().get() != 0);
    testOk1(ntAttribute->getName().get() != 0);
    testOk1(ntAttribute->getValue().get() != 0);
    testOk1(ntAttribute->getTags().get() != 0);
    testOk1(ntAttribute->getDescriptor().get() != 0);
    testOk1(ntAttribute->getAlarm().get() != 0);
    testOk1(ntAttribute->getTimeStamp().get() != 0);

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntAttribute->attachTimeStamp(pvTimeStamp))
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
    if (ntAttribute->attachAlarm(pvAlarm))
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
    ntAttribute->getDescriptor()->put("This is a test NTAttribute");

    // dump ntAttribute
    std::cout << *ntAttribute->getPVStructure() << std::endl;

}

void test_wrap()
{
    testDiag("test_wrap");

    NTAttributePtr nullPtr = NTAttribute::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTAttribute::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTAttributeBuilderPtr builder = NTAttribute::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTAttribute::isCompatible(pvStructure)==true);
    NTAttributePtr ptr = NTAttribute::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTAttribute::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTAttribute) {
    testPlan(35);
    test_builder();
    test_ntattribute();
    test_wrap();
    return testDone();
}


