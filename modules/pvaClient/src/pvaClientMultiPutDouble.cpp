/* pvaClientMultiPutDouble.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.03
 */

#include <pv/convert.h>
#include <epicsMath.h>

#define epicsExportSharedSymbols

#include <pv/pvaClientMultiChannel.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::nt;
using namespace std;

namespace epics { namespace pvaClient {


PvaClientMultiPutDoublePtr PvaClientMultiPutDouble::create(
    PvaClientMultiChannelPtr const &pvaMultiChannel,
    PvaClientChannelArray const &pvaClientChannelArray)
{
    PvaClientMultiPutDoublePtr pvaClientMultiPutDouble(
         new PvaClientMultiPutDouble(pvaMultiChannel,pvaClientChannelArray));
    return pvaClientMultiPutDouble;
}

PvaClientMultiPutDouble::PvaClientMultiPutDouble(
     PvaClientMultiChannelPtr const &pvaClientMultiChannel,
     PvaClientChannelArray const &pvaClientChannelArray)
: pvaClientMultiChannel(pvaClientMultiChannel),
  pvaClientChannelArray(pvaClientChannelArray),
  nchannel(pvaClientChannelArray.size()),
  pvaClientPut(std::vector<PvaClientPutPtr>(nchannel,PvaClientPutPtr())),
  isPutConnected(false)
{
    if(PvaClient::getDebug()) cout<< "PvaClientMultiPutDouble::PvaClientMultiPutDouble()\n";
}



PvaClientMultiPutDouble::~PvaClientMultiPutDouble()
{
    if(PvaClient::getDebug()) cout<< "PvaClientMultiPutDouble::~PvaClientMultiPutDouble()\n";
}


void PvaClientMultiPutDouble::connect()
{
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               pvaClientPut[i] = pvaClientChannelArray[i]->createPut();
               pvaClientPut[i]->issueConnect();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientPut[i]->waitConnect();
               if(status.isOK()) continue;
               string message = string("channel ") + pvaClientChannelArray[i]->getChannelName()
                   + " PvaChannelPut::waitConnect " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    isPutConnected = true;
}

void PvaClientMultiPutDouble::put(shared_vector<double> const &data)
{
    if(!isPutConnected) connect();
    if(data.size()!=nchannel) {
         throw std::runtime_error("data has wrong size");
    }
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               if(!pvaClientPut[i]) pvaClientPut[i]=pvaClientChannelArray[i]->createPut("value");
               PVStructurePtr pvTop = pvaClientPut[i]->getData()->getPVStructure();
               PVScalarPtr pvScalar= pvTop->getSubField<PVScalar>("value");
               if(pvScalar && ScalarTypeFunc::isNumeric(pvScalar->getScalar()->getScalarType())) {
                   getConvert()->fromDouble(pvScalar,data[i]);
                   pvaClientPut[i]->issuePut();
               } else {
                   string message = string("channel ")
                       + pvaClientChannelArray[i]->getChannelName()
                       + " is not a numeric scalar";
                   throw std::runtime_error(message);
               }
         }
         if(isConnected[i]) {
              Status status = pvaClientPut[i]->waitPut();
              if(status.isOK())  continue;
              string message = string("channel ") + pvaClientChannelArray[i]->getChannelName()
                   + " PvaChannelPut::waitPut " + status.getMessage();
              throw std::runtime_error(message);
         }
    }
}

}}
