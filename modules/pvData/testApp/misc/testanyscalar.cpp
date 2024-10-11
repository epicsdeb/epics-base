
#include <sstream>

#include <pv/pvIntrospect.h>

#include <pv/pvUnitTest.h>
#include <testMain.h>

#include "pv/anyscalar.h"

namespace pvd = epics::pvData;

namespace {

void test_empty()
{
    testDiag("test_empty()");
    pvd::AnyScalar O;
    testOk1(O.empty());
    testOk1(!O);

    testThrows(pvd::AnyScalar::bad_cast, O.ref<double>());
    testThrows(pvd::AnyScalar::bad_cast, O.as<double>());
}

void test_ctor()
{
    testDiag("test_ctor()");
    pvd::AnyScalar A(10),
              B(10.0),
              C("foo"),
              D(std::string("bar"));

    testEqual(A.type(), pvd::pvInt);
    testEqual(B.type(), pvd::pvDouble);
    testEqual(C.type(), pvd::pvString);
    testEqual(D.type(), pvd::pvString);

    testEqual(A.ref<pvd::int32>(), 10);
    testEqual(B.ref<double>(), 10);
    testEqual(C.ref<std::string>(), "foo");
    testEqual(D.ref<std::string>(), "bar");
}

void test_ctor_void()
{
    testDiag("test_ctor_void()");

    pvd::int32 i = 42;
    pvd::AnyScalar A(pvd::pvInt, (void*)&i);

    testEqual(A.type(), pvd::pvInt);
    testEqual(A.ref<pvd::int32>(), 42);

    std::string s("hello");
    pvd::AnyScalar B(pvd::pvString, (void*)&s);

    testEqual(B.type(), pvd::pvString);
    testEqual(B.ref<std::string>(), "hello");
}

void test_basic()
{
    testDiag("test_basic()");
    pvd::AnyScalar I(42);

    testOk1(!I.empty());
    testOk1(!!I);

    testEqual(I.type(), pvd::pvInt);
    testEqual(I.ref<pvd::int32>(), 42);
    testEqual(I.as<pvd::int32>(), 42);
    testEqual(I.as<double>(), 42.0);
    testEqual(I.as<std::string>(), "42");

    const pvd::AnyScalar I2(I);

    testEqual(I2.type(), pvd::pvInt);
    testEqual(I2.ref<pvd::int32>(), 42);
    testEqual(I2.as<pvd::int32>(), 42);
    testEqual(I2.as<double>(), 42.0);
    testEqual(I2.as<std::string>(), "42");

    testThrows(pvd::AnyScalar::bad_cast, I.ref<double>());

    {
        std::ostringstream strm;
        strm<<I;
        testEqual(strm.str(), "42");
    }

    I.ref<pvd::int32>() = 43;

    testEqual(I.ref<pvd::int32>(), 43);
    testEqual(I.as<pvd::int32>(), 43);
    testEqual(I.as<double>(), 43.0);

    I = pvd::AnyScalar("hello");

    testEqual(I.type(), pvd::pvString);
    testEqual(I.ref<std::string>(), "hello");
    testEqual(I.as<std::string>(), "hello");

    testThrows(pvd::AnyScalar::bad_cast, I.ref<pvd::int32>());

    {
        pvd::AnyScalar O(I);
        testOk1(!I.empty());
        testOk1(!O.empty());

        testEqual(I.ref<std::string>(), "hello");
        testEqual(O.ref<std::string>(), "hello");
    }

    {
        pvd::AnyScalar O;
        I.swap(O);
        testOk1(I.empty());
        testOk1(!O.empty());

        testThrows(pvd::AnyScalar::bad_cast, I.ref<std::string>());
        testEqual(O.ref<std::string>(), "hello");

        I.swap(O);
    }
}

void test_swap()
{
    testDiag("test_swap()");

    // pvd::AnyScalar::swap() has 3 cases each for LHS and RHS
    // nil, string, and non-string
    // So we have 9 cases to test

    {
        pvd::AnyScalar A, B;
        A.swap(B);
        testOk1(A.empty());
        testOk1(B.empty());
    }
    {
        pvd::AnyScalar A, B("hello");
        A.swap(B);
        testEqual(A.ref<std::string>(), "hello");
        testOk1(B.empty());
    }
    {
        pvd::AnyScalar A, B(40);
        A.swap(B);
        testEqual(A.ref<pvd::int32>(), 40);
        testOk1(B.empty());
    }

    {
        pvd::AnyScalar A("world"), B;
        A.swap(B);
        testOk1(A.empty());
        testEqual(B.ref<std::string>(), "world");
    }
    {
        pvd::AnyScalar A("world"), B("hello");
        A.swap(B);
        testEqual(A.ref<std::string>(), "hello");
        testEqual(B.ref<std::string>(), "world");
    }
    {
        pvd::AnyScalar A("world"), B(40);
        A.swap(B);
        testEqual(A.ref<pvd::int32>(), 40);
        testEqual(B.ref<std::string>(), "world");
    }

    {
        pvd::AnyScalar A(39), B;
        A.swap(B);
        testOk1(A.empty());
        testEqual(B.ref<pvd::int32>(), 39);
    }
    {
        pvd::AnyScalar A(39), B("hello");
        A.swap(B);
        testEqual(A.ref<std::string>(), "hello");
        testEqual(B.ref<pvd::int32>(), 39);
    }
    {
        pvd::AnyScalar A(39), B(40);
        A.swap(B);
        testEqual(A.ref<pvd::int32>(), 40);
        testEqual(B.ref<pvd::int32>(), 39);
    }
}

void test_move()
{
    testDiag("test_move()");
#if __cplusplus>=201103L
    {
        pvd::AnyScalar x, y(std::move(x));
        testOk1(x.empty());
        testOk1(y.empty());
    }
    {
        pvd::AnyScalar x(5), y(std::move(x));
        testOk1(x.empty());
        testEqual(y.ref<pvd::int32>(), 5);
    }
    {
        pvd::AnyScalar x("hello"), y(std::move(x));
        testOk1(x.empty());
        testEqual(y.ref<std::string>(), "hello");
    }

    {
        pvd::AnyScalar x, y;
        y = std::move(x);
        testOk1(x.empty());
        testOk1(y.empty());
    }
    {
        pvd::AnyScalar x, y(5);
        y = std::move(x);
        testOk1(x.empty());
        testOk1(y.empty());
    }
    {
        pvd::AnyScalar x, y("test");
        y = std::move(x);
        testOk1(x.empty());
        testOk1(y.empty());
    }
#else
    testSkip(12, "No c++11");
#endif
}

} // namespace

MAIN(testanyscalar)
{
    testPlan(75);
    try {
        test_empty();
        test_ctor();
        test_ctor_void();
        test_basic();
        test_swap();
        test_move();
    }catch(std::exception& e){
        testAbort("Unexpected exception: %s", e.what());
    }
    return testDone();
}
