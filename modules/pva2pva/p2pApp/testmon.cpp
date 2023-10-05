
#include <epicsAtomic.h>
#include <epicsGuard.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/epicsException.h>
#include <pv/monitor.h>
#include <pv/thread.h>
#include <pv/serverContext.h>

#include "server.h"

#include "utilities.h"

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

typedef epicsGuard<epicsMutex> Guard;

namespace {

pvd::PVStructurePtr makeRequest(size_t bsize)
{
    pvd::StructureConstPtr dtype(pvd::getFieldCreate()->createFieldBuilder()
                                 ->addNestedStructure("record")
                                    ->addNestedStructure("_options")
                                        ->add("queueSize", pvd::pvString) // yes, really.  PVA wants a string
                                    ->endNested()
                                 ->endNested()
                                 ->createStructure());

    pvd::PVStructurePtr ret(pvd::getPVDataCreate()->createPVStructure(dtype));
    ret->getSubFieldT<pvd::PVScalar>("record._options.queueSize")->putFrom<pvd::int32>(bsize);

    return ret;
}

struct TestMonitor {
    TestProvider::shared_pointer upstream;
    TestPV::shared_pointer test1;
    ScalarAccessor<pvd::int32> test1_x, test1_y;

    GWServerChannelProvider::shared_pointer gateway;

    TestChannelRequester::shared_pointer client_req;
    pva::Channel::shared_pointer client;

    // prepare providers and connect the client channel, don't setup monitor
    TestMonitor()
        :upstream(new TestProvider())
        ,test1(upstream->addPV("test1", pvd::getFieldCreate()->createFieldBuilder()
                               ->add("x", pvd::pvInt)
                               ->add("y", pvd::pvInt)
                               ->createStructure()))
        ,test1_x(test1->value, "x")
        ,test1_y(test1->value, "y")
        ,gateway(new GWServerChannelProvider(upstream))
        ,client_req(new TestChannelRequester)
        ,client(gateway->createChannel("test1", client_req))
    {
        testDiag("pre-test setup");
        if(!client)
            testAbort("channel \"test1\" not connected");
        test1_x = 1;
        test1_y = 2;
    }

    ~TestMonitor()
    {
        client->destroy();
        gateway->destroy(); // noop atm.
    }

    void test_event()
    {
        testDiag("Push the initial event through from upstream to downstream");

        TestChannelMonitorRequester::shared_pointer mreq(new TestChannelMonitorRequester);
        pvd::Monitor::shared_pointer mon(client->createMonitor(mreq, makeRequest(2)));
        if(!mon) testAbort("Failed to create monitor");

        testEqual(mreq->eventCnt, 0u);
        testOk1(mon->start().isSuccess());
        upstream->dispatch(); // trigger monitorEvent() from upstream to gateway

        testEqual(mreq->eventCnt, 1u);
        pva::MonitorElementPtr elem(mon->poll());
        testOk1(!!elem.get());
        if(!!elem.get()) testEqual(toString(*elem->changedBitSet), "{0}");
        else testFail("oops");
        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==1);

        if(elem) mon->release(elem);

        testOk1(!mon->poll());

        mon->destroy();
    }

