/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <stdexcept>

#include <testMain.h>

#include <pv/pvUnitTest.h>
#include <pv/valueBuilder.h>

namespace pvd = epics::pvData;

namespace {
struct MyTest {
    MyTest() { testPass("MyTest::MyTest"); }
    ~MyTest() { testPass("MyTest::~MyTest"); }
    void test1() { testPass("test1"); }
    void test2() { testPass("test2"); }
};
}//namespace

MAIN(testUnitTest)
{
    testPlan(10);
    try {
        TEST_METHOD(MyTest, test1);
        TEST_METHOD(MyTest, test2);

        {
            std::string x("hello");
            testEqual(x, "hello");
            testEqual(x, "hello")<<" Extra";
        }

        pvd::PVStructurePtr S(pvd::ValueBuilder()
                              .addNested("alarm")
                                .add<pvd::pvInt>("severity", 1)
                                .add<pvd::pvString>("message", "hello")
                              .endNested()
                              .buildPVStructure());

        testFieldEqual<pvd::PVInt>(S, "alarm.severity", 1);
        testFieldEqual<pvd::PVString>(S, "alarm.message", "hello")<<" More";

    }catch(std::exception& e){
        testAbort("Unhandled exception: %s", e.what());
    }
    return testDone();
}
