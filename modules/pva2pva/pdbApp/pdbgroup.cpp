
#include <stdio.h>

// rediect stdio/stderr for iocsh
#include <epicsStdio.h>

#include <epicsAtomic.h>
#include <dbAccess.h>
#include <dbChannel.h>
#include <dbStaticLib.h>
#include <asLib.h>

#include <pv/pvAccess.h>
#include <pv/configuration.h>
#include <pv/epicsException.h>

#include "helper.h"
#include "pdbgroup.h"
#include "pdb.h"

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

size_t PDBGroupPV::num_instances;
size_t PDBGroupChannel::num_instances;
size_t PDBGroupPut::num_instances;
size_t PDBGroupMonitor::num_instances;

typedef epicsGuard<epicsMutex> Guard;

void pdb_group_event(void *user_arg, struct dbChannel *chan,
                     int eventsRemaining, struct db_field_log *pfl)
{
    DBEvent *evt=(DBEvent*)user_arg;
    unsigned idx = evt->index;
    try{
        PDBGroupPV::shared_pointer self(std::tr1::static_pointer_cast<PDBGroupPV>(((PDBGroupPV*)evt->self)->shared_from_this()));
        PDBGroupPV::Info& info = self->members[idx];

        PDBGroupPV::interested_remove_t temp;
        {

            Guard G(self->lock);

            self->scratch.clear();
            if(evt->dbe_mask&DBE_PROPERTY || !self->monatomic)
            {
                DBScanLocker L(dbChannelRecord(info.chan));
                self->members[idx].pvif->put(self->scratch, evt->dbe_mask, pfl);

            } else {
                // we ignore 'pfl' (and the dbEvent queue) when collecting an atomic snapshot

                DBManyLocker L(info.locker); // lock only those records in the triggers list
                FOREACH(PDBGroupPV::Info::triggers_t::const_iterator, it, end, info.triggers)
                {
                    size_t i = *it;
                    // go get a consistent snapshot we must ignore the db_field_log which came through the dbEvent buffer
                    LocalFL FL(NULL, self->members[i].chan); // create a read fl if needed
                    self->members[i].pvif->put(self->scratch, evt->dbe_mask, FL.pfl);
                }
            }

            if(!(evt->dbe_mask&DBE_PROPERTY)) {
                if(!info.had_initial_VALUE) {
                    info.had_initial_VALUE = true;
                    assert(self->initial_waits>0);
                    self->initial_waits--;
                }
            } else {
                if(!info.had_initial_PROPERTY) {
                    info.had_initial_PROPERTY = true;
                    assert(self->initial_waits>0);
                    self->initial_waits--;
                }
            }

            if(self->initial_waits==0) {
                self->interested_iterating = true;

                FOREACH(PDBGroupPV::interested_t::const_iterator, it, end, self->interested) {
                    PDBGroupMonitor& mon = **it;
                    mon.post(G, self->scratch); // G unlocked
                }

                {
                    Guard G(self->lock);

                    assert(self->interested_iterating);

                    while(!self->interested_add.empty()) {
                        PDBGroupPV::interested_t::iterator first(self->interested_add.begin());
                        self->interested.insert(*first);
                        self->interested_add.erase(first);
                    }

                    temp.swap(self->interested_remove);
                    for(PDBGroupPV::interested_remove_t::iterator it(temp.begin()),
                        end(temp.end()); it != end; ++it)
                    {
                        self->interested.erase(static_cast<PDBGroupMonitor*>(it->get()));
                    }

                    self->interested_iterating = false;

                    self->finalizeMonitor();
                }
            }
        }

    }catch(std::tr1::bad_weak_ptr&){
        /* We are racing destruction of the PDBGroupPV, but things are ok.
         * The destructor is running, but has not completed db_cancel_event()
         * so storage is still valid.
         * Just do nothing
         */
    }catch(std::exception& e){
        std::cerr<<"Unhandled exception in pdb_group_event(): "<<e.what()<<"\n"
                 <<SHOW_EXCEPTION(e)<<"\n";
    }
}

PDBGroupPV::PDBGroupPV()
    :pgatomic(false)
    ,monatomic(false)
    ,interested_iterating(false)
    ,initial_waits(0)
{
    epics::atomic::increment(num_instances);
}

PDBGroupPV::~PDBGroupPV()
{
    epics::atomic::decrement(num_instances);
}

pva::Channel::shared_pointer
PDBGroupPV::connect(const std::tr1::shared_ptr<PDBProvider>& prov,
                    const pva::ChannelRequester::shared_pointer& req)
{
    PDBGroupChannel::shared_pointer ret(new PDBGroupChannel(shared_from_this(), prov, req));

    ret->cred.update(req);

    ret->aspvt.resize(members.size());
    for(size_t i=0, N=members.size(); i<N; i++)
    {
        ret->aspvt[i].add(members[i].chan, ret->cred);
    }

    return ret;
}

