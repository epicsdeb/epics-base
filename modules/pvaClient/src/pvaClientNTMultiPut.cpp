/* PvaClientNTMultiPut.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.03
 */

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

PvaClientNTMultiPutPtr PvaClientNTMultiPut::create(
    PvaClientMultiChannelPtr const &pvaMultiChannel,
    PvaClientChannelArray const &pvaClientChannelArray)
{
    return PvaClientNTMultiPutPtr(
        new PvaClientNTMultiPut(pvaMultiChannel,pvaClientChannelArray));
}

PvaClientNTMultiPut::PvaClientNTMultiPut(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray)
: pvaClientMultiChannel(pvaClientMultiChannel),
  pvaClientChannelArray(pvaClientChannelArray),
  nchannel(pvaClientChannelArray.size()),
  unionValue(shared_vector<PVUnionPtr>(nchannel,PVUnionPtr())),
  value(shared_vector<PVFieldPtr>(nchannel,PVFieldPtr())),
  isConnected(false)
{
     if(PvaClient::getDebug()) cout<< "PvaClientNTMultiPut::PvaClientNTMultiPut()\n";
}


PvaClientNTMultiPut::~PvaClientNTMultiPut()
{
    if(PvaClient::getDebug()) cout<< "PvaClientNTMultiPut::~PvaClientNTMultiPut()\n";
}

void PvaClientNTMultiPut::connect()
{
    pvaClientPut.resize(nchannel);
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
               string message = string("channel ") +pvaClientChannelArray[i]->getChannelName()
                    + " PvaChannelPut::waitConnect " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
              pvaClientPut[i]->issueGet();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               Status status = pvaClientPut[i]->waitGet();
               if(status.isOK()) continue;
               string message = string("channel ") +pvaClientChannelArray[i]->getChannelName()
                    + " PvaChannelPut::waitGet " + status.getMessage();
               throw std::runtime_error(message);
         }
    }
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
             value[i] = pvaClientPut[i]->getData()->getValue();
             FieldBuilderPtr builder = fieldCreate->createFieldBuilder();
             builder->add("value",value[i]->getField());
             unionValue[i] = pvDataCreate->createPVUnion(builder->createUnion());
         }
    }
    this->isConnected = true;
}

shared_vector<PVUnionPtr> PvaClientNTMultiPut::getValues()
{
    if(!isConnected) connect();
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               if(!pvaClientPut[i]){
                   pvaClientPut[i] = pvaClientChannelArray[i]->createPut();                
                   pvaClientPut[i]->connect();
                   pvaClientPut[i]->get();
                   value[i] = pvaClientPut[i]->getData()->getValue();
                   FieldCreatePtr fieldCreate = getFieldCreate();
                   PVDataCreatePtr pvDataCreate = getPVDataCreate();
                   FieldBuilderPtr builder = fieldCreate->createFieldBuilder();
                   builder->add("value",value[i]->getField());
                   unionValue[i] = pvDataCreate->createPVUnion(builder->createUnion());
               }
         }
    }     
    return unionValue;
}

void PvaClientNTMultiPut::put()
{
    if(!isConnected) connect();
    shared_vector<epics::pvData::boolean> isConnected = pvaClientMultiChannel->getIsConnected();
    for(size_t i=0; i<nchannel; ++i)
    {
         if(isConnected[i]) {
               if(!pvaClientPut[i]){
                   pvaClientPut[i] = pvaClientChannelArray[i]->createPut();            
                   pvaClientPut[i]->connect();
                   pvaClientPut[i]->get();
                   value[i] = pvaClientPut[i]->getData()->getValue();
                   FieldCreatePtr fieldCreate = getFieldCreate();
                   PVDataCreatePtr pvDataCreate = getPVDataCreate();
                   FieldBuilderPtr builder = fieldCreate->createFieldBuilder();
                   builder->add("value",value[i]->getField());
                   unionValue[i] = pvDataCreate->createPVUnion(builder->createUnion());
               }
               value[i]->copy(*unionValue[i]->get());
               pvaClientPut[i]->issuePut();
         }
    }
    for(size_t i=0; i<nchannel; ++i)
    {
        if(isConnected[i]) {
            Status status = pvaClientPut[i]->waitPut();
            if(status.isOK())  continue;
            string message = string("channel ") +pvaClientChannelArray[i]->getChannelName()
                + " PvaChannelPut::waitPut " + status.getMessage();
            throw std::runtime_error(message);
         }
    }
}


}}
