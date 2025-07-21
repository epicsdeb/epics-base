#ifndef PDBSINGLE_H
#define PDBSINGLE_H

#include <deque>

#include <dbAccess.h>
#include <dbNotify.h>
#include <asLib.h>

#include <dbEvent.h>

#include <pv/pvAccess.h>

#include "helper.h"
#include "pvahelper.h"
#include "pvif.h"
#include "pdb.h"

struct PDBSingleMonitor;

struct QSRV_API PDBSinglePV : public PDBPV
{
    POINTER_DEFINITIONS(PDBSinglePV);
    weak_pointer weakself;
    inline shared_pointer shared_from_this() { return shared_pointer(weakself); }

    /* this dbChannel is shared by all operations,
     * which is safe as it's modify-able field(s) (addr.pfield)
     * are only access while the underlying record
     * is locked.
     */
    DBCH chan;
    // used for DBE_PROPERTY subscription when chan has filters
    DBCH chan2;
    PDBProvider::shared_pointer provider;

    // only for use in pdb_single_event()
    // which is not concurrent for VALUE/PROPERTY.
    epics::pvData::BitSet scratch;

    epicsMutex lock;

    p2p::auto_ptr<ScalarBuilder> builder;
    p2p::auto_ptr<PVIF> pvif;

    epics::pvData::PVStructurePtr complete; // complete copy from subscription

    typedef std::set<PDBSingleMonitor*> interested_t;
    bool interested_iterating;
    interested_t interested, interested_add;

    typedef std::set<BaseMonitor::shared_pointer> interested_remove_t;
    interested_remove_t interested_remove;

    DBEvent evt_VALUE, evt_PROPERTY;
    bool hadevent_VALUE, hadevent_PROPERTY;

    static size_t num_instances;

    PDBSinglePV(DBCH& chan,
                const PDBProvider::shared_pointer& prov);
    virtual ~PDBSinglePV();

    void activate();

    virtual
    epics::pvAccess::Channel::shared_pointer
        connect(const std::tr1::shared_ptr<PDBProvider>& prov,
                const epics::pvAccess::ChannelRequester::shared_pointer& req) OVERRIDE FINAL;

    void addMonitor(PDBSingleMonitor*);
    void removeMonitor(PDBSingleMonitor*);
    void finalizeMonitor();
};

struct PDBSingleChannel : public BaseChannel,
        public std::tr1::enable_shared_from_this<PDBSingleChannel>
{
    POINTER_DEFINITIONS(PDBSingleChannel);

    PDBSinglePV::shared_pointer pv;
    // storage referenced from aspvt
    ASCred cred;
    ASCLIENT aspvt;

    static size_t num_instances;

    PDBSingleChannel(const PDBSinglePV::shared_pointer& pv,
                const epics::pvAccess::ChannelRequester::shared_pointer& req);
    virtual ~PDBSingleChannel();

    virtual epics::pvAccess::ChannelPut::shared_pointer createChannelPut(
            epics::pvAccess::ChannelPutRequester::shared_pointer const & requester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest) OVERRIDE FINAL;
    virtual epics::pvData::Monitor::shared_pointer createMonitor(
            epics::pvData::MonitorRequester::shared_pointer const & requester,
            epics::pvData::PVStructure::shared_pointer const & pvRequest) OVERRIDE FINAL;

    virtual void printInfo(std::ostream& out) OVERRIDE FINAL;
};

struct PDBSinglePut : public epics::pvAccess::ChannelPut,
        public std::tr1::enable_shared_from_this<PDBSinglePut>
{
    POINTER_DEFINITIONS(PDBSinglePut);

    typedef epics::pvAccess::ChannelPutRequester requester_t;
    PDBSingleChannel::shared_pointer channel;
    requester_t::weak_pointer requester;

    epics::pvData::BitSetPtr changed, wait_changed;
    epics::pvData::PVStructurePtr pvf;
    p2p::auto_ptr<PVIF> pvif, wait_pvif;
    processNotify notify;
    int notifyBusy; // atomic: 0 - idle, 1 - active, 2 - being cancelled

    // effectively const after ctor
    PVIF::proc_t doProc;
    bool doWait;

    static size_t num_instances;

    PDBSinglePut(const PDBSingleChannel::shared_pointer& channel,
                 const epics::pvAccess::ChannelPutRequester::shared_pointer& requester,
                 const epics::pvData::PVStructure::shared_pointer& pvReq);
    virtual ~PDBSinglePut();

    virtual void destroy() OVERRIDE FINAL { pvif.reset(); channel.reset(); requester.reset(); }
    virtual std::tr1::shared_ptr<epics::pvAccess::Channel> getChannel() OVERRIDE FINAL { return channel; }
    virtual void cancel() OVERRIDE FINAL;
    virtual void lastRequest() OVERRIDE FINAL {}
    virtual void put(
            epics::pvData::PVStructure::shared_pointer const & pvPutStructure,
            epics::pvData::BitSet::shared_pointer const & putBitSet) OVERRIDE FINAL;
    virtual void get() OVERRIDE FINAL;
};

struct PDBSingleMonitor : public BaseMonitor
{
    POINTER_DEFINITIONS(PDBSingleMonitor);

    const PDBSinglePV::shared_pointer pv;

    static size_t num_instances;

    PDBSingleMonitor(const PDBSinglePV::shared_pointer& pv,
                     const requester_t::shared_pointer& requester,
                     const epics::pvData::PVStructure::shared_pointer& pvReq);
    virtual ~PDBSingleMonitor();

    virtual void onStart() OVERRIDE FINAL;
    virtual void onStop() OVERRIDE FINAL;
    virtual void requestUpdate() OVERRIDE FINAL;

    virtual void destroy() OVERRIDE FINAL;
};

#endif // PDBSINGLE_H
