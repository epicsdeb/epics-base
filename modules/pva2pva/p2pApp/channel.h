#ifndef CHANNEL_H
#define CHANNEL_H

#include <pv/pvAccess.h>

#include "chancache.h"

struct GWChannel : public epics::pvAccess::Channel
{
    POINTER_DEFINITIONS(GWChannel);
    static size_t num_instances;
    weak_pointer weakref;

    const ChannelCacheEntry::shared_pointer entry;
    const requester_type::weak_pointer requester;
    const std::string address; // address of client on GW server side
    const epics::pvAccess::ChannelProvider::weak_pointer server_provder;

    GWChannel(const ChannelCacheEntry::shared_pointer& e,
              const epics::pvAccess::ChannelProvider::weak_pointer& srvprov,
              const requester_type::weak_pointer&,
              const std::string& addr);
    virtual ~GWChannel();


    // for Requester
    virtual std::string getRequesterName();

    // for Destroyable
    virtual void destroy();

    // for Channel
    virtual std::tr1::shared_ptr<epics::pvAccess::ChannelProvider> getProvider();
    virtual std::string getRemoteAddress();

    virtual ConnectionState getConnectionState();
    virtual std::string getChannelName();
    virtual std::tr1::shared_ptr<epics::pvAccess::ChannelRequester> getChannelRequester();

    virtual void getField(epics::pvAccess::GetFieldRequester::shared_pointer const & requester,
                          std::string const & subField);
    virtual epics::pvAccess::AccessRights getAccessRights(epics::pvData::PVField::shared_pointer const & pvField);
    virtual epics::pvAccess::ChannelProcess::shared_pointer createChannelProcess(
            epics::pvAccess::ChannelProcessRequester::shared_pointer const & channelProcessRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvAccess::ChannelGet::shared_pointer createChannelGet(
            epics::pvAccess::ChannelGetRequester::shared_pointer const & channelGetRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvAccess::ChannelPut::shared_pointer createChannelPut(
            epics::pvAccess::ChannelPutRequester::shared_pointer const & channelPutRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvAccess::ChannelPutGet::shared_pointer createChannelPutGet(
            epics::pvAccess::ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvAccess::ChannelRPC::shared_pointer createChannelRPC(
            epics::pvAccess::ChannelRPCRequester::shared_pointer const & channelRPCRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvData::Monitor::shared_pointer createMonitor(
            epics::pvData::MonitorRequester::shared_pointer const & monitorRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvAccess::ChannelArray::shared_pointer createChannelArray(
            epics::pvAccess::ChannelArrayRequester::shared_pointer const & channelArrayRequester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest);

    virtual void printInfo(std::ostream& out);

};

#endif // CHANNEL_H