// caller must not hold lock
void PDBGroupPV::addMonitor(PDBGroupMonitor *mon)
{
    Guard G(lock);
    if(interested.empty() && interested_add.empty()) {
        // first monitor
        // start subscriptions

        size_t ievts = 0;
        for(size_t i=0; i<members.size(); i++) {
            PDBGroupPV::Info& info = members[i];

            if(!!info.evt_VALUE) {
                db_event_enable(info.evt_VALUE.subscript);
                db_post_single_event(info.evt_VALUE.subscript);
                ievts++;
                info.had_initial_VALUE = false;
            } else {
                info.had_initial_VALUE = true;
            }
            assert(info.evt_PROPERTY.subscript);
            db_event_enable(info.evt_PROPERTY.subscript);
            db_post_single_event(info.evt_PROPERTY.subscript);
            ievts++;
            info.had_initial_PROPERTY = false;
        }
        initial_waits = ievts;

    } else if(initial_waits==0) {
        // new subscriber and already had initial update
        mon->post(G);
    } // else new subscriber, but no initial update.  so just wait

    if(interested_iterating)
        interested_add.insert(mon);
    else
        interested.insert(mon);
}

// caller must not hold lock
void PDBGroupPV::removeMonitor(PDBGroupMonitor *mon)
{
    Guard G(lock);

    if(interested_add.erase(mon)) {
        // and+remove while iterating.  no-op

    } else if(interested_iterating) {
        // keep the monitor alive until we've finished iterating
        interested_remove.insert(mon->shared_from_this());

    } else {
        interested.erase(mon);
        finalizeMonitor();
    }
}

// must hold lock
void PDBGroupPV::finalizeMonitor()
{
    assert(!interested_iterating);

    if(!interested.empty())
        return;

    // last subscriber
    for(size_t i=0; i<members.size(); i++) {
        PDBGroupPV::Info& info = members[i];

        if(!!info.evt_VALUE) {
            db_event_disable(info.evt_VALUE.subscript);
        }
        db_event_disable(info.evt_PROPERTY.subscript);
    }
}

void PDBGroupPV::show(int lvl)
{
    // no locking as we only print things which are const after initialization

    printf("  Atomic Get/Put:%s Monitor:%s Members:%zu\n",
           pgatomic?"yes":"no", monatomic?"yes":"no", members.size());

    if(lvl<=1)
        return;

    for(members_t::const_iterator it(members.begin()), end(members.end());
        it != end; ++it)
    {
        const Info& info = *it;
        printf("  ");
        info.attachment.show(); // printf()s
        printf("\t<-> %s\n", dbChannelName(info.chan));
    }
}


PDBGroupChannel::PDBGroupChannel(const PDBGroupPV::shared_pointer& pv,
                                 const std::tr1::shared_ptr<pva::ChannelProvider>& prov,
                                 const pva::ChannelRequester::shared_pointer& req)
    :BaseChannel(pv->name, prov, req, pv->fielddesc)
    ,pv(pv)
{
    epics::atomic::increment(num_instances);
}

PDBGroupChannel::~PDBGroupChannel()
{
    epics::atomic::decrement(num_instances);
}

void PDBGroupChannel::printInfo(std::ostream& out)
{
    out<<"PDBGroupChannel";
}

pva::ChannelPut::shared_pointer
PDBGroupChannel::createChannelPut(
        pva::ChannelPutRequester::shared_pointer const & requester,
        pvd::PVStructure::shared_pointer const & pvRequest)
{
    PDBGroupPut::shared_pointer ret(new PDBGroupPut(shared_from_this(), requester, pvRequest));
    requester->channelPutConnect(pvd::Status(), ret, fielddesc);
    return ret;
}

pva::Monitor::shared_pointer
PDBGroupChannel::createMonitor(
        pva::MonitorRequester::shared_pointer const & requester,
        pvd::PVStructure::shared_pointer const & pvRequest)
{
    PDBGroupMonitor::shared_pointer ret(new PDBGroupMonitor(pv->shared_from_this(), requester, pvRequest));
    ret->weakself = ret;
    assert(!!pv->complete);
    guard_t G(pv->lock);
    ret->connect(G, pv->complete);
    return ret;
}