    void test_share()
    {
        // here both downstream monitors are on the same Channel,
        // which would be inefficient, and slightly unrealistic, w/ real PVA,
        // but w/ TestProvider makes no difference
        testDiag("Test two downstream monitors sharing the same upstream");

        TestChannelMonitorRequester::shared_pointer mreq(new TestChannelMonitorRequester);
        pvd::Monitor::shared_pointer mon(client->createMonitor(mreq, makeRequest(2)));
        if(!mon) testAbort("Failed to create monitor");


        TestChannelMonitorRequester::shared_pointer mreq2(new TestChannelMonitorRequester);
        pvd::Monitor::shared_pointer mon2(client->createMonitor(mreq2, makeRequest(2)));
        if(!mon2) testAbort("Failed to create monitor2");

        testOk1(mreq->eventCnt==0);
        testOk1(mreq2->eventCnt==0);
        testOk1(mon->start().isSuccess());
        testOk1(mon2->start().isSuccess());
        upstream->dispatch(); // trigger monitorEvent() from upstream to gateway

        testOk1(mreq->eventCnt==1);
        testOk1(mreq2->eventCnt==1);

        pva::MonitorElementPtr elem(mon->poll());
        pva::MonitorElementPtr elem2(mon2->poll());
        testOk1(!!elem.get());
        testOk1(!!elem2.get());
        testOk1(elem!=elem2);
        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==1);
        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==2);
        testOk1(elem2 && elem2->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==1);
        testOk1(elem2 && elem2->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==2);

        if(elem) mon->release(elem);
        if(elem2) mon2->release(elem2);

        testOk1(!mon->poll());
        testOk1(!mon2->poll());

        testDiag("explicitly push an update");
        test1_x = 42;
        test1_y = 43;
        pvd::BitSet changed;
        changed.set(1); // only indicate that 'x' changed
        test1->post(changed);

        elem = mon->poll();
        elem2 = mon2->poll();
        testOk1(!!elem.get());
        testOk1(!!elem2.get());
        testOk1(elem!=elem2);
        if(elem) testDiag("elem changed '%s' overflow '%s'", toString(*elem->changedBitSet).c_str(), toString(*elem->overrunBitSet).c_str());
        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==42);
        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==2);
        if(elem2) testDiag("elem2 changed '%s' overflow '%s'", toString(*elem2->changedBitSet).c_str(), toString(*elem2->overrunBitSet).c_str());
        testOk1(elem2 && elem2->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==42);
        testOk1(elem2 && elem2->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==2);

        if(elem) mon->release(elem);
        if(elem2) mon2->release(elem2);

        testOk1(!mon->poll());
        testOk1(!mon2->poll());

        mon->destroy();
        mon2->destroy();
    }

    void test_ds_no_start()
    {
        testDiag("Test downstream monitor never start()s");

        TestChannelMonitorRequester::shared_pointer mreq(new TestChannelMonitorRequester);
        pvd::Monitor::shared_pointer mon(client->createMonitor(mreq, makeRequest(2)));
        if(!mon) testAbort("Failed to create monitor");

        upstream->dispatch(); // trigger monitorEvent() from upstream to gateway

        testOk1(mreq->eventCnt==0);
        testOk1(!mon->poll());

        pvd::BitSet changed;
        changed.set(1);
        test1_x=50;
        test1->post(changed, false);
        test1_x=51;
        test1->post(changed, false);
        test1_x=52;
        test1->post(changed, false);
        test1_x=53;
        test1->post(changed);

        testOk1(!mon->poll());

        mon->destroy();
    }

    void test_overflow_upstream()
    {
        testDiag("Check behavour when upstream monitor overflows (mostly transparent)");

        TestChannelMonitorRequester::shared_pointer mreq(new TestChannelMonitorRequester);
        pvd::Monitor::shared_pointer mon(client->createMonitor(mreq, makeRequest(2)));
        if(!mon) testAbort("Failed to create monitor");

        testOk1(mreq->eventCnt==0);
        testOk1(mon->start().isSuccess());
        upstream->dispatch(); // trigger monitorEvent() from upstream to gateway
        testOk1(mreq->eventCnt==1);

        testDiag("poll initial update");
        pva::MonitorElementPtr elem(mon->poll());
        testOk1(!!elem.get());
        if(elem) mon->release(elem);

        testOk1(!mon->poll());

        // queue 4 events input buffer of size 2
        // don't notify downstream until after overflow has occurred
        pvd::BitSet changed;
        changed.set(1);
        test1_x=50;
        testDiag("post 50");
        test1->post(changed, false);
        test1_x=51;
        testDiag("post 51");
        test1->post(changed, false);
        test1_x=52;
        testDiag("post 52");
        test1->post(changed, false);
        test1_x=53;
        testDiag("post 53");
        test1->post(changed);

        elem = mon->poll();
        testOk1(!!elem.get());

        testDiag("XX %d", elem ? elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get() : -42);
        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==50);
        testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->changedBitSet->nextSetBit(2)==-1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(0)==-1);

        if(elem) mon->release(elem);

        elem = mon->poll();
        testOk1(!!elem.get());

        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==51);
        testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->changedBitSet->nextSetBit(2)==-1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(0)==-1);

        if(elem) mon->release(elem);

        elem = mon->poll();
        testOk1(!!elem.get());

        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==53);
        testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->changedBitSet->nextSetBit(2)==-1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(2)==-1);

        if(elem) mon->release(elem);

        testOk1(!mon->poll());

        mon->destroy();
    }

    void test_overflow_downstream()
    {
        testDiag("Check behavour when downstream monitor overflows");

        TestChannelMonitorRequester::shared_pointer mreq(new TestChannelMonitorRequester);
        pvd::Monitor::shared_pointer mon(client->createMonitor(mreq, makeRequest(2)));
        if(!mon) testAbort("Failed to create monitor");

        testOk1(mreq->eventCnt==0);
        testOk1(mon->start().isSuccess());
        upstream->dispatch(); // trigger monitorEvent() from upstream to gateway
        testOk1(mreq->eventCnt==1);

        testDiag("poll initial update");
        pva::MonitorElementPtr elem(mon->poll());
        testOk1(!!elem.get());
        if(elem) mon->release(elem);

        // queue 4 events into buffer of size 2 (plus the overflow element)
        // notify downstream after each update
        // so the upstream queue never overflows
        pvd::BitSet changed;
        changed.set(1);
        test1_x=50;
        test1->post(changed);
        test1_x=51;
        test1->post(changed);
        test1_x=52;
        test1->post(changed);
        test1_x=53;
        test1->post(changed);

        elem = mon->poll();
        testOk1(!!elem.get());

        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==50);
        testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->changedBitSet->nextSetBit(2)==-1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(0)==-1);

        if(elem) mon->release(elem);

        elem = mon->poll();
        testOk1(!!elem.get());

        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==51);
        testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->changedBitSet->nextSetBit(2)==-1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(0)==-1);

        if(elem) mon->release(elem);

        elem = mon->poll();
        testOk1(!!elem.get());

        testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==53);
        testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->changedBitSet->nextSetBit(2)==-1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(0)==1);
        testOk1(elem && elem->overrunBitSet->nextSetBit(2)==-1);

        if(elem) mon->release(elem);

        testOk1(!mon->poll());

        mon->destroy();
    }
};

} // namespace

MAIN(testmon)
{
    testPlan(79);
    TEST_METHOD(TestMonitor, test_event);
    TEST_METHOD(TestMonitor, test_share);
    TEST_METHOD(TestMonitor, test_ds_no_start);
    TEST_METHOD(TestMonitor, test_overflow_upstream);
    TEST_METHOD(TestMonitor, test_overflow_downstream);
    TestProvider::testCounts();
    int ok = 1;
    size_t temp;
#define TESTC(name) temp=epicsAtomicGetSizeT(&name::num_instances); ok &= temp==0; testDiag("num. live "  #name " %u", (unsigned)temp)
    TESTC(GWChannel);
    TESTC(ChannelCacheEntry::CRequester);
    TESTC(ChannelCacheEntry);
    TESTC(MonitorCacheEntry);
    TESTC(MonitorUser);
#undef TESTC
    testOk(ok, "All instances free'd");
    return testDone();
}
