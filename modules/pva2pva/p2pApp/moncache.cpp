
#include <epicsAtomic.h>
#include <errlog.h>

#include <epicsMutex.h>
#include <epicsTimer.h>

#include <pv/pvAccess.h>

#define epicsExportSharedSymbols
#include "helper.h"
#include "pva2pva.h"
#include "chancache.h"

namespace pva = epics::pvAccess;
namespace pvd = epics::pvData;

size_t MonitorCacheEntry::num_instances;
size_t MonitorUser::num_instances;

namespace {
// fetch scalar value or default
template<typename T>
T getS(const pvd::PVStructurePtr& s, const char* name, T dft)
{
    try{
        return s->getSubFieldT<pvd::PVScalar>(name)->getAs<T>();
    }catch(std::runtime_error& e){
        return dft;
    }
}
}

MonitorCacheEntry::MonitorCacheEntry(ChannelCacheEntry *ent, const pvd::PVStructure::shared_pointer& pvr)
    :chan(ent)
    ,bufferSize(getS<pvd::uint32>(pvr, "record._options.queueSize", 2)) // should be same default as pvAccess, but not required
    ,havedata(false)
    ,done(false)
    ,nwakeups(0)
    ,nevents(0)
{
    epicsAtomicIncrSizeT(&num_instances);
}

MonitorCacheEntry::~MonitorCacheEntry()
{
    pvd::Monitor::shared_pointer M;
    M.swap(mon);
    if(M) {
        M->destroy();
    }
    epicsAtomicDecrSizeT(&num_instances);
    const_cast<ChannelCacheEntry*&>(chan) = NULL; // spoil to fault use after free
}

void
MonitorCacheEntry::monitorConnect(pvd::Status const & status,
                                  pvd::MonitorPtr const & monitor,
                                  pvd::StructureConstPtr const & structure)
{
    interested_t::vector_type tonotify;
    {
        Guard G(mutex());
        if(typedesc) {
            // we shouldn't have to deal with monitor type change since we
            // destroy() Monitors on Channel disconnect.
            std::cerr<<"monitorConnect() w/ new type.  Monitor has outlived it's connection.\n";
            monitor->stop();
            //TODO: unlisten()
            return;
        }
        typedesc = structure;

        if(status.isSuccess()) {
            startresult = monitor->start();
        } else {
            startresult = status;
        }

        if(startresult.isSuccess()) {
            lastelem.reset(new pvd::MonitorElement(pvd::getPVDataCreate()->createPVStructure(structure)));
        }

        // set typedesc and startresult for futured MonitorUsers
        // and copy snapshot of already interested MonitorUsers
        tonotify = interested.lock_vector();
    }

    if(!startresult.isSuccess())
        std::cout<<"upstream monitor start() fails\n";

    shared_pointer self(weakref); // keeps us alive all MonitorUsers are destroy()ed

    for(interested_t::vector_type::const_iterator it = tonotify.begin(),
        end = tonotify.end(); it!=end; ++it)
    {
        pvd::MonitorRequester::shared_pointer req((*it)->req);
        if(req) {
            req->monitorConnect(startresult, *it, structure);
        }
    }
}

