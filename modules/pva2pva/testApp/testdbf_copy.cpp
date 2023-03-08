#include <stdexcept>

#include <pv/pvUnitTest.h>
#include <testMain.h>

#include <dbStaticLib.h>
#include <epicsTypes.h>
#include <pv/valueBuilder.h>
#include <pv/pvData.h>


#include "pvif.h"

namespace pvd = epics::pvData;

namespace {
template<pvd::ScalarType ENUM, typename DBF, typename E>
void testPVD2DBR_scalar(unsigned dbf, typename pvd::meta::arg_type<typename pvd::ScalarTypeTraits<ENUM>::type>::type V, E expect)
{
    testDiag("testPVD2DBR_scalar(%s, %s)", pvd::ScalarTypeFunc::name(ENUM), dbGetFieldTypeString(dbf));

    pvd::PVStructure::shared_pointer top(pvd::ValueBuilder()
                                         .add<ENUM>("value", V)
                                         .buildPVStructure());

    DBF buf;

    copyPVD2DBF(top->getSubFieldT("value"), &buf, dbf, NULL);

    testEqual(buf, expect);
}

void testPVD2DBR_enum()
{
    testDiag("testPVD2DBR_enum()");

    pvd::shared_vector<std::string> choices(3);
    choices[0] = "zero";
    choices[1] = "one";
    choices[2] = "two";

    pvd::PVStructure::shared_pointer top(pvd::ValueBuilder()
                                         .addNested("value")
                                             .add<pvd::pvInt>("index", 1)
                                             .add("choices", pvd::static_shared_vector_cast<const void>(pvd::freeze(choices)))
                                         .endNested()
                                         .buildPVStructure());

    {
        epicsEnum16 ival;

        copyPVD2DBF(top->getSubFieldT("value"), &ival, DBF_ENUM, NULL);
        testEqual(ival, 1);

        ival = 0;

        testOk1(!!top->getSubField("value"));
        copyPVD2DBF(top->getSubFieldT("value"), &ival, DBF_USHORT, NULL);
        testEqual(ival, 1);
    }

    {
        epicsUInt32 ival;

        copyPVD2DBF(top->getSubFieldT("value"), &ival, DBF_LONG, NULL);
        testEqual(ival, 1u);
    }

    char sval[MAX_STRING_SIZE];

    copyPVD2DBF(top->getSubFieldT("value"), sval, DBF_STRING, NULL);
    testEqual(std::string(sval) , "one");
}

void testPVD2DBR_array()
{
    testDiag("testPVD2DBR_array()");

    pvd::shared_vector<const pvd::uint32> arr;
    {
        pvd::shared_vector<pvd::uint32> iarr(3);
        iarr[0] = 1; iarr[1] = 2; iarr[2] = 3;
        arr = pvd::freeze(iarr);
    }

    pvd::PVStructure::shared_pointer top(pvd::ValueBuilder()
                                         .add("value", pvd::static_shared_vector_cast<const void>(arr))
                                         .buildPVStructure());

    {
        epicsUInt16 sarr[5];

        {
            long nreq = 5;
            copyPVD2DBF(top->getSubFieldT("value"), sarr, DBF_SHORT, &nreq);
            testEqual(nreq, 3);
        }

        testEqual(sarr[0], arr[0]);
        testEqual(sarr[1], arr[1]);
        testEqual(sarr[2], arr[2]);
    }

    {
        char sarr[MAX_STRING_SIZE*5];

        {
            long nreq = 5;
            copyPVD2DBF(top->getSubFieldT("value"), sarr, DBF_STRING, &nreq);
            testEqual(nreq, 3);
        }

        testEqual(sarr[0*MAX_STRING_SIZE+0], '1');
        testEqual(int(sarr[0*MAX_STRING_SIZE+1]), int('\0'));
        testEqual(sarr[1*MAX_STRING_SIZE+0], '2');
        testEqual(int(sarr[1*MAX_STRING_SIZE+1]), int('\0'));
        testEqual(sarr[2*MAX_STRING_SIZE+0], '3');
        testEqual(int(sarr[2*MAX_STRING_SIZE+1]), int('\0'));
    }
}

template<typename input_t, typename output_t>
void testDBR2PVD_scalar(const input_t& input,
                        const output_t& expect)
{
    pvd::ScalarType  in = (pvd::ScalarType)pvd::ScalarTypeID<input_t>::value;
    pvd::ScalarType out = (pvd::ScalarType)pvd::ScalarTypeID<output_t>::value;
    testDiag("testDBR2PVD_scalar(%s, %s)", pvd::ScalarTypeFunc::name(in), pvd::ScalarTypeFunc::name(out));

    pvd::PVStructure::shared_pointer top(pvd::getPVDataCreate()->createPVStructure(pvd::getFieldCreate()->createFieldBuilder()
                                                                                   ->add("value", out) // initially zero or ""
                                                                                   ->createStructure()));
    pvd::PVStringArray::const_svector choices;
    pvd::BitSet changed;

    pvd::shared_vector<input_t> buf(1);
    buf[0] = input;

    copyDBF2PVD(pvd::static_shared_vector_cast<const void>(pvd::freeze(buf)),
                top->getSubFieldT("value"), changed, choices);

    output_t actual = top->getSubFieldT<pvd::PVScalar>("value")->getAs<output_t>();
    testEqual(actual, expect);
    testOk1(changed.get(top->getSubFieldT("value")->getFieldOffset()));
}

template<typename input_t>
void testDBR2PVD_enum(const input_t& input, pvd::int32 expect)
{
    testDiag("testDBR2PVD_enum()");

    pvd::shared_vector<const std::string> choices;
    {
        pvd::shared_vector<std::string> temp(3);
        temp[0] = "zero";
        temp[1] = "one";
        temp[2] = "two";
        choices = pvd::freeze(temp);
    }

    pvd::PVStructure::shared_pointer top(pvd::ValueBuilder()
                                         .addNested("value")
                                             .add<pvd::pvInt>("index", 0)
                                             .add("choices", pvd::static_shared_vector_cast<const void>(choices))
                                         .endNested()
                                         .buildPVStructure());

    pvd::BitSet changed;

    pvd::shared_vector<input_t> buf(1);
    buf[0] = input;

    copyDBF2PVD(pvd::static_shared_vector_cast<const void>(pvd::freeze(buf)),
                top->getSubFieldT("value"), changed, choices);

    pvd::int32 actual = top->getSubFieldT<pvd::PVScalar>("value.index")->getAs<pvd::int32>();
    testShow()<<top;
    testShow()<<changed;
    testEqual(actual, expect);
    testOk1(changed.get(top->getSubFieldT("value.index")->getFieldOffset()));
}

void testDBR2PVD_array()
{
    testDiag("testDBR2PVD_array()");

    pvd::PVStructure::shared_pointer top(pvd::getPVDataCreate()->createPVStructure(pvd::getFieldCreate()->createFieldBuilder()
                                                                                   ->addArray("value", pvd::pvInt) // initially zero or ""
                                                                                   ->createStructure()));
    pvd::PVStringArray::const_svector choices;
    pvd::BitSet changed;

    {
        pvd::shared_vector<pvd::uint32> buf(3);
        buf[0] = 1; buf[1] = 2; buf[2] = 3;
        copyDBF2PVD(pvd::static_shared_vector_cast<const void>(pvd::freeze(buf)),
                    top->getSubFieldT("value"), changed, choices);

        pvd::PVIntArray::const_svector arr(top->getSubFieldT<pvd::PVIntArray>("value")->view());
        testEqual(arr.size(), 3u);
        testEqual(arr[0], 1);
        testEqual(arr[1], 2);
        testEqual(arr[2], 3);
        testOk1(changed.get(top->getSubFieldT("value")->getFieldOffset()));
    }

    changed.clear();

    {
        pvd::shared_vector<std::string> buf(4);
        buf[0] = "4";
        buf[1] = "5";
        buf[2] = "6";
        buf[3] = "7";

        copyDBF2PVD(pvd::static_shared_vector_cast<const void>(pvd::freeze(buf)),
                    top->getSubFieldT("value"), changed, choices);

        pvd::PVIntArray::const_svector arr(top->getSubFieldT<pvd::PVIntArray>("value")->view());
        testEqual(arr.size(), 4u);
        testEqual(arr[0], 4);
        testEqual(arr[1], 5);
        testEqual(arr[2], 6);
        testEqual(arr[3], 7);
        testOk1(changed.get(top->getSubFieldT("value")->getFieldOffset()));
    }
}

}

