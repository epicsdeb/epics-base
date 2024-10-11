/* testIntrospect.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2010.09 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <sstream>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/standardField.h>

using namespace epics::pvData;
using std::string;

static FieldCreatePtr fieldCreate;
static PVDataCreatePtr pvDataCreate;
static StandardFieldPtr standardField;

static void testScalarCommon(ScalarType stype,
    bool isInteger,bool isNumeric,bool isPrimitive)
{
    ScalarConstPtr pscalar = fieldCreate->createScalar(stype);
    Type type = pscalar->getType();
    testOk1(type==scalar);

    std::ostringstream oss;
    oss << type;
    testOk1(oss.str().compare("scalar")==0);

    ScalarType scalarType = pscalar->getScalarType();
    testOk1(scalarType==stype);
    testOk1(ScalarTypeFunc::isInteger(scalarType)==isInteger);
    testOk1(ScalarTypeFunc::isNumeric(scalarType)==isNumeric);
    testOk1(ScalarTypeFunc::isPrimitive(scalarType)==isPrimitive);
}

static void testScalar() {
    testDiag("testScalar");
    testScalarCommon(pvBoolean,false,false,true);
    testScalarCommon(pvByte,true,true,true);
    testScalarCommon(pvShort,true,true,true);
    testScalarCommon(pvInt,true,true,true);
    testScalarCommon(pvLong,true,true,true);
    testScalarCommon(pvFloat,false,true,true);
    testScalarCommon(pvDouble,false,true,true);
    testScalarCommon(pvString,false,false,false);
}

static void testScalarArrayCommon(ScalarType stype,
    bool isInteger,bool isNumeric,bool isPrimitive,
    Array::ArraySizeType sizeType = Array::variable, size_t size = 0)
{
    ScalarArrayConstPtr pscalar;
    switch (sizeType)
    {
        case Array::variable:
            pscalar = fieldCreate->createScalarArray(stype);
            size = 0;
            break;
        case Array::bounded:
            pscalar = fieldCreate->createBoundedScalarArray(stype, size);
            break;
        case Array::fixed:
            pscalar = fieldCreate->createFixedScalarArray(stype, size);
            break;
        default:
            throw std::invalid_argument("unsupported array size type");
    }

    Type type = pscalar->getType();
    testOk1(type==scalarArray);

    std::ostringstream oss;
    oss << type;
    testOk1(oss.str().compare("scalarArray")==0);

    testOk1(pscalar->getArraySizeType()==sizeType);
    testOk1(pscalar->getMaximumCapacity()==size);

    ScalarType scalarType = pscalar->getElementType();
    testOk1(scalarType==stype);
    testOk1(ScalarTypeFunc::isInteger(scalarType)==isInteger);
    testOk1(ScalarTypeFunc::isNumeric(scalarType)==isNumeric);
    testOk1(ScalarTypeFunc::isPrimitive(scalarType)==isPrimitive);
}

static void testScalarArray() {
    testDiag("testScalarArray");
    testScalarArrayCommon(pvBoolean,false,false,true);
    testScalarArrayCommon(pvByte,true,true,true);
    testScalarArrayCommon(pvShort,true,true,true);
    testScalarArrayCommon(pvInt,true,true,true);
    testScalarArrayCommon(pvLong,true,true,true);
    testScalarArrayCommon(pvFloat,false,true,true);
    testScalarArrayCommon(pvDouble,false,true,true);
    testScalarArrayCommon(pvString,false,false,false);

    testScalarArrayCommon(pvBoolean,false,false,true,Array::bounded,10);
    testScalarArrayCommon(pvByte,true,true,true,Array::bounded,10);
    testScalarArrayCommon(pvShort,true,true,true,Array::bounded,10);
    testScalarArrayCommon(pvInt,true,true,true,Array::bounded,10);
    testScalarArrayCommon(pvLong,true,true,true,Array::bounded,10);
    testScalarArrayCommon(pvFloat,false,true,true,Array::bounded,10);
    testScalarArrayCommon(pvDouble,false,true,true,Array::bounded,10);
    testScalarArrayCommon(pvString,false,false,false,Array::bounded,10);

    testScalarArrayCommon(pvBoolean,false,false,true,Array::fixed,16);
    testScalarArrayCommon(pvByte,true,true,true,Array::fixed,16);
    testScalarArrayCommon(pvShort,true,true,true,Array::fixed,16);
    testScalarArrayCommon(pvInt,true,true,true,Array::fixed,16);
    testScalarArrayCommon(pvLong,true,true,true,Array::fixed,16);
    testScalarArrayCommon(pvFloat,false,true,true,Array::fixed,16);
    testScalarArrayCommon(pvDouble,false,true,true,Array::fixed,16);
    testScalarArrayCommon(pvString,false,false,false,Array::fixed,16);
}

static void testStructure()
{
    testDiag("testStructure");
    StringArray names1(2);
    names1[0] = "innerA";
    names1[1] = "innerB";
    FieldConstPtrArray fields1(2);
    fields1[0] = fieldCreate->createScalar(pvDouble);
    fields1[1] = fieldCreate->createScalarArray(pvString);

    StructureConstPtr struct1 = fieldCreate->createStructure(names1, fields1);

    testOk1(struct1->getNumberFields()==2);
    testOk1(struct1->getField("innerA")==fields1[0]);
    testOk1(struct1->getField("innerB")==fields1[1]);
    testOk1(struct1->getFieldIndex("innerA")==0);
    testOk1(struct1->getFieldIndex("innerB")==1);
    testOk1(struct1->getField(0)==fields1[0]);
    testOk1(struct1->getField(1)==fields1[1]);
    testOk1(struct1->getFieldName(0)==names1[0]);
    testOk1(struct1->getFieldName(1)==names1[1]);

    testOk1(struct1->getField("nonexistent").get()==NULL);
    try {
        FieldConstPtr field(struct1->getField(9999));
        testFail("struct1->getField(9999): missing expected exception");
    } catch (std::out_of_range& e) {
        testPass("struct1->getField(9999): caught expected exception: %s", e.what());
    }

    testOk1(struct1->getFieldT("innerA")==fields1[0]);
    testOk1(struct1->getFieldT("innerB")==fields1[1]);
    testOk1(struct1->getFieldT(0)==fields1[0]);
    testOk1(struct1->getFieldT(1)==fields1[1]);

    try {
        FieldConstPtr field(struct1->getFieldT("nonexistent"));
        testFail("struct1->getFieldT('nonexistent'): missing required exception");
    } catch (std::runtime_error& e) {
        testPass("struct1->getFieldT('nonexistent'): caught expected exception: %s", e.what());
    }

    try {
        FieldConstPtr field(struct1->getFieldT(9999));
        testFail("struct1->getFieldT(9999): missing required exception");
    } catch (std::out_of_range& e) {
        testPass("struct1->getFieldT(9999): caught expected exception: %s", e.what());
    }

    testOk1(struct1->getField<Scalar>("innerA").get()!=NULL);
    testOk1(struct1->getField<Structure>("innerA").get()==NULL);
    testOk1(struct1->getField<ScalarArray>(1).get()!=NULL);
    testOk1(struct1->getField<Structure>(1).get()==NULL);

    testOk1(struct1->getFieldT<Scalar>("innerA").get()!=NULL);
    try {
        StructureConstPtr s(struct1->getFieldT<Structure>("innerA"));
        testFail("struct1->getFieldT<Structure>('innnerA'): missing required exception");
    } catch (std::runtime_error& e) {
        testPass("struct1->getFieldT<Structure>('innnerA'): caught expected exception: %s", e.what());
    }

    testOk1(struct1->getFieldT<ScalarArray>(1).get()!=NULL);
    try {
        StructureConstPtr s(struct1->getFieldT<Structure>(1));
        testFail("struct1->getFieldT<Structure>(1): missing required exception");
    } catch (std::runtime_error& e) {
        testPass("struct1->getFieldT<Structure>(1): caught expected exception: %s", e.what());
    }

    testOk1(struct1->getID() == Structure::DEFAULT_ID);

    testOk1(fields1 == struct1->getFields()); // vector equality

    StringArray names2(2);
    names2[0] = "outerA";
    names2[1] = "outerB";
    FieldConstPtrArray fields2(2);
    fields2[0] = fieldCreate->createScalar(pvInt);
    fields2[1] = std::tr1::static_pointer_cast<const Field>(struct1);

    StructureConstPtr struct2 = fieldCreate->createStructure(names2, fields2);

    testOk1(struct2->getNumberFields()==2); // not recursive
    testOk1(struct2->getField(1)==fields2[1]);

    StructureArrayConstPtr struct1arr = fieldCreate->createStructureArray(struct1);

    testOk1(struct1arr->getStructure()==struct1);
    testOk1(struct1arr->getID()=="structure[]");
}

static void testUnion()
{
    testDiag("testUnion");
    StringArray names1(2);
    names1[0] = "innerA";
    names1[1] = "innerB";
    FieldConstPtrArray fields1(2);
    fields1[0] = fieldCreate->createScalar(pvDouble);
    fields1[1] = fieldCreate->createScalarArray(pvString);

    UnionConstPtr union1 = fieldCreate->createUnion(names1, fields1);

    testOk1(union1->getNumberFields()==2);
    testOk1(union1->getField("innerA")==fields1[0]);
    testOk1(union1->getField("innerB")==fields1[1]);
    testOk1(union1->getFieldIndex("innerA")==0);
    testOk1(union1->getFieldIndex("innerB")==1);
    testOk1(union1->getField(0)==fields1[0]);
    testOk1(union1->getField(1)==fields1[1]);
    testOk1(union1->getFieldName(0)==names1[0]);
    testOk1(union1->getFieldName(1)==names1[1]);

    testOk1(union1->getField("nonexistent").get()==NULL);

    try {
        FieldConstPtr field(union1->getField(9999).get());
        testFail("union1->getField(9999): missing expected exception");
    } catch (std::out_of_range& e) {
        testPass("union1->getField(9999): caught expected exception: %s", e.what());
    }

    testOk1(union1->getFieldT("innerA")==fields1[0]);
    testOk1(union1->getFieldT("innerB")==fields1[1]);
    testOk1(union1->getFieldT(0)==fields1[0]);
    testOk1(union1->getFieldT(1)==fields1[1]);

    try {
        FieldConstPtr field(union1->getFieldT("nonexistent"));
        testFail("union1->getFieldT('nonexistent'): missing required exception");
    } catch (std::runtime_error& e) {
        testPass("union1->getFieldT('nonexistent'): caught expected exception: %s", e.what());
    }

    try {
        FieldConstPtr field(union1->getFieldT(9999));
        testFail("union1->getFieldT(9999): missing required exception");
    } catch (std::out_of_range& e) {
        testPass("union1->getFieldT(9999): caught expected exception: %s", e.what());
    }

    testOk1(union1->getField<Scalar>("innerA").get()!=NULL);
    testOk1(union1->getField<Structure>("innerA").get()==NULL);
    testOk1(union1->getField<ScalarArray>(1).get()!=NULL);
    testOk1(union1->getField<Structure>(1).get()==NULL);

    testOk1(union1->getFieldT<Scalar>("innerA").get()!=NULL);
    try {
        StructureConstPtr s(union1->getFieldT<Structure>("innerA"));
        testFail("union1->getFieldT<Structure>('innnerA'): missing required exception");
    } catch (std::runtime_error& e) {
        testPass("union1->getFieldT<Structure>('innnerA'): caught expected exception: %s", e.what());
    }

    testOk1(union1->getFieldT<ScalarArray>(1).get()!=NULL);
    try {
        StructureConstPtr s(union1->getFieldT<Structure>(1));
        testFail("union1->getFieldT<Structure>(1): missing required exception");
    } catch (std::runtime_error& e) {
        testPass("union1->getFieldT<Structure>(1): caught expected exception: %s", e.what());
    }

    testOk1(union1->getID() == Union::DEFAULT_ID);

    testOk1(fields1 == union1->getFields()); // vector equality

    StringArray names2(2);
    names2[0] = "outerA";
    names2[1] = "outerB";
    FieldConstPtrArray fields2(2);
    fields2[0] = fieldCreate->createScalar(pvInt);
    fields2[1] = std::tr1::static_pointer_cast<const Field>(union1);

    UnionConstPtr union2 = fieldCreate->createUnion(names2, fields2);

    testOk1(union2->getNumberFields()==2); // not recursive
    testOk1(union2->getField(1)==fields2[1]);

    UnionArrayConstPtr union1arr = fieldCreate->createUnionArray(union1);

    testOk1(union1arr->getUnion()==union1);
    testOk1(union1arr->getID()=="union[]");

    UnionConstPtr variantUnion1 = fieldCreate->createVariantUnion();

    testOk1(variantUnion1->getNumberFields()==0);
    testOk1(variantUnion1->getID() == Union::ANY_ID);

    UnionArrayConstPtr variantUnion1arr = fieldCreate->createVariantUnionArray();

    testOk1(variantUnion1arr->getUnion()==variantUnion1);
    testOk1(variantUnion1arr->getID()=="any[]");

}

static void testBoundedString()
{
    testDiag("testBoundedString");

    BoundedStringConstPtr bs = fieldCreate->createBoundedString(8);

    Type type = bs->getType();
    testOk1(type==scalar);

    ScalarType scalarType = bs->getScalarType();
    testOk1(scalarType==pvString);

    testOk1(bs->getMaximumLength()==8);
}


#define testExcept(EXCEPT, CMD) try{ CMD; testFail( "No exception from: " #CMD); } \
catch(EXCEPT& e) {testPass("Got expected exception from: " #CMD);} \
catch(std::exception& e) {testFail("Got wrong exception %s(%s) from: " #CMD, typeid(e).name(),e.what());} \
catch(...) {testFail("Got unknown execption from: " #CMD);}

static void testError()
{
    testDiag("testError");
    ScalarType invalidtype = (ScalarType)9999;

    testExcept(std::invalid_argument, ScalarTypeFunc::getScalarType("invalidtype"));

    testExcept(std::invalid_argument, ScalarTypeFunc::elementSize(invalidtype));

    testExcept(std::invalid_argument, ScalarTypeFunc::name(invalidtype));

    testOk1(!ScalarTypeFunc::isInteger(invalidtype));
    testOk1(!ScalarTypeFunc::isUInteger(invalidtype));
    testOk1(!ScalarTypeFunc::isNumeric(invalidtype));
    testOk1(!ScalarTypeFunc::isPrimitive(invalidtype));

    testExcept(std::invalid_argument, fieldCreate->createScalar(invalidtype));
    testExcept(std::invalid_argument, fieldCreate->createScalarArray(invalidtype));

    StringArray names;
    FieldConstPtrArray fields(1);

    // fails because names.size()!=fields.size()
    testExcept(std::invalid_argument, fieldCreate->createStructure(names,fields));

    names.resize(1);;

    // fails because names[0].size()==0
    testExcept(std::invalid_argument, fieldCreate->createStructure(names,fields));

    names[0] = "hello";

    // fails because fields[0].get()==NULL
    testExcept(std::invalid_argument, fieldCreate->createStructure(names,fields));

    fields[0] = std::tr1::static_pointer_cast<const Field>(fieldCreate->createScalar(pvDouble));

    testOk1(fieldCreate->createStructure(names,fields).get()!=NULL);
}

static void testMapping()
{
#define OP(TYPE, ENUM) \
    printf(#TYPE ": sizeof %u typeid '%s' ScalarTypeID %d\n",\
        (unsigned)sizeof(TYPE), typeid(TYPE).name(),\
        epics::pvData::ScalarTypeID<TYPE>::value);\
    testOk1(typeid(ScalarTypeTraits<ENUM>::type)==typeid(TYPE)); \
            testOk1(ENUM==(ScalarType)ScalarTypeID<TYPE>::value); \
            testOk1(ENUM==(ScalarType)ScalarTypeID<const TYPE>::value);
    OP(epics::pvData::boolean, pvBoolean)
    OP(int8, pvByte)
    OP(int16, pvShort)
    OP(int32, pvInt)
    OP(int64, pvLong)
    OP(uint8, pvUByte)
    OP(uint16, pvUShort)
    OP(uint32, pvUInt)
    OP(uint64, pvULong)
    OP(float, pvFloat)
    OP(double, pvDouble)
    OP(string, pvString)
#undef OP

}

MAIN(testIntrospect)
{
    testPlan(358);
    fieldCreate = getFieldCreate();
    pvDataCreate = getPVDataCreate();
    standardField = getStandardField();
    testScalar();
    testScalarArray();
    testStructure();
    testUnion();
    testBoundedString();
    testError();
    testMapping();
    return testDone();
}
