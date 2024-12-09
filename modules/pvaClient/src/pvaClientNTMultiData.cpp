/* pvaClientNTMultiData.cpp */
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

PvaClientNTMultiDataPtr PvaClientNTMultiData::create(
    UnionConstPtr const & u,
    PvaClientMultiChannelPtr const &pvaMultiChannel,
    PvaClientChannelArray const &pvaClientChannelArray,
    PVStructurePtr const &  pvRequest)
{
    return PvaClientNTMultiDataPtr(
         new PvaClientNTMultiData(u,pvaMultiChannel,pvaClientChannelArray,pvRequest));
}

PvaClientNTMultiData::PvaClientNTMultiData(
         UnionConstPtr const & u,
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         PVStructurePtr const &  pvRequest)
: pvaClientMultiChannel(pvaClientMultiChannel),
  pvaClientChannelArray(pvaClientChannelArray),
  nchannel(pvaClientChannelArray.size()),
  gotAlarm(false),
  gotTimeStamp(false)
  
{
    if(PvaClient::getDebug()) cout<< "PvaClientNTMultiData::PvaClientNTMultiData()\n";
    changeFlags =  shared_vector<epics::pvData::boolean>(nchannel);
    topPVStructure.resize(nchannel);
    
    unionValue.resize(nchannel);
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    for(size_t i=0; i< nchannel; ++i) {
         topPVStructure[i] = PVStructurePtr();
         unionValue[i] = pvDataCreate->createPVUnion(u);
    }
    NTMultiChannelBuilderPtr builder = NTMultiChannel::createBuilder();
    builder->value(u)->addIsConnected();
    if(pvRequest->getSubField("field.alarm"))
    {
         gotAlarm = true;
         builder->addAlarm();
         builder->addSeverity();
         builder->addStatus();
         builder->addMessage();
         severity.resize(nchannel);
         status.resize(nchannel);
         message.resize(nchannel);

    }
    if(pvRequest->getSubField("field.timeStamp")) {
        gotTimeStamp = true;
        builder->addTimeStamp();
        builder->addSecondsPastEpoch();
        builder->addNanoseconds();
        builder->addUserTag();
        secondsPastEpoch.resize(nchannel);
        nanoseconds.resize(nchannel);
        userTag.resize(nchannel);
    }
    ntMultiChannelStructure = builder->createStructure();
}


PvaClientNTMultiData::~PvaClientNTMultiData()
{
    if(PvaClient::getDebug()) cout<< "PvaClientNTMultiData::~PvaClientNTMultiData()\n";
}

void PvaClientNTMultiData::setPVStructure(
        PVStructurePtr const &pvStructure,size_t index)
{
    topPVStructure[index] = pvStructure;
}

shared_vector<epics::pvData::boolean> PvaClientNTMultiData::getChannelChangeFlags()
{
    return changeFlags;
}

size_t PvaClientNTMultiData::getNumber()
{
    return nchannel;
}



void PvaClientNTMultiData::startDeltaTime()
{
    for(size_t i=0; i<nchannel; ++i)
    {
        topPVStructure[i] = PVStructurePtr();
        if(gotAlarm)
        {
            alarm.setSeverity(noAlarm);
            alarm.setStatus(noStatus);
            alarm.setMessage("");
            severity[i] = invalidAlarm;
            status[i] = undefinedStatus;
            message[i] = "not connected";
        }
        if(gotTimeStamp)
        {
            timeStamp.getCurrent();
            secondsPastEpoch[i] = 0;
            nanoseconds[i] = 0;
            userTag[i] = 0;
        }
    }
}


