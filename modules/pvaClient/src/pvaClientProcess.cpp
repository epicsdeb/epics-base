/* pvaClientProcess.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.02
 */

#include <pv/event.h>

#define epicsExportSharedSymbols

#include <pv/pvaClient.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvaClient {

class ChannelProcessRequesterImpl : public ChannelProcessRequester
{
    PvaClientProcess::weak_pointer pvaClientProcess;
    PvaClient::weak_pointer pvaClient;
public:
    ChannelProcessRequesterImpl(
        PvaClientProcessPtr const & pvaClientProcess,
        PvaClientPtr const &pvaClient)
    : pvaClientProcess(pvaClientProcess),
      pvaClient(pvaClient)
    {}
    virtual ~ChannelProcessRequesterImpl() {
        if(PvaClient::getDebug()) std::cout << "~ChannelProcessRequesterImpl" << std::endl;
    }

    virtual std::string getRequesterName() {
        PvaClientProcessPtr clientProcess(pvaClientProcess.lock());
        if(!clientProcess) return string("clientProcess is null");
        return clientProcess->getRequesterName();
    }

    virtual void message(std::string const & message, MessageType messageType) {
        PvaClientProcessPtr clientProcess(pvaClientProcess.lock());
        if(!clientProcess) return;
        clientProcess->message(message,messageType);
    }

    virtual void channelProcessConnect(
        const Status& status,
        ChannelProcess::shared_pointer const & channelProcess)
    {
        PvaClientProcessPtr clientProcess(pvaClientProcess.lock());
        if(!clientProcess) return;
        clientProcess->channelProcessConnect(status,channelProcess);
    }

    virtual void processDone(
        const Status& status,
        ChannelProcess::shared_pointer const & ChannelProcess)
    {
        PvaClientProcessPtr clientProcess(pvaClientProcess.lock());
        if(!clientProcess) return;
        clientProcess->processDone(status,ChannelProcess);
    }
};

PvaClientProcessPtr PvaClientProcess::create(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
{
    if(PvaClient::getDebug()) {
         cout<< "PvaClientProcess::create(pvaClient,channelName,pvRequest)\n"
             << " channelName " <<  pvaClientChannel->getChannel()->getChannelName()
             << " pvRequest " << pvRequest
             << endl;
    }
    PvaClientProcessPtr channelProcess(new PvaClientProcess(pvaClient,pvaClientChannel,pvRequest));
    channelProcess->channelProcessRequester = ChannelProcessRequesterImplPtr(
        new ChannelProcessRequesterImpl(channelProcess,pvaClient));
    return channelProcess;
}


PvaClientProcess::PvaClientProcess(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
: pvaClient(pvaClient),
  pvaClientChannel(pvaClientChannel),
  pvRequest(pvRequest),
  connectState(connectIdle),
  processState(processIdle)
{
    if(PvaClient::getDebug()) {
        cout<< "PvaClientProcess::PvaClientProcess()"
            << " channelName " << pvaClientChannel->getChannel()->getChannelName()
            << endl;
    }
}

PvaClientProcess::~PvaClientProcess()
{
    if(PvaClient::getDebug()) {
        cout<< "PvaClientProcess::~PvaClientProcess()"
            << " channelName " << pvaClientChannel->getChannel()->getChannelName()
            << endl;
    }
}

string PvaClientProcess::getRequesterName()
{
     return pvaClientChannel->getRequesterName();
}

void PvaClientProcess::message(string const & message,MessageType messageType)
{
    pvaClientChannel->message(message,messageType);
}

void PvaClientProcess::channelProcessConnect(
    const Status& status,
    ChannelProcess::shared_pointer const & channelProcess)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::channelProcessConnect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << endl;
    }
    {
        Lock xx(mutex);
        channelProcessConnectStatus = status;
        if(status.isOK()) {
            this->channelProcess = channelProcess;
            connectState = connected;
        }    
        waitForConnect.signal();
    }
    PvaClientProcessRequesterPtr  req(pvaClientProcessRequester.lock());
    if(req) {
          req->channelProcessConnect(status,shared_from_this());
    }
}

void PvaClientProcess::processDone(
    const Status& status,
    ChannelProcess::shared_pointer const & channelProcess)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::processDone"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << endl;
    }
    {
        Lock xx(mutex);
        channelProcessStatus = status;
        processState = processComplete;
        waitForProcess.signal();
    }
    PvaClientProcessRequesterPtr  req(pvaClientProcessRequester.lock());
    if(req) {
          req->processDone(status,shared_from_this());
    }
}

void PvaClientProcess::connect()
{
     if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::connect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    issueConnect();
    Status status = waitConnect();
    if(status.isOK()) return;
    string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
        + " PvaClientProcess::connect " + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientProcess::issueConnect()
{
     if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::issueConnect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(connectState!=connectIdle) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " pvaClientProcess already connected ";
        throw std::runtime_error(message);
    }
    connectState = connectActive;
    channelProcessConnectStatus = Status(Status::STATUSTYPE_ERROR, "connect active");
    channelProcess = pvaClientChannel->getChannel()->createChannelProcess(channelProcessRequester,pvRequest);
}

Status PvaClientProcess::waitConnect()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::waitConnect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    waitForConnect.wait();
    return channelProcessConnectStatus;
}

void PvaClientProcess::process()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::process"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    issueProcess();
    Status status = waitProcess();
    if(status.isOK()) return;
    string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
        + " PvaClientProcess::process" + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientProcess::issueProcess()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::issueProcess"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(connectState==connectIdle) connect();
    if(processState==processActive) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " PvaClientProcess::issueProcess process aleady active ";
        throw std::runtime_error(message);
    }
    processState = processActive;
    channelProcess->process();
}

Status PvaClientProcess::waitProcess()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::waitProcess"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    waitForProcess.wait();
    processState = processComplete;
    return channelProcessStatus;
}

void PvaClientProcess::setRequester(PvaClientProcessRequesterPtr const & pvaClientProcessRequester)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientProcess::setRequester"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    this->pvaClientProcessRequester = pvaClientProcessRequester;
}

PvaClientChannelPtr PvaClientProcess::getPvaClientChannel()
{
    return pvaClientChannel;
}


}}
