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

    NTNDArrayAttributeBuilderPtr builder = NTNDArrayAttribute::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            addTags()->
            addAlarm()->
            addTimeStamp()->
            add("extra",fieldCreate->createScalar(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTNDArrayAttribute::is_a(structure));
    testOk1(structure->getID() == NTNDArrayAttribute::URI);
    testOk1(structure->getNumberFields() == 9);
    testOk1(structure->getField("name").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("tags").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("sourceType").get() != 0);
    testOk1(structure->getField("source").get() != 0);

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

void test_ntndarrayAttribute()
{
    testDiag("test_ntndarrayAttribute");

    NTNDArrayAttributeBuilderPtr builder = NTNDArrayAttribute::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTNDArrayAttributePtr ntNDArrayAttribute = builder->
            addTags()->
            addAlarm()->
            addTimeStamp()->
            create();
    testOk1(ntNDArrayAttribute.get() != 0);

    testOk1(NTNDArrayAttribute::is_a(ntNDArrayAttribute->getPVStructure()));
    testOk1(NTNDArrayAttribute::isCompatible(ntNDArrayAttribute->getPVStructure()));

    testOk1(ntNDArrayAttribute->getPVStructure().get() != 0);
    testOk1(ntNDArrayAttribute->getName().get() != 0);
    testOk1(ntNDArrayAttribute->getValue().get() != 0);
    testOk1(ntNDArrayAttribute->getTags().get() != 0);
    testOk1(ntNDArrayAttribute->getDescriptor().get() != 0);
    testOk1(ntNDArrayAttribute->getAlarm().get() != 0);
    testOk1(ntNDArrayAttribute->getTimeStamp().get() != 0);
    testOk1(ntNDArrayAttribute->getSourceType().get() != 0);
    testOk1(ntNDArrayAttribute->getSource().get() != 0);

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntNDArrayAttribute->attachTimeStamp(pvTimeStamp))
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
    if (ntNDArrayAttribute->attachAlarm(pvAlarm))
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
    ntNDArrayAttribute->getDescriptor()->put("This is a test NTNDArrayAttribute");

    // dump ntNDArrayAttribute
    std::cout << *ntNDArrayAttribute->getPVStructure() << std::endl;

}

void test_wrap()
{
    testDiag("test_wrap");

    NTNDArrayAttributePtr nullPtr = NTNDArrayAttribute::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTNDArrayAttribute::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTNDArrayAttributeBuilderPtr builder = NTNDArrayAttribute::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTNDArrayAttribute::isCompatible(pvStructure)==true);
    NTNDArrayAttributePtr ptr = NTNDArrayAttribute::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTNDArrayAttribute::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTNDArrayAttribute) {
    testPlan(39);
    test_builder();
    test_ntndarrayAttribute();
    test_wrap();
    return testDone();
}


