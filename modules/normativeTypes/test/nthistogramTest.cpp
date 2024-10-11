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

    NTHistogramBuilderPtr builder = NTHistogram::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            value(pvLong)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTHistogram::is_a(structure));
    testOk1(structure->getID() == NTHistogram::URI);
    testOk1(structure->getNumberFields() == 7);
    testOk1(structure->getField("ranges").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);

    testOk(dynamic_pointer_cast<const ScalarArray>(structure->getField("ranges")).get() != 0 &&
            dynamic_pointer_cast<const ScalarArray>(structure->getField("ranges"))->getElementType() == pvDouble, "ranges array element type");

    testOk(dynamic_pointer_cast<const ScalarArray>(structure->getField("value")).get() != 0 &&
            dynamic_pointer_cast<const ScalarArray>(structure->getField("value"))->getElementType() == pvLong, "value array element type");

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

void test_nthistogram()
{
    testDiag("test_nthistogram");

    NTHistogramBuilderPtr builder = NTHistogram::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    NTHistogramPtr ntHistogram = builder->
            value(pvInt)->
            addDescriptor()->
            addAlarm()->
            addTimeStamp()->
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            create();
    testOk1(ntHistogram.get() != 0);

    testOk1(NTHistogram::is_a(ntHistogram->getPVStructure()));
    testOk1(NTHistogram::isCompatible(ntHistogram->getPVStructure()));

    testOk1(ntHistogram->getPVStructure().get() != 0);
    testOk1(ntHistogram->getDescriptor().get() != 0);
    testOk1(ntHistogram->getAlarm().get() != 0);
    testOk1(ntHistogram->getTimeStamp().get() != 0);
    testOk1(ntHistogram->getRanges().get() != 0);
    testOk1(ntHistogram->getValue().get() != 0);
    testOk1(ntHistogram->getValue<PVIntArray>().get() != 0);
    //
    // example how to set ranges
    //
    PVDoubleArray::svector newRanges;
    newRanges.push_back(-100.0);
    newRanges.push_back(0.0);
    newRanges.push_back(100.0);

    PVDoubleArrayPtr pvRangesField = ntHistogram->getRanges();
    pvRangesField->replace(freeze(newRanges));

    //
    // example how to get ranges
    //
    PVDoubleArray::const_svector ranges(pvRangesField->view());

    testOk1(ranges.size() == 3);
    testOk1(ranges[0] == -100.0);
    testOk1(ranges[1] == 0.0);
    testOk1(ranges[2] == 100.0);

    //
    // example how to set value
    //
    PVIntArray::svector newValue;
    newValue.push_back(1);
    newValue.push_back(2);

    PVIntArrayPtr pvValueField = ntHistogram->getValue<PVIntArray>();
    pvValueField->replace(freeze(newValue));

    //
    // example how to get values for each bin
    //
    PVIntArray::const_svector value(pvValueField->view());

    testOk1(value.size() == 2);
    testOk1(value[0] == 1);
    testOk1(value[1] == 2);

    //
    // test isValid
    //
    testOk1(ntHistogram->isValid());
    //
    // timeStamp ops
    //
    PVTimeStamp pvTimeStamp;
    if (ntHistogram->attachTimeStamp(pvTimeStamp))
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
    if (ntHistogram->attachAlarm(pvAlarm))
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
    ntHistogram->getDescriptor()->put("This is a test NTHistogram");

    // dump NTHistogram
    std::cout << *ntHistogram->getPVStructure() << std::endl;

}


void test_wrap()
{
    testDiag("test_wrap");

    NTHistogramPtr nullPtr = NTHistogram::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTHistogram::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTHistogramBuilderPtr builder = NTHistogram::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            value(pvInt)->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;

    testOk1(NTHistogram::isCompatible(pvStructure)==true);
    NTHistogramPtr ptr = NTHistogram::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTHistogram::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

void test_extra()
{
    testDiag("test_extra");

    NTHistogramBuilderPtr builder = NTHistogram::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    StructureConstPtr structure = builder->
            value(pvInt)->
            addTimeStamp()->
            add("function", getFieldCreate()->createScalar(pvString))->
            createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTHistogram::is_a(structure));
    testOk1(structure->getID() == NTHistogram::URI);
    testOk1(structure->getNumberFields() == 4);
    testOk1(structure->getField("ranges").get() != 0);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("function").get() != 0);

    testOk(dynamic_pointer_cast<const Scalar>(structure->getField("function")).get() != 0 &&
            dynamic_pointer_cast<const Scalar>(structure->getField("function"))->getScalarType() == pvString, "function type");

    std::cout << *structure << std::endl;
}


MAIN(testNTHistogram) {
    testPlan(52);
    test_builder();
    test_nthistogram();
    test_wrap();
    test_extra();
    return testDone();
}


