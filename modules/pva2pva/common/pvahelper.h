#ifndef PVAHELPER_H
#define PVAHELPER_H

#include <deque>

#include <epicsGuard.h>

#include <pv/pvAccess.h>

template<typename T, typename A>
bool getS(const epics::pvData::PVStructurePtr& S, const char *name, A& val)
{
    epics::pvData::PVScalarPtr F(S->getSubField<epics::pvData::PVScalar>(name));
    if(F)
        val = F->getAs<T>();
    return !!F;
}

struct BaseChannel : public epics::pvAccess::Channel
{
    BaseChannel(const std::string& name,
                const std::tr1::weak_ptr<epics::pvAccess::ChannelProvider>& prov,
                const epics::pvAccess::ChannelRequester::shared_pointer& req,
                const epics::pvData::StructureConstPtr& dtype
                )
        :pvname(name), provider(prov), requester(req), fielddesc(dtype)
    {}
    virtual ~BaseChannel() {}

    mutable epicsMutex lock;
    typedef epicsGuard<epicsMutex> guard_t;

    const std::string pvname;
    const epics::pvAccess::ChannelProvider::weak_pointer provider;
    const requester_type::weak_pointer requester;
    const epics::pvData::StructureConstPtr fielddesc;

    // assume Requester methods not called after destory()
    virtual std::string getRequesterName() OVERRIDE
    { return getChannelRequester()->getRequesterName(); }

    virtual void destroy() OVERRIDE FINAL {}

    virtual std::tr1::shared_ptr<epics::pvAccess::ChannelProvider> getProvider() OVERRIDE FINAL
    { return epics::pvAccess::ChannelProvider::shared_pointer(provider); }
    virtual std::string getRemoteAddress() OVERRIDE
    { return getRequesterName(); }

    virtual std::string getChannelName() OVERRIDE FINAL { return pvname; }
    virtual std::tr1::shared_ptr<epics::pvAccess::ChannelRequester> getChannelRequester() OVERRIDE FINAL
    { return requester_type::shared_pointer(requester); }

    virtual void getField(epics::pvAccess::GetFieldRequester::shared_pointer const & requester,std::string const & subField) OVERRIDE
    { requester->getDone(epics::pvData::Status(), fielddesc); }

    virtual void printInfo(std::ostream& out) OVERRIDE {
        out<<"Channel '"<<pvname<<"' "<<getRemoteAddress()<<"\n";
    }
};

/**
 * Helper which implements a Monitor queue.
 * connect()s to a complete copy of a PVStructure.
 * When this struct has changed, post(BitSet) should be called.
 *
 * Derived class may use onStart(), onStop(), and requestUpdate()
 * to react to subscriber events.
 */
struct BaseMonitor : public epics::pvAccess::Monitor
{
    POINTER_DEFINITIONS(BaseMonitor);
    weak_pointer weakself;
    inline shared_pointer shared_from_this() { return shared_pointer(weakself); }

    typedef epics::pvAccess::MonitorRequester requester_t;

    epicsMutex& lock; // not held during any callback
    typedef epicsGuard<epicsMutex> guard_t;
    typedef epicsGuardRelease<epicsMutex> unguard_t;

private:
    const requester_t::weak_pointer requester;

    epics::pvData::PVStructurePtr complete;
    epics::pvData::BitSet changed, overflow;

    typedef std::deque<epics::pvAccess::MonitorElementPtr> buffer_t;
    bool inoverflow;
    bool running;
    size_t nbuffers;
    buffer_t inuse, empty;

public:
    BaseMonitor(epicsMutex& lock,
                const requester_t::weak_pointer& requester,
                const epics::pvData::PVStructure::shared_pointer& pvReq)
        :lock(lock)
        ,requester(requester)
        ,inoverflow(false)
        ,running(false)
        ,nbuffers(2)
    {}

    virtual ~BaseMonitor() {destroy();}

    inline const epics::pvData::PVStructurePtr& getValue() { return complete; }

    //! Must call before first post().  Sets .complete and calls monitorConnect()
    //! @note that value will never by accessed except by post() and requestUpdate()
    void connect(guard_t& guard, const epics::pvData::PVStructurePtr& value)
    {
        guard.assertIdenticalMutex(lock);
        epics::pvData::StructureConstPtr dtype(value->getStructure());
        epics::pvData::PVDataCreatePtr create(epics::pvData::getPVDataCreate());
        BaseMonitor::shared_pointer self(shared_from_this());
        requester_t::shared_pointer req(requester.lock());

        assert(!complete); // can't call twice

        complete = value;
        empty.resize(nbuffers);
        for(size_t i=0; i<empty.size(); i++) {
            empty[i].reset(new epics::pvAccess::MonitorElement(create->createPVStructure(dtype)));
        }

        if(req) {
            unguard_t U(guard);
            epics::pvData::Status sts;
            req->monitorConnect(sts, self, dtype);
        }
    }

    struct no_overflow {};

    //! post update if queue not full, if full return false w/o overflow
    bool post(guard_t& guard, const epics::pvData::BitSet& updated, no_overflow)
    {
        guard.assertIdenticalMutex(lock);
        requester_t::shared_pointer req;

        if(!complete || !running) return false;

        changed |= updated;

        if(empty.empty()) return false;

        if(p_postone())
            req = requester.lock();
        inoverflow = false;

        if(req) {
            unguard_t U(guard);
            req->monitorEvent(shared_from_this());
        }
        return true;
    }

