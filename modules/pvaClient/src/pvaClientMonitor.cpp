/* pvaClientMonitor.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.03
 */

#include <sstream>
#include <pv/event.h>
#include <pv/bitSetUtil.h>

#define epicsExportSharedSymbols

#include <pv/pvaClient.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvaClient {

class MonitorRequesterImpl : public MonitorRequester
{
    PvaClientMonitor::weak_pointer pvaClientMonitor;
    PvaClient::weak_pointer pvaClient;
public:
    MonitorRequesterImpl(
        PvaClientMonitorPtr const & pvaClientMonitor,
        PvaClientPtr const &pvaClient)
    : pvaClientMonitor(pvaClientMonitor),
      pvaClient(pvaClient)
    {}
    virtual ~MonitorRequesterImpl() {
        if(PvaClient::getDebug()) std::cout << "~MonitorRequesterImpl" << std::endl;
    }

    virtual std::string getRequesterName() {
        PvaClientMonitorPtr clientMonitor(pvaClientMonitor.lock());
        if(!clientMonitor) return string("pvaClientMonitor is null");
        return clientMonitor->getRequesterName();
    }

    virtual void message(std::string const & message, MessageType messageType) {
        PvaClientMonitorPtr clientMonitor(pvaClientMonitor.lock());
        if(!clientMonitor) return;
        clientMonitor->message(message,messageType);
    }

    virtual void monitorConnect(
        const Status& status,
        Monitor::shared_pointer const & monitor,
        Structure::const_shared_pointer const & structure)
    {
        PvaClientMonitorPtr clientMonitor(pvaClientMonitor.lock());
        if(!clientMonitor) return;
        clientMonitor->monitorConnect(status,monitor,structure);
    }

    virtual void unlisten(MonitorPtr const & monitor)
    {
        PvaClientMonitorPtr clientMonitor(pvaClientMonitor.lock());
        if(!clientMonitor) return;
        clientMonitor->unlisten(monitor);
    }

    virtual void monitorEvent(MonitorPtr const & monitor)
    {
        PvaClientMonitorPtr clientMonitor(pvaClientMonitor.lock());
        if(!clientMonitor) return;
        clientMonitor->monitorEvent(monitor);
    }
};


PvaClientMonitorPtr PvaClientMonitor::create(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
{
    PvaClientMonitorPtr clientMonitor(new PvaClientMonitor(pvaClient,pvaClientChannel,pvRequest));
    clientMonitor->monitorRequester = MonitorRequesterImplPtr(
        new MonitorRequesterImpl(clientMonitor,pvaClient));
    return clientMonitor;
}

PvaClientMonitorPtr PvaClientMonitor::create(
        PvaClientPtr const &pvaClient,
        std::string const & channelName,
        std::string const & providerName,
        std::string const & request,
        PvaClientChannelStateChangeRequesterPtr const & stateChangeRequester,
        PvaClientMonitorRequesterPtr const & monitorRequester)
{
    if(PvaClient::getDebug()) {
         cout<< "PvaClientMonitor::create(pvaClient,channelName,providerName,request,stateChangeRequester,monitorRequester)\n"
             << " channelName " <<  channelName
             << " providerName " <<  providerName
             << " request " << request
             << endl;
    }
    CreateRequest::shared_pointer createRequest(CreateRequest::create());
    PVStructurePtr pvRequest(createRequest->createRequest(request));
    if(!pvRequest) throw std::runtime_error(createRequest->getMessage());
    PvaClientChannelPtr pvaClientChannel = pvaClient->createChannel(channelName,providerName);
    PvaClientMonitorPtr clientMonitor(new PvaClientMonitor(pvaClient,pvaClientChannel,pvRequest));
    clientMonitor->monitorRequester = MonitorRequesterImplPtr(
        new MonitorRequesterImpl(clientMonitor,pvaClient));
    if(stateChangeRequester) clientMonitor->pvaClientChannelStateChangeRequester = stateChangeRequester;
    if(monitorRequester) clientMonitor->pvaClientMonitorRequester = monitorRequester;
    pvaClientChannel->setStateChangeRequester(clientMonitor);
    pvaClientChannel->issueConnect();
    return clientMonitor;
}


PvaClientMonitor::PvaClientMonitor(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
: pvaClient(pvaClient),
  pvaClientChannel(pvaClientChannel),
  pvRequest(pvRequest),
  isStarted(false),
  connectState(connectIdle),
  userPoll(false),
  userWait(false)
{
    if(PvaClient::getDebug()) {
         cout<< "PvaClientMonitor::PvaClientMonitor\n"
             << " channelName " << pvaClientChannel->getChannel()->getChannelName()
             << endl;
    }
}

PvaClientMonitor::~PvaClientMonitor()
{
    if(PvaClient::getDebug()) {
        cout<< "PvaClientMonitor::~PvaClientMonitor"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(monitor) {
       if(isStarted) monitor->stop();
    }
}

void PvaClientMonitor::channelStateChange(PvaClientChannelPtr const & channel, bool isConnected)
{
    if(PvaClient::getDebug()) {
           cout<< "PvaClientMonitor::channelStateChange"
               << " channelName " << channel->getChannelName()
               << " isConnected " << (isConnected ? "true" : "false")
               << endl;
    }
    if(isConnected&&!monitor)
    {
        connectState = connectActive;
        monitor = pvaClientChannel->getChannel()->createMonitor(monitorRequester,pvRequest);
    }
    PvaClientChannelStateChangeRequesterPtr req(pvaClientChannelStateChangeRequester.lock());
    if(req) {
          req->channelStateChange(channel,isConnected);
    }
}

void PvaClientMonitor::event(PvaClientMonitorPtr const & monitor)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::event"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    PvaClientMonitorRequesterPtr req(pvaClientMonitorRequester.lock());
    if(req) req->event(monitor);
}

void PvaClientMonitor::checkMonitorState()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::checkMonitorState"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " connectState " << connectState
           << endl;
    }
    if(connectState==connectIdle) {
         connect();
         if(!isStarted) start();
         return;
    }
    if(connectState==connectActive){
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " "
            + monitorConnectStatus.getMessage();
        throw std::runtime_error(message);
    }
}

