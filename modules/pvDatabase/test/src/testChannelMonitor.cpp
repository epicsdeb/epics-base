/* testChannelMonitor.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include <epicsUnitTest.h>
#include <testMain.h>

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/channelProviderLocal.h>
#include <pv/serverContext.h>
#include <pv/event.h>
#include <pv/clientFactory.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
namespace TR1 = std::tr1;

static bool debug = true;

PVStructurePtr createTestPvStructure()
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    StandardFieldPtr standardField = getStandardField()
;
    PVDataCreatePtr pvDataCreate = getPVDataCreate();

    return pvDataCreate->createPVStructure(
        fieldCreate->createFieldBuilder()->
            add("id",pvInt) ->
            add("x",pvInt) ->
            add("y",pvInt) ->
            add("z",pvInt) ->
            add("alarm",standardField->alarm()) ->
            add("timeStamp",standardField->timeStamp()) ->
            createStructure());
}

class ChannelMonitorRequesterImpl : public MonitorRequester
{
public:

    ChannelMonitorRequesterImpl(const std::string& channelName_)
        : channelName(channelName_)
        , lastReceivedPvStructure(createTestPvStructure())
        , lastReceivedBitSet()
    {
    }

    virtual string getRequesterName()
    {
        return "ChannelMonitorRequesterImpl";
    }

    virtual void message(const std::string& message, MessageType messageType)
    {
        cout << "[" << getRequesterName() << "] message(" << message << ", " << getMessageTypeName(messageType) << ")" << endl;
    }

    virtual void monitorConnect(const epics::pvData::Status& status, const Monitor::shared_pointer& /*monitor*/, const Structure::const_shared_pointer& /*structure*/)
    {
        if (status.isSuccess()) {
            // show warning
            if (!status.isOK()) {
                cout << "[" << channelName << "] channel monitor create: " << status << endl;
            }
            connectionEvent.signal();
        }
        else {
            cout << "[" << channelName << "] failed to create channel monitor: " << status << endl;
        }
    }

    virtual void monitorEvent(const Monitor::shared_pointer& monitor)
    {
        MonitorElement::shared_pointer element;
        while ((element = monitor->poll())) {
            cout << "changed/overrun " << *element->changedBitSet << '/' << *element->overrunBitSet << endl;
            if (!lastReceivedBitSet) {
                lastReceivedBitSet = BitSet::create(element->changedBitSet->size());
            }
            lastReceivedBitSet->clear();
            *lastReceivedBitSet |= *element->changedBitSet;
            lastReceivedPvStructure->copyUnchecked(*element->pvStructurePtr);
            monitor->release(element);
        }
    }

    virtual void unlisten(const Monitor::shared_pointer& /*monitor*/)
    {
    }

    bool waitUntilConnected(double timeOut)
    {
        return connectionEvent.wait(timeOut);
    }

    PVStructurePtr getLastReceivedPvStructure()
    {
        return lastReceivedPvStructure;
    }

    BitSetPtr getLastReceivedBitSet()
    {
        return lastReceivedBitSet;
    }

private:
    Event event;
    Event connectionEvent;
    string channelName;
    PVStructurePtr lastReceivedPvStructure;
    BitSetPtr lastReceivedBitSet;
};

class ChannelRequesterImpl : public ChannelRequester
{
private:
    Event event;

public:

    virtual string getRequesterName()
    {
        return "ChannelRequesterImpl";
    };

    virtual void message(const std::string& message, MessageType messageType)
    {
        cout << "[" << getRequesterName() << "] message(" << message << ", " << getMessageTypeName(messageType) << ")" << endl;
    }

    virtual void channelCreated(const Status& status, const Channel::shared_pointer& channel)
    {
        if (status.isSuccess()) {
            // show warning
            if (!status.isOK()) {
                cout << "[" << channel->getChannelName() << "] channel create: " << status << endl;
            }
        }
        else {
            cout << "[" << channel->getChannelName() << "] failed to create a channel: " << status << endl;
        }
    }

    virtual void channelStateChange(const Channel::shared_pointer& /*channel*/, Channel::ConnectionState connectionState)
    {
        if (connectionState == Channel::CONNECTED) {
            event.signal();
        }
        else {
            cout << Channel::ConnectionStateNames[connectionState] << endl;
            exit(3);
        }
    }

    bool waitUntilConnected(double timeOut)
    {
        return event.wait(timeOut);
    }
};