MAIN(testdbf_copy)
{
    testPlan(53);
    try{
        testPVD2DBR_scalar<pvd::pvDouble, double>(DBF_DOUBLE, 42.2, 42.2);
        testPVD2DBR_scalar<pvd::pvDouble, pvd::uint16>(DBF_USHORT, 42.2, 42u);

        testPVD2DBR_scalar<pvd::pvInt, pvd::int32>(DBF_LONG, 42, 42);
        testPVD2DBR_scalar<pvd::pvInt, char[MAX_STRING_SIZE]>(DBF_STRING, 42, std::string("42"));

        testPVD2DBR_scalar<pvd::pvLong, pvd::int64>(DBF_INT64, 42, 42);
        testPVD2DBR_scalar<pvd::pvLong, char[MAX_STRING_SIZE]>(DBF_STRING, 42, std::string("42"));

        testPVD2DBR_scalar<pvd::pvUShort, pvd::uint16>(DBF_USHORT, 41u, 41);
        testPVD2DBR_scalar<pvd::pvByte, pvd::int8>(DBF_CHAR, 41, 41);

        testPVD2DBR_scalar<pvd::pvString, char[MAX_STRING_SIZE]>(DBF_STRING, "hello", std::string("hello"));
        testPVD2DBR_scalar<pvd::pvString, pvd::int32>(DBF_LONG, "-100", -100);

        //testPVD2DBR_scalar<pvd::pvBoolean, pvd::int8>(DBF_CHAR, true, 1);

        testPVD2DBR_enum();

        testPVD2DBR_array();

        testDBR2PVD_scalar<double, double>(42.2, 42.2);
        testDBR2PVD_scalar<pvd::uint16, double>(42u, 42.0);
        testDBR2PVD_scalar<pvd::int32, pvd::int32>(-41, -41);
        testDBR2PVD_scalar<std::string, pvd::int32>("-41", -41);
        testDBR2PVD_scalar<std::string, std::string>("hello", "hello");
        testDBR2PVD_scalar<pvd::int32, std::string>(-42, "-42");

        testDBR2PVD_enum<pvd::uint32>(2, 2);
        testDBR2PVD_enum<std::string>("two", 2);

        testDBR2PVD_array();
    }catch(std::exception& e){
        testFail("Unexpected exception: %s", e.what());
    }
    return testDone();
}
