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

    NTTableBuilderPtr builder = NTTable::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            addColumn("column0", pvDouble)->
            addColumn("column1", pvString)->
            addColumn("column2", pvInt)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTTable::is_a(structure));
    testOk1(structure->getID() == NTTable::URI);
    testOk1(structure->getNumberFields() == 7);
    testOk1(structure->getField("labels").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("extra1").get() != 0);
    testOk1(structure->getField("extra2").get() != 0);

    StructureConstPtr s = dynamic_pointer_cast<const Structure>(structure->getField("value"));
#define TEST_COLUMN(name, type) \
    testOk(s.get() != 0 && \
           s->getField(name).get() != 0 && \
           dynamic_pointer_cast<const ScalarArray>(s->getField(name)).get() != 0 && \
           dynamic_pointer_cast<const ScalarArray>(s->getField(name))->getElementType() == type, \
           name " check");
    TEST_COLUMN("column0", pvDouble);
    TEST_COLUMN("column1", pvString);
    TEST_COLUMN("column2", pvInt);
#undef TEST_COLUMN

    std::cout << *structure << std::endl;

    // duplicate test
    try
    {
        structure = builder->
                addColumn("column0", pvDouble)->
                addColumn("column0", pvString)->
                createStructure();
        testFail("duplicate column name");
    } catch (std::runtime_error &) {
        testPass("duplicate column name");
    }
}

void test_labels()
{
    testDiag("test_labels");

    NTTableBuilderPtr builder = NTTable::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            addColumn("column0", pvDouble)->
            addColumn("column1", pvString)->
            addColumn("column2", pvInt)->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTTable::isCompatible(pvStructure)==true);
    std::cout << *pvStructure << std::endl;

    PVStringArrayPtr labels = pvStructure->getSubField<PVStringArray>("labels");
    testOk1(labels.get() != 0);
    testOk1(labels->getLength() == 3);

    PVStringArray::const_svector l(labels->view());
    testOk1(l[0] == "column0");
    testOk1(l[1] == "column1");
    testOk1(l[2] == "column2");
}

void test_nttable()
{
    testDiag("test_nttable");

    NTTableBuilderPtr builder = NTTable::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTTablePtr ntTable = builder->
            addColumn("column0", pvDouble)->
            addColumn("column1", pvString)->
            addColumn("column2", pvInt)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            create();
    testOk1(ntTable.get() != 0);

    testOk1(NTTable::is_a(ntTable->getPVStructure()));
    testOk1(NTTable::isCompatible(ntTable->getPVStructure()));

    testOk1(ntTable->getPVStructure().get() != 0);
    testOk1(ntTable->getDescriptor().get() != 0);
    testOk1(ntTable->getAlarm().get() != 0);
    testOk1(ntTable->getTimeStamp().get() != 0);
    testOk1(ntTable->getLabels().get() != 0);

    testOk1(ntTable->getColumn<PVDoubleArray>("column0").get() != 0);
    testOk1(ntTable->getColumn<PVStringArray>("column1").get() != 0);
    testOk1(ntTable->getColumn<PVIntArray>("column2").get() != 0);

    testOk1(ntTable->getColumn("invalid").get() == 0);

    //
    // example how to set column values
    //
    PVIntArray::svector newValues;
    newValues.push_back(1);
    newValues.push_back(2);
    newValues.push_back(8);

    PVIntArrayPtr intColumn = ntTable->getColumn<PVIntArray>("column2");
    intColumn->replace(freeze(newValues));

    //
    // example how to get column values
    //
    PVIntArray::const_svector values(intColumn->view());

    testOk1(values.size() == 3);
    testOk1(values[0] == 1);
    testOk1(values[1] == 2);
    testOk1(values[2] == 8);

    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntTable->attachTimeStamp(pvTimeStamp))
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
    if (ntTable->attachAlarm(pvAlarm))
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
    ntTable->getDescriptor()->put("This is a test NTTable");

    // dump NTTable
    std::cout << *ntTable->getPVStructure() << std::endl;

}

void test_wrap()
{
    testDiag("test_wrap");

    NTTablePtr nullPtr = NTTable::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTTable::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTTableBuilderPtr builder = NTTable::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            addColumn("column0", pvDouble)->
            addColumn("column1", pvString)->
            addColumn("column2", pvInt)->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    NTTablePtr ptr = NTTable::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTTable::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTTable) {
    testPlan(50);
    test_builder();
    test_labels();
    test_nttable();
    test_wrap();
    return testDone();
}