void PvaClientNTMultiData::endDeltaTime(bool valueOnly)
{
    for(size_t i=0; i<nchannel; ++i)
    {
        PVStructurePtr pvst = topPVStructure[i];
        changeFlags[i] = false;
        if(pvst&&unionValue[i]) {
            changeFlags[i] = true;
            if(valueOnly) {
                PVFieldPtr pvValue = pvst->getSubField("value");
                if(pvValue) {
                    unionValue[i]->set(pvst->getSubField("value"));
                }
            } else {
                unionValue[i]->set(pvst);
            }
            if(gotAlarm)
            {
                PVIntPtr pvSeverity = pvst->getSubField<PVInt>("alarm.severity");
                PVIntPtr pvStatus = pvst->getSubField<PVInt>("alarm.status");
                PVStringPtr pvMessage = pvst->getSubField<PVString>("alarm.message");
                if(pvSeverity&&pvStatus&&pvMessage) {
                    severity[i] = pvSeverity->get();
                    status[i] = pvStatus->get();
                    message[i] = pvMessage->get();
                } else {
                    severity[i] = undefinedAlarm;
                    status[i] = undefinedStatus;
                    message[i] = "no alarm field";
                }
            }
            if(gotTimeStamp)
            {
                PVLongPtr pvEpoch = pvst->getSubField<PVLong>("timeStamp.secondsPastEpoch");
                PVIntPtr pvNano = pvst->getSubField<PVInt>("timeStamp.nanoseconds");
                PVIntPtr pvTag = pvst->getSubField<PVInt>("timeStamp.userTag");
                if(pvEpoch&&pvNano&&pvTag) {
                    secondsPastEpoch[i] = pvEpoch->get();
                    nanoseconds[i] = pvNano->get();
                    userTag[i] = pvTag->get();
                }
            }
        }
    }
}

TimeStamp PvaClientNTMultiData::getTimeStamp()
{
    pvTimeStamp.get(timeStamp);
    return timeStamp;
}

NTMultiChannelPtr PvaClientNTMultiData::getNTMultiChannel()
{
    PVStructurePtr pvStructure = getPVDataCreate()->createPVStructure(ntMultiChannelStructure);
    NTMultiChannelPtr ntMultiChannel = NTMultiChannel::wrap(pvStructure);
    ntMultiChannel->getChannelName()->replace(pvaClientMultiChannel->getChannelNames());
    shared_vector<PVUnionPtr> val(nchannel);
    for(size_t i=0; i<nchannel; ++i) val[i] = unionValue[i];
    ntMultiChannel->getValue()->replace(freeze(val));
    shared_vector<epics::pvData::boolean> connected = pvaClientMultiChannel->getIsConnected();
    shared_vector<epics::pvData::boolean> isConnected(nchannel);
    for(size_t i=0; i<nchannel; ++i) isConnected[i] = connected[i];
    ntMultiChannel->getIsConnected()->replace(freeze(isConnected));
    if(gotAlarm)
    {
        shared_vector<int32> sev(nchannel);
        for(size_t i=0; i<nchannel; ++i) sev[i] = severity[i];
        ntMultiChannel->getSeverity()->replace(freeze(sev));
        shared_vector<int32> sta(nchannel);
        for(size_t i=0; i<nchannel; ++i) sta[i] = status[i];
        ntMultiChannel->getStatus()->replace(freeze(sta));
        shared_vector<string> mes(nchannel);
        for(size_t i=0; i<nchannel; ++i) mes[i] = message[i];
        ntMultiChannel->getMessage()->replace(freeze(mes));
    }
    if(gotTimeStamp)
    {
        shared_vector<int64> sec(nchannel);
        for(size_t i=0; i<nchannel; ++i) sec[i] = secondsPastEpoch[i];
        ntMultiChannel->getSecondsPastEpoch()->replace(freeze(sec));
        shared_vector<int32> nano(nchannel);
        for(size_t i=0; i<nchannel; ++i) nano[i] = nanoseconds[i];
        ntMultiChannel->getNanoseconds()->replace(freeze(nano));
        shared_vector<int32> tag(nchannel);
        for(size_t i=0; i<nchannel; ++i) tag[i] = userTag[i];
        ntMultiChannel->getUserTag()->replace(freeze(tag));
    }
    return ntMultiChannel;
}

}}
