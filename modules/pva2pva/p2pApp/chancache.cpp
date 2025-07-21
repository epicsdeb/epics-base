#include <stdio.h>

#include <epicsAtomic.h>
#include <errlog.h>

#include <epicsMutex.h>
#include <epicsTimer.h>

#include <pv/epicsException.h>
#include <pv/serverContext.h>
#include <pv/pvAccess.h>

#define epicsExportSharedSymbols
#include "pva2pva.h"
#include "helper.h"
#include "chancache.h"
#include "channel.h"

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

size_t ChannelCacheEntry::num_instances;

ChannelCacheEntry::ChannelCacheEntry(ChannelCache* c, const std::string& n)
    :channelName(n), cache(c), dropPoke(true)
{
    epicsAtomicIncrSizeT(&num_instances);
}

ChannelCacheEntry::~ChannelCacheEntry()
{
    // Should *not* be holding cache->cacheLock
    if(channel.get())
        channel->destroy(); // calls channelStateChange() w/ DESTROY
    epicsAtomicDecrSizeT(&num_instances);
}

std::string
ChannelCacheEntry::CRequester::getRequesterName()
{
    return "GWClient";
}

size_t ChannelCacheEntry::CRequester::num_instances;

ChannelCacheEntry::CRequester::CRequester(const ChannelCacheEntry::shared_pointer& p)
    :chan(p)
{
    epicsAtomicIncrSizeT(&num_instances);
}

ChannelCacheEntry::CRequester::~CRequester()
{
    epicsAtomicDecrSizeT(&num_instances);
}

// for ChannelRequester
void
ChannelCacheEntry::CRequester::channelCreated(const pvd::Status& status,
                            pva::Channel::shared_pointer const & channel)
{}

void
ChannelCacheEntry::CRequester::channelStateChange(pva::Channel::shared_pointer const & channel,
                                pva::Channel::ConnectionState connectionState)
{
    ChannelCacheEntry::shared_pointer chan(this->chan.lock());
    if(!chan)
        return;

    {
        Guard G(chan->cache->cacheLock);

        assert(chan->channel.get()==channel.get());

        switch(connectionState)
        {
        case pva::Channel::DISCONNECTED:
        case pva::Channel::DESTROYED:
            // Drop from cache
            chan->cache->entries.erase(chan->channelName);
            // keep 'chan' as a reference so that actual destruction doesn't happen which cacheLock is held
            break;
        default:
            break;
        }
    }

    // fanout notification
    ChannelCacheEntry::interested_t::vector_type interested(chan->interested.lock_vector()); // Copy

    FOREACH(ChannelCacheEntry::interested_t::vector_type::const_iterator, it, end, interested)
    {
        GWChannel *chan = it->get();
        pva::ChannelRequester::shared_pointer req(chan->requester.lock());
        if(req)
            req->channelStateChange(*it, connectionState);
    }
}


struct ChannelCache::cacheClean : public epicsTimerNotify
{
    ChannelCache *cache;
    cacheClean(ChannelCache *c) : cache(c) {}
    epicsTimerNotify::expireStatus expire(const epicsTime &currentTime)
    {
        // keep a reference to any cache entrys being removed so they
        // aren't destroyed while cacheLock is held
        std::set<ChannelCacheEntry::shared_pointer> cleaned;

        {
            Guard G(cache->cacheLock);
            cache->cleanerRuns++;

            ChannelCache::entries_t::iterator cur=cache->entries.begin(), next, end=cache->entries.end();
            while(cur!=end) {
                next = cur;
                ++next;

                if(!cur->second->dropPoke && cur->second->interested.empty()) {
                    cleaned.insert(cur->second);
                    cache->entries.erase(cur);
                    cache->cleanerDust++;
                } else {
                    cur->second->dropPoke = false;
                }

                cur = next;
            }
        }
        return epicsTimerNotify::expireStatus(epicsTimerNotify::restart, 30.0);
    }
};

ChannelCache::ChannelCache(const pva::ChannelProvider::shared_pointer& prov)
    :provider(prov)
    ,timerQueue(&epicsTimerQueueActive::allocate(1, epicsThreadPriorityCAServerLow-2))
    ,cleaner(new cacheClean(this))
    ,cleanerRuns(0)
    ,cleanerDust(0)
{
    if(!provider)
        throw std::logic_error("Missing 'pva' provider");
    assert(timerQueue);
    cleanTimer = &timerQueue->createTimer();
    cleanTimer->start(*cleaner, 30.0);
}

ChannelCache::~ChannelCache()
{
    entries_t E;
    {
        Guard G(cacheLock);

        cleanTimer->destroy();
        timerQueue->release();
        delete cleaner;

        entries_t E;
        E.swap(entries);
    }
}

ChannelCacheEntry::shared_pointer
ChannelCache::lookup(const std::string& newName)
{
    ChannelCacheEntry::shared_pointer ret;

    Guard G(cacheLock);

    entries_t::const_iterator it = entries.find(newName);

    if(it==entries.end()) {
        // first request, create ChannelCacheEntry
        //TODO: async lookup

        ChannelCacheEntry::shared_pointer ent(new ChannelCacheEntry(this, newName));
        ent->requester.reset(new ChannelCacheEntry::CRequester(ent));

        entries[newName] = ent;

        pva::Channel::shared_pointer M;
        {
            // unlock to call createChannel()
            epicsGuardRelease<epicsMutex> U(G);

            M = provider->createChannel(newName, ent->requester);
            if(!M)
                THROW_EXCEPTION2(std::runtime_error, "Failed to createChannel");
        }
        ent->channel = M;

        if(M->isConnected())
            ret = ent; // immediate connect, mostly for unit-tests (thus delayed connect not covered)

    } else if(it->second->channel && it->second->channel->isConnected()) {
        // another request, and hey we're connected this time

        ret = it->second;
        it->second->dropPoke = true;

    } else {
        // not connected yet, but a client is still interested
        it->second->dropPoke = true;
    }

    return ret;
}
