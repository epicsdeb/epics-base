/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef PVUNITTEST_H
#define PVUNITTEST_H

#include <sstream>
#include <typeinfo>

#include <epicsUnitTest.h>

#include <pv/sharedPtr.h>
#include <pv/epicsException.h>
#include <pv/pvData.h>

namespace detail {

template<class C, void (C::*M)()>
void test_method(const char *kname, const char *mname)
{
    try {
        testDiag("------- %s::%s --------", kname, mname);
        C inst;
        (inst.*M)();
    } catch(std::exception& e) {
        PRINT_EXCEPTION(e);
        testAbort("unexpected exception: %s", e.what());
    }
}

class epicsShareClass testPassx
{
    std::ostringstream strm;
    const bool dotest, pass;
    bool alive;
public:
    testPassx() :dotest(false), pass(false), alive(true) {}
    explicit testPassx(bool r) :dotest(true), pass(r), alive(true) {}
    ~testPassx();
    template<typename T>
    inline testPassx& operator<<(const T& v) {
        strm<<v;
        return *this;
    }

    // allow testPassx to be returned
    // move ctor masquerading as copy ctor
    testPassx(testPassx& o);
private:
    testPassx& operator=(const testPassx&);
};

template<typename LHS, typename RHS>
inline testPassx testEqualx(const char *nLHS, const char *nRHS, const LHS& l, const RHS& r)
{
    return testPassx(l==r)<<nLHS<<" ("<<l<<") == "<<nRHS<<" ("<<r<<")";
}

template<typename LHS, typename RHS>
inline testPassx testNotEqualx(const char *nLHS, const char *nRHS, const LHS& l, const RHS& r)
{
    return testPassx(l!=r)<<nLHS<<" ("<<l<<") != "<<nRHS<<" ("<<r<<")";
}

}//namespace detail

/** @defgroup testhelpers Unit testing helpers
 *
 * Helper functions for writing unit tests.
 *
 @include unittest.cpp
 *
 * @{
 */

/** Run a class method as a test.
 *
 * Each invocation of TEST_METHOD() constructs a new instance of 'klass' on the stack.
 * Thus constructor and destructor can be used for common test setup and tear down.
 @code
 namespace { // anon
     struct MyTest {
           MyTest() { } // setup
           ~MyTest() { } // tear down
           void test1() {}
           void test2() {}
     };
 } // namespace anon
 MAIN(somename) {
       testPlan(0);
       TEST_METHOD(MyTest, test1)
       TEST_METHOD(MyTest, test2)
       return testDone();
 }
 @endcode
 */
#define TEST_METHOD(klass, method) ::detail::test_method<klass, &klass::method>(#klass, #method)

/** Compare equality.  print left and right hand values and expression strings
 *
 @code
 int x=5;
 testEqual(x, 5);
 // prints "ok 1 - x (5) == 5 (5)\n"
 testEqual(x, 6)<<" oops";
 // prints "not ok 1 - x (5) == 6 (6) oops\n"
 @endcode
 */
#define testEqual(LHS, RHS) ::detail::testEqualx(#LHS, #RHS, LHS, RHS)

#define testNotEqual(LHS, RHS) ::detail::testNotEqualx(#LHS, #RHS, LHS, RHS)

/** Pass/fail from boolean
 *
 @code
 bool y=true;
 testTrue(y);
 // prints "ok 1 - y\n"
 testTrue(!y)<<" oops";
 // prints "not ok 1 - !y oops\n"
 @endcode
 */
#define testTrue(B) ::detail::testPassx(!!(B))<<#B

/** Test that a given block throws an exception
 *
 @code
 testThrows(std::runtime_error, somefunc(5))
 @endcode
 */
#define testThrows(EXC, CODE) try{ CODE; testFail("unexpected success of " #CODE); }catch(EXC& e){testPass("catch expected exception: %s", e.what());}

/** Print test output w/o testing
 *
 @code
 testShow()<<"Foo";
 @endcode
 */
#define testShow() ::detail::testPassx()

/** Compare value of PVStructure field
 *
 @code
 PVStructurePtr x(.....);
 testFieldEqual<epics::pvData::PVInt>(x, "alarm.severity", 1);
 @endcode
 */
template<typename PVD>
::detail::testPassx
testFieldEqual(const std::tr1::shared_ptr<const epics::pvData::PVStructure>& val, const char *name, typename PVD::value_type expect)
{
    if(!val) {
        return ::detail::testPassx(false)<<" null structure pointer";
    }
    typename PVD::const_shared_pointer fval(val->getSubField<PVD>(name));
    if(!fval) {
        epics::pvData::PVUnion::const_shared_pointer uval(val->getSubField<epics::pvData::PVUnion>(name));
        if(uval)
            fval = uval->get<PVD>();
    }
    if(!fval) {
        return ::detail::testPassx(false)<<" field '"<<name<<"' with type "<<typeid(PVD).name()<<" does not exist";
    } else {
        typename PVD::value_type actual(fval->get());
        return ::detail::testPassx(actual==expect)<<name<<" ("<<actual<<") == "<<expect;
    }
}

template<typename PVD>
::detail::testPassx
testFieldEqual(const std::tr1::shared_ptr<const epics::pvData::PVStructure>& val, const char *name, typename PVD::const_svector expect)
{
    if(!val) {
        return ::detail::testPassx(false)<<" null structure pointer";
    }
    typename PVD::const_shared_pointer fval(val->getSubField<PVD>(name));
    if(!fval) {
        return ::detail::testPassx(false)<<" field '"<<name<<"' with type "<<typeid(PVD).name()<<" does not exist";
    } else {
        typename PVD::const_svector actual(fval->view());
        return ::detail::testPassx(actual==expect)<<name<<" ("<<actual<<") == "<<expect;
    }
}

/** @} */

#endif // PVUNITTEST_H