string PvaClientMonitor::getRequesterName()
{
     return pvaClientChannel->getRequesterName();
}

void PvaClientMonitor::message(string const & message,MessageType messageType)
{
    pvaClientChannel->message(message,messageType);
}

void PvaClientMonitor::monitorConnect(
    const Status& status,
    Monitor::shared_pointer const & monitor,
    StructureConstPtr const & structure)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::monitorConnect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << endl;
    }
    {
        Lock xx(mutex);
        monitorConnectStatus = status;
        if(status.isOK()) {
            this->monitor = monitor;
        } else {
             stringstream ss;
             ss << pvRequest;
             string message = string("\nPvaClientMonitor::monitorConnect)")
               + "\nchannelName=" + pvaClientChannel->getChannel()->getChannelName()
               + "\npvRequest\n" + ss.str()
               + "\nerror\n" + status.getMessage();
             monitorConnectStatus = Status(Status::STATUSTYPE_ERROR,message);
             waitForConnect.signal();
             PvaClientMonitorRequesterPtr req(pvaClientMonitorRequester.lock());
             if(req) req->monitorConnect(status,shared_from_this(),structure);
             return;
        }
    }
    bool signal = (connectState==connectWait) ? true : false;
    connectState = connected;
    if(isStarted) {
        if(PvaClient::getDebug()) {
            cout << "PvaClientMonitor::monitorConnect"
               << " channelName " <<  pvaClientChannel->getChannel()->getChannelName()
               << " is already started "
               << endl;
        }
        waitForConnect.signal();
        PvaClientMonitorRequesterPtr req(pvaClientMonitorRequester.lock());
        if(req) req->monitorConnect(status,shared_from_this(),structure);
        return;
    }
    pvaClientData = PvaClientMonitorData::create(structure);
    pvaClientData->setMessagePrefix(pvaClientChannel->getChannel()->getChannelName());
    if(signal) {
        if(PvaClient::getDebug()) {
             cout << "PvaClientMonitor::monitorConnect calling waitForConnect.signal\n";
        }
        waitForConnect.signal();
        if(PvaClient::getDebug()) {
             cout << "PvaClientMonitor::monitorConnect calling start\n";
        }
        start();
    } else {
        if(PvaClient::getDebug()) {
             cout << "PvaClientMonitor::monitorConnect calling start\n";
        }
        start();
    }
    PvaClientMonitorRequesterPtr req(pvaClientMonitorRequester.lock());
    if(req) req->monitorConnect(status,shared_from_this(),structure);
}

void PvaClientMonitor::monitorEvent(MonitorPtr const & monitor)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::monitorEvent"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    PvaClientMonitorRequesterPtr req = pvaClientMonitorRequester.lock();
    if(req) req->event(shared_from_this());
    if(userWait) waitForEvent.signal();
}

