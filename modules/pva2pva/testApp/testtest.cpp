// Test the testing utilities

#include <epicsGuard.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/monitor.h>
#include <pv/thread.h>

#include "utilities.h"

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

namespace {

void testmonitor()
{
    testDiag("Test Test* monitoring");

    pvd::FieldCreatePtr fieldCreate(pvd::getFieldCreate());
    pvd::PVDataCreatePtr create(pvd::getPVDataCreate());

    testDiag("Setup TestProvider with \"test\"");
    TestProvider::shared_pointer prov(new TestProvider);
    TestPV::shared_pointer pv(prov->addPV("test",
                                          fieldCreate->createFieldBuilder()
                                          ->add("x", pvd::pvInt)
                                          ->add("y", pvd::pvInt)
                                          ->createStructure()));

    ScalarAccessor<pvd::int32> x(pv->value, "x"),
                               y(pv->value, "y");

    x = 42;
    y = 15;

    testDiag("Create channel");
    TestChannelRequester::shared_pointer creq(new TestChannelRequester);
    pva::Channel::shared_pointer chan(prov->createChannel("test", creq));

    testOk1(!!chan.get());
    testOk1(creq->waitForConnect());
    testOk1(chan==creq->chan);
    if(!chan)
        testAbort("Create channel failed");

    testDiag("Create monitor");
    TestChannelMonitorRequester::shared_pointer mreq(new TestChannelMonitorRequester);
    pvd::Monitor::shared_pointer mon;
    {
        pvd::PVStructurePtr pvreq(create->createPVStructure(fieldCreate->createFieldBuilder()
                                                            ->add("junk", pvd::pvInt)
                                                            ->createStructure()));
        mon = chan->createMonitor(mreq, pvreq);
    }

    testOk1(!!mon.get());
    testOk1(mreq->connectStatus.isSuccess());
    testOk1(!!mreq->dtype.get());
    testOk1(mreq->eventCnt==0);

    if(!mon)
        testAbort("Create monitor failed");

    testDiag("ensure queue is initially empty");
    testOk1(!mon->poll());

    testDiag("Start monitor and check initial update");
    testOk1(mon->start().isSuccess());

    pva::MonitorElementPtr elem(mon->poll());
    testOk1(!!elem.get());

    if(elem) testDiag("elem changed '%s' overflow '%s'", toString(*elem->changedBitSet).c_str(), toString(*elem->overrunBitSet).c_str());
    if(elem) testDiag("elem x=%d y=%d",
                      elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get(),
                      elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get());
    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==42);
    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==15);
    testOk1(elem && elem->changedBitSet->nextSetBit(0)==0); // initial update shows all changed
    testOk1(elem && elem->changedBitSet->nextSetBit(1)==-1);
    testOk1(elem && elem->overrunBitSet->nextSetBit(0)==-1);
    testOk1(elem && elem->overrunBitSet->isEmpty());
    if(elem) mon->release(elem);

    testDiag("ensure start() queues only one");
    testOk1(!mon->poll());

    testDiag("Change both fields, only push 'x'");
    x = 43;
    y = 16;
    pvd::BitSet changed;
    changed.set(1); // only notify that 'x' changed
    pv->post(changed);

    testOk1(mreq->eventCnt==1);

    elem = mon->poll();
    testOk1(!!elem.get());

    if(elem) testDiag("elem changed '%s' overflow '%s'", toString(*elem->changedBitSet).c_str(), toString(*elem->overrunBitSet).c_str());
    if(elem) testDiag("elem x=%d y=%d",
                      elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get(),
                      elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get());
    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==43);
    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==15);
    testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
    testOk1(elem && elem->changedBitSet->nextSetBit(2)==-1);
    testOk1(elem && elem->overrunBitSet->isEmpty());
    if(elem) mon->release(elem);

    testDiag("ensure queues are empty");
    testOk1(!mon->poll());

    testDiag("overflow queue");
    x = 44;
    pv->post(changed, false);
    x = 45;
    pv->post(changed, false);
    x = 46;
    pv->post(changed, false);

    testOk1(mreq->eventCnt==1);

    x = 47;
    pv->post(changed);

    testOk1(mreq->eventCnt==2);

    elem = mon->poll();
    testOk1(!!elem.get());

    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==44);
    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==15);
    testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
    testOk1(elem && elem->overrunBitSet->isEmpty());
    if(elem) mon->release(elem); // overflow element is queued here

    elem = mon->poll();
    testOk1(!!elem.get());

    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==45);
    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==15);
    testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
    testOk1(elem && elem->overrunBitSet->isEmpty());
    if(elem) mon->release(elem);

    elem = mon->poll();
    testOk1(!!elem.get());

    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("x")->get()==47);
    testOk1(elem && elem->pvStructurePtr->getSubFieldT<pvd::PVInt>("y")->get()==15);
    testOk1(elem && elem->changedBitSet->nextSetBit(0)==1);
    testOk1(elem && elem->overrunBitSet->nextSetBit(0)==1);
    testOk1(elem && elem->overrunBitSet->nextSetBit(2)==-1);
    if(elem) mon->release(elem);

    testDiag("ensure queues are empty");
    testOk1(!mon->poll());

    testOk1(mreq->eventCnt==2);

    mon->destroy();

    chan->destroy();
}
} // namespace

MAIN(testtest)
{
    testPlan(46);
    testmonitor();
    TestProvider::testCounts();
    return testDone();
}
