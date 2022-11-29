/* pvaClientNTMultiMonitor.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.03
 */

#include <epicsThread.h>
#include <pv/standardField.h>
#include <pv/convert.h>
#include <epicsMath.h>

#define epicsExportSharedSymbols

#include <pv/pvaClientMultiChannel.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::nt;
using namespace std;

namespace epics { namespace pvaClient {

PvaClientNTMultiMonitorPtr PvaClientNTMultiMonitor::create(
    PvaClientMultiChannelPtr const &pvaMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         PVStructurePtr const &  pvRequest)
{
    UnionConstPtr u = getFieldCreate()->createVariantUnion();
    PvaClientNTMultiMonitorPtr pvaClientNTMultiMonitor(
         new PvaClientNTMultiMonitor(u,pvaMultiChannel,pvaClientChannelArray,pvRequest));
    return pvaClientNTMultiMonitor;
}

PvaClientNTMultiMonitor::PvaClientNTMultiMonitor(
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
    if(PvaClient::getDebug()) cout<< "PvaClientNTMultiMonitor::PvaClientNTMultiMonitor()\n";
}


PvaClientNTMultiMonitor::~PvaClientNTMultiMonitor()
{
    if(PvaClient::getDebug()) cout<< "PvaClientNTMultiMonitor::~PvaClientNTMultiMonitor()\n";
}


void PvaClientNTMultiMonitor::connect()
{
    pvaClientMonitor.resize(nchannel);
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               pvaClientMonitor[i] = pvaClientChannelArray[i]->createMonitor(pvRequest);
               pvaClientMonitor[i]->issueConnect();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientMonitor[i]->waitConnect();
               if(status.isOK()) continue;
               string message = string("channel ") +pvaClientChannelArray[i]->getChannelName()
                    + " PvaChannelMonitor::waitConnect " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) pvaClientMonitor[i]->start();
    }
    this->isConnected = true;
}

bool PvaClientNTMultiMonitor::poll(bool valueOnly)
{
    if(!isConnected) connect();
    bool result = false;
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected(); 
    pvaClientNTMultiData->startDeltaTime();  
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
             if(!pvaClientMonitor[i]){
                  pvaClientMonitor[i]=pvaClientChannelArray[i]->createMonitor(pvRequest);
                  pvaClientMonitor[i]->connect();
                  pvaClientMonitor[i]->start();
              }    
              if(pvaClientMonitor[i]->poll()) {  
                   pvaClientNTMultiData->setPVStructure(
                       pvaClientMonitor[i]->getData()->getPVStructure(),i);    
                   pvaClientMonitor[i]->releaseEvent();
                   result = true;
              }
         }
    } 
    if(result) pvaClientNTMultiData->endDeltaTime(valueOnly);  
    return result;
}

bool PvaClientNTMultiMonitor::waitEvent(double waitForEvent)
{
    if(poll()) return true;
    TimeStamp start;
    start.getCurrent();
    TimeStamp now;
    while(true) {
          epicsThreadSleep(.1);
          if(poll()) return true;
          now.getCurrent();
          double diff = TimeStamp::diff(now,start);
          if(diff>=waitForEvent) break;
    }
    return false;
}

PvaClientNTMultiDataPtr PvaClientNTMultiMonitor::getData()
{
    return pvaClientNTMultiData;
}

}}
