/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <epicsUnitTest.h>
#include <testMain.h>

#include "../src/validator.h"
#include <pv/ntndarray.h>
#include <pv/pvIntrospect.h>

using namespace epics::nt;
using namespace epics::pvData;

static epics::pvData::FieldCreatePtr FC;

void test_is()
{
    testDiag("test_is");

    // Result::is<Scalar> must be valid for Scalars of any type
    for(int i = pvBoolean; i <= pvString; ++i) {
        ScalarType t = static_cast<ScalarType>(i);
        testOk(Result(FC->createScalar(t)).is<Scalar>().valid(),
            "Result(Scalar<%s>).is<Scalar>().valid()", ScalarTypeFunc::name(t));
    }

    // Result::is<ScalarArray> must be valid for ScalarArray of any type
    for(int i = pvBoolean; i <= pvString; ++i) {
        ScalarType t = static_cast<ScalarType>(i);
        testOk(Result(FC->createScalarArray(t)).is<ScalarArray>().valid(),
            "Result(ScalarArray<%s>).is<ScalarArray>().valid()", ScalarTypeFunc::name(t));
    }

    {
        // Result::is<Scalar> must be invalid for non-Scalar
        Result result(FC->createScalarArray(pvInt));
        result.is<Scalar>();
        testOk(!result.valid(), "!Result(ScalarArray<pvInt>).is<Scalar>.valid()");
        testOk1(result.errors.at(0) == Result::Error("", Result::Error::IncorrectType));
    }

    {
        // Result::is<ScalarArray> must be invalid for non-ScalarArray
        Result result(FC->createScalar(pvInt));
        result.is<ScalarArray>();
        testOk(!result.valid(), "!Result(ScalarArray<pvInt>).is<Scalar>.valid()");
        testOk1(result.errors.at(0) == Result::Error("", Result::Error::IncorrectType));
    }
}

void test_is_id()
{
    testDiag("test_is_id");

    FieldBuilderPtr FB(FieldBuilder::begin());

    {
        // Both type and ID match for Structure
        Result result(FB->setId("TEST_ID")->createStructure());
        result.is<Structure>("TEST_ID");
        testOk(result.valid(), "Result(Structure['TEST_ID']).is<Structure>('TEST_ID').valid()");
    }

    {
        // Both type and ID match for Union
        UnionConstPtr un(FB->
            setId("TEST_ID")->
            add("A", pvInt)->
            add("B", pvString)->
            createUnion()
        );
        Result result(un);
        result.is<Union>("TEST_ID");
        testOk(result.valid(), "Result(Union{A:int,B:string}['TEST_ID']).is<Union>('TEST_ID').valid()");
    }

    {
        // Both type and ID match for Variant Union
        Result result(FB-> createUnion());
        result.is<Union>(Union::ANY_ID);
        testOk(result.valid(), "Result(Union).is<Union>('%s').valid()", Union::ANY_ID.c_str());
    }

    {
        // ID matches, type doesn't
        Result result(FB->setId("TEST_ID")->createStructure());
        result.is<Union>("TEST_ID");
        testOk(!result.valid(), "!Result(Union['TEST_ID']).is<Structure>('TEST_ID').valid()");
        testOk1(result.errors.at(0) == Result::Error("", Result::Error::IncorrectType));
    }
    
    {
        // Type matches, ID doesn't
        Result result(FB->setId("WRONG_ID")->createStructure());
        result.is<Structure>("TEST_ID");
        testOk(!result.valid(), "!Result(Structure['WRONG_ID']).is<Structure>('TEST_ID').valid()");
        testOk1(result.errors.at(0) == Result::Error("", Result::Error::IncorrectId));
    }

    {
        // Neither type nor ID match (ID is not even checked in this case since it doesn't exist)
        Result result(FC->createScalar(pvDouble));
        result.is<Structure>("SOME_ID");
        testOk(!result.valid(), "!Result(Scalar).is<Structure>('SOME_ID').valid()");
        testOk1(result.errors.at(0) == Result::Error("", Result::Error::IncorrectType));
    }
}

void test_has()
{
    testDiag("test_has");

    FieldBuilderPtr FB(FieldBuilder::begin());

    StructureConstPtr struc(FB->
        add("A", pvInt)->
        add("B", pvString)->
        createStructure()
    );

    std::string strucRepr("Structure{A:int,B:String}");

    {
        // Test that struc has both A and B, both being Scalars
        Result result(struc);
        result
           .has<Scalar>("A")
           .has<Scalar>("B");

        testOk(result.valid(),
            "Result(%s).has<Scalar>('A').has<Scalar>('B').valid()",
            strucRepr.c_str());
    }

    {
        // Test that struc does not have a field B of type ScalarArray
        Result result(struc);
        result
           .has<Scalar>("A")
           .has<ScalarArray>("B");
        testOk(!result.valid(), 
            "!Result(%s).has<Scalar>('A').has<ScalarArray>('B').valid()",
            strucRepr.c_str());
        testOk1(result.errors.at(0) == Result::Error("B", Result::Error::IncorrectType));
    }

    {
        // Test that struc does not have a field C
        Result result(struc);
        result
           .has<Scalar>("A")
           .has<Scalar>("C");
        testOk(!result.valid(), 
            "!Result(%s).has<Scalar>('A').has<Scalar>('C').valid()",
            strucRepr.c_str());
        testOk1(result.errors.at(0) == Result::Error("C", Result::Error::MissingField));
    }

    {
        // Test that 'has' fails for non-structure-like Fields
        Result result(FC->createScalar(pvByte));
        result.has<Scalar>("X");
        testOk(!result.valid(), "!Result(Scalar<pvByte>).has<Scalar>('X').valid()");
        testOk1(result.errors.at(0) == Result::Error("", Result::Error::IncorrectType));
    }
}

