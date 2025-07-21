
#include <epicsAtomic.h>
#include <errlog.h>
#include <epicsEvent.h>
#include <epicsUnitTest.h>
#include <dbUnitTest.h>

#include <pv/pvUnitTest.h>
#include <pv/pvAccess.h>

#define epicsExportSharedSymbols
#include <utilities.h>
#include <helper.h>

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

static size_t countTestChannelRequester;

TestChannelRequester::TestChannelRequester()
    :laststate(pva::Channel::NEVER_CONNECTED)
{
    epicsAtomicIncrSizeT(&countTestChannelRequester);
}


TestChannelRequester::~TestChannelRequester()
{
    epicsAtomicDecrSizeT(&countTestChannelRequester);
}

void TestChannelRequester::channelCreated(const pvd::Status& status, pva::Channel::shared_pointer const & channel)
{
    testDiag("channelCreated %s", channel ? channel->getChannelName().c_str() : "<fails>");
    Guard G(lock);
    laststate = pva::Channel::CONNECTED;
    this->status = status;
    chan = channel;
    wait.trigger();
}

void TestChannelRequester::channelStateChange(pva::Channel::shared_pointer const & channel,
                                              pva::Channel::ConnectionState connectionState)
{
    testDiag("channelStateChange %s %d", channel->getChannelName().c_str(), (int)connectionState);
    Guard G(lock);
    laststate = connectionState;
    wait.trigger();
}

bool TestChannelRequester::waitForConnect()
{
    Guard G(lock);
    assert(chan);
    while(true) {
        pva::Channel::ConnectionState cur = chan->getConnectionState();
        switch(cur) {
        case pva::Channel::NEVER_CONNECTED:
            break;
        case pva::Channel::CONNECTED:
            return true;
        case pva::Channel::DISCONNECTED:
        case pva::Channel::DESTROYED:
            return false;
        }
        UnGuard U(G);
        wait.wait();
    }

}

static size_t countTestChannelGetRequester;

TestChannelGetRequester::TestChannelGetRequester()
    :connected(false)
    ,done(false)
{
    epicsAtomicIncrSizeT(&countTestChannelGetRequester);
}

TestChannelGetRequester::~TestChannelGetRequester()
{
    epicsAtomicDecrSizeT(&countTestChannelGetRequester);
}

void TestChannelGetRequester::channelGetConnect(const epics::pvData::Status &status,
                                                const epics::pvAccess::ChannelGet::shared_pointer &get,
                                                const epics::pvData::Structure::const_shared_pointer &structure)
{
    if(connected)
        testFail("channelGetConnect() called twice");
    statusConnect = status;
    channelGet = get;
    fielddesc = structure;
    connected = true;
}

void TestChannelGetRequester::getDone(const epics::pvData::Status &status,
                                      const epics::pvAccess::ChannelGet::shared_pointer &get,
                                      const epics::pvData::PVStructure::shared_pointer &pvStructure,
                                      const epics::pvData::BitSet::shared_pointer &bitSet)
{
    statusDone = status;
    channelGet = get;
    value = pvStructure;
    changed = bitSet;
    done = true;
}

TestChannelPutRequester::TestChannelPutRequester()
    :connected(false)
    ,doneGet(false)
    ,donePut(false)
{}
TestChannelPutRequester::~TestChannelPutRequester() {}

void TestChannelPutRequester::channelPutConnect(
        const epics::pvData::Status& status,
        epics::pvAccess::ChannelPut::shared_pointer const & channelPut,
        epics::pvData::Structure::const_shared_pointer const & structure)
{
    statusConnect = status;
    put = channelPut;
    fielddesc = structure;
    connected = true;
}

void TestChannelPutRequester::putDone(
        const epics::pvData::Status& status,
        epics::pvAccess::ChannelPut::shared_pointer const & channelPut)
{
    statusPut = status;
    put = channelPut;
    donePut = true;
}

