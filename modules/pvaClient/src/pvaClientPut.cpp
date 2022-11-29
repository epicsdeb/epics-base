/* pvaClientPut.cpp */
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

class ChannelPutRequesterImpl : public ChannelPutRequester
{
    PvaClientPut::weak_pointer pvaClientPut;
    PvaClient::weak_pointer pvaClient;
public:
    ChannelPutRequesterImpl(
        PvaClientPutPtr const & pvaClientPut,
        PvaClientPtr const &pvaClient)
    : pvaClientPut(pvaClientPut),
      pvaClient(pvaClient)
    {}
    virtual ~ChannelPutRequesterImpl() {
        if(PvaClient::getDebug()) std::cout << "~ChannelPutRequesterImpl" << std::endl;
    }

    virtual std::string getRequesterName() {
        PvaClientPutPtr clientPut(pvaClientPut.lock());
        if(!clientPut) return string("clientPut is null");
        return clientPut->getRequesterName();
    }

    virtual void message(std::string const & message, MessageType messageType) {
        PvaClientPutPtr clientPut(pvaClientPut.lock());
        if(!clientPut) return;
        clientPut->message(message,messageType);
    }

    virtual void channelPutConnect(
        const Status& status,
        ChannelPut::shared_pointer const & channelPut,
        Structure::const_shared_pointer const & structure)
    {
        PvaClientPutPtr clientPut(pvaClientPut.lock());
        if(!clientPut) return;
        clientPut->channelPutConnect(status,channelPut,structure);
    }

    virtual void getDone(
        const Status& status,
        ChannelPut::shared_pointer const & channelPut,
        PVStructurePtr const & pvStructure,
        BitSet::shared_pointer const & bitSet)
    {
        PvaClientPutPtr clientPut(pvaClientPut.lock());
        if(!clientPut) return;
        clientPut->getDone(status,channelPut,pvStructure,bitSet);
    }

    virtual void putDone(
        const Status& status,
        ChannelPut::shared_pointer const & channelPut)
    {
        PvaClientPutPtr clientPut(pvaClientPut.lock());
        if(!clientPut) return;
        clientPut->putDone(status,channelPut);
    }
};

PvaClientPutPtr PvaClientPut::create(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
{
    PvaClientPutPtr clientPut(new PvaClientPut(pvaClient,pvaClientChannel,pvRequest));
    clientPut->channelPutRequester = ChannelPutRequesterImplPtr(
        new ChannelPutRequesterImpl(clientPut,pvaClient));
    return clientPut;
}


PvaClientPut::PvaClientPut(
        PvaClientPtr const &pvaClient,
        PvaClientChannelPtr const & pvaClientChannel,
        PVStructurePtr const &pvRequest)
: pvaClient(pvaClient),
  pvaClientChannel(pvaClientChannel),
  pvRequest(pvRequest),
  connectState(connectIdle),
  putState(putIdle)
{
    if(PvaClient::getDebug()) {
         cout<< "PvaClientPut::PvaClientPut"
             << " channelName " <<  pvaClientChannel->getChannel()->getChannelName()
             << endl;
    }
}

PvaClientPut::~PvaClientPut()
{
    if(PvaClient::getDebug()) {
        cout<< "PvaClientPut::~PvaClientPut"
           << " channelName " <<  pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
}


void PvaClientPut::checkConnectState()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::checkConnectState"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(connectState==connectIdle){
          connect();
    }
    if(connectState==connectActive){
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " "
            + channelPutConnectStatus.getMessage();
        throw std::runtime_error(message);
    }
}

string PvaClientPut::getRequesterName()
{
     return pvaClientChannel->getRequesterName();
}

void PvaClientPut::message(string const & message,MessageType messageType)
{
    pvaClientChannel->message(message,messageType);
}

void PvaClientPut::channelPutConnect(
    const Status& status,
    ChannelPut::shared_pointer const & channelPut,
    StructureConstPtr const & structure)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::channelPutConnect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << endl;
    }
    {
        Lock xx(mutex);
        channelPutConnectStatus = status;
        if(status.isOK()) {
            this->channelPut = channelPut;
            connectState = connected;
            pvaClientData = PvaClientPutData::create(structure);
            pvaClientData->setMessagePrefix(channelPut->getChannel()->getChannelName());
        }    
        waitForConnect.signal();
    }
    PvaClientPutRequesterPtr  req(pvaClientPutRequester.lock());
    if(req) {
          req->channelPutConnect(status,shared_from_this());
    }
}

