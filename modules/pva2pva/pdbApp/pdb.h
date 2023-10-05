#ifndef PDB_H
#define PDB_H

#include <dbEvent.h>
#include <asLib.h>

#include <pv/configuration.h>
#include <pv/pvAccess.h>

#include "weakmap.h"

#include <pv/qsrv.h>

struct PDBProvider;

struct PDBPV
{
    POINTER_DEFINITIONS(PDBPV);

    epics::pvData::StructureConstPtr fielddesc;

    PDBPV() {}
    virtual ~PDBPV() {}

    virtual
    epics::pvAccess::Channel::shared_pointer
        connect(const std::tr1::shared_ptr<PDBProvider>& prov,
                const epics::pvAccess::ChannelRequester::shared_pointer& req) =0;

    // print info to stdout (with iocsh redirection)
    virtual void show(int lvl) {}
};

struct QSRV_API PDBProvider : public epics::pvAccess::ChannelProvider,
                                     public epics::pvAccess::ChannelFind,
                                     public std::tr1::enable_shared_from_this<PDBProvider>
{
    POINTER_DEFINITIONS(PDBProvider);

    explicit PDBProvider(const epics::pvAccess::Configuration::const_shared_pointer& =epics::pvAccess::Configuration::const_shared_pointer());
    virtual ~PDBProvider();

    // ChannelProvider
    virtual void destroy() OVERRIDE FINAL;
    virtual std::string getProviderName() OVERRIDE FINAL;
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(std::string const & channelName,
                                             epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester) OVERRIDE FINAL;
    virtual epics::pvAccess::ChannelFind::shared_pointer channelList(epics::pvAccess::ChannelListRequester::shared_pointer const & channelListRequester) OVERRIDE FINAL;
    virtual epics::pvAccess::Channel::shared_pointer createChannel(std::string const & channelName,
                                                                   epics::pvAccess::ChannelRequester::shared_pointer const & channelRequester,
                                           short priority = PRIORITY_DEFAULT) OVERRIDE FINAL;
    virtual epics::pvAccess::Channel::shared_pointer createChannel(std::string const & channelName,
                                                                   epics::pvAccess::ChannelRequester::shared_pointer const & channelRequester,
                                                                   short priority, std::string const & address) OVERRIDE FINAL;

    // ChannelFind
    virtual std::tr1::shared_ptr<ChannelProvider> getChannelProvider() OVERRIDE FINAL { return shared_from_this(); }
    virtual void cancel() OVERRIDE FINAL {/* our channelFind() is synchronous, so nothing to cancel */}

    typedef std::map<std::string, PDBPV::shared_pointer> persist_pv_map_t;
    persist_pv_map_t persist_pv_map;

    typedef weak_value_map<std::string, PDBPV> transient_pv_map_t;
    transient_pv_map_t transient_pv_map;

    dbEventCtx event_context;

    typedef std::list<std::string> group_files_t;
    static group_files_t group_files;

    static size_t num_instances;
};

QSRV_API
void QSRVRegistrar_counters();

class AsWritePvt {
    void * pvt;
public:
    AsWritePvt() :pvt(NULL) {}
    explicit AsWritePvt(void * pvt): pvt(pvt) {}
    ~AsWritePvt() {
        asTrapWriteAfterWrite(pvt);
    }
    void swap(AsWritePvt& o) {
        std::swap(pvt, o.pvt);
    }
private:
    AsWritePvt(const AsWritePvt&);
    AsWritePvt& operator=(const AsWritePvt&);
};

#endif // PDB_H
