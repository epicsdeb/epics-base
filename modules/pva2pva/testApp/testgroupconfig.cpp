#include <iostream>

#include <pv/pvUnitTest.h>
#include <testMain.h>

#include "pdbgroup.h"

namespace pvd = epics::pvData;

namespace {

void test_parse()
{
    testDiag("test_parse()");

    const char txt[] =
    "{\"grpa\":{\n"
    "  \"+atomic\":false,\n"
    "  \"fld\":{\n"
    "    \"+type\": \"simple\","
    "    \"+channel\": \"VAL\","
    "    \"+putorder\": -4"
    "  },\n"
    "  \"\":{\n"
    "    \"+type\": \"top\""
    "  }\n"
    " },\n"
    " \"grpb\":{\n"
    " }\n"
    "}";

    GroupConfig conf;
    GroupConfig::parse(txt, "rec", conf);

    testOk(conf.warning.empty(), "Warnings: %s", conf.warning.c_str());

    testOk1(!conf.groups["grpa"].atomic);
    testOk1(conf.groups["grpa"].atomic_set);

    testEqual(conf.groups["grpa"].fields["fld"].type, "simple");
    testEqual(conf.groups["grpa"].fields["fld"].channel, "rec.VAL");
    testEqual(conf.groups["grpa"].fields["fld"].putorder, -4);

    testEqual(conf.groups["grpa"].fields[""].type, "top");
}

void test_fail()
{
    testDiag("test_fail()");

    {
        GroupConfig conf;
        testThrows(std::runtime_error, GroupConfig::parse("{", "", conf));
    }
    {
        GroupConfig conf;
        testThrows(std::runtime_error, GroupConfig::parse("{\"G\":{\"F\":{\"K\":{}}}}", "", conf));
    }
    {
        GroupConfig conf;
        testThrows(std::runtime_error, GroupConfig::parse("{\"G\":5}", "", conf));
    }
}

} // namespace

MAIN(testgroupconfig)
{
    testPlan(10);
    try {
        test_parse();
        test_fail();
    }catch(std::exception& e){
        testAbort("Unexpected exception: %s", e.what());
    }
    return testDone();
}
