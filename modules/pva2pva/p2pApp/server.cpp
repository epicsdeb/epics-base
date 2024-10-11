#include <stdio.h>

#include <epicsAtomic.h>
#include <epicsString.h>
#include <epicsTimer.h>

#include <pv/logger.h>
#include <pv/pvIntrospect.h> /* for pvdVersion.h */
#include <pv/epicsException.h>
#include <pv/serverContext.h>
#include <pv/logger.h>

#define epicsExportSharedSymbols
#include "helper.h"
#include "pva2pva.h"
#include "server.h"

#if defined(PVDATA_VERSION_INT)
#if PVDATA_VERSION_INT > VERSION_INT(7,0,0,0)
#  define USE_MSTATS
#endif
#endif

namespace pva = epics::pvAccess;
namespace pvd = epics::pvData;

std::tr1::shared_ptr<pva::ChannelProvider>
GWServerChannelProvider::getChannelProvider()
{
    return shared_from_this();
}

// Called from UDP search thread with no locks held
// Called from TCP threads (for search w/ TCP)
pva::ChannelFind::shared_pointer
GWServerChannelProvider::channelFind(std::string const & channelName,
                                     pva::ChannelFindRequester::shared_pointer const & channelFindRequester)
{
    pva::ChannelFind::shared_pointer ret;
    bool found = false;

    if(!channelName.empty())
    {
        LOG(pva::logLevelDebug, "Searching for '%s'", channelName.c_str());
        ChannelCacheEntry::shared_pointer ent(cache.lookup(channelName));
        if(ent) {
            found = true;
            ret = shared_from_this();
        }
    }

    // unlock for callback

    channelFindRequester->channelFindResult(pvd::Status::Ok, ret, found);

    return ret;
}

// The return value of this function is ignored
// The newly created channel is given to the ChannelRequester
pva::Channel::shared_pointer
GWServerChannelProvider::createChannel(std::string const & channelName,
                                       pva::ChannelRequester::shared_pointer const & channelRequester,
                                       short priority, std::string const & addressx)
{
    GWChannel::shared_pointer ret;
    std::string address = channelRequester->getRequesterName();

    if(!channelName.empty())
    {
        Guard G(cache.cacheLock);

        ChannelCacheEntry::shared_pointer ent(cache.lookup(channelName)); // recursively locks cacheLock

        if(ent)
        {
            ret.reset(new GWChannel(ent, shared_from_this(), channelRequester, address));
            ent->interested.insert(ret);
            ret->weakref = ret;
        }
    }

    if(!ret) {
        pvd::Status S(pvd::Status::STATUSTYPE_ERROR, "Not found");
        channelRequester->channelCreated(S, ret);
    } else {
        channelRequester->channelCreated(pvd::Status::Ok, ret);
        channelRequester->channelStateChange(ret, pva::Channel::CONNECTED);
    }

    return ret; // ignored by caller
}

void GWServerChannelProvider::destroy() {}

GWServerChannelProvider::GWServerChannelProvider(const pva::ChannelProvider::shared_pointer& prov)
    :cache(prov)
{}

GWServerChannelProvider::~GWServerChannelProvider() {}

void ServerConfig::drop(const char *client, const char *channel)
{
    if(!client)
        client= "";
    if(!channel)
        channel = "";
    // TODO: channel glob match

    FOREACH(clients_t::const_iterator, it, end, clients)
    {
        if(client[0]!='\0' && client[0]!='*' && it->first!=client)
            continue;

        const GWServerChannelProvider::shared_pointer& prov(it->second);

        ChannelCacheEntry::shared_pointer entry;

        // find the channel, if it's there
        {
            Guard G(prov->cache.cacheLock);

            ChannelCache::entries_t::iterator it = prov->cache.entries.find(channel);
            if(it==prov->cache.entries.end())
                continue;

            std::cout<<"Drop from "<<it->first<<" : "<<it->second->channelName<<"\n";

            entry = it->second;
            prov->cache.entries.erase(it); // drop out of cache (TODO: not required)
        }

        // trigger client side disconnect (recursively calls call CRequester::channelStateChange())
        // TODO: shouldn't need this
        entry->channel->destroy();

    }
}

void ServerConfig::status_server(int lvl, const char *server)
{
    if(!server)
        server = "";

    FOREACH(servers_t::const_iterator, it, end, servers)
    {
        if(server[0]!='\0' && server[0]!='*' && it->first!=server)
            continue;

        const pva::ServerContext::shared_pointer& serv(it->second);
        std::cout<<"==> Server: "<<it->first<<"\n";
        serv->printInfo(std::cout);
        std::cout<<"<== Server: "<<it->first<<"\n\n";
        // TODO: print client list somehow
    }
}

