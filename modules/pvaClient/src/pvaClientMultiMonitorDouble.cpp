/* pvaClientMultiMonitorDouble.cpp */
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
#include <epicsMath.h>

#define epicsExportSharedSymbols

#include <pv/pvaClientMultiChannel.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::nt;
using namespace std;

namespace epics { namespace pvaClient {


PvaClientMultiMonitorDoublePtr PvaClientMultiMonitorDouble::create(
    PvaClientMultiChannelPtr const &pvaMultiChannel,
    PvaClientChannelArray const &pvaClientChannelArray)
{
    PvaClientMultiMonitorDoublePtr pvaClientMultiMonitorDouble(
         new PvaClientMultiMonitorDouble(pvaMultiChannel,pvaClientChannelArray));
    return pvaClientMultiMonitorDouble;
}

PvaClientMultiMonitorDouble::PvaClientMultiMonitorDouble(
     PvaClientMultiChannelPtr const &pvaClientMultiChannel,
     PvaClientChannelArray const &pvaClientChannelArray)
: pvaClientMultiChannel(pvaClientMultiChannel),
  pvaClientChannelArray(pvaClientChannelArray),
  nchannel(pvaClientChannelArray.size()),
  doubleValue(shared_vector<double>(nchannel,epicsNAN)),
  pvaClientMonitor(std::vector<PvaClientMonitorPtr>(nchannel,PvaClientMonitorPtr())),
  isMonitorConnected(false)
{
     if(PvaClient::getDebug()) cout<< "PvaClientMultiMonitorDouble::PvaClientMultiMonitorDouble()\n";
}

PvaClientMultiMonitorDouble::~PvaClientMultiMonitorDouble()
{
    if(PvaClient::getDebug()) cout<< "PvaClientMultiMonitorDouble::~PvaClientMultiMonitorDouble()\n";
}

void PvaClientMultiMonitorDouble::connect()
{
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    string request = "value";
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               pvaClientMonitor[i] = pvaClientChannelArray[i]->createMonitor(request);
               pvaClientMonitor[i]->issueConnect();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientMonitor[i]->waitConnect();
               if(status.isOK()) continue;
               string message = string("channel ") + pvaClientChannelArray[i]->getChannelName()
                   + " PvaChannelMonitor::waitConnect " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
     for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) pvaClientMonitor[i]->start();
    }
    isMonitorConnected = true;
}

bool PvaClientMultiMonitorDouble::poll()
{
    if(!isMonitorConnected){
         connect();
         epicsThreadSleep(.1);
    }
    bool result = false;
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
              if(!pvaClientMonitor[i]){
                  pvaClientMonitor[i] = pvaClientChannelArray[i]->createMonitor("value");
                  pvaClientMonitor[i]->issueConnect();
                  Status status = pvaClientMonitor[i]->waitConnect();
                  if(!status.isOK()) {
                     string message = string("channel ") + pvaClientChannelArray[i]->getChannelName()
                        + " PvaChannelMonitor::waitConnect " + status.getMessage();
                     throw std::runtime_error(message);
                  }   
                  pvaClientMonitor[i]->start();
              }    
              if(pvaClientMonitor[i]->poll()) {
                   doubleValue[i] = pvaClientMonitor[i]->getData()->getDouble();
                   pvaClientMonitor[i]->releaseEvent();
                   result = true;
              }
         }
    }
    return result;
}

bool PvaClientMultiMonitorDouble::waitEvent(double waitForEvent)
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

shared_vector<double> PvaClientMultiMonitorDouble::get()
{
    return doubleValue;
}


}}