void PvaClientPut::getDone(
    const Status& status,
    ChannelPut::shared_pointer const & channelPut,
    PVStructurePtr const & pvStructure,
    BitSetPtr const & bitSet)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::getDone"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << endl;
    }
    {
        Lock xx(mutex);
        channelGetPutStatus = status;
        if(status.isOK()) {
            PVStructurePtr pvs = pvaClientData->getPVStructure();
            pvs->copyUnchecked(*pvStructure,*bitSet);
            BitSetPtr bs = pvaClientData->getChangedBitSet();
            bs->clear();
            *bs |= *bitSet;
        }    
        putState = putComplete;
        waitForGetPut.signal();
    }
    PvaClientPutRequesterPtr  req(pvaClientPutRequester.lock());
    if(req) {
          req->getDone(status,shared_from_this());
    }
}

void PvaClientPut::putDone(
    const Status& status,
    ChannelPut::shared_pointer const & channelPut)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::putDone"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " status.isOK " << (status.isOK() ? "true" : "false")
           << endl;
    }
    {
        Lock xx(mutex);
        channelGetPutStatus = status;
        putState = putComplete;
        waitForGetPut.signal();
    }
    PvaClientPutRequesterPtr  req(pvaClientPutRequester.lock());
    if(req) { req->putDone(status,shared_from_this());}
}

void PvaClientPut::connect()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::connect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    issueConnect();
    Status status = waitConnect();
    if(status.isOK()) return;
    string message = string("channel ")
        + pvaClientChannel->getChannel()->getChannelName()
        + " PvaClientPut::connect "
        + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientPut::issueConnect()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::issueConnect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(connectState!=connectIdle) {
        string message = string("channel ") + pvaClientChannel->getChannel()->getChannelName()
            + " pvaClientPut already connected ";
        throw std::runtime_error(message);
    }
    connectState = connectActive;
    channelPutConnectStatus = Status(Status::STATUSTYPE_ERROR, "connect active");
    channelPut = pvaClientChannel->getChannel()->createChannelPut(channelPutRequester,pvRequest);

}

Status PvaClientPut::waitConnect()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::waitConnect"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    waitForConnect.wait();
    return channelPutConnectStatus;
}

void PvaClientPut::get()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::get"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    issueGet();
    Status status = waitGet();
    if(status.isOK()) return;
    string message = string("channel ")
        +  pvaClientChannel->getChannel()->getChannelName()
        + " PvaClientPut::get "
        + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientPut::issueGet()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::issueGet"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    if(connectState==connectIdle) connect();
    if(putState==getActive || putState==putActive) {
        string message = string("channel ")
            + pvaClientChannel->getChannel()->getChannelName()
            +  "PvaClientPut::issueGet get or put aleady active ";
        throw std::runtime_error(message);
    }
    putState = getActive;
    channelPut->get();
}

Status PvaClientPut::waitGet()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::waitGet"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    waitForGetPut.wait();
    putState = putComplete;
    return channelGetPutStatus;
}

void PvaClientPut::put()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::put"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    issuePut();
    Status status = waitPut();
    if(status.isOK()) return;
    string message = string("channel ")
        + pvaClientChannel->getChannel()->getChannelName()
        + " PvaClientPut::put "
        + status.getMessage();
    throw std::runtime_error(message);
}

void PvaClientPut::issuePut()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::issuePut"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << " pvStructure\n" << pvaClientData->getPVStructure()
           << " bitSet " << *pvaClientData->getChangedBitSet() << endl
           << endl;
    }   
    if(connectState==connectIdle) connect();
    if(putState==getActive || putState==putActive) {
         string message = string("channel ")
            + pvaClientChannel->getChannel()->getChannelName()
            +  " PvaClientPut::issuePut get or put aleady active ";
         throw std::runtime_error(message);
    }
    putState = putActive;
    channelPut->put(pvaClientData->getPVStructure(),pvaClientData->getChangedBitSet());
}

Status PvaClientPut::waitPut()
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::waitPut"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    waitForGetPut.wait();
    putState = putComplete;
    if(channelGetPutStatus.isOK()) pvaClientData->getChangedBitSet()->clear();
    return channelGetPutStatus;
}

PvaClientPutDataPtr PvaClientPut::getData()
{
    if(PvaClient::getDebug()) {
           cout<< "PvaClientPut::getData"
               << " channelName " << pvaClientChannel->getChannel()->getChannelName()
               << endl;
    }
    checkConnectState();
    if(putState==putIdle) get();
    return pvaClientData;
}

void PvaClientPut::setRequester(PvaClientPutRequesterPtr const & pvaClientPutRequester)
{
    if(PvaClient::getDebug()) {
        cout << "PvaClientPut::setRequester"
           << " channelName " << pvaClientChannel->getChannel()->getChannelName()
           << endl;
    }
    this->pvaClientPutRequester = pvaClientPutRequester;
}

PvaClientChannelPtr PvaClientPut::getPvaClientChannel()
{
    return pvaClientChannel;
}

}}
