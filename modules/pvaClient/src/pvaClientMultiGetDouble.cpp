/* pvaClientMultiGetDouble.cpp */
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


PvaClientMultiGetDoublePtr PvaClientMultiGetDouble::create(
    PvaClientMultiChannelPtr const &pvaMultiChannel,
    PvaClientChannelArray const &pvaClientChannelArray)
{
    PvaClientMultiGetDoublePtr pvaClientMultiGetDouble(
         new PvaClientMultiGetDouble(pvaMultiChannel,pvaClientChannelArray));
    return pvaClientMultiGetDouble;
}

PvaClientMultiGetDouble::PvaClientMultiGetDouble(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray)
: pvaClientMultiChannel(pvaClientMultiChannel),
  pvaClientChannelArray(pvaClientChannelArray),
  nchannel(pvaClientChannelArray.size()),
  doubleValue( shared_vector<double>(nchannel)),
  pvaClientGet(std::vector<PvaClientGetPtr>(nchannel,PvaClientGetPtr())),
  isGetConnected(false)
{
    if(PvaClient::getDebug()) cout<< "PvaClientMultiGetDouble::PvaClientMultiGetDouble()\n";
}

PvaClientMultiGetDouble::~PvaClientMultiGetDouble()
{
    if(PvaClient::getDebug()) cout<< "PvaClientMultiGetDouble::~PvaClientMultiGetDouble()\n";
}

void PvaClientMultiGetDouble::connect()
{
    shared_vector<epics::pvData::boolean>isConnected = pvaClientMultiChannel->getIsConnected();
    string request = "value";
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               pvaClientGet[i] = pvaClientChannelArray[i]->createGet(request);
               pvaClientGet[i]->issueConnect();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientGet[i]->waitConnect();
               if(status.isOK()) continue;
               string message = string("channel ") + pvaClientChannelArray[i]->getChannelName()
                   + " PvaChannelGet::waitConnect " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    isGetConnected = true;
}

shared_vector<double> PvaClientMultiGetDouble::get()
{
    if(!isGetConnected) connect();
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               if(!pvaClientGet[i]) pvaClientGet[i]=pvaClientChannelArray[i]->createGet("value");
               pvaClientGet[i]->issueGet();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientGet[i]->waitGet();
               if(status.isOK()) continue;
               string message = string("channel ") + pvaClientChannelArray[i]->getChannelName()
                   + " PvaChannelGet::waitGet " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
        if(isConnected[i])
        {
            PVStructurePtr pvStructure = pvaClientGet[i]->getData()->getPVStructure();
            PVScalarPtr pvScalar(pvStructure->getSubField<PVScalar>("value"));
            if(pvScalar) {
                ScalarType scalarType = pvScalar->getScalar()->getScalarType();
                if(ScalarTypeFunc::isNumeric(scalarType)) {
                    doubleValue[i] = getConvert()->toDouble(pvScalar);
                } else {
                    doubleValue[i] = epicsNAN;
                }
            } else {
                doubleValue[i] = epicsNAN;
            }
        } else {
            doubleValue[i] = epicsNAN;
        }
    }
    return doubleValue;
}

}}