void test_maybe_has()
{
    testDiag("test_maybe_has");

    FieldBuilderPtr FB(FieldBuilder::begin());

    StructureConstPtr struc(FB->
        add("A", pvInt)->
        add("B", pvString)->
        createStructure()
    );

    std::string strucRepr("Structure{A:int,B:String}");

    {
        // Test that struc maybe has A and B, both being Scalars
        Result result(struc);
        result
           .maybeHas<Scalar>("A")
           .maybeHas<Scalar>("B");

        testOk(result.valid(),
            "Result(%s).maybeHas<Scalar>('A').maybeHas<Scalar>('B').valid()",
            strucRepr.c_str());
    }

    {
        // Test that if struc has a field B, it must be of type ScalarArray
        Result result(struc);
        result
           .maybeHas<Scalar>("A")
           .maybeHas<ScalarArray>("B");
        testOk(!result.valid(), 
            "!Result(%s).maybeHas<Scalar>('A').maybeHas<ScalarArray>('B').valid()",
            strucRepr.c_str());
        testOk1(result.errors.at(0) == Result::Error("B", Result::Error::IncorrectType));
    }

    {
        // Test that struc maybe has A (which it does) and B (which it doesn't)
        Result result(struc);
        result
           .maybeHas<Scalar>("A")
           .maybeHas<Scalar>("C");
        testOk(result.valid(), 
            "Result(%s).maybeHas<Scalar>('A').maybeHas<Scalar>('C').valid()",
            strucRepr.c_str());
    }

    {
        // Test that 'maybeHas' fails for non-structure-like Fields
        Result result(FC->createScalar(pvByte));
        result.maybeHas<Scalar>("X");
        testOk(!result.valid(), "!Result(Scalar<pvByte>).maybeHas<Scalar>('X').valid()");
        testOk1(result.errors.at(0) == Result::Error("", Result::Error::IncorrectType));
    }
}

Result& isStructABC(Result& result)
{
    return result
        .is<Structure>("ABC")
        .has<Scalar>("A")
        .has<ScalarArray>("B")
        .maybeHas<Scalar>("C");
}

void test_has_fn()
{
    testDiag("test_has_fn");
    FieldBuilderPtr FB(FieldBuilder::begin());

    {
        StructureConstPtr inner(FB->
            setId("ABC")->
            add("A", pvInt)->
            addArray("B", pvDouble)->
            add("C", pvString)->
            createStructure()
        );

        Result result(FB->add("inner", inner)->createStructure());
        result.has<&isStructABC>("inner");

        testOk(result.valid(), "Result({inner:<valid structABC>}).has<&isStructAbc>('inner').valid()");
    }

    {
        StructureConstPtr inner(FB->
            setId("ABC")->
            add("A", pvInt)->
            addArray("B", pvDouble)->
            createStructure()
        );

        Result result(FB->add("inner", inner)->createStructure());
        result.has<&isStructABC>("inner");

        testOk(result.valid(), "Result({inner:<valid structABC w/o C>}).has<&isStructAbc>('inner').valid()");
    }

    {
        StructureConstPtr inner(FB->
            setId("XYZ")->
            add("A", pvInt)->
            addArray("B", pvDouble)->
            createStructure()
        );

        Result result(FB->add("inner", inner)->createStructure());
        result.has<&isStructABC>("inner");

        testOk(!result.valid(), "!Result({inner:<structABC wrong id>}).has<&isStructAbc>('inner').valid()");
        testOk1(result.errors.at(0) == Result::Error("inner", Result::Error::IncorrectId));
    }

    {
        StructureConstPtr inner(FB->
            setId("XYZ")->
            add("A", pvInt)->
            add("B", pvDouble)->
            createStructure()
        );

        Result result(FB->add("inner", inner)->createStructure());
        result.has<&isStructABC>("inner");

        testOk(!result.valid(), "!Result({inner:<structABC wrong id and fields>}).has<&isStructAbc>('inner').valid()");
        testOk1(result.errors.size() == 2);
    }
}

MAIN(testValidator) {
    testPlan(56);
    FC = epics::pvData::getFieldCreate();
    test_is();
    test_is_id();
    test_has();
    test_maybe_has();
    test_has_fn();
    return testDone();
}