void PvaClientMonitor::unlisten(MonitorPtr const & monitor)
{
    if(PvaClient::getDebug()) cout << "PvaClientMonitor::unlisten\n";
    PvaClientMonitorRequesterPtr req = pvaClientMonitorRequester.lock();
    if(req) {
        req->unlisten();
    }
}


void PvaClientMonitor::connect()
{
    if(PvaClient::getDebug()) cout << "PvaClientMonitor::connect\n";
    issueConnect();
    Status status = waitConnect();
    if(status.isOK()) return;
    string message = string("channel ")
        + pvaClientChannel->getChannel()->getChannelName()
        + " PvaClientMonitor::connect "
        + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientMonitor::issueConnect()
{
    if(PvaClient::getDebug()) cout << "PvaClientMonitor::issueConnect\n";
    if(connectState!=connectIdle) {
        string message = string("channel ")
            + pvaClientChannel->getChannel()->getChannelName()
            + " pvaClientMonitor already connected ";
        throw std::runtime_error(message);
    }
    connectState = connectWait;
    monitor = pvaClientChannel->getChannel()->createMonitor(monitorRequester,pvRequest);
}

Status PvaClientMonitor::waitConnect()
{
    if(PvaClient::getDebug()) {
       cout << "PvaClientMonitor::waitConnect "
         << pvaClientChannel->getChannel()->getChannelName()
         << endl;
    }
    waitForConnect.wait();
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::waitConnect"
             << " monitorConnectStatus " << (monitorConnectStatus.isOK() ? "connected" : "not connected")
             << endl;
    }
    return monitorConnectStatus;
}

void PvaClientMonitor::setRequester(PvaClientMonitorRequesterPtr const & pvaClientMonitorRequester)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::setRequester"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    this->pvaClientMonitorRequester = pvaClientMonitorRequester;
}

void PvaClientMonitor::start()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::start"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " connectState " << connectState
           << endl;
    }
    if(isStarted) {
        return;
    }
    if(connectState==connectIdle) connect();
    if(connectState!=connected) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " PvaClientMonitor::start illegal state ";
        throw std::runtime_error(message);
    }
    isStarted = true;
    monitor->start();
}

void PvaClientMonitor::start(string const & request)
{
    if(PvaClient::getDebug()) {
        cout<< "PvaMonitor::start(request)"
            << " request " << request
            << endl;
    }
    PvaClientPtr client(pvaClient.lock());
    if(!client) throw std::runtime_error("pvaClient was deleted");
    if(!pvaClientChannel->getChannel()->isConnected()) {
        client->message(
             "PvaClientMonitor::start(request) but not connected",
             errorMessage);
        return;
    }
    CreateRequest::shared_pointer createRequest(CreateRequest::create());
    PVStructurePtr pvr(createRequest->createRequest(request));
    if(!pvr) throw std::runtime_error(createRequest->getMessage());
    if(monitor) {
       if(isStarted) monitor->stop();
    }
    monitorRequester.reset();
    monitor.reset();
    isStarted = false;
    connectState = connectIdle;
    userPoll = false;
    userWait = false;
    monitorRequester = MonitorRequesterImplPtr(
        new MonitorRequesterImpl(shared_from_this(),client));
    pvRequest = pvr;
    connect();
}


void PvaClientMonitor::stop()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::stop"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(!isStarted) return;
    isStarted = false;
    monitor->stop();
}

bool PvaClientMonitor::poll()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::poll"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    checkMonitorState();
    monitorElement = monitor->poll();
    if(!monitorElement) return false;
    userPoll = true;
    pvaClientData->setData(monitorElement);
    return true;
}

bool PvaClientMonitor::waitEvent(double secondsToWait)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::waitEvent"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(!isStarted) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " PvaClientMonitor::waitEvent illegal state ";
        throw std::runtime_error(message);
    }
    if(poll()) return true;
    userWait = true;
    if(secondsToWait==0.0) {
        waitForEvent.wait();
    } else {
        waitForEvent.wait(secondsToWait);
    }
    userWait = false;
    return poll();
}

void PvaClientMonitor::releaseEvent()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::releaseEvent"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(!userPoll) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " PvaClientMonitor::releaseEvent did not call poll";
        throw std::runtime_error(message);
    }
    userPoll = false;
    monitor->release(monitorElement);
}

PvaClientChannelPtr PvaClientMonitor::getPvaClientChannel()
{
    return pvaClientChannel;
}

PvaClientMonitorDataPtr PvaClientMonitor::getData()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientMonitor::getData"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    checkMonitorState();
    return pvaClientData;
}


}}