void TestChannelPutRequester::getDone(
        const epics::pvData::Status& status,
        epics::pvAccess::ChannelPut::shared_pointer const & channelPut,
        epics::pvData::PVStructure::shared_pointer const & pvStructure,
        epics::pvData::BitSet::shared_pointer const & bitSet)
{
    statusGet = status;
    put = channelPut;
    value = pvStructure;
    changed = bitSet;
    doneGet = true;
}


static size_t countTestChannelMonitorRequester;

TestChannelMonitorRequester::TestChannelMonitorRequester()
    :connected(false)
    ,unlistend(false)
    ,eventCnt(0)
{
    epicsAtomicIncrSizeT(&countTestChannelMonitorRequester);
}

TestChannelMonitorRequester::~TestChannelMonitorRequester()
{
    epicsAtomicDecrSizeT(&countTestChannelMonitorRequester);
}

void TestChannelMonitorRequester::monitorConnect(pvd::Status const & status,
                                                 pvd::MonitorPtr const & monitor,
                                                 pvd::StructureConstPtr const & structure)
{
    testDiag("monitorConnect %p %d", monitor.get(), (int)status.isSuccess());
    Guard G(lock);
    connectStatus = status;
    dtype = structure;
    connected = true;
    wait.trigger();
}

void TestChannelMonitorRequester::monitorEvent(pvd::MonitorPtr const & monitor)
{
    testDiag("monitorEvent %p", monitor.get());
    mon = monitor;
    eventCnt++;
    wait.trigger();
}

void TestChannelMonitorRequester::unlisten(pvd::MonitorPtr const & monitor)
{
    testDiag("unlisten %p", monitor.get());
    Guard G(lock);
    unlistend = true;
    wait.trigger();
}

bool TestChannelMonitorRequester::waitForConnect()
{
    Guard G(lock);
    while(!connected) {
        UnGuard U(G);
        wait.wait();
    }
    return true;
}

bool TestChannelMonitorRequester::waitForEvent()
{
    Guard G(lock);
    size_t icnt = eventCnt;
    while(!unlistend && eventCnt==icnt) {
        UnGuard U(G);
        wait.wait();
    }
    return !unlistend;
}

static size_t countTestPVChannel;

TestPVChannel::TestPVChannel(const std::tr1::shared_ptr<TestPV> &pv,
                             const std::tr1::shared_ptr<pva::ChannelRequester> &req)
    :BaseChannel(pv->name, pv->provider, req, pv->dtype)
    ,pv(pv)
    ,state(CONNECTED)
{
    epicsAtomicIncrSizeT(&countTestPVChannel);
}

TestPVChannel::~TestPVChannel()
{
    epicsAtomicDecrSizeT(&countTestPVChannel);
}

TestPVChannel::ConnectionState TestPVChannel::getConnectionState()
{
    Guard G(pv->lock);
    return state;
}

void TestPVChannel::getField(pva::GetFieldRequester::shared_pointer const & requester,std::string const & subField)
{
    Guard G(pv->lock);

    //TODO subField?
    requester->getDone(pvd::Status(), pv->dtype);
}

pvd::Monitor::shared_pointer
TestPVChannel::createMonitor(
        pvd::MonitorRequester::shared_pointer const & requester,
        pvd::PVStructure::shared_pointer const & pvRequest)
{
    shared_pointer self(weakself);
    TestPVMonitor::shared_pointer ret(new TestPVMonitor(self, requester, 2));
    {
        Guard G(pv->lock);
        monitors.insert(ret);
        static_cast<TestPVMonitor*>(ret.get())->weakself = ret; // save wrapped weak ref
    }
    testDiag("TestPVChannel::createMonitor %s %p", pv->name.c_str(), ret.get());
    requester->monitorConnect(pvd::Status(), ret, pv->dtype);
    return ret;
}

static size_t countTestPVMonitor;