// notificaton from upstream client that its monitor queue has
// become not empty (transition from empty to not empty)
// will not be called again unless we completely empty the upstream queue.
// If we don't then it is our responsibly to schedule more poll().
// Note: this probably means this is a PVA client RX thread.
void
MonitorCacheEntry::monitorEvent(pvd::MonitorPtr const & monitor)
{
    /* PVA is being tricky, the Monitor* passed to monitorConnect()
     * isn't the same one we see here!
     * The original was a ChannelMonitorImpl, we now see a MonitorStrategyQueue
     * owned by the original, which delegates deserialization and accumulation
     * of deltas into complete events for us.
     * However, we don't want to keep the MonitorStrategyQueue as it's
     * destroy() method is a no-op!
     */

    epicsAtomicIncrSizeT(&nwakeups);

    shared_pointer self(weakref); // keeps us alive in case all MonitorUsers are destroy()ed

    pva::MonitorElementPtr update;

    typedef std::vector<MonitorUser::shared_pointer> dsnotify_t;
    dsnotify_t dsnotify;

    {
        Guard G(mutex()); // MCE and MU guarded by the same mutex
        if(!havedata)
            havedata = true;

        //TODO: flow control, if all MU buffers are full, break before poll()==NULL
        while((update=monitor->poll()))
        {
            epicsAtomicIncrSizeT(&nevents);

            lastelem->pvStructurePtr->copyUnchecked(*update->pvStructurePtr,
                                                    *update->changedBitSet);
            *lastelem->changedBitSet = *update->changedBitSet;
            *lastelem->overrunBitSet = *update->overrunBitSet;
            monitor->release(update);
            update.reset();

            interested_t::iterator IIT(interested); // recursively locks interested.mutex() (assumes this->mutex() is interestd.mutex())
            for(interested_t::value_pointer pusr = IIT.next(); pusr; pusr = IIT.next())
            {
                MonitorUser *usr = pusr.get();

                {
                    Guard G(usr->mutex());
                    if(usr->initial)
                        continue; // no start() yet
                    // TODO: track overflow when !running (after stop())?
                    if(!usr->running || usr->empty.empty()) {
                        usr->inoverflow = true;

                        /* overrun |= lastelem->overrun           // upstream overflows
                         * overrun |= changed & lastelem->changed // downstream overflows
                         * changed |= lastelem->changed           // accumulate changes
                         */

                        *usr->overflowElement->overrunBitSet |= *lastelem->overrunBitSet;
                        usr->overflowElement->overrunBitSet->or_and(*usr->overflowElement->changedBitSet,
                                                                    *lastelem->changedBitSet);
                        *usr->overflowElement->changedBitSet |= *lastelem->changedBitSet;

                        usr->overflowElement->pvStructurePtr->copyUnchecked(*lastelem->pvStructurePtr,
                                                                            *lastelem->changedBitSet);

                        epicsAtomicIncrSizeT(&usr->ndropped);
                        continue;
                    }
                    // we only come out of overflow when downstream release()s an element to us
                    // empty.empty() does not imply inoverflow,
                    // however inoverflow does imply empty.empty()
                    assert(!usr->inoverflow);

                    if(usr->filled.empty())
                        dsnotify.push_back(pusr);

                    pvd::MonitorElementPtr elem(usr->empty.front());

                    *elem->overrunBitSet = *lastelem->overrunBitSet;
                    *elem->changedBitSet = *lastelem->changedBitSet;
                    // Note: can't use changed mask to optimize this copy since we don't know
                    //       the state of the free element
                    elem->pvStructurePtr->copyUnchecked(*lastelem->pvStructurePtr);

                    usr->filled.push_back(elem);
                    usr->empty.pop_front();

                    epicsAtomicIncrSizeT(&usr->nevents);
                }
            }
        }
    }

    // unlock here, race w/ stop(), unlisten()?
    //TODO: notify from worker thread

    FOREACH(dsnotify_t::iterator, it,end,dsnotify) {
        MonitorUser *usr = (*it).get();
        pvd::MonitorRequester::shared_pointer req(usr->req);
        epicsAtomicIncrSizeT(&usr->nwakeups);
        req->monitorEvent(*it); // notify when first item added to empty queue, may call poll(), release(), and others
    }
}

// notificaton from upstream client that no more monitor updates will come, ever
void
MonitorCacheEntry::unlisten(pvd::MonitorPtr const & monitor)
{
    pvd::Monitor::shared_pointer M;
    interested_t::vector_type tonotify;
    {
        Guard G(mutex());
        M.swap(mon);
        tonotify = interested.lock_vector();
        // assume that upstream won't call monitorEvent() again

        // cause future downstream start() to error
        startresult = pvd::Status(pvd::Status::STATUSTYPE_ERROR, "upstream unlisten()");
    }
    if(M) {
        M->destroy();
    }
    FOREACH(interested_t::vector_type::iterator, it, end, tonotify) {
        MonitorUser *usr = it->get();
        pvd::MonitorRequester::shared_pointer req(usr->req);
        if(usr->inuse.empty()) // TODO: what about stopped?
            req->unlisten(*it);
    }
}

