/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Michael Davidsaver */

#include <fstream>
#include <iostream>
#include <algorithm>
#include <limits>
#include <typeinfo>
#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <float.h>
#include <epicsMath.h>

#include <epicsUnitTest.h>
#include <testMain.h>

#include "pv/typeCast.h"

using std::string;

namespace {
    // Mangle value to be printable as per print_convolute<>
    template<typename T>
    inline
    typename epics::pvData::detail::print_convolute<T>::return_t
    print(T v) {
        return epics::pvData::detail::print_convolute<T>::op(v);
    }

    template<typename T>
    struct testequal {
        static bool op(T A, T B) {return A==B; }
    };
    template<>
    struct testequal<double> {
        static bool op(double A, double B) {return fabs(A-B)<1e-15; }
    };
    template<>
    struct testequal<float> {
        static bool op(float A, float B) {return fabs(A-B)<1e-7; }
    };

    template<typename TO, typename FROM>
    struct testcase {
        static void op(TO expect, FROM inp)
        {
            std::ostringstream msg;
            TO actual;
            try {
                actual = ::epics::pvData::castUnsafe<TO,FROM>(inp);
                //actual = ::epics::pvData::detail::cast_helper<TO,FROM>::op(inp);
            } catch(std::runtime_error& e) {
                msg<<"Failed to cast "
                   <<print(inp)<<" ("<<typeid(FROM).name()<<") -> "
                   <<print(expect)<<" ("<<typeid(TO).name()<<")\n Error: "
                   <<typeid(e).name()<<"("<<e.what()<<")";
                testFail("%s", msg.str().c_str());
                return;
            }
            if(!testequal<TO>::op(actual, expect)) {
                msg<<"Failed cast gives unexpected value "
                   <<print(inp)<<" ("<<typeid(FROM).name()<<") -> "
                   <<print(expect)<<" ("<<typeid(TO).name()<<") yields: "
                   <<print(actual);
                testFail("%s", msg.str().c_str());
                return;
            }
            msg<<"cast "
               <<print(inp)<<" ("<<typeid(FROM).name()<<") -> "
               <<print(expect)<<" ("<<typeid(TO).name()<<") yields: "
               <<print(actual);
            testPass("%s", msg.str().c_str());
            return;
        }
    };

    template<typename TO, typename FROM>
    struct testfail {
        static void op(FROM inp)
        {
            std::ostringstream msg;
            TO actual;
            try {
                actual = ::epics::pvData::castUnsafe<TO,FROM>(inp);
                msg<<"Failed to generate expected error "
                   <<print(inp)<<" ("<<typeid(FROM).name()<<") -> ("
                   <<typeid(TO).name()<<") yields: "
                   <<actual;
                testFail("%s", msg.str().c_str());
                return;
            } catch(std::runtime_error& e) {
                msg<<"Got expected error "
                   <<print(inp)<<" ("<<typeid(FROM).name()<<") -> ("
                   <<typeid(TO).name()<<") fails with: "
                   <<e.what();
                testPass("%s", msg.str().c_str());
                return;
            }
        }
    };


// Test cast
#define TEST(TTO, VTO, TFRO, VFRO) testcase<TTO, TFRO>::op(VTO, VFRO)

// Test cast and reverse
#define TEST2(TTO, VTO, TFRO, VFRO) TEST(TTO, VTO, TFRO, VFRO); TEST(TFRO, VFRO, TTO, VTO)

#define FAIL(TTO, TFRO, VFRO) testfail<TTO,TFRO>::op(VFRO)

} // end namespace


