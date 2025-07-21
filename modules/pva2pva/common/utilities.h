#ifndef UTILITIES_H
#define UTILITIES_H

#include <deque>
#include <sstream>

#include <errlog.h>
#include <epicsEvent.h>
#include <epicsUnitTest.h>
#include <dbUnitTest.h>

#include <pv/pvUnitTest.h>
#include <pv/pvAccess.h>

#include "pvahelper.h"
#include "weakmap.h"
#include "weakset.h"

struct TestPV;
struct TestPVChannel;
struct TestPVMonitor;
struct TestProvider;

// minimally useful boilerplate which must appear *everywhere*
#define DUMBREQUESTER(NAME) \
    virtual std::string getRequesterName() OVERRIDE { return #NAME; }

template<typename T>
inline std::string toString(const T& tbs)
{
    std::ostringstream oss;
    oss << tbs;
    return oss.str();
}

// Boilerplate reduction for accessing a scalar field
template<typename T>
struct ScalarAccessor {
    epics::pvData::PVScalar::shared_pointer field;
    typedef T value_type;
    ScalarAccessor(const epics::pvData::PVStructurePtr& s, const char *name)
        :field(s->getSubFieldT<epics::pvData::PVScalar>(name))
    {}
    operator value_type() {
        return field->getAs<T>();
    }
    ScalarAccessor& operator=(T v) {
        field->putFrom<T>(v);
        return *this;
    }
    ScalarAccessor& operator+=(T v) {
        field->putFrom<T>(field->getAs<T>()+v);
        return *this;
    }
};

struct TestChannelRequester : public epics::pvAccess::ChannelRequester
{
    POINTER_DEFINITIONS(TestChannelRequester);
    DUMBREQUESTER(TestChannelRequester)

    epicsMutex lock;
    epicsEvent wait;
    epics::pvAccess::Channel::shared_pointer chan;
    epics::pvData::Status status;
    epics::pvAccess::Channel::ConnectionState laststate;
    TestChannelRequester();
    virtual ~TestChannelRequester();
    virtual void channelCreated(const epics::pvData::Status& status, epics::pvAccess::Channel::shared_pointer const & channel);
    virtual void channelStateChange(epics::pvAccess::Channel::shared_pointer const & channel, epics::pvAccess::Channel::ConnectionState connectionState);

    bool waitForConnect();
};

struct TestChannelFieldRequester : public epics::pvAccess::GetFieldRequester
{
    POINTER_DEFINITIONS(TestChannelFieldRequester);
    DUMBREQUESTER(TestChannelFieldRequester)

    bool done;
    epics::pvData::Status status;
    epics::pvData::FieldConstPtr fielddesc;

    TestChannelFieldRequester() :done(false) {}
    virtual ~TestChannelFieldRequester() {}

    virtual void getDone(
           const epics::pvData::Status& status,
           epics::pvData::FieldConstPtr const & field)
    {
        this->status = status;
        fielddesc = field;
        done = true;
    }
};

struct TestChannelGetRequester : public epics::pvAccess::ChannelGetRequester
{
    POINTER_DEFINITIONS(TestChannelGetRequester);
    DUMBREQUESTER(TestChannelGetRequester)

    bool connected, done;
    epics::pvData::Status statusConnect, statusDone;
    epics::pvAccess::ChannelGet::shared_pointer channelGet;
    epics::pvData::Structure::const_shared_pointer fielddesc;
    epics::pvData::PVStructure::shared_pointer value;
    epics::pvData::BitSet::shared_pointer changed;

    TestChannelGetRequester();
    virtual ~TestChannelGetRequester();

    virtual void channelGetConnect(
            const epics::pvData::Status& status,
            epics::pvAccess::ChannelGet::shared_pointer const & channelGet,
            epics::pvData::Structure::const_shared_pointer const & structure);

    virtual void getDone(
            const epics::pvData::Status& status,
            epics::pvAccess::ChannelGet::shared_pointer const & channelGet,
            epics::pvData::PVStructure::shared_pointer const & pvStructure,
            epics::pvData::BitSet::shared_pointer const & bitSet);
};

struct TestChannelPutRequester : public epics::pvAccess::ChannelPutRequester
{
    POINTER_DEFINITIONS(TestChannelPutRequester);
    DUMBREQUESTER(TestChannelPutRequester)

    bool connected, doneGet, donePut;
    epics::pvData::Status statusConnect, statusPut, statusGet;
    epics::pvAccess::ChannelPut::shared_pointer put;
    epics::pvData::Structure::const_shared_pointer fielddesc;
    epics::pvData::PVStructure::shared_pointer value;
    epics::pvData::BitSet::shared_pointer changed;

    TestChannelPutRequester();
    virtual ~TestChannelPutRequester();

    virtual void channelPutConnect(
            const epics::pvData::Status& status,
            epics::pvAccess::ChannelPut::shared_pointer const & channelPut,
            epics::pvData::Structure::const_shared_pointer const & structure);

    virtual void putDone(
            const epics::pvData::Status& status,
            epics::pvAccess::ChannelPut::shared_pointer const & channelPut);

    virtual void getDone(
            const epics::pvData::Status& status,
            epics::pvAccess::ChannelPut::shared_pointer const & channelPut,
            epics::pvData::PVStructure::shared_pointer const & pvStructure,
            epics::pvData::BitSet::shared_pointer const & bitSet);
};

struct TestChannelMonitorRequester : public epics::pvData::MonitorRequester
{
    POINTER_DEFINITIONS(TestChannelMonitorRequester);
    DUMBREQUESTER(TestChannelMonitorRequester)

    epicsMutex lock;
    epicsEvent wait;
    bool connected;
    bool unlistend;
    size_t eventCnt;
    epics::pvData::Status connectStatus;
    epics::pvData::MonitorPtr mon;
    epics::pvData::StructureConstPtr dtype;

    TestChannelMonitorRequester();
    virtual ~TestChannelMonitorRequester();

    virtual void monitorConnect(epics::pvData::Status const & status,
                                epics::pvData::MonitorPtr const & monitor,
                                epics::pvData::StructureConstPtr const & structure);
    virtual void monitorEvent(epics::pvData::MonitorPtr const & monitor);
    virtual void unlisten(epics::pvData::MonitorPtr const & monitor);

    bool waitForConnect();
    bool waitForEvent();
};

struct TestPVChannel : public BaseChannel
{
    POINTER_DEFINITIONS(TestPVChannel);
    DUMBREQUESTER(TestPVChannel)
    std::tr1::weak_ptr<TestPVChannel> weakself;

    const std::tr1::shared_ptr<TestPV> pv;
    ConnectionState state;

    typedef weak_set<TestPVMonitor> monitors_t;
    monitors_t monitors;

    TestPVChannel(const std::tr1::shared_ptr<TestPV>& pv,
                  const std::tr1::shared_ptr<epics::pvAccess::ChannelRequester>& req);
    virtual ~TestPVChannel();

    virtual std::string getRemoteAddress() { return "localhost:1234"; }
    virtual ConnectionState getConnectionState();

    virtual void getField(epics::pvAccess::GetFieldRequester::shared_pointer const & requester,std::string const & subField);

    virtual epics::pvData::Monitor::shared_pointer createMonitor(
            epics::pvData::MonitorRequester::shared_pointer const & monitorRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);
};

struct TestPVMonitor : public epics::pvData::Monitor
{
    POINTER_DEFINITIONS(TestPVMonitor);
    std::tr1::weak_ptr<TestPVMonitor> weakself;

    const TestPVChannel::shared_pointer channel;
    const epics::pvData::MonitorRequester::weak_pointer requester;

    bool running;
    bool finalize;
    bool inoverflow;
    bool needWakeup;

    TestPVMonitor(const TestPVChannel::shared_pointer& ch,
                  const epics::pvData::MonitorRequester::shared_pointer& req,
                  size_t bsize);
    virtual ~TestPVMonitor();

    virtual void destroy();

    virtual epics::pvData::Status start();
    virtual epics::pvData::Status stop();
    virtual epics::pvData::MonitorElementPtr poll();
    virtual void release(epics::pvData::MonitorElementPtr const & monitorElement);

    std::deque<epics::pvData::MonitorElementPtr> buffer, free;
    epics::pvData::MonitorElementPtr overflow;
};

struct TestPV
{
    POINTER_DEFINITIONS(TestPV);
    std::tr1::weak_ptr<TestPV> weakself;

    const std::string name;
    std::tr1::weak_ptr<TestProvider> const provider;

    epics::pvData::PVDataCreatePtr factory;

    const epics::pvData::StructureConstPtr dtype;
    epics::pvData::PVStructurePtr value;

    TestPV(const std::string& name,
           const std::tr1::shared_ptr<TestProvider>& provider,
           const epics::pvData::StructureConstPtr& dtype);
    ~TestPV();

    void post(bool notify = true);
    void post(const epics::pvData::BitSet& changed, bool notify = true);

    void disconnect();

    mutable epicsMutex lock;

    typedef weak_set<TestPVChannel> channels_t;
    channels_t channels;
    friend struct TestProvider;
};

struct TestProvider : public epics::pvAccess::ChannelProvider, std::tr1::enable_shared_from_this<TestProvider>
{
    POINTER_DEFINITIONS(TestProvider);

    virtual std::string getProviderName() { return "TestProvider"; }

    virtual void destroy();

    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(std::string const & channelName,
                                             epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);
    virtual epics::pvAccess::ChannelFind::shared_pointer channelList(epics::pvAccess::ChannelListRequester::shared_pointer const & channelListRequester);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(std::string const & channelName,epics::pvAccess::ChannelRequester::shared_pointer const & channelRequester,
                                           short priority = PRIORITY_DEFAULT);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(std::string const & channelName, epics::pvAccess::ChannelRequester::shared_pointer const & channelRequester,
                                           short priority, std::string const & address);

    TestProvider();
    virtual ~TestProvider();

    TestPV::shared_pointer addPV(const std::string& name, const epics::pvData::StructureConstPtr& tdef);

    void dispatch();

    mutable epicsMutex lock;
    typedef weak_value_map<std::string, TestPV> pvs_t;
    pvs_t pvs;

    static void testCounts();
};

struct TestIOC {
    bool hasInit;
    TestIOC() : hasInit(false) {
        testdbPrepare();
    }
    ~TestIOC() {
        this->shutdown();
        testdbCleanup();
    }
    void init() {
        if(!hasInit) {
            eltc(0);
            testIocInitOk();
            eltc(1);
            hasInit = true;
        }
    }
    void shutdown() {
        if(hasInit) {
            testIocShutdownOk();
            hasInit = false;
        }
    }
};

#endif // UTILITIES_H