std::string
MonitorCacheEntry::getRequesterName()
{
    return "MonitorCacheEntry";
}

MonitorUser::MonitorUser(const MonitorCacheEntry::shared_pointer &e)
    :entry(e)
    ,initial(true)
    ,running(false)
    ,inoverflow(false)
    ,nevents(0)
    ,ndropped(0)
{
    epicsAtomicIncrSizeT(&num_instances);
}

MonitorUser::~MonitorUser()
{
    epicsAtomicDecrSizeT(&num_instances);
}

// downstream server closes monitor
void
MonitorUser::destroy()
{
    {
        Guard G(mutex());
        running = false;
    }
}

pvd::Status
MonitorUser::start()
{
    pvd::MonitorRequester::shared_pointer req(this->req.lock());
    if(!req)
        return pvd::Status(pvd::Status::STATUSTYPE_FATAL, "already dead");

    bool doEvt = false;
    {
        Guard G(entry->mutex()); // MCE and MU have share a lock

        if(!entry->startresult.isSuccess())
            return entry->startresult;

        pvd::PVStructurePtr lval;
        if(entry->havedata)
            lval = entry->lastelem->pvStructurePtr;
        pvd::StructureConstPtr typedesc(entry->typedesc);

        if(initial) {
            initial = false;

            empty.resize(entry->bufferSize);
            pvd::PVDataCreatePtr fact(pvd::getPVDataCreate());
            for(unsigned i=0; i<empty.size(); i++) {
                empty[i].reset(new pvd::MonitorElement(fact->createPVStructure(typedesc)));
            }

            // extra element to accumulate updates during overflow
            overflowElement.reset(new pvd::MonitorElement(fact->createPVStructure(typedesc)));
        }

        doEvt = filled.empty();

        if(lval && !empty.empty()) {
            //already running, notify of initial element

            const pva::MonitorElementPtr& elem(empty.front());
            elem->pvStructurePtr->copy(*lval);
            elem->changedBitSet->set(0); // indicate all changed
            elem->overrunBitSet->clear();
            filled.push_back(elem);
            empty.pop_front();
        }

        doEvt &= !filled.empty();
        running = true;
    }
    if(doEvt)
        req->monitorEvent(shared_pointer(weakref)); // TODO: worker thread?
    return pvd::Status();
}

pvd::Status
MonitorUser::stop()
{
    Guard G(mutex());
    running = false;
    return pvd::Status::Ok;
}

pva::MonitorElementPtr
MonitorUser::poll()
{
    Guard G(mutex());
    pva::MonitorElementPtr ret;
    if(!filled.empty()) {
        ret = filled.front();
        inuse.insert(ret); // track which ones are out for client use
        filled.pop_front();
        //TODO: track lost buffers w/ wrapped shared_ptr?
    }
    return ret;
}

void
MonitorUser::release(pva::MonitorElementPtr const & monitorElement)
{
    Guard G(mutex());
    //TODO: ifdef DEBUG? (only track inuse when debugging?)
    std::set<epics::pvData::MonitorElementPtr>::iterator it = inuse.find(monitorElement);
    if(it!=inuse.end()) {
        inuse.erase(it);

        if(inoverflow) { // leaving overflow condition

            // to avoid copy, enqueue the current overflowElement
            // and replace it with the element being release()d

            filled.push_back(overflowElement);
            overflowElement = monitorElement;
            overflowElement->changedBitSet->clear();
            overflowElement->overrunBitSet->clear();

            inoverflow = false;
        } else {
            // push_back empty element
            empty.push_back(monitorElement);
        }
    } else {
        // oh no, we've been given an element which we didn't give to downstream
        //TODO: check empty and filled lists to see if this is one of ours, of from somewhere else
        throw std::invalid_argument("Can't release MonitorElement not in use");
    }
    // TODO: pipeline window update?
}

std::string
MonitorUser::getRequesterName()
{
    return "MonitorCacheEntry";
}
