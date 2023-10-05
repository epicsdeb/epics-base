/* pvaClientGet.cpp */
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

class ChannelGetRequesterImpl : public ChannelGetRequester
{
    PvaClientGet::weak_pointer pvaClientGet;
    PvaClient::weak_pointer pvaClient;
public:
    ChannelGetRequesterImpl(
        PvaClientGetPtr const & pvaClientGet,
        PvaClientPtr const &pvaClient)
    : pvaClientGet(pvaClientGet),
      pvaClient(pvaClient)
    {}
    virtual ~ChannelGetRequesterImpl() {
        if(PvaClient::getDebug()) std::cout << "~ChannelGetRequesterImpl" << std::endl;
    }

    virtual std::string getRequesterName() {
        PvaClientGetPtr clientGet(pvaClientGet.lock());
        if(!clientGet) return string("clientGet is null");
        return clientGet->getRequesterName();
    }

    virtual void message(std::string const & message, MessageType messageType) {
        PvaClientGetPtr clientGet(pvaClientGet.lock());
        if(!clientGet) return;
        clientGet->message(message,messageType);
    }

    virtual void channelGetConnect(
        const Status& status,
        ChannelGet::shared_pointer const & channelGet,
        Structure::const_shared_pointer const & structure)
    {
        PvaClientGetPtr clientGet(pvaClientGet.lock());
        if(!clientGet) return;
        clientGet->channelGetConnect(status,channelGet,structure);
    }

    virtual void getDone(
        const Status& status,
        ChannelGet::shared_pointer const & channelGet,
        PVStructurePtr const & pvStructure,
        BitSet::shared_pointer const & bitSet)
    {
        PvaClientGetPtr clientGet(pvaClientGet.lock());
        if(!clientGet) return;
        clientGet->getDone(status,channelGet,pvStructure,bitSet);
    }
};

PvaClientGetPtr PvaClientGet::create(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
{
    if(PvaClient::getDebug()) {
         cout<< "PvaClientGet::create(pvaClient,channelName,pvRequest)\n"
             << " channelName " <<  pvaClientChannel->getChannel()->getChannelName()
             << " pvRequest " << pvRequest
             << endl;
    }
    PvaClientGetPtr clientGet(new PvaClientGet(pvaClient,pvaClientChannel,pvRequest));
    clientGet->channelGetRequester = ChannelGetRequesterImplPtr(
        new ChannelGetRequesterImpl(clientGet,pvaClient));
    return clientGet;
}

PvaClientGet::PvaClientGet(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
: pvaClient(pvaClient),
  pvaClientChannel(pvaClientChannel),
  pvRequest(pvRequest),
  connectState(connectIdle),
  getState(getIdle)
{
     if(PvaClient::getDebug()) {
        cout << "PvaClientGet::PvaClientGet channelName "
             << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
}

PvaClientGet::~PvaClientGet()
{
    if(PvaClient::getDebug()) {
        cout<< "PvaClientGet::~PvaClientGet channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
}


void PvaClientGet::checkConnectState()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::checkConnectState channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    if(!pvaClientChannel->getChannel()->isConnected()) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " PvaClientGet::checkConnectState channel not connected ";
        throw std::runtime_error(message);
    }
    if(connectState==connectIdle) {
        connect();
    }
    if(connectState==connectActive){
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " "
            + channelGetConnectStatus.getMessage();
        throw std::runtime_error(message);
    }
}

string PvaClientGet::getRequesterName()
{
    return pvaClientChannel->getRequesterName();
}

void PvaClientGet::message(string const & message,MessageType messageType)
{
    pvaClientChannel->message(message,messageType);
}

void PvaClientGet::channelGetConnect(
    const Status& status,
    ChannelGet::shared_pointer const & channelGet,
    StructureConstPtr const & structure)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::channelGetConnect channelName "
           << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << "\n";
    }
    {
        Lock xx(mutex);
        channelGetConnectStatus = status;
        if(status.isOK()) {
            this->channelGet = channelGet;
            connectState = connected;
            pvaClientData = PvaClientGetData::create(structure);
            pvaClientData->setMessagePrefix(channelGet->getChannel()->getChannelName());
        }    
        waitForConnect.signal();
    }
    PvaClientGetRequesterPtr  req(pvaClientGetRequester.lock());
    if(req) {
          req->channelGetConnect(status,shared_from_this());
    }    
}

void PvaClientGet::getDone(
    const Status& status,
    ChannelGet::shared_pointer const & channelGet,
    PVStructurePtr const & pvStructure,
    BitSetPtr const & bitSet)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::getDone channelName "
           << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << "\n";
    }
    {
        Lock xx(mutex);
        channelGetStatus = status;
        if(status.isOK()) {
            pvaClientData->setData(pvStructure,bitSet);
        }
        getState = getComplete;
        waitForGet.signal();
    }
    PvaClientGetRequesterPtr  req(pvaClientGetRequester.lock());
    if(req) {
          req->getDone(status,shared_from_this());
    }
}

void PvaClientGet::connect()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::connect channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    issueConnect();
    Status status = waitConnect();
    if(status.isOK()) return;
    string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
         + " PvaClientGet::connect " + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientGet::issueConnect()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::issueConnect channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    if(connectState!=connectIdle) {
        string message = string("channel ")  + pvaClientChannel->getChannel()->getChannelName()
            + " pvaClientGet already connected ";
        throw std::runtime_error(message);
    }
    connectState = connectActive;
    channelGetConnectStatus = Status(Status::STATUSTYPE_ERROR, "connect active");
    channelGet = pvaClientChannel->getChannel()->createChannelGet(channelGetRequester,pvRequest);
}

Status PvaClientGet::waitConnect()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::waitConnect channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    waitForConnect.wait();
    return channelGetConnectStatus;
}

void PvaClientGet::get()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::get channelName "
          << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    issueGet();
    Status status = waitGet();
    if(status.isOK()) return;
    string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " PvaClientGet::get " + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientGet::issueGet()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::issueGet channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    if(connectState==connectIdle) connect();
    if(getState==getActive) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " PvaClientGet::issueGet get aleady active ";
        throw std::runtime_error(message);
    }
    getState = getActive;
    channelGet->get();
}

Status PvaClientGet::waitGet()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::waitGet channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    waitForGet.wait();
    return channelGetStatus;
}
PvaClientGetDataPtr PvaClientGet::getData()
{
    if(PvaClient::getDebug()) {
           cout<< "PvaClientGet::getData  channelName "
               << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    checkConnectState();
    if(getState==getIdle) get();
    return pvaClientData;
}

void PvaClientGet::setRequester(PvaClientGetRequesterPtr const & pvaClientGetRequester)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientGet::setRequester channelName "
           << pvaClientChannel->getChannel()->getChannelName() << "\n";
    }
    this->pvaClientGetRequester = pvaClientGetRequester;
}

PvaClientChannelPtr PvaClientGet::getPvaClientChannel()
{
    return pvaClientChannel;
}



}}
