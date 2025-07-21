/* testPVUnion.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2014.07 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::string;
using std::cout;
using std::endl;

static FieldCreatePtr fieldCreate = getFieldCreate();
static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static StandardFieldPtr standardField = getStandardField();
static StandardPVFieldPtr standardPVField = getStandardPVField();

static void testPVUnionType()
{
    testDiag("testPVUnionType");
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(
        standardField->regUnion(
            fieldCreate->createFieldBuilder()->
                add("doubleValue", pvDouble)->
                add("intValue", pvInt)->
                add("timeStamp",standardField->timeStamp())->
                createUnion(),
            "alarm,timeStamp"));
    PVUnionPtr pvValue = pvStructure->getSubField<PVUnion>("value");
    PVStructurePtr pvTime= pvValue->select<PVStructure>(2);
    TimeStamp timeStamp;
    timeStamp.getCurrent();
    PVTimeStamp pvTimeStamp;
    pvTimeStamp.attach(pvTime);
    pvTimeStamp.set(timeStamp);
    testOk1(
        pvTime->getSubField<PVLong>("secondsPastEpoch")->get()
        ==
        pvValue->get<PVStructure>()->getSubField<PVLong>("secondsPastEpoch")->get()
    );
    PVDoublePtr pvDouble = pvValue->select<PVDouble>("doubleValue");
    pvDouble->put(1e5);
    testOk1(pvDouble->get()==pvValue->get<PVDouble>()->get());
    PVIntPtr pvInt = pvValue->select<PVInt>("intValue");
    pvInt->put(15);
    testOk1(pvInt->get()==pvValue->get<PVInt>()->get());
    std::cout << "testPVUnion PASSED" << std::endl;
}

static void testPVUnionArray()
{
    testDiag("testPVUnionArray");
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(
        standardField->unionArray(
            fieldCreate->createFieldBuilder()->
                add("doubleValue", pvDouble)->
                add("intValue", pvInt)->
                add("timeStamp",standardField->timeStamp())->
                createUnion(),
            "alarm,timeStamp"));
    PVUnionArrayPtr pvValue = pvStructure->getSubField<PVUnionArray>("value");
    size_t num = 3;
    shared_vector<PVUnionPtr> unions(3);
    for(size_t i=0; i<num; ++i)
    {
        unions[i] = pvDataCreate->createPVUnion(pvValue->getUnionArray()->getUnion());
    }
    unions[0]->select("doubleValue");
    unions[1]->select("intValue");
    unions[2]->select("timeStamp");
    PVDoublePtr pvDouble = unions[0]->get<PVDouble>();
    pvDouble->put(1.235);
    PVIntPtr pvInt = unions[1]->get<PVInt>();
    pvInt->put(5);
    PVStructurePtr pvTime = unions[2]->get<PVStructure>();
    TimeStamp timeStamp;
    timeStamp.getCurrent();
    PVTimeStamp pvTimeStamp;
    pvTimeStamp.attach(pvTime);
    pvTimeStamp.set(timeStamp);
    pvValue->replace(freeze(unions));
    shared_vector<const PVUnionPtr> sharedUnions = pvValue->view();
    testOk1(pvDouble->get()==sharedUnions[0]->get<PVDouble>()->get());
    testOk1(pvInt->get()==sharedUnions[1]->get<PVInt>()->get());
    testOk1(
        pvTime->getSubField<PVLong>("secondsPastEpoch")->get()
        ==
        sharedUnions[2]->get<PVStructure>()->getSubField<PVLong>("secondsPastEpoch")->get()
    );
    std::cout << "testPVUnionArray PASSED" << std::endl;
}

static void testClearUnion()
{
    testDiag("testClearUnion w/ variant");

    PVUnionPtr variant(pvDataCreate->createPVVariantUnion());

    testOk(!variant->get(), "Initially empty");

    variant->select(PVUnion::UNDEFINED_INDEX);

    testOk(!variant->get(), "Still empty");

    {
        PVFieldPtr bval(pvDataCreate->createPVScalar<PVBoolean>());
        variant->set(bval);

        testOk(variant->get().get()==bval.get(), "Now with bool");

        variant->set(PVUnion::UNDEFINED_INDEX, PVFieldPtr());

        testOk(!variant->get(), "Again empty");
    }

    {
        PVFieldPtr bval(pvDataCreate->createPVScalar<PVBoolean>());
        variant->set(bval);

        testOk(variant->get().get()==bval.get(), "Now with bool");

        variant->select(PVUnion::UNDEFINED_INDEX);

        testOk(!variant->get(), "Again empty");
    }

    testDiag("testClearUnion w/ discriminating");

    PVUnionPtr discrim(pvDataCreate->createPVUnion(
                fieldCreate->createFieldBuilder()->
                        add("doubleValue", pvDouble)->
                        add("intValue", pvInt)->
                        add("timeStamp",standardField->timeStamp())->
                    createUnion()
                ));


    testOk(!discrim->get(), "Initially empty");

    discrim->select(PVUnion::UNDEFINED_INDEX);

    testOk(!discrim->get(), "Still empty");

    {
        PVFieldPtr bval(pvDataCreate->createPVScalar<PVDouble>());

        try {
            discrim->set(bval);
            testFail("set(bval) with UNDEFINED_INDEX didn't fail as expected");
        }catch (std::invalid_argument&){
            testPass("cause expected invalid_argument");
        }

        testOk(!discrim->get(), "Still empty");

        discrim->select("doubleValue");
        discrim->set(bval);

        testOk(discrim->get().get()==bval.get(), "Now with bool");

        discrim->set(PVUnion::UNDEFINED_INDEX, PVFieldPtr());

        testOk(!discrim->get(), "Again empty");
    }

    {
        PVFieldPtr bval(pvDataCreate->createPVScalar<PVDouble>());

        testOk(!discrim->get(), "Still empty");

        discrim->set("doubleValue", bval);

        testOk(discrim->get().get()==bval.get(), "Now with bool");

        discrim->set(PVUnion::UNDEFINED_INDEX, PVFieldPtr());

        testOk(!discrim->get(), "Again empty");
    }
}

MAIN(testPVUnion)
{
    testPlan(21);
    testPVUnionType();
    testPVUnionArray();
    testClearUnion();
    return testDone();
}