TestPVMonitor::TestPVMonitor(const TestPVChannel::shared_pointer& ch,
              const pvd::MonitorRequester::shared_pointer& req,
              size_t bsize)
    :channel(ch)
    ,requester(req)
    ,running(false)
    ,finalize(false)
    ,inoverflow(false)
    ,needWakeup(false)
{
    pvd::PVDataCreatePtr fact(pvd::PVDataCreate::getPVDataCreate());
    for(size_t i=0; i<bsize; i++) {
        pva::MonitorElementPtr elem(new pvd::MonitorElement(fact->createPVStructure(channel->pv->dtype)));
        free.push_back(elem);
    }
    overflow.reset(new pvd::MonitorElement(fact->createPVStructure(channel->pv->dtype)));
    overflow->changedBitSet->set(0); // initially all changed
    epicsAtomicIncrSizeT(&countTestPVMonitor);
}

TestPVMonitor::~TestPVMonitor()
{
    epicsAtomicDecrSizeT(&countTestPVMonitor);
}

void TestPVMonitor::destroy()
{
    Guard G(channel->pv->lock);

    shared_pointer self(weakself);
    channel->monitors.erase(self); // ensure we don't get more notifications
}

pvd::Status TestPVMonitor::start()
{
    testDiag("TestPVMonitor::start %p", this);

    Guard G(channel->pv->lock);
    if(finalize && buffer.empty())
        return pvd::Status();

    if(running)
        return pvd::Status();
    running = true;

    // overflow element does double duty to hold this monitor's copy
    overflow->pvStructurePtr->copyUnchecked(*channel->pv->value);

    if(this->buffer.empty()) {
        needWakeup = true;
        testDiag(" need wakeup");
    }

    if(!this->free.empty()) {
        pva::MonitorElementPtr monitorElement(this->free.front());

        if(overflow->changedBitSet->isEmpty()) {
            overflow->changedBitSet->set(0); // initial update has all changed
            overflow->overrunBitSet->clear();
        }

        monitorElement->pvStructurePtr->copyUnchecked(*overflow->pvStructurePtr);
        *monitorElement->changedBitSet = *overflow->changedBitSet;
        *monitorElement->overrunBitSet = *overflow->overrunBitSet;
        overflow->changedBitSet->clear();
        overflow->overrunBitSet->clear();

        buffer.push_back(monitorElement);
        this->free.pop_front();
        testDiag(" push current");

    } else {
        inoverflow = true;
        overflow->changedBitSet->clear();
        overflow->changedBitSet->set(0);
        testDiag(" push overflow");
    }

    return pvd::Status();
}

pvd::Status TestPVMonitor::stop()
{
    testDiag("TestPVMonitor::stop %p", this);
    Guard G(channel->pv->lock);
    running = false;
    return pvd::Status();
}

pva::MonitorElementPtr TestPVMonitor::poll()
{
    pva::MonitorElementPtr ret;
    Guard G(channel->pv->lock);
    if(!buffer.empty()) {
        ret = buffer.front();
        buffer.pop_front();
    }
    testDiag("TestPVMonitor::poll %p %p", this, ret.get());
    return ret;
}

void TestPVMonitor::release(pva::MonitorElementPtr const & monitorElement)
{
    Guard G(channel->pv->lock);
    testDiag("TestPVMonitor::release %p %p", this, monitorElement.get());

    if(inoverflow) {
        // buffer.empty() may be true if all elements poll()d by user
        assert(this->free.empty());

        monitorElement->pvStructurePtr->copyUnchecked(*overflow->pvStructurePtr);
        *monitorElement->changedBitSet = *overflow->changedBitSet;
        *monitorElement->overrunBitSet = *overflow->overrunBitSet;

        overflow->changedBitSet->clear();
        overflow->overrunBitSet->clear();

        buffer.push_back(monitorElement);
        testDiag("TestPVMonitor::release overflow resume %p %p", this, monitorElement.get());
        inoverflow = false;
    } else {
        this->free.push_back(monitorElement);
    }
}