void ServerConfig::status_client(int lvl, const char *client, const char *channel)
{
    if(!client)
        client= "";
    if(!channel)
        channel = "";

    bool iswild = strchr(channel, '?') || strchr(channel, '*');

    FOREACH(clients_t::const_iterator, it, end, clients)
    {
        if(client[0]!='\0' && client[0]!='*' && it->first!=client)
            continue;

        const GWServerChannelProvider::shared_pointer& prov(it->second);

        std::cout<<"==> Client: "<<it->first<<"\n";

        ChannelCache::entries_t entries;

        size_t ncache, ncleaned, ndust;
        {
            Guard G(prov->cache.cacheLock);

            ncache = prov->cache.entries.size();
            ncleaned = prov->cache.cleanerRuns;
            ndust = prov->cache.cleanerDust;

            if(lvl>0) {
                if(!iswild) { // no string or some glob pattern
                    entries = prov->cache.entries; // copy of std::map
                } else { // just one channel
                    ChannelCache::entries_t::iterator it(prov->cache.entries.find(channel));
                    if(it!=prov->cache.entries.end())
                        entries[it->first] = it->second;
                }
            }
        }

        std::cout<<"Cache has "<<ncache<<" channels.  Cleaned "
                <<ncleaned<<" times closing "<<ndust<<" channels\n";

        if(lvl<=0)
            continue;

        FOREACH(ChannelCache::entries_t::const_iterator, it2, end2, entries)
        {
            const std::string& channame = it2->first;
            if(iswild && !epicsStrGlobMatch(channame.c_str(), channel))
                continue;

            ChannelCacheEntry& E = *it2->second;
            ChannelCacheEntry::mon_entries_t::lock_vector_type mons;
            size_t nsrv, nmon;
            bool dropflag;
            const char *chstate;
            {
                Guard G(E.mutex());
                chstate = pva::Channel::ConnectionStateNames[E.channel->getConnectionState()];
                nsrv = E.interested.size();
                nmon = E.mon_entries.size();
                dropflag = E.dropPoke;

                if(lvl>1)
                    mons = E.mon_entries.lock_vector();
            }

            std::cout<<chstate
                     <<" Client Channel '"<<channame
                     <<"' used by "<<nsrv<<" Server channel(s) with "
                     <<nmon<<" unique subscription(s) "
                     <<(dropflag?'!':'_')<<"\n";

            if(lvl<=1)
                continue;

            FOREACH(ChannelCacheEntry::mon_entries_t::lock_vector_type::const_iterator, it2, end2, mons) {
                MonitorCacheEntry& ME =  *it2->second;

                MonitorCacheEntry::interested_t::vector_type usrs;
                size_t nsrvmon;
#ifdef USE_MSTATS
                pvd::Monitor::Stats mstats;
#endif
                bool hastype, hasdata, isdone;
                {
                    Guard G(ME.mutex());

                    nsrvmon = ME.interested.size();
                    hastype = !!ME.typedesc;
                    hasdata = !!ME.lastelem;
                    isdone = ME.done;

#ifdef USE_MSTATS
                    if(ME.mon)
                        ME.mon->getStats(mstats);
#endif

                    if(lvl>2)
                        usrs = ME.interested.lock_vector();
                }

                // TODO: how to describe pvRequest in a compact way...
                std::cout<<"  Client Monitor used by "<<nsrvmon<<" Server monitors, "
                         <<"Has "<<(hastype?"":"not ")
                         <<"opened, Has "<<(hasdata?"":"not ")
                         <<"recv'd some data, Has "<<(isdone?"":"not ")<<"finalized\n"
                           "    "<<      epicsAtomicGetSizeT(&ME.nwakeups)<<" wakeups "
                         <<epicsAtomicGetSizeT(&ME.nevents)<<" events\n";
#ifdef USE_MSTATS
                if(mstats.nempty || mstats.nfilled || mstats.noutstanding)
                    std::cout<<"    US monitor queue "<<mstats.nfilled
                             <<" filled, "<<mstats.noutstanding
                             <<" outstanding, "<<mstats.nempty<<" empty\n";
#endif
                if(lvl<=2)
                    continue;

                FOREACH(MonitorCacheEntry::interested_t::vector_type::const_iterator, it3, end3, usrs) {
                    MonitorUser& MU = **it3;

                    size_t nempty, nfilled, nused, total;
                    std::string remote;
                    bool isrunning;
                    {
                        Guard G(MU.mutex());

                        nempty = MU.empty.size();
                        nfilled = MU.filled.size();
                        nused = MU.inuse.size();
                        isrunning = MU.running;

                        GWChannel::shared_pointer srvchan(MU.srvchan.lock());
                        if(srvchan)
                            remote = srvchan->address;
                        else
                            remote = "<unknown>";
                    }
                    total = nempty + nfilled + nused;

                    std::cout<<"    Server monitor from "
                             <<remote
                             <<(isrunning?"":" Paused")
                             <<" buffer "<<nfilled<<"/"<<total
                             <<" out "<<nused<<"/"<<total
                             <<" "<<epicsAtomicGetSizeT(&MU.nwakeups)<<" wakeups "
                             <<epicsAtomicGetSizeT(&MU.nevents)<<" events "
                             <<epicsAtomicGetSizeT(&MU.ndropped)<<" drops\n";
                }
            }



        }


        std::cout<<"<== Client: "<<it->first<<"\n\n";
    }
}
