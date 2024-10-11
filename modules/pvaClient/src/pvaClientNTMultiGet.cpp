/* pvaClientNTMultiGet.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.03
 */

#include <epicsMath.h>

#define epicsExportSharedSymbols

#include <pv/pvaClientMultiChannel.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::nt;
using namespace std;

namespace epics { namespace pvaClient {


PvaClientNTMultiGetPtr PvaClientNTMultiGet::create(
    PvaClientMultiChannelPtr const &pvaMultiChannel,
    PvaClientChannelArray const &pvaClientChannelArray,
    PVStructurePtr const &  pvRequest)
{
    UnionConstPtr u = getFieldCreate()->createVariantUnion();
    PvaClientNTMultiGetPtr pvaClientNTMultiGet(
         new PvaClientNTMultiGet(u,pvaMultiChannel,pvaClientChannelArray,pvRequest));
    return pvaClientNTMultiGet;
}

PvaClientNTMultiGet::PvaClientNTMultiGet(
         UnionConstPtr const & u,
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         PVStructurePtr const &  pvRequest)
: pvaClientMultiChannel(pvaClientMultiChannel),
  pvaClientChannelArray(pvaClientChannelArray),
  pvRequest(pvRequest),
  nchannel(pvaClientChannelArray.size()),
  pvaClientNTMultiData(
       PvaClientNTMultiData::create(
           u,
           pvaClientMultiChannel,
           pvaClientChannelArray,
           pvRequest)),
  isConnected(false)
{
    if(PvaClient::getDebug()) cout<< "PvaClientNTMultiGet::PvaClientNTMultiGet()\n";
}

PvaClientNTMultiGet::~PvaClientNTMultiGet()
{
    if(PvaClient::getDebug()) cout<< "PvaClientNTMultiGet::~PvaClientNTMultiGet()\n";
}

void PvaClientNTMultiGet::connect()
{
    pvaClientGet.resize(nchannel);
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               pvaClientGet[i] = pvaClientChannelArray[i]->createGet(pvRequest);
               pvaClientGet[i]->issueConnect();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientGet[i]->waitConnect();
               if(status.isOK()) continue;
               string message = string("channel ") +pvaClientChannelArray[i]->getChannelName()
                    + " PvaChannelGet::waitConnect " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    this->isConnected = true;
}

void PvaClientNTMultiGet::get(bool valueOnly)
{
    if(!isConnected) connect();
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();

    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               if(!pvaClientGet[i]){
                   pvaClientGet[i]=pvaClientChannelArray[i]->createGet(pvRequest);
                   pvaClientGet[i]->connect();
               }    
               pvaClientGet[i]->issueGet();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientGet[i]->waitGet();
               if(status.isOK()) continue;
               string message = string("channel ") +pvaClientChannelArray[i]->getChannelName()
                    + " PvaChannelGet::waitGet " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    pvaClientNTMultiData->startDeltaTime();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
              pvaClientNTMultiData->setPVStructure(pvaClientGet[i]->getData()->getPVStructure(),i);
         }
    }
    pvaClientNTMultiData->endDeltaTime(valueOnly);
}

PvaClientNTMultiDataPtr PvaClientNTMultiGet::getData()
{
    return pvaClientNTMultiData;
}

}}