static void test()
{
    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();
    string recordName = "positions";
    PVStructurePtr pvStructure = createTestPvStructure();
    PVRecordPtr pvRecord = PVRecord::create(recordName,pvStructure);
    master->addRecord(pvRecord);
    pvRecord = master->findRecord(recordName);
    {
        pvRecord->lock();
        pvRecord->process();
        pvRecord->unlock();
    }
    if(debug) {cout << "processed positions"  << endl; }
    ServerContext::shared_pointer ctx = startPVAServer("local",0,true,true);
    testOk1(ctx.get() != 0);

    ClientFactory::start();
    ChannelProvider::shared_pointer provider = ChannelProviderRegistry::clients()->getProvider("pva");

    cout << "creating channel: " << recordName << endl;
    TR1::shared_ptr<ChannelRequesterImpl> channelRequesterImpl(new ChannelRequesterImpl());

    Channel::shared_pointer channel = provider->createChannel(recordName, channelRequesterImpl);
    bool channelConnected = channelRequesterImpl->waitUntilConnected(1.0);
    testOk1(channelConnected);
    if (channelConnected) {
        string remoteAddress = channel->getRemoteAddress();
        cout << "remote address: " << remoteAddress << endl;
    }

    string request = "";
    PVStructure::shared_pointer pvRequest = CreateRequest::create()->createRequest(request);
    TR1::shared_ptr<ChannelMonitorRequesterImpl> cmRequesterImpl(new ChannelMonitorRequesterImpl(channel->getChannelName()));
    Monitor::shared_pointer monitor = channel->createMonitor(cmRequesterImpl, pvRequest);
    bool monitorConnected = cmRequesterImpl->waitUntilConnected(1.0);
    testOk1(monitorConnected);
    Status status = monitor->start();
    testOk1(status.isOK());
    epicsThreadSleep(1);

    //  Set id, x
    {
        pvRecord->beginGroupPut();
        PVIntPtr id = pvStructure->getSubField<PVInt>("id");
        id->put(1);
        PVIntPtr x = pvStructure->getSubField<PVInt>("x");
        x->put(1);
        pvRecord->endGroupPut();
    }
    epicsThreadSleep(1);

    //  Changed set for (id,x): 0 unset, 1 set, 2 set, 3 unset, 4 unset
    BitSetPtr changedSet = cmRequesterImpl->getLastReceivedBitSet();
    testOk1(!changedSet->get(0) && changedSet->get(1) && changedSet->get(2) && !changedSet->get(3) && !changedSet->get(4));
    testOk1(*pvStructure == *cmRequesterImpl->getLastReceivedPvStructure());

    //  Set id, y
    {
        pvRecord->beginGroupPut();
        PVIntPtr id = pvStructure->getSubField<PVInt>("id");
        id->put(2);
        PVIntPtr y = pvStructure->getSubField<PVInt>("y");
        y->put(2);
        pvRecord->endGroupPut();
    }
    epicsThreadSleep(1);

    //  Changed set for (id,y): 0 unset, 1 set, 2 unset, 3 set, 4 unset
    changedSet = cmRequesterImpl->getLastReceivedBitSet();
    testOk1(!changedSet->get(0) && changedSet->get(1) && !changedSet->get(2) && changedSet->get(3) && !changedSet->get(4));
    testOk1(*pvStructure == *cmRequesterImpl->getLastReceivedPvStructure());

    //  Set id, z
    {
        pvRecord->beginGroupPut();
        PVIntPtr id = pvStructure->getSubField<PVInt>("id");
        id->put(3);
        PVIntPtr z = pvStructure->getSubField<PVInt>("z");
        z->put(3);
        pvRecord->endGroupPut();
    }
    epicsThreadSleep(1);

    //  Changed set for (id,z): 0 unset, 1 set, 2 unset, 3 unset, 4 set
    changedSet = cmRequesterImpl->getLastReceivedBitSet();
    testOk1(!changedSet->get(0) && changedSet->get(1) && !changedSet->get(2) && !changedSet->get(3) && changedSet->get(4));
    testOk1(*pvStructure == *cmRequesterImpl->getLastReceivedPvStructure());

    status = monitor->stop();
    testOk1(status.isOK());

    // Test master field
    request = "field(_)";
    pvRequest = CreateRequest::create()->createRequest(request);
    cmRequesterImpl = TR1::shared_ptr<ChannelMonitorRequesterImpl>(new ChannelMonitorRequesterImpl(channel->getChannelName()));
    monitor = channel->createMonitor(cmRequesterImpl, pvRequest);
    monitorConnected = cmRequesterImpl->waitUntilConnected(1.0);
    testOk1(monitorConnected);
    status = monitor->start();
    testOk1(status.isOK());
    epicsThreadSleep(1);
    {
        pvRecord->beginGroupPut();
        PVIntPtr id = pvStructure->getSubField<PVInt>("id");
        id->put(4);
        PVIntPtr x = pvStructure->getSubField<PVInt>("x");
        x->put(4);
        pvRecord->endGroupPut();
    }
    epicsThreadSleep(1);
    //  Changed set with master field requested: 0 set
    changedSet = cmRequesterImpl->getLastReceivedBitSet();
    testOk1(changedSet->get(0));
    testOk1(*pvStructure == *cmRequesterImpl->getLastReceivedPvStructure());

    status = monitor->stop();
    testOk1(status.isOK());

}

MAIN(testChannelMonitor)
{
    testPlan(16);
    test();
    return 0;
}