MAIN(testTypeCast)
{
    testPlan(124);

try {

    int8_t xint8=0;
    uint8_t xuint8=0;
    int16_t xint16=0;
    uint16_t xuint16=0;
    int32_t xint32=0;
    uint32_t xuint32=0;
    int64_t xint64=0;
    uint64_t xuint64=0;
    float xfloat=0.0;
    double xdouble=0.0;
    string xstring("0");
    const char* xcstring = "0";

    typedef float float_t;
    typedef double double_t;
    typedef string string_t;
    typedef const char* cstring_t;

    // force all possibilities to be compiled
#define CHECK(M, N) x## M = ::epics::pvData::castUnsafe<M ##_t>(x## N); \
    std::transform(&x ## N, &x ## N+1, &x ## M, ::epics::pvData::castUnsafe<M ##_t,N ##_t>)
//#define CHECK(M, N) x## M = ::epics::pvData::detail::cast_helper<M ##_t,N ##_t>::op(x## N)
    CHECK(int8, int8);
    CHECK(int8, uint8);
    CHECK(int8, int16);
    CHECK(int8, uint16);
    CHECK(int8, int32);
    CHECK(int8, uint32);
    CHECK(int8, int64);
    CHECK(int8, uint64);
    CHECK(int8, float);
    CHECK(int8, double);
    CHECK(int8, string);
    CHECK(int8, cstring);

    CHECK(uint8, int8);
    CHECK(uint8, uint8);
    CHECK(uint8, int16);
    CHECK(uint8, uint16);
    CHECK(uint8, int32);
    CHECK(uint8, uint32);
    CHECK(uint8, int64);
    CHECK(uint8, uint64);
    CHECK(uint8, float);
    CHECK(uint8, double);
    CHECK(uint8, string);
    CHECK(uint8, cstring);

    CHECK(int16, int8);
    CHECK(int16, uint8);
    CHECK(int16, int16);
    CHECK(int16, uint16);
    CHECK(int16, int32);
    CHECK(int16, uint32);
    CHECK(int16, int64);
    CHECK(int16, uint64);
    CHECK(int16, float);
    CHECK(int16, double);
    CHECK(int16, string);
    CHECK(int16, cstring);

    CHECK(uint16, int8);
    CHECK(uint16, uint8);
    CHECK(uint16, int16);
    CHECK(uint16, uint16);
    CHECK(uint16, int32);
    CHECK(uint16, uint32);
    CHECK(uint16, int64);
    CHECK(uint16, uint64);
    CHECK(uint16, float);
    CHECK(uint16, double);
    CHECK(uint16, string);
    CHECK(uint16, cstring);

    CHECK(int32, int8);
    CHECK(int32, uint8);
    CHECK(int32, int16);
    CHECK(int32, uint16);
    CHECK(int32, int32);
    CHECK(int32, uint32);
    CHECK(int32, int64);
    CHECK(int32, uint64);
    CHECK(int32, float);
    CHECK(int32, double);
    CHECK(int32, string);
    CHECK(int32, cstring);

    CHECK(uint32, int8);
    CHECK(uint32, uint8);
    CHECK(uint32, int16);
    CHECK(uint32, uint16);
    CHECK(uint32, int32);
    CHECK(uint32, uint32);
    CHECK(uint32, int64);
    CHECK(uint32, uint64);
    CHECK(uint32, float);
    CHECK(uint32, double);
    CHECK(uint32, string);
    CHECK(uint32, cstring);

    CHECK(int64, int8);
    CHECK(int64, uint8);
    CHECK(int64, int16);
    CHECK(int64, uint16);
    CHECK(int64, int32);
    CHECK(int64, uint32);
    CHECK(int64, int64);
    CHECK(int64, uint64);
    CHECK(int64, float);
    CHECK(int64, double);
    CHECK(int64, string);
    CHECK(int64, cstring);

    CHECK(uint64, int8);
    CHECK(uint64, uint8);
    CHECK(uint64, int16);
    CHECK(uint64, uint16);
    CHECK(uint64, int32);
    CHECK(uint64, uint32);
    CHECK(uint64, int64);
    CHECK(uint64, uint64);
    CHECK(uint64, float);
    CHECK(uint64, double);
    CHECK(uint64, string);
    CHECK(uint64, cstring);

    CHECK(float, int8);
    CHECK(float, uint8);
    CHECK(float, int16);
    CHECK(float, uint16);
    CHECK(float, int32);
    CHECK(float, uint32);
    CHECK(float, int64);
    CHECK(float, uint64);
    CHECK(float, float);
    CHECK(float, double);
    CHECK(float, string);
    CHECK(float, cstring);

    CHECK(double, int8);
    CHECK(double, uint8);
    CHECK(double, int16);
    CHECK(double, uint16);
    CHECK(double, int32);
    CHECK(double, uint32);
    CHECK(double, int64);
    CHECK(double, uint64);
    CHECK(double, float);
    CHECK(double, double);
    CHECK(double, string);
    CHECK(double, cstring);

    CHECK(string, int8);
    CHECK(string, uint8);
    CHECK(string, int16);
    CHECK(string, uint16);
    CHECK(string, int32);
    CHECK(string, uint32);
    CHECK(string, int64);
    CHECK(string, uint64);
    CHECK(string, float);
    CHECK(string, double);
    CHECK(string, string);
    CHECK(string, cstring);

    // cast to const char* not supported
#undef CHECK

    testDiag("Integer signed <=> unsigned");

    TEST2(uint8_t,  std::numeric_limits<uint8_t>::max(),  int8_t, -1);
    TEST2(uint16_t, std::numeric_limits<uint16_t>::max(), int8_t, -1);
    TEST2(uint32_t, std::numeric_limits<uint32_t>::max(), int8_t, -1);
    TEST2(uint64_t, std::numeric_limits<uint64_t>::max(), int8_t, -1);

    testDiag("Integer unsigned promote (and demote when in range)");

    TEST2(uint16_t, 0xff, uint8_t, 0xff);
    TEST2(uint32_t, 0xffff, uint16_t, 0xffff);
    TEST2(uint64_t, 0xffffffffu, uint32_t, 0xffffffffu);

    TEST2(int16_t, 0x7f, int8_t, 0x7f);
    TEST2(int32_t, 0x7fff, int16_t, 0x7fff);
    TEST2(int64_t, 0x7fffffff, int32_t, 0x7fffffff);

    testDiag("Double to int w/ round towards zero (aka truncation)");

    TEST(int32_t, 2, double, 2.1);
    TEST(int32_t, 2, double, 2.5);
    TEST(int32_t, 2, double, 2.7);
    TEST(int32_t, -2, double, -2.1);
    TEST(int32_t, -2, double, -2.5);
    TEST(int32_t, -2, double, -2.7);

    testDiag("Float to int w/ round towards zero (aka truncation)");

    TEST(int32_t, 2, float, 2.1f);
    TEST(int32_t, 2, float, 2.5f);
    TEST(int32_t, 2, float, 2.7f);
    TEST(int32_t, -2, float, -2.1f);
    TEST(int32_t, -2, float, -2.5f);
    TEST(int32_t, -2, float, -2.7f);

    testDiag("string Printing/parsing");

    TEST2(string, "1", int32_t, 1);
    TEST2(string, "-1", int32_t, -1);
    TEST2(string, "1", int8_t, 1);
    TEST2(string, "-1", int8_t, -1);
    TEST2(string, "1", uint8_t, 1);

    TEST2(string, "127", int32_t, std::numeric_limits<int8_t>::max());
    TEST2(string, "-128", int32_t, std::numeric_limits<int8_t>::min());
    TEST2(string, "255", int32_t, std::numeric_limits<uint8_t>::max());

    TEST2(string, "32767", int32_t, std::numeric_limits<int16_t>::max());
    TEST2(string, "-32768", int32_t, std::numeric_limits<int16_t>::min());
    TEST2(string, "65535", int32_t, std::numeric_limits<uint16_t>::max());

    TEST2(string, "2147483647", int32_t, std::numeric_limits<int32_t>::max());
    TEST2(string, "-2147483648", int32_t, std::numeric_limits<int32_t>::min());
    TEST2(string, "4294967295", uint32_t, std::numeric_limits<uint32_t>::max());

    TEST2(string, "9223372036854775807", int64_t, std::numeric_limits<int64_t>::max());
    TEST2(string, "-9223372036854775808", int64_t, std::numeric_limits<int64_t>::min());
    TEST2(string, "18446744073709551615", uint64_t, std::numeric_limits<uint64_t>::max());

    TEST2(string, "1.1", double, 1.1);
    TEST2(string, "1.1e+100", double, 1.1e100);
    TEST2(string, "1.1e-100", double, 1.1e-100);

    TEST(double, 1.1e100, string, "1.1E+100");
    TEST(double, 1.1e100, const char*, "1.1E+100");

    // any non-zero value is true
    TEST(string, "true", epics::pvData::boolean, 100);

    TEST2(string, "true", epics::pvData::boolean, 1);
    TEST2(string, "false", epics::pvData::boolean, 0);

    // Case insensitive
    TEST(epics::pvData::boolean, 1, string, "True");
    TEST(epics::pvData::boolean, 0, string, "False");
    TEST(epics::pvData::boolean, 1, string, "TRUE");
    TEST(epics::pvData::boolean, 0, string, "FALSE");

    testDiag("string Parsing");

    TEST(int32_t, 15, string, "0xf");
    TEST(int32_t, 15, string, "0xF");
    TEST(int32_t, -15, string, "-0xF");
    TEST(int32_t, 16, string, "0x10");
    TEST(int32_t, -16, string, "-0x10");

    TEST(int32_t, 7, string, "07");
    TEST(int32_t, 8, string, "010");
    TEST(int32_t, -7, string, "-07");
    TEST(int32_t, -8, string, "-010");

    TEST(int64_t, 15, string, "0xf");
    TEST(int64_t, 15, string, "0xF");
    TEST(int64_t, -15, string, "-0xF");
    TEST(int64_t, 16, string, "0x10");
    TEST(int64_t, -16, string, "-0x10");

    TEST(int64_t, 7, string, "07");
    TEST(int64_t, 8, string, "010");
    TEST(int64_t, -7, string, "-07");
    TEST(int64_t, -8, string, "-010");

    testDiag("string parsing errors");

    FAIL(int32_t, string, "hello!");
    FAIL(int32_t, string, "42 is the answer");
    FAIL(int64_t, string, "hello!");
    FAIL(int64_t, string, "42 is the answer");
    FAIL(double, string, "hello!");
    FAIL(double, string, "42 is the answer");

    FAIL(int8_t, string, "1000");
    FAIL(int8_t, string, "-1000");

    FAIL(double, string, "1e+1000");
    FAIL(double, string, "-1e+1000");

    FAIL(epics::pvData::boolean, string, "hello");
    FAIL(epics::pvData::boolean, string, "1");
    FAIL(epics::pvData::boolean, string, "0");
    FAIL(epics::pvData::boolean, string, "T");
    FAIL(epics::pvData::boolean, string, "F");

    testDiag("Floating point overflows");

    TEST(float, FLT_MAX, double, 1e300);
    TEST(float, -FLT_MAX, double, -1e300);
    TEST(float, FLT_MIN, double, 1e-300);
    TEST(float, -FLT_MIN, double, -1e-300);

    xfloat = ::epics::pvData::castUnsafe<float,double>(epicsNAN);
    testOk(isnan( xfloat ), "Test cast double NAN to float NAN yields: %f", xfloat);

    {
        string result[3];
        const int32_t in[3] = { 1234, 506001, 42424242 };

        testDiag("Test vcast int32 -> string");
        epics::pvData::castUnsafeV(3, epics::pvData::pvString, (void*)result,
                                   epics::pvData::pvInt, (void*)in);
        testDiag("Yields %s %s %s", result[0].c_str(), result[1].c_str(), result[2].c_str());

        testOk1(result[0]=="1234");
        testOk1(result[1]=="506001");
        testOk1(result[2]=="42424242");
    }

} catch(std::exception& e) {
    testAbort("Uncaught exception: %s", e.what());
}

    return testDone();
}
