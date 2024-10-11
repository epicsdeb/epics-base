/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */


#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/valueBuilder.h>
#include <pv/pvData.h>
#include <pv/createRequest.h>
#include <pv/pvUnitTest.h>

namespace pvd = epics::pvData;

namespace {

void testBuild()
{
    testDiag("testBuild()");
    pvd::ValueBuilder builder;
    builder.add<pvd::pvInt>("A", 42)
           .add<pvd::pvShort>("B", 43);

    pvd::PVStructurePtr val(builder.buildPVStructure());

    testOk1(val->getStructure()->getID()=="structure");
    {
        pvd::PVIntPtr A(val->getSubField<pvd::PVInt>("A"));
        testOk1(!!A && A->get()==42);
    }
    {
        pvd::PVShortPtr B(val->getSubField<pvd::PVShort>("B"));
        testOk1(!!B && B->get()==43);
    }
}

void testSubStruct()
{
    testDiag("testSubStruct()");
    pvd::PVStructurePtr val(pvd::ValueBuilder("foo")
                            .add<pvd::pvInt>("A", 42)
                            .addNested("X", pvd::structure, "bar")
                                .add<pvd::pvInt>("Y", 111)
                                .add<pvd::pvString>("Z", "hello world")
                            .endNested()
                            .add<pvd::pvInt>("B", 43)
                            .buildPVStructure());

    testOk1(val->getStructure()->getID()=="foo");
    testOk1(val->getSubFieldT<pvd::PVStructure>("X")->getStructure()->getID()=="bar");
    {
        pvd::PVIntPtr A(val->getSubField<pvd::PVInt>("A"));
        testOk1(!!A && A->get()==42);
    }
    {
        pvd::PVIntPtr B(val->getSubField<pvd::PVInt>("B"));
        testOk1(!!B && B->get()==43);
    }
    {
        pvd::PVIntPtr Y(val->getSubField<pvd::PVInt>("X.Y"));
        testOk1(!!Y && Y->get()==111);
    }
    {
        pvd::PVStringPtr Y(val->getSubField<pvd::PVString>("X.Z"));
        testOk1(!!Y && Y->get()=="hello world");
    }
}

void testReplace()
{
    testDiag("testReplace()");
    pvd::PVStructurePtr val(pvd::ValueBuilder("foo")
                            .add<pvd::pvInt>("A", 42)
                            .add<pvd::pvInt>("A", 43)
                            .addNested("X", pvd::structure, "bar")
                                .add<pvd::pvInt>("Y", 111)
                            .endNested()
                            .addNested("X", pvd::structure, "baz")
                                .add<pvd::pvInt>("Y", 112)
                            .endNested()
                            .buildPVStructure());

    testDiag("top id %s", val->getStructure()->getID().c_str());
    testOk1(val->getStructure()->getID()=="foo");
    testDiag("X id %s", val->getSubFieldT<pvd::PVStructure>("X")->getStructure()->getID().c_str());
    testOk1(val->getSubFieldT<pvd::PVStructure>("X")->getStructure()->getID()=="baz");
    {
        pvd::PVIntPtr A(val->getSubField<pvd::PVInt>("A"));
        testOk1(!!A && A->get()==43);
    }
    {
        pvd::PVIntPtr Y(val->getSubField<pvd::PVInt>("X.Y"));
        testOk1(!!Y && Y->get()==112);
    }
}

void testAppend()
{
    testDiag("testAppend()");

    pvd::PVStructurePtr base(pvd::createRequest("field(foo)record[bar=5]"));

    testOk1(!!base->getSubField<pvd::PVStructure>("field.foo"));
    testOk1(!base->getSubField<pvd::PVStructure>("field.other"));
    testOk1(base->getSubFieldT<pvd::PVString>("record._options.bar")->get()=="5");
    testOk1(!base->getSubField<pvd::PVStructure>("record._options.foo"));

    pvd::PVStructurePtr mod(pvd::ValueBuilder(*base)
                            .addNested("field")
                                .addNested("other")
                                .endNested()
                            .endNested()
                            .addNested("record")
                                .addNested("_options")
                                    .add<pvd::pvInt>("bar", 4)
                                    .add<pvd::pvInt>("foo", 1)
                                .endNested()
                            .endNested()
                            .buildPVStructure());

    testOk1(base->getField().get()!=mod->getField().get());
    testOk1(base->getField()!=mod->getField());

    // base unchanged
    testOk1(!!base->getSubField<pvd::PVStructure>("field.foo"));
    testOk1(!base->getSubField<pvd::PVStructure>("field.other"));
    testOk1(base->getSubFieldT<pvd::PVString>("record._options.bar")->get()=="5");
    testOk1(!base->getSubField<pvd::PVStructure>("record._options.foo"));

    testOk1(!!mod->getSubField<pvd::PVStructure>("field.foo"));
    testOk1(!!mod->getSubField<pvd::PVStructure>("field.other"));
    testOk1(mod->getSubFieldT<pvd::PVInt>("record._options.bar")->get()==4);
    testOk1(mod->getSubFieldT<pvd::PVInt>("record._options.foo")->get()==1);
}

void testArray()
{
    testDiag("testArray()");

    pvd::ValueBuilder builder;

    pvd::shared_vector<pvd::int32> V(2);
    V[0] = 1;
    V[1] = 2;
    pvd::shared_vector<const pvd::int32> SV(pvd::freeze(V));

    pvd::PVStructurePtr S(builder
                          .add("foo", pvd::static_shared_vector_cast<const void>(SV))
                          .buildPVStructure());

    pvd::PVIntArrayPtr I(S->getSubFieldT<pvd::PVIntArray>("foo"));

    pvd::PVIntArray::const_svector out(I->view());

    testFieldEqual<pvd::PVIntArray>(S, "foo", SV);
}

} // namespace

MAIN(testValueBuilder)
{
    testPlan(28);
    try {
        testBuild();
        testSubStruct();
        testReplace();
        testAppend();
        testArray();
    }catch(std::exception& e){
        PRINT_EXCEPTION(e);
        testAbort("Unexpected exception: %s", e.what());
    }
    return testDone();
}
