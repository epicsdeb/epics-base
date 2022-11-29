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

void test_builder(bool extraFields)
{
    testDiag("test_builder");

    NTNDArrayBuilderPtr builder = NTNDArray::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    builder->addDescriptor()->
             addTimeStamp()->          
             addAlarm()->  
             addDisplay();

    if (extraFields)
    {
        builder->add("extra1",fieldCreate->createScalar(pvString))->
                 add("extra2",fieldCreate->createScalarArray(pvString));
    }

    StructureConstPtr structure = builder->createStructure();
    testOk1(structure.get() != 0);
    if (!structure)
        return;

    testOk1(NTNDArray::is_a(structure));
    testOk1(structure->getID() == NTNDArray::URI);
    testOk1(structure->getField("value").get() != 0);
    testOk1(structure->getField("compressedSize").get() != 0);
    testOk1(structure->getField("uncompressedSize").get() != 0);
    testOk1(structure->getField("codec").get() != 0);
    testOk1(structure->getField("dimension").get() != 0);
    testOk1(structure->getField("uniqueId").get() != 0);
    testOk1(structure->getField("dataTimeStamp").get() != 0);
    testOk1(structure->getField("attribute").get() != 0);
    testOk1(structure->getField("descriptor").get() != 0);
    testOk1(structure->getField("alarm").get() != 0);
    testOk1(structure->getField("timeStamp").get() != 0);
    testOk1(structure->getField("display").get() != 0);
    if (extraFields)
    {
        testOk1(structure->getField("extra1").get() != 0);
        testOk1(structure->getField("extra2").get() != 0);
    }
    std::cout << *structure << std::endl;

}

void test_all()
{
    testDiag("test_builder");

    NTNDArrayBuilderPtr builder = NTNDArray::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            addDescriptor()->
            addTimeStamp()->          
            addAlarm()->  
            addDisplay()->  
            add("extra1",fieldCreate->createScalar(pvString)) ->
            add("extra2",fieldCreate->createScalarArray(pvString)) ->
            createPVStructure();
    testOk1(NTNDArray::is_a(pvStructure)==true);
    testOk1(NTNDArray::isCompatible(pvStructure)==true);
}


void test_wrap()
{
    testDiag("test_wrap");

    NTNDArrayPtr nullPtr = NTNDArray::wrap(PVStructurePtr());
    testOk(nullPtr.get() == 0, "nullptr wrap");

    nullPtr = NTNDArray::wrap(
                getPVDataCreate()->createPVStructure(
                    NTField::get()->createTimeStamp()
                    )
                );
    testOk(nullPtr.get() == 0, "wrong type wrap");


    NTNDArrayBuilderPtr builder = NTNDArray::createBuilder();
    testOk(builder.get() != 0, "Got builder");

    PVStructurePtr pvStructure = builder->
            createPVStructure();
    testOk1(pvStructure.get() != 0);
    if (!pvStructure)
        return;
    testOk1(NTNDArray::isCompatible(pvStructure)==true);

    NTNDArrayPtr ptr = NTNDArray::wrap(pvStructure);
    testOk(ptr.get() != 0, "wrap OK");

    ptr = NTNDArray::wrapUnsafe(pvStructure);
    testOk(ptr.get() != 0, "wrapUnsafe OK");
}

MAIN(testNTNDArray) {
    testPlan(60);
    test_builder(true);
    test_builder(false);
    test_builder(false); // called twice to test caching
    test_all();
    test_wrap();
    return testDone();
}


