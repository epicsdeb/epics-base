/* pvaClientRPC.cpp */
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
#include <pv/rpcService.h>

#define epicsExportSharedSymbols

#include <pv/pvaClient.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvaClient {

class RPCRequesterImpl : public ChannelRPCRequester
{
    PvaClientRPC::weak_pointer pvaClientRPC;
    PvaClient::weak_pointer pvaClient;
public:
    RPCRequesterImpl(
        PvaClientRPCPtr const & pvaClientRPC,
        PvaClientPtr const &pvaClient)
    : pvaClientRPC(pvaClientRPC),
      pvaClient(pvaClient)
    {}
    virtual ~RPCRequesterImpl() {
        if(PvaClient::getDebug()) std::cout << "~RPCRequesterImpl" << std::endl;
    }

    virtual std::string getRequesterName() {
        PvaClientRPCPtr clientRPC(pvaClientRPC.lock());
        if(!clientRPC) return string("pvaClientRPC is null");
        return clientRPC->getRequesterName();
    }

    virtual void message(std::string const & message, MessageType messageType) {
        PvaClientRPCPtr clientRPC(pvaClientRPC.lock());
        if(!clientRPC) return;
        clientRPC->message(message,messageType);
    }

    virtual void channelRPCConnect(
        const Status& status,
        ChannelRPC::shared_pointer const & channelRPC)
    {
        PvaClientRPCPtr clientRPC(pvaClientRPC.lock());
        if(!clientRPC) return;
        clientRPC->rpcConnect(status,channelRPC);
    }

    virtual void requestDone(
        const Status& status,
        ChannelRPC::shared_pointer const & channelRPC,
        PVStructure::shared_pointer const & pvResponse)
    {
        PvaClientRPCPtr clientRPC(pvaClientRPC.lock());
        if(!clientRPC) return;
        clientRPC->requestDone(status,channelRPC,pvResponse);
    }
};

PvaClientRPCPtr PvaClientRPC::create(
        PvaClientPtr const &pvaClient,
        Channel::shared_pointer const & channel)
{
     StructureConstPtr structure(getFieldCreate()->createStructure());
     PVStructurePtr pvRequest(getPVDataCreate()->createPVStructure(structure));
     return create(pvaClient,channel,pvRequest);
}
PvaClientRPCPtr PvaClientRPC::create(
        PvaClientPtr const &pvaClient,
        Channel::shared_pointer const & channel,
        PVStructurePtr const &pvRequest)
{
    PvaClientRPCPtr epv(new PvaClientRPC(pvaClient,channel,pvRequest));
    epv->rpcRequester = RPCRequesterImplPtr(
        new RPCRequesterImpl(epv,pvaClient));
    return epv;
}

PvaClientRPC::PvaClientRPC(
        PvaClientPtr const &pvaClient,
        Channel::shared_pointer const & channel,
        PVStructurePtr const &pvRequest)
:
  connectState(connectIdle),
  pvaClient(pvaClient),
  channel(channel),
  pvRequest(pvRequest),
  rpcState(rpcIdle),
  responseTimeout(0.0)
{
    if(PvaClient::getDebug()) {
         cout<< "PvaClientRPC::PvaClientRPC()"
             << " channelName " << channel->getChannelName()
             << endl;
    }
}

PvaClientRPC::~PvaClientRPC()
{
    if(PvaClient::getDebug()) {
        string channelName("disconnected");
        Channel::shared_pointer chan(channel.lock());
        if(chan) channelName = chan->getChannelName();
        cout<< "PvaClientRPC::~PvaClientRPC"
           << " channelName " << channelName
           << endl;
    }
}

void PvaClientRPC::checkRPCState()
{
    if(PvaClient::getDebug()) {
        string channelName("disconnected");
        Channel::shared_pointer chan(channel.lock());
        if(chan) channelName = chan->getChannelName();
        cout << "PvaClientRPC::checkRPCState"
           << " channelName " << channelName
           << " connectState " << connectState
           << endl;
    }
    if(connectState==connectIdle) connect();
}

string PvaClientRPC::getRequesterName()
{
     PvaClientPtr yyy = pvaClient.lock();
     if(!yyy) return string("PvaClientRPC::getRequesterName() PvaClient isDestroyed");
     return yyy->getRequesterName();
}

void PvaClientRPC::message(string const & message,MessageType messageType)
{
    PvaClientPtr yyy = pvaClient.lock();
    if(!yyy) return;
    yyy->message(message, messageType);
}

void PvaClientRPC::rpcConnect(
    const Status& status,
    ChannelRPC::shared_pointer const & channelRPC)
{
    Channel::shared_pointer chan(channel.lock());
    if(PvaClient::getDebug()) {
        string channelName("disconnected");
        Channel::shared_pointer chan(channel.lock());
        if(chan) channelName = chan->getChannelName();
        cout << "PvaClientRPC::rpcConnect"
           << " channelName " << channelName
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << endl;
    }
    if(!chan) return;
    connectStatus = status;
    connectState = connected;
    if(PvaClient::getDebug()) {
         cout << "PvaClientRPC::rpcConnect calling waitForConnect.signal\n";
    }
    waitForConnect.signal();

}