    //! post update of pending changes.  eg. call from requestUpdate()
    bool post(guard_t& guard)
    {
        guard.assertIdenticalMutex(lock);
        bool oflow;
        requester_t::shared_pointer req;

        if(!complete || !running) return false;

        if(empty.empty()) {
            oflow = inoverflow = true;

        } else {

            if(p_postone())
                req = requester.lock();
            oflow = inoverflow = false;
        }

        if(req) {
            unguard_t U(guard);
            req->monitorEvent(shared_from_this());
        }
        return !oflow;
    }

    //! post update with changed and overflowed masks (eg. when updates were lost in some upstream queue)
    bool post(guard_t& guard,
              const epics::pvData::BitSet& updated,
              const epics::pvData::BitSet& overflowed)
    {
        guard.assertIdenticalMutex(lock);
        bool oflow;
        requester_t::shared_pointer req;

        if(!complete || !running) return false;

        if(empty.empty()) {
            oflow = inoverflow = true;
            overflow |= overflowed;
            overflow.or_and(updated, changed);
            changed |= updated;

        } else {

            changed |= updated;
            if(p_postone())
                req = requester.lock();
            oflow = inoverflow = false;
        }

        if(req) {
            unguard_t U(guard);
            req->monitorEvent(shared_from_this());
        }
        return !oflow;
    }

    //! post update with changed
    bool post(guard_t& guard, const epics::pvData::BitSet& updated) {
        bool oflow;
        requester_t::shared_pointer req;

        if(!complete || !running) return false;

        if(empty.empty()) {
            oflow = inoverflow = true;
            overflow.or_and(updated, changed);
            changed |= updated;

        } else {

            changed |= updated;
            if(p_postone())
                req = requester.lock();
            oflow = inoverflow = false;
        }

        if(req) {
            unguard_t U(guard);
            req->monitorEvent(shared_from_this());
        }
        return !oflow;
    }

private:
    bool p_postone()
    {
        bool ret;
        // assume lock is held
        assert(!empty.empty());

        epics::pvAccess::MonitorElementPtr& elem = empty.front();

        elem->pvStructurePtr->copyUnchecked(*complete);
        *elem->changedBitSet = changed;
        *elem->overrunBitSet = overflow;

        overflow.clear();
        changed.clear();

        ret = inuse.empty();
        inuse.push_back(elem);
        empty.pop_front();
        return ret;
    }
public:

    // for special handling when MonitorRequester start()s or stop()s
    virtual void onStart() {}
    virtual void onStop() {}
    //! called when within release() when the opportunity exists to end the overflow condition
    //! May do nothing, or lock and call post()
    virtual void requestUpdate() {guard_t G(lock); post(G);}

    virtual void destroy()
    {
        stop();
    }

private:
    virtual epics::pvData::Status start()
    {
        epics::pvData::Status ret;
        bool notify = false;
        BaseMonitor::shared_pointer self;
        {
            guard_t G(lock);
            if(running) return ret;
            running = true;
            if(!complete) return ret; // haveType() not called (error?)
            inoverflow = empty.empty();
            if(!inoverflow) {

                // post complete event
                overflow.clear();
                changed.clear();
                changed.set(0);
                notify = true;
            }
        }
        if(notify) onStart(); // may result in post()
        return ret;
    }

    virtual epics::pvData::Status stop()
    {
        BaseMonitor::shared_pointer self;
        bool notify;
        epics::pvData::Status ret;
        {
            guard_t G(lock);
            notify = running;
            running = false;
        }
        if(notify) onStop();
        return ret;
    }

    virtual epics::pvAccess::MonitorElementPtr poll()
    {
        epics::pvAccess::MonitorElementPtr ret;
        guard_t G(lock);
        if(running && complete && !inuse.empty()) {
            ret = inuse.front();
            inuse.pop_front();
        }
        return ret;
    }

    virtual void release(epics::pvAccess::MonitorElementPtr const & elem)
    {
        BaseMonitor::shared_pointer self;
        {
            guard_t G(lock);
            empty.push_back(elem);
            if(inoverflow)
                self = weakself.lock(); //TODO: concurrent release?
        }
        if(self)
            self->requestUpdate(); // may result in post()
    }
public:
    virtual void getStats(Stats& s) const
    {
        guard_t G(lock);
        s.nempty = empty.size();
        s.nfilled = inuse.size();
        s.noutstanding = nbuffers - s.nempty - s.nfilled;
    }
};

template<class CP>
struct BaseChannelProviderFactory : epics::pvAccess::ChannelProviderFactory
{
    typedef CP provider_type;
    std::string name;
    epicsMutex lock;
    std::tr1::weak_ptr<CP> last_shared;

    BaseChannelProviderFactory(const char *name) :name(name) {}
    virtual ~BaseChannelProviderFactory() {}

    virtual std::string getFactoryName() { return name; }

    virtual epics::pvAccess::ChannelProvider::shared_pointer sharedInstance()
    {
        epicsGuard<epicsMutex> G(lock);
        std::tr1::shared_ptr<CP> ret(last_shared.lock());
        if(!ret) {
            ret.reset(new CP());
            last_shared = ret;
        }
        return ret;
    }

    virtual epics::pvAccess::ChannelProvider::shared_pointer newInstance(const std::tr1::shared_ptr<epics::pvAccess::Configuration>&)
    {
        std::tr1::shared_ptr<CP> ret(new CP());
        return ret;
    }
};

#endif // PVAHELPER_H