static size_t countTestPV;

TestPV::TestPV(const std::string& name,
               const std::tr1::shared_ptr<TestProvider>& provider,
               const pvd::StructureConstPtr& dtype)
    :name(name)
    ,provider(provider)
    ,factory(pvd::PVDataCreate::getPVDataCreate())
    ,dtype(dtype)
    ,value(factory->createPVStructure(dtype))
{
    epicsAtomicIncrSizeT(&countTestPV);
}

TestPV::~TestPV()
{
    epicsAtomicDecrSizeT(&countTestPV);
}

void TestPV::post(bool notify)
{
    pvd::BitSet changed;
    changed.set(0); // all
    post(changed, notify);
}

void TestPV::post(const pvd::BitSet& changed, bool notify)
{
    testDiag("post %s %d changed '%s'", name.c_str(), (int)notify, toString(changed).c_str());
    Guard G(lock);

    channels_t::vector_type toupdate(channels.lock_vector());

    FOREACH(channels_t::vector_type::const_iterator, it, end, toupdate) // channel
    {
        TestPVChannel *chan = it->get();

        TestPVChannel::monitors_t::vector_type tomon(chan->monitors.lock_vector());
        FOREACH(TestPVChannel::monitors_t::vector_type::const_iterator, it2, end2, tomon) // monitor/subscription
        {
            TestPVMonitor *mon = it2->get();

            if(!mon->running)
                continue;

            mon->overflow->pvStructurePtr->copyUnchecked(*value, changed);

            if(mon->free.empty()) {
                mon->inoverflow = true;
                mon->overflow->overrunBitSet->or_and(*mon->overflow->changedBitSet, changed); // oflow |= prev_changed & new_changed
                *mon->overflow->changedBitSet |= changed;
                testDiag("overflow changed '%s' overrun '%s'",
                         toString(*mon->overflow->changedBitSet).c_str(),
                         toString(*mon->overflow->overrunBitSet).c_str());

            } else {
                assert(!mon->inoverflow);

                if(mon->buffer.empty())
                    mon->needWakeup = true;

                pvd::MonitorElementPtr& elem(mon->free.front());
                // Note: can't use 'changed' to optimize this copy since we don't know
                //       the state of the free element
                elem->pvStructurePtr->copyUnchecked(*mon->overflow->pvStructurePtr);
                *elem->changedBitSet = changed;
                elem->overrunBitSet->clear(); // redundant/paranoia

                mon->buffer.push_back(elem);
                mon->free.pop_front();
                testDiag("push %p changed '%s' overflow '%s'", elem.get(),
                         toString(*elem->changedBitSet).c_str(),
                         toString(*elem->overrunBitSet).c_str());
            }

            if(mon->needWakeup && notify) {
                testDiag(" wakeup");
                mon->needWakeup = false;
                pva::MonitorRequester::shared_pointer req(mon->requester.lock());
                UnGuard U(G);
                if(req)
                    req->monitorEvent(*it2);
            }
        }
    }
}

void TestPV::disconnect()
{
    Guard G(lock);
    channels_t::vector_type toupdate(channels.lock_vector());

    FOREACH(channels_t::vector_type::const_iterator, it, end, toupdate) // channel
    {
        TestPVChannel *chan = it->get();

        chan->state = TestPVChannel::DISCONNECTED;
        {
            pva::ChannelRequester::shared_pointer req(chan->requester.lock());
            UnGuard U(G);
            if(req)
                req->channelStateChange(*it, TestPVChannel::DISCONNECTED);
        }
    }
}

static size_t countTestProvider;

TestProvider::TestProvider()
{
    epicsAtomicIncrSizeT(&countTestProvider);
}

TestProvider::~TestProvider()
{
    epicsAtomicDecrSizeT(&countTestProvider);
}

void TestProvider::destroy()
{
    // TODO: disconnect all?
}