PDBGroupPut::PDBGroupPut(const PDBGroupChannel::shared_pointer& channel,
                         const requester_type::shared_pointer& requester,
                         const epics::pvData::PVStructure::shared_pointer &pvReq)
    :channel(channel)
    ,requester(requester)
    ,atomic(channel->pv->pgatomic)
    ,doWait(false)
    ,doProc(PVIF::ProcPassive)
    ,changed(new pvd::BitSet(channel->fielddesc->getNumberFields()))
    ,pvf(pvd::getPVDataCreate()->createPVStructure(channel->fielddesc))
{
    epics::atomic::increment(num_instances);
    try {
        getS<pvd::boolean>(pvReq, "record._options.atomic", atomic);

        getS<pvd::boolean>(pvReq, "record._options.block", doWait);

        std::string proccmd;
        if(getS<std::string>(pvReq, "record._options.process", proccmd)) {
            if(proccmd=="true") {
                doProc = PVIF::ProcForce;
            } else if(proccmd=="false") {
                doProc = PVIF::ProcInhibit;
                doWait = false; // no point in waiting
            } else if(proccmd=="passive") {
                doProc = PVIF::ProcPassive;
            } else {
                requester->message("process= expects: true|false|passive", pva::warningMessage);
            }
        }
    }catch(std::exception& e){
        requester->message(std::string("Error processing request options: ")+e.what());
    }

    pvf->getSubFieldT<pvd::PVBoolean>("record._options.atomic")->put(atomic);


    const size_t npvs = channel->pv->members.size();
    pvif.resize(npvs);
    for(size_t i=0; i<npvs; i++)
    {
        PDBGroupPV::Info& info = channel->pv->members[i];

        pvif[i].reset(info.builder->attach(pvf, info.attachment));
    }
}

PDBGroupPut::~PDBGroupPut()
{
    epics::atomic::decrement(num_instances);
}

void PDBGroupPut::put(pvd::PVStructure::shared_pointer const & value,
                       pvd::BitSet::shared_pointer const & changed)
{
    // assume value may be a different struct each time... lot of wasted prep work
    const size_t npvs = channel->pv->members.size();
    std::vector<std::tr1::shared_ptr<PVIF> > putpvif(npvs);
    pvd::shared_vector<AsWritePvt> asWritePvt(npvs);

    for(size_t i=0; i<npvs; i++)
    {
        PDBGroupPV::Info& info = channel->pv->members[i];

        AsWritePvt wrt(asTrapWriteWithData(
                           channel->aspvt.at(i).aspvt,
                           &channel->cred.user[0],
                           &channel->cred.host[0],
                           info.chan.chan,
                           info.chan->final_type,
                           info.chan->final_no_elements,
                           NULL
                           )
                       );
        asWritePvt[i].swap(wrt);

        if(!info.allowProc) continue;
        putpvif[i].reset(info.builder->attach(value, info.attachment));
    }

    pvd::Status ret;
    if(atomic) {
        DBManyLocker L(channel->pv->locker);
        for(size_t i=0; ret && i<npvs; i++) {
            if(!putpvif[i].get()) continue;

            ret |= putpvif[i]->get(*changed, doProc, channel->aspvt[i].canWrite());
        }

    } else {
        for(size_t i=0; ret && i<npvs; i++)
        {
            if(!putpvif[i].get()) continue;

            PDBGroupPV::Info& info = channel->pv->members[i];

            DBScanLocker L(dbChannelRecord(info.chan));

            ret |= putpvif[i]->get(*changed,
                                   info.allowProc ? doProc : PVIF::ProcInhibit,
                                   channel->aspvt[i].canWrite());
        }
    }

    requester_type::shared_pointer req(requester.lock());
    if(req)
        req->putDone(ret, shared_from_this());
}

void PDBGroupPut::get()
{
    const size_t npvs = pvif.size();

    changed->clear();
    if(atomic) {
        DBManyLocker L(channel->pv->locker);
        for(size_t i=0; i<npvs; i++) {
            LocalFL FL(NULL, channel->pv->members[i].chan);
            pvif[i]->put(*changed, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, FL.pfl);
        }
    } else {

        for(size_t i=0; i<npvs; i++)
        {
            PDBGroupPV::Info& info = channel->pv->members[i];

            DBScanLocker L(dbChannelRecord(info.chan));
            LocalFL FL(NULL, info.chan);
            pvif[i]->put(*changed, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, FL.pfl);
        }
    }
    //TODO: report unused fields as changed?
    changed->clear();
    changed->set(0);

    requester_type::shared_pointer req(requester.lock());
    if(req)
        req->getDone(pvd::Status(), shared_from_this(), pvf, changed);
}

PDBGroupMonitor::PDBGroupMonitor(const PDBGroupPV::shared_pointer& pv,
                 const epics::pvAccess::MonitorRequester::weak_pointer &requester,
                 const pvd::PVStructure::shared_pointer& pvReq)
    :BaseMonitor(pv->lock, requester, pvReq)
    ,pv(pv)
{
    epics::atomic::increment(num_instances);
}

PDBGroupMonitor::~PDBGroupMonitor()
{
    destroy();
    epics::atomic::decrement(num_instances);
}

void PDBGroupMonitor::destroy()
{
    BaseMonitor::destroy();
    PDBGroupPV::shared_pointer pv;
    {
        Guard G(lock);
        this->pv.swap(pv);
    }
}

void PDBGroupMonitor::onStart()
{
    pv->addMonitor(this);
}

void PDBGroupMonitor::onStop()
{
    pv->removeMonitor(this);
}

void PDBGroupMonitor::requestUpdate()
{
    Guard G(pv->lock);
    post(G);
}
