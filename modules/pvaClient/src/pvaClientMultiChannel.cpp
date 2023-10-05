/* pvaClientMultiChannel.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.02
 */

#include <map>
#include <pv/event.h>
#include <pv/lock.h>

#define epicsExportSharedSymbols

#include <pv/pvaClientMultiChannel.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvaClient {

static FieldCreatePtr fieldCreate = getFieldCreate();
static CreateRequest::shared_pointer  createRequestPvt = CreateRequest::create();

PvaClientMultiChannelPtr PvaClientMultiChannel::create(
   PvaClientPtr const &pvaClient,
   shared_vector<const string> const & channelNames,
   string const & providerName,
   size_t maxNotConnected,
   shared_vector<const string> const & providerNames)
{
    return PvaClientMultiChannelPtr(
        new PvaClientMultiChannel(
        pvaClient,channelNames,providerName,maxNotConnected,providerNames));
}


PvaClientMultiChannel::PvaClientMultiChannel(
    PvaClientPtr const &pvaClient,
    shared_vector<const string> const & channelNames,
    string const & providerName,
    size_t maxNotConnected,
    shared_vector<const string> const & providerNames)
: pvaClient(pvaClient),
  channelNames(channelNames),
  providerName(providerName),
  maxNotConnected(maxNotConnected),
  providerNames(providerNames),
  numChannel(channelNames.size()),
  numProviderNames(providerNames.size()),
  numConnected(0),
  firstConnect(true),
  pvaClientChannelArray(PvaClientChannelArray(numChannel,PvaClientChannelPtr())),
  isConnected(shared_vector<epics::pvData::boolean>(numChannel,false)),
  createRequest(CreateRequest::create())
{
    if(PvaClient::getDebug()) cout<< "PvaClientMultiChannel::PvaClientMultiChannel()\n";
}

PvaClientMultiChannel::~PvaClientMultiChannel()
{
    if(PvaClient::getDebug()) cout<< "PvaClientMultiChannel::~PvaClientMultiChannel()\n";
}

void PvaClientMultiChannel::checkConnected()
{
    if(firstConnect) {
        connect();
        firstConnect = false;
    }
}

shared_vector<const string> PvaClientMultiChannel::getChannelNames()
{
    return channelNames;
}

Status PvaClientMultiChannel::connect(double timeout)
{
    if(!firstConnect) return Status::Ok;
    firstConnect = false;
    for(size_t i=0; i< numChannel; ++i) {
        if(numProviderNames<=i) {
            pvaClientChannelArray[i] = pvaClient->createChannel(channelNames[i],providerName);
        } else {
            pvaClientChannelArray[i] = pvaClient->createChannel(channelNames[i],providerNames[i]);
        }    
        pvaClientChannelArray[i]->issueConnect();
    }
    Status returnStatus = Status::Ok;
    Status status = Status::Ok;
    size_t numBad = 0;
    for(size_t i=0; i< numChannel; ++i) {
        if(numBad==0) {
            status = pvaClientChannelArray[i]->waitConnect(timeout);
        } else {
            status = pvaClientChannelArray[i]->waitConnect(.001);
        }
        if(status.isOK()) {
            ++numConnected;
            isConnected[i] = true;
        } else {
            if(returnStatus.isOK()) returnStatus = status;
            ++numBad;
        }
    }
    return numBad>maxNotConnected ? returnStatus : Status::Ok;
}


bool PvaClientMultiChannel::allConnected()
{
    return (numConnected==numChannel) ? true : false;
}

bool PvaClientMultiChannel::connectionChange()
{
    for(size_t i=0; i<numChannel; ++i) {
         PvaClientChannelPtr pvaClientChannel = pvaClientChannelArray[i];
         Channel::shared_pointer channel = pvaClientChannel->getChannel();
         Channel::ConnectionState stateNow = channel->getConnectionState();
         bool connectedNow = stateNow==Channel::CONNECTED ? true : false;
         if(connectedNow!=isConnected[i]) return true;
    }
    return false;
}

shared_vector<epics::pvData::boolean>  PvaClientMultiChannel::getIsConnected()
{
    for(size_t i=0; i<numChannel; ++i) {
         PvaClientChannelPtr pvaClientChannel = pvaClientChannelArray[i];
         if(!pvaClientChannel) {
              isConnected[i] = false;
              continue;
         }
         Channel::shared_pointer channel = pvaClientChannel->getChannel();
         Channel::ConnectionState stateNow = channel->getConnectionState();
         isConnected[i] = (stateNow==Channel::CONNECTED) ? true : false;
    }
    return isConnected;
}

PvaClientChannelArray PvaClientMultiChannel::getPvaClientChannelArray()
{
    return pvaClientChannelArray;
}

PvaClientPtr PvaClientMultiChannel::getPvaClient()
{
    return pvaClient;
}



PvaClientMultiGetDoublePtr PvaClientMultiChannel::createGet()
{
    checkConnected();
    return PvaClientMultiGetDouble::create(shared_from_this(),pvaClientChannelArray);
}


PvaClientMultiPutDoublePtr PvaClientMultiChannel::createPut()
{
    checkConnected();
    return PvaClientMultiPutDouble::create(shared_from_this(),pvaClientChannelArray);
}


PvaClientMultiMonitorDoublePtr PvaClientMultiChannel::createMonitor()
{
    checkConnected();
     return PvaClientMultiMonitorDouble::create(shared_from_this(), pvaClientChannelArray);
}

PvaClientNTMultiPutPtr PvaClientMultiChannel::createNTPut()
{
    checkConnected();
    return PvaClientNTMultiPut::create(shared_from_this(), pvaClientChannelArray);
}


PvaClientNTMultiGetPtr PvaClientMultiChannel::createNTGet(std::string const &request)
{
    checkConnected();
    PVStructurePtr pvRequest = createRequest->createRequest(request);
    if(!pvRequest) {
        string message = " PvaClientMultiChannel::createNTGet invalid pvRequest: "
             + createRequest->getMessage();
        throw std::runtime_error(message);
    }
    return PvaClientNTMultiGet::create(shared_from_this(), pvaClientChannelArray,pvRequest);
}

PvaClientNTMultiMonitorPtr PvaClientMultiChannel::createNTMonitor(std::string const &request)
{
    checkConnected();
    PVStructurePtr pvRequest = createRequest->createRequest(request);
    if(!pvRequest) {
        string message = " PvaClientMultiChannel::createNTMonitor invalid pvRequest: "
             + createRequest->getMessage();
        throw std::runtime_error(message);
    }
    return PvaClientNTMultiMonitor::create(shared_from_this(), pvaClientChannelArray,pvRequest);
}


}}