void PvaClientRPC::requestDone(
        const Status& status,
        ChannelRPC::shared_pointer const & channelRPC,
        PVStructure::shared_pointer const & pvResponse)
{
    PvaClientRPCRequesterPtr req = pvaClientRPCRequester.lock();
    {
        Lock xx(mutex);
        requestStatus = status;
        if(PvaClient::getDebug()) {
             string channelName("disconnected");
             Channel::shared_pointer chan(channel.lock());
             if(chan) channelName = chan->getChannelName();
             cout << "PvaClientRPC::requestDone"
                  << " channelName " << channelName
                  << endl;
        }
        if(rpcState!=rpcActive) {
             string channelName("disconnected");
             Channel::shared_pointer chan(channel.lock());
             if(chan) channelName = chan->getChannelName();
             string message = "channel "
                 + channelName
                 +" PvaClientRPC::requestDone"
                 + " but not active";
             throw std::runtime_error(message);
        }
        if(req && (responseTimeout<=0.0)) {
            rpcState = rpcIdle;
        } else {
            rpcState = rpcComplete;
            if(!req) this->pvResponse = pvResponse;
            waitForDone.signal();
        }
    }
    if(req) {
        req->requestDone(status,shared_from_this(),pvResponse);
    }
}

void PvaClientRPC::connect()
{
    if(PvaClient::getDebug()) cout << "PvaClientRPC::connect\n";
    issueConnect();
    Status status = waitConnect();
    if(status.isOK()) return;
    Channel::shared_pointer chan(channel.lock());
    string channelName("disconnected");
    if(chan) channelName = chan->getChannelName();
    string message = string("channel ")
        + channelName
        + " PvaClientRPC::connect "
        + status.getMessage();
    throw RPCRequestException(Status::STATUSTYPE_ERROR,message);
}

void PvaClientRPC::issueConnect()
{
    if(PvaClient::getDebug()) cout << "PvaClientRPC::issueConnect\n";
    Channel::shared_pointer chan(channel.lock());
    if(connectState!=connectIdle) {
        string channelName("disconnected");
        if(chan) channelName = chan->getChannelName();
        string message = string("channel ")
            + channelName
            + " pvaClientRPC already connected ";
        throw std::runtime_error(message);
    }
    if(chan) {
        connectState = connectActive;
        channelRPC = chan->createChannelRPC(rpcRequester,pvRequest);
        return;
    }
    throw std::runtime_error("PvaClientRPC::issueConnect() but channel disconnected");
}

Status PvaClientRPC::waitConnect()
{
    if(PvaClient::getDebug()) cout << "PvaClientRPC::waitConnect\n";
    if(connectState==connected) {
         if(!connectStatus.isOK()) connectState = connectIdle;
         return connectStatus;
    }
    if(connectState!=connectActive) {
        Channel::shared_pointer chan(channel.lock());
        string channelName("disconnected");
        if(chan) channelName = chan->getChannelName();
        string message = string("channel ")
            + channelName
            + " PvaClientRPC::waitConnect illegal connect state ";
        throw std::runtime_error(message);
    }
    if(PvaClient::getDebug()) {
        cout << "PvaClientRPC::waitConnect calling waitForConnect.wait\n";
    }
    waitForConnect.wait();
    connectState = connectStatus.isOK() ? connected : connectIdle;
    if(PvaClient::getDebug()) {
        cout << "PvaClientRPC::waitConnect"
             << " connectStatus " << (connectStatus.isOK() ? "connected" : "not connected");
    }
    return connectStatus;
}

PVStructure::shared_pointer PvaClientRPC::request(PVStructure::shared_pointer const & pvArgument)
{
    checkRPCState();
    {
        Lock xx(mutex);
        if(rpcState!=rpcIdle) {
            Channel::shared_pointer chan(channel.lock());
            string channelName("disconnected");
            if(chan) channelName = chan->getChannelName();
            string message = "channel "
                + channelName
                + " PvaClientRPC::request request aleady active ";
            throw std::runtime_error(message);
        }
        rpcState = rpcActive;
    }
    channelRPC->request(pvArgument);
    if(responseTimeout>0.0) {
        waitForDone.wait(responseTimeout);
    } else {
        waitForDone.wait();
    }
    Lock xx(mutex);
    if(rpcState!=rpcComplete) {
        Channel::shared_pointer chan(channel.lock());
            string channelName("disconnected");
            if(chan) channelName = chan->getChannelName();
        string message = "channel "
            + channelName
            + " PvaClientRPC::request request timeout ";
        throw RPCRequestException(Status::STATUSTYPE_ERROR,message);
    }
    rpcState = rpcIdle;
    if(requestStatus.isOK()) return pvResponse;
    Channel::shared_pointer chan(channel.lock());
    string channelName("disconnected");
    if(chan) channelName = chan->getChannelName();
    string message = "channel "
            + channelName
            + " PvaClientRPC::request status ";
    message += requestStatus.getMessage();
    throw RPCRequestException(Status::STATUSTYPE_ERROR,message);
}


void PvaClientRPC::request(
    PVStructure::shared_pointer const & pvArgument,
    PvaClientRPCRequesterPtr const & pvaClientRPCRequester)
{
    checkRPCState();
    this->pvaClientRPCRequester = pvaClientRPCRequester;
    if(responseTimeout<=0.0) {
         {
             Lock xx(mutex);
             if(rpcState!=rpcIdle) {
                 Channel::shared_pointer chan(channel.lock());
                 string channelName("disconnected");
                 if(chan) channelName = chan->getChannelName();
                 string message = "channel "
                    + channelName
                    + " PvaClientRPC::request request aleady active ";
                 throw std::runtime_error(message);
            }
            rpcState = rpcActive;
        }
        channelRPC->request(pvArgument);
        return;
    }
    request(pvArgument);
}


}}