pva::ChannelFind::shared_pointer
TestProvider::channelList(pva::ChannelListRequester::shared_pointer const & requester)
{
    pva::ChannelFind::shared_pointer ret;
    pvd::PVStringArray::const_svector names;
    requester->channelListResult(pvd::Status(pvd::Status::STATUSTYPE_FATAL, "Not implemented"),
                                 ret,
                                 names,
                                 true);
    return ret;
}

pva::ChannelFind::shared_pointer
TestProvider::channelFind(std::string const & channelName,
                          pva::ChannelFindRequester::shared_pointer const & requester)
{
    pva::ChannelFind::shared_pointer ret;
    requester->channelFindResult(pvd::Status(pvd::Status::STATUSTYPE_FATAL, "Not implemented"),
                                 ret, false);
    return ret;
}

pva::Channel::shared_pointer
TestProvider::createChannel(std::string const & channelName,pva::ChannelRequester::shared_pointer const & requester,
                                       short priority)
{
    return createChannel(channelName, requester, priority, "<unused>");
}

pva::Channel::shared_pointer
TestProvider::createChannel(std::string const & channelName,
                            pva::ChannelRequester::shared_pointer const & requester,
                            short priority, std::string const & address)
{
    pva::Channel::shared_pointer ret;

    {
        Guard G(lock);

        TestPV::shared_pointer pv(pvs.find(channelName));
        if(pv) {
            TestPVChannel::shared_pointer chan(new TestPVChannel(pv, requester));
            pv->channels.insert(chan);
            chan->weakself = chan;
            ret = chan;
        }
    }

    if(ret) {
        requester->channelCreated(pvd::Status(), ret);
    } else {
        requester->channelCreated(pvd::Status(pvd::Status::STATUSTYPE_ERROR, "PV not found"), ret);
    }
    testDiag("createChannel %s %p", channelName.c_str(), ret.get());
    return ret;
}

TestPV::shared_pointer
TestProvider::addPV(const std::string& name, const pvd::StructureConstPtr& tdef)
{
    Guard G(lock);
    TestPV::shared_pointer ret(new TestPV(name, shared_from_this(), tdef));
    pvs.insert(name, ret);
    return ret;
}

void TestProvider::dispatch()
{
    Guard G(lock);
    testDiag("TestProvider::dispatch");

    pvs_t::lock_vector_type allpvs(pvs.lock_vector());
    FOREACH(pvs_t::lock_vector_type::const_iterator, pvit, pvend, allpvs)
    {
        TestPV *pv = pvit->second.get();
        TestPV::channels_t::vector_type channels(pv->channels.lock_vector());

        FOREACH(TestPV::channels_t::vector_type::const_iterator, chit, chend, channels)
        {
            TestPVChannel *chan = chit->get();
            TestPVChannel::monitors_t::vector_type monitors(chan->monitors.lock_vector());

            if(!chan->isConnected())
                continue;

            FOREACH(TestPVChannel::monitors_t::vector_type::const_iterator, monit, monend, monitors)
            {
                TestPVMonitor *mon = monit->get();

                if(mon->finalize || !mon->running)
                    continue;

                if(mon->needWakeup) {
                    testDiag("  wakeup monitor %p", mon);
                    mon->needWakeup = false;
                    pva::MonitorRequester::shared_pointer req(mon->requester.lock());
                    UnGuard U(G);
                    if(req)
                        req->monitorEvent(*monit);
                }
            }
        }
    }
}

void TestProvider::testCounts()
{
    int ok = 1;
    size_t temp;
#define TESTC(name) temp=epicsAtomicGetSizeT(&count##name); ok &= temp==0; testDiag("num. live "  #name " %u", (unsigned)temp)
    TESTC(TestChannelMonitorRequester);
    TESTC(TestChannelRequester);
    TESTC(TestProvider);
    TESTC(TestPV);
    TESTC(TestPVChannel);
    TESTC(TestPVMonitor);
#undef TESTC
    testOk(ok, "All instances free'd");
}
