/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>

#include <stdlib.h>
#include <string.h>

#include <epicsString.h>
#include <epicsGuard.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <epicsTime.h>

#define epicsExportSharedSymbols
#include <pv/pvdVersion.h>
#include <pv/sharedPtr.h>
#include "pv/reftrack.h"

namespace {

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

struct refgbl_t {
    epicsMutex lock;
    typedef std::map<std::string, const size_t*> counters_t;
    counters_t counters;
} *refgbl;

void refgbl_init(void *)
{
    try {
        refgbl = new refgbl_t;
    } catch(std::exception& e) {
        std::cerr<<"Failed to initialize global ref. counter registry :"<<e.what()<<"\n";
    }
}

epicsThreadOnceId refgbl_once = EPICS_THREAD_ONCE_INIT;

void refgbl_setup()
{
    epicsThreadOnce(&refgbl_once, &refgbl_init, 0);
    if(!refgbl)
        throw std::runtime_error("Failed to initialize global ref. counter registry");
}

} // namespace

namespace epics {

void registerRefCounter(const char *name, const size_t* counter)
{
    refgbl_setup();
    Guard G(refgbl->lock);
    refgbl->counters[name] = counter;
}

void unregisterRefCounter(const char *name, const size_t* counter)
{
    refgbl_setup();
    Guard G(refgbl->lock);
    refgbl_t::counters_t::iterator it(refgbl->counters.find(name));
    if(it!=refgbl->counters.end() && it->second==counter)
        refgbl->counters.erase(it);
}

size_t readRefCounter(const char *name)
{
    refgbl_setup();
    Guard G(refgbl->lock);
    refgbl_t::counters_t::iterator it(refgbl->counters.find(name));
    if(it==refgbl->counters.end())
        return 0;
    return atomic::get(*it->second);
}

const RefSnapshot::Count&
RefSnapshot::operator[](const std::string& name) const
{
    static const Count zero;

    cnt_map_t::const_iterator it(counts.find(name));
    return it==counts.end() ? zero : it->second;
}

void RefSnapshot::update()
{
    refgbl_t::counters_t counters;
    {
        refgbl_setup();
        Guard G(refgbl->lock);
        counters = refgbl->counters; // copy
    }

    counts.clear();

    for(refgbl_t::counters_t::const_iterator it=counters.begin(),
                                            end=counters.end();
        it!=end; ++it)
    {
        size_t cnt = atomic::get(*it->second);

        counts[it->first] = Count(cnt, 0);
    }
}

RefSnapshot RefSnapshot::operator-(const RefSnapshot& rhs) const
{
    RefSnapshot ret;

    RefSnapshot::cnt_map_t::const_iterator lit = counts.begin(),
                                           lend= counts.end(),
                                           rit = rhs.counts.begin(),
                                           rend= rhs.counts.end();

    while(lit!=lend || rit!=rend)
    {
        if(lit==lend || (rit!=rend && lit->first > rit->first)) {
            ret.counts[rit->first] = RefSnapshot::Count(0, -long(rit->second.current));
            ++rit;

        } else if(rit==rend || lit->first < rit->first) {
            ret.counts[lit->first] = RefSnapshot::Count(lit->second.current, long(lit->second.current));
            ++lit;

        } else { // !end and lit->first == rit->first
            ret.counts[lit->first] = RefSnapshot::Count(lit->second.current,
                                                        long(lit->second.current) - long(rit->second.current));
            ++lit;
            ++rit;
        }
    }

    return ret;
}

std::ostream& operator<<(std::ostream& strm, const RefSnapshot& snap)
{
    for(RefSnapshot::const_iterator it = snap.begin(), end = snap.end(); it!=end; ++it)
    {
        if(it->second.delta==0) continue;
        strm<<it->first<<":\t"<<it->second.current<<" (delta "<<it->second.delta<<")\n";
    }
    return strm;
}

struct RefMonitor::Impl : public epicsThreadRunable
{
    RefMonitor& owner;
    epics::auto_ptr<epicsThread> worker;
    epicsMutex lock;
    epicsEvent wakeup;
    RefSnapshot prev;
    bool done;
    double period;
    Impl(RefMonitor* owner) :owner(*owner), done(false), period(10.0) {}
    virtual ~Impl() {}

    virtual void run()
    {
        Guard G(lock);
        while(!done) {
            RefSnapshot current, P;
            P = prev; // copy
            {
                UnGuard U(G);

                current.update();

                owner.show(current-P);
            }

            prev.swap(current);

            {
                UnGuard U(G);
                wakeup.wait(period);
            }
        }
    }
};

RefMonitor::RefMonitor()
    :impl(new Impl(this))
{}
RefMonitor::~RefMonitor()
{
    stop();
    delete impl;
}

void RefMonitor::start(double period)
{
    Guard G(impl->lock);
    if(impl->worker.get()) return;

    impl->done = false;
    impl->period = period;
    impl->worker.reset(new epicsThread(*impl,
                                       "RefMonitor",
                                       epicsThreadGetStackSize(epicsThreadStackBig),
                                       epicsThreadPriorityMin));
    impl->worker->start();
}

void RefMonitor::stop()
{
    epics::auto_ptr<epicsThread> W;
    {
        Guard G(impl->lock);
        if(!impl->worker.get()) return;
        epics::swap(W, impl->worker);
        impl->done = true;
    }

    impl->wakeup.signal();
    W->exitWait();

    W.reset();
}

bool RefMonitor::running() const
{
    Guard G(impl->lock);
    return !!impl->worker.get();
}

void RefMonitor::current()
{
    RefSnapshot current, P;
    current.update();

    {
        Guard G(impl->lock);
        P = impl->prev; // copy
    }

    show(current-P, true);
}

void RefMonitor::show(const RefSnapshot &snap, bool complete)
{
    char buf[80];
    epicsTime::getCurrent().strftime(buf, sizeof(buf), "%a %b %d %Y %H:%M:%S.%f");
    buf[sizeof(buf)-1] = '\0';
    std::cerr<<buf<<" : References\n";

    for(RefSnapshot::const_iterator it = snap.begin(), end = snap.end(); it!=end; ++it)
    {
        // print if delta!=0 or (complete && current!=0)
        if(it->second.delta==0 && (!complete || it->second.current==0)) continue;
        std::cerr<<it->first<<":\t"<<it->second.current<<" (delta "<<it->second.delta<<")\n";
    }
}

} // namespace epics


char* epicsRefSnapshotCurrent()
{
    try {
        epics::RefSnapshot snap;
        snap.update();
        std::ostringstream strm;
        strm<<snap;
        const char *str = strm.str().c_str();
        char *ret = (char*)malloc(strlen(str)+1);
        if(ret)
            strcpy(ret, str);
        return ret;
    }catch(std::exception& e){
        return epicsStrDup(e.what());
    }
}
