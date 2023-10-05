
#include <vector>
#include <utility>

#include <errlog.h>
#include <epicsString.h>
#include <epicsAtomic.h>

// printfs in this file will be redirected for capture
#include <epicsStdio.h>

#include <dbAccess.h>
#include <dbChannel.h>
#include <dbStaticLib.h>
#include <dbNotify.h>

#include <dbEvent.h>

#include <pv/pvAccess.h>
#include <pv/configuration.h>

#include "helper.h"
#include "pdbsingle.h"
#include "pvif.h"
#ifdef USE_MULTILOCK
#  include "pdbgroup.h"
#endif

#include <epicsExport.h>

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

int PDBProviderDebug;

namespace {

struct Splitter {
    const char sep, *cur, *end;
    Splitter(const char *s, char sep)
        :sep(sep), cur(s)
    {
        assert(s);
        end = strchr(cur, sep);
    }
    bool operator!() const { return !cur; }
    bool snip(std::string& ret) {
        if(!cur) return false;
        if(end) ret = std::string(cur, end-cur);
        else    ret = std::string(cur);
        if(end) {
            cur = end+1;
            end = strchr(cur, sep);
        } else {
            cur = NULL;
        }
        return true;
    }
};

struct GroupMemberInfo {
    GroupMemberInfo() :putorder(0) {}

    std::string pvname, // aka. name passed to dbChannelOpen()
                pvfldname; // PVStructure sub-field
    std::string structID; // ID to assign to sub-field
    std::string type; // mapping type
    typedef std::set<std::string> triggers_t;
    triggers_t triggers; // names in GroupInfo::members_names which are post()d on events from pvfldname
    int putorder;

    bool operator<(const GroupMemberInfo& o) const {
        return putorder<o.putorder;
    }
};

struct GroupInfo {
    GroupInfo(const std::string& name) : name(name),atomic(Unset),hastriggers(false) {}
    std::string name, structID;

    typedef std::vector<GroupMemberInfo> members_t;
    members_t members;

    typedef std::map<std::string, size_t> members_map_t;
    members_map_t members_map;

    typedef std::set<std::string> triggers_set_t;
    typedef std::map<std::string, triggers_set_t> triggers_t;
    triggers_t triggers;

    enum tribool {Unset,True,False} atomic;
    bool hastriggers;
};

// Iterates all PDB records and gathers info() to construct PDB groups
struct PDBProcessor
{
    typedef std::map<std::string, GroupInfo> groups_t;
    groups_t groups;

    // validate trigger mappings and process into bit map form
    void resolveTriggers()
    {
        FOREACH(groups_t::iterator, it, end, groups) { // for each group
            GroupInfo& info = it->second;

            if(info.hastriggers) {
                FOREACH(GroupInfo::triggers_t::iterator, it2, end2, info.triggers) { // for each trigger source
                    const std::string& src = it2->first;
                    GroupInfo::triggers_set_t& targets = it2->second;

                    GroupInfo::members_map_t::iterator it2x = info.members_map.find(src);
                    if(it2x==info.members_map.end()) {
                        fprintf(stderr, "Error: Group \"%s\" defines triggers from non-existant field \"%s\"\n",
                                info.name.c_str(), src.c_str());
                        continue;
                    }
                    GroupMemberInfo& srcmem = info.members[it2x->second];

                    if(PDBProviderDebug>2)
                        fprintf(stderr, "  pdb trg '%s.%s'  -> ",
                                info.name.c_str(), src.c_str());

                    FOREACH(GroupInfo::triggers_set_t::const_iterator, it3, end3, targets) { // for each trigger target
                        const std::string& target = *it3;

                        if(target=="*") {
                            for(size_t i=0; i<info.members.size(); i++) {
                                if(info.members[i].pvname.empty())
                                    continue;
                                srcmem.triggers.insert(info.members[i].pvfldname);
                                if(PDBProviderDebug>2)
                                    fprintf(stderr, "%s, ", info.members[i].pvfldname.c_str());
                            }

                        } else {

                            GroupInfo::members_map_t::iterator it3x = info.members_map.find(target);
                            if(it3x==info.members_map.end()) {
                                fprintf(stderr, "Error: Group \"%s\" defines triggers to non-existant field \"%s\"\n",
                                        info.name.c_str(), target.c_str());
                                continue;
                            }
                            const GroupMemberInfo& targetmem = info.members[it3x->second];

                            if(targetmem.pvname.empty()) {
                                if(PDBProviderDebug>2)
                                    fprintf(stderr, "<ignore: %s>, ", targetmem.pvfldname.c_str());

                            } else {
                                // and finally, update source BitSet
                                srcmem.triggers.insert(targetmem.pvfldname);
                                if(PDBProviderDebug>2)
                                    fprintf(stderr, "%s, ", targetmem.pvfldname.c_str());
                            }
                        }
                    }

                    if(PDBProviderDebug>2) fprintf(stderr, "\n");
                }
            } else {
                if(PDBProviderDebug>1) fprintf(stderr, "  pdb default triggers for '%s'\n", info.name.c_str());

                FOREACH(GroupInfo::members_t::iterator, it2, end2, info.members) {
                    GroupMemberInfo& mem = *it2;
                    if(mem.pvname.empty())
                        continue;

                    mem.triggers.insert(mem.pvfldname); // default is self trigger
                }
            }
        }
    }

    PDBProcessor()
    {
#ifdef USE_MULTILOCK
        GroupConfig conf;
#endif

        // process info(Q:Group, ...)
        for(pdbRecordIterator rec; !rec.done(); rec.next())
        {
            const char *json = rec.info("Q:group");
            if(!json) continue;
#ifndef USE_MULTILOCK
            static bool warned;
            if(!warned) {
                warned = true;
                fprintf(stderr, "%s: ignoring info(Q:Group, ...\n", rec.name());
            }
#endif
            if(PDBProviderDebug>2) {
                fprintf(stderr, "%s: info(Q:Group, ...\n", rec.name());
            }

#ifdef USE_MULTILOCK
            try {
                GroupConfig::parse(json, rec.name(), conf);
                if(!conf.warning.empty())
                    fprintf(stderr, "%s: warning(s) from info(Q:group, ...\n%s", rec.name(), conf.warning.c_str());
            }catch(std::exception& e){
                fprintf(stderr, "%s: Error parsing info(\"Q:group\", ... : %s\n",
                        rec.record()->name, e.what());
            }
#endif
        }

        // process group definition files
        for(PDBProvider::group_files_t::const_iterator it(PDBProvider::group_files.begin()), end(PDBProvider::group_files.end());
            it != end; ++it)
        {
            std::ifstream jfile(it->c_str());
            if(!jfile.is_open()) {
                fprintf(stderr, "Error opening \"%s\"\n", it->c_str());
                continue;
            }

            std::vector<char> contents;
            size_t pos=0u;
            while(true) {
                contents.resize(pos+1024u);
                if(!jfile.read(&contents[pos], contents.size()-pos))
                    break;
                pos += jfile.gcount();
            }

            if(jfile.bad() || !jfile.eof()) {
                fprintf(stderr, "Error reading \"%s\"\n", it->c_str());
                continue;
            }

            contents.push_back('\0');
            const char *json = &contents[0];

            if(PDBProviderDebug>2) {
                fprintf(stderr, "Process dbGroup file \"%s\"\n", it->c_str());
            }

#ifdef USE_MULTILOCK
            try {
                GroupConfig::parse(json, "", conf);
                if(!conf.warning.empty())
                    fprintf(stderr, "warning(s) from dbGroup file \"%s\"\n%s", it->c_str(), conf.warning.c_str());
            }catch(std::exception& e){
                fprintf(stderr, "Error from dbGroup file \"%s\"\n%s", it->c_str(), e.what());
            }
#endif
        }

#ifdef USE_MULTILOCK
        for(GroupConfig::groups_t::const_iterator git=conf.groups.begin(), gend=conf.groups.end();
            git!=gend; ++git)
        {
            const std::string& grpname = git->first;
            const GroupConfig::Group& grp = git->second;
            try {

                if(dbChannelTest(grpname.c_str())==0) {
                    fprintf(stderr, "%s : Error: Group name conflicts with record name.  Ignoring...\n", grpname.c_str());
                    continue;
                }

                groups_t::iterator it = groups.find(grpname);
                if(it==groups.end()) {
                    // lazy creation of group
                    std::pair<groups_t::iterator, bool> ins(groups.insert(std::make_pair(grpname, GroupInfo(grpname))));
                    it = ins.first;
                }
                GroupInfo *curgroup = &it->second;

                if(!grp.id.empty())
                    curgroup->structID = grp.id;

                for(GroupConfig::Group::fields_t::const_iterator fit=grp.fields.begin(), fend=grp.fields.end();
                    fit!=fend; ++fit)
                {
                    const std::string& fldname = fit->first;
                    const GroupConfig::Field& fld = fit->second;

                    if(curgroup->members_map.find(fldname) != curgroup->members_map.end()) {
                        fprintf(stderr, "%s.%s Warning: ignoring duplicate mapping %s\n",
                                grpname.c_str(), fldname.c_str(),
                                fld.channel.c_str());
                        continue;
                    }

                    curgroup->members.push_back(GroupMemberInfo());
                    GroupMemberInfo& info = curgroup->members.back();
                    info.pvname = fld.channel;
                    info.pvfldname = fldname;
                    info.structID = fld.id;
                    info.putorder = fld.putorder;
                    info.type = fld.type;
                    curgroup->members_map[fldname] = (size_t)-1; // placeholder  see below

                    if(PDBProviderDebug>2) {
                        fprintf(stderr, "  pdb map '%s.%s' <-> '%s'\n",
                                curgroup->name.c_str(),
                                curgroup->members.back().pvfldname.c_str(),
                                curgroup->members.back().pvname.c_str());
                    }

                    if(!fld.trigger.empty()) {
                        GroupInfo::triggers_t::iterator it = curgroup->triggers.find(fldname);
                        if(it==curgroup->triggers.end()) {
                            std::pair<GroupInfo::triggers_t::iterator, bool> ins(curgroup->triggers.insert(
                                                                                     std::make_pair(fldname, GroupInfo::triggers_set_t())));
                            it = ins.first;
                        }

                        Splitter sep(fld.trigger.c_str(), ',');
                        std::string target;

                        while(sep.snip(target)) {
                            curgroup->hastriggers = true;
                            it->second.insert(target);
                        }
                    }
                }

                if(grp.atomic_set) {
                    GroupInfo::tribool V = grp.atomic ? GroupInfo::True : GroupInfo::False;

                    if(curgroup->atomic!=GroupInfo::Unset && curgroup->atomic!=V)
                        fprintf(stderr, "%s Warning: pdb atomic setting inconsistent '%s'\n",
                                grpname.c_str(), curgroup->name.c_str());

                    curgroup->atomic=V;

                    if(PDBProviderDebug>2)
                        fprintf(stderr, "  pdb atomic '%s' %s\n",
                                curgroup->name.c_str(), curgroup->atomic ? "YES" : "NO");
                }

            }catch(std::exception& e){
                fprintf(stderr, "Error processing Q:group \"%s\" : %s\n",
                        grpname.c_str(), e.what());
            }
        }

        // re-sort GroupInfo::members to ensure the shorter names appear first
        // allows use of 'existing' PVIFBuilder on leaves.
        for(groups_t::iterator it = groups.begin(), end = groups.end(); it!=end; ++it)
        {
            GroupInfo& info = it->second;
            std::sort(info.members.begin(),
                      info.members.end());

            info.members_map.clear();

            for(size_t i=0, N=info.members.size(); i<N; i++)
            {
                info.members_map[info.members[i].pvfldname] = i;
            }
        }

        resolveTriggers();
        // must not re-sort members after this point as resolveTriggers()
        // has stored array indicies.
#endif
    }

};
}

size_t PDBProvider::num_instances;

std::list<std::string> PDBProvider::group_files;

PDBProvider::PDBProvider(const epics::pvAccess::Configuration::const_shared_pointer &)
{
    /* Long view
     * 1. PDBProcessor collects info() tags and builds config of groups and group fields
     *    (including those w/o a dbChannel)
     * 2. Build pvd::Structure and discard those w/o dbChannel
     * 3. Build the lockers for the triggers of each group field
     */
    PDBProcessor proc;
    pvd::FieldCreatePtr fcreate(pvd::getFieldCreate());
    pvd::PVDataCreatePtr pvbuilder(pvd::getPVDataCreate());

    pvd::StructureConstPtr _options(fcreate->createFieldBuilder()
                                    ->addNestedStructure("_options")
                                        ->add("queueSize", pvd::pvUInt)
                                        ->add("atomic", pvd::pvBoolean)
                                    ->endNested()
                                    ->createStructure());

#ifdef USE_MULTILOCK
    // assemble group PVD structure definitions and build dbLockers
    FOREACH(PDBProcessor::groups_t::const_iterator, it, end, proc.groups)
    {
        const GroupInfo &info=it->second;
        try{
            if(persist_pv_map.find(info.name)!=persist_pv_map.end())
                throw std::runtime_error("name already in used");

            PDBGroupPV::shared_pointer pv(new PDBGroupPV());
            pv->weakself = pv;
            pv->name = info.name;

            pv->pgatomic = info.atomic!=GroupInfo::False; // default true if Unset
            pv->monatomic = info.hastriggers;

            // some gymnastics because Info isn't copyable
            pvd::shared_vector<PDBGroupPV::Info> members;
            typedef std::map<std::string, size_t> members_map_t;
            members_map_t members_map;
            {
                size_t nchans = 0;
                for(size_t i=0, N=info.members.size(); i<N; i++)
                    if(!info.members[i].pvname.empty())
                        nchans++;
                pvd::shared_vector<PDBGroupPV::Info> temp(nchans);
                members.swap(temp);
            }

            std::vector<dbCommon*> records(members.size());

            pvd::FieldBuilderPtr builder(fcreate->createFieldBuilder());
            builder = builder->add("record", _options);

            if(!info.structID.empty())
                builder = builder->setId(info.structID);

            for(size_t i=0, J=0, N=info.members.size(); i<N; i++)
            {
                const GroupMemberInfo &mem = info.members[i];

                // parse down attachment point to build/traverse structure
                FieldName parts(mem.pvfldname);

                if(!parts.empty()) {
                    for(size_t j=0; j<parts.size()-1; j++) {
                        if(parts[j].isArray())
                            builder = builder->addNestedStructureArray(parts[j].name);
                        else
                            builder = builder->addNestedStructure(parts[j].name);
                    }
                }

                if(!mem.structID.empty())
                    builder = builder->setId(mem.structID);

                DBCH chan;
                if(!mem.pvname.empty()) {
                    DBCH temp(mem.pvname);
                    unsigned ftype = dbChannelFieldType(temp);

                    // can't include in multi-locking
                    if(ftype>=DBF_INLINK && ftype<=DBF_FWDLINK)
                        throw std::runtime_error("Can't include link fields in group");

                    chan.swap(temp);
                }

                std::tr1::shared_ptr<PVIFBuilder> pvifbuilder(PVIFBuilder::create(mem.type, chan.chan));

                if(!parts.empty())
                    builder = pvifbuilder->dtype(builder, parts.back().name);
                else
                    builder = pvifbuilder->dtype(builder, "");

                if(!parts.empty()) {
                    for(size_t j=0; j<parts.size()-1; j++)
                        builder = builder->endNested();
                }

                if(!mem.pvname.empty()) {
                    members_map[mem.pvfldname] = J;
                    PDBGroupPV::Info& info = members[J];

                    DBCH chan2;
                    if(chan.chan && (ellCount(&chan.chan->pre_chain)>0 || ellCount(&chan.chan->post_chain)>0)) {
                        DBCH temp(mem.pvname);
                        info.chan2.swap(chan2);
                    }

                    info.allowProc = mem.putorder != std::numeric_limits<int>::min();
                    info.builder = PTRMOVE(pvifbuilder);
                    assert(info.builder.get());

                    info.attachment.swap(parts);
                    info.chan.swap(chan);

                    // info.triggers populated below

                    assert(info.chan);
                    records[J] = dbChannelRecord(info.chan);

                    J++;
                }
            }
            pv->members.swap(members);

            pv->fielddesc = builder->createStructure();
            pv->complete = pvbuilder->createPVStructure(pv->fielddesc);

            pv->complete->getSubFieldT<pvd::PVBoolean>("record._options.atomic")->put(pv->monatomic);

            DBManyLock L(&records[0], records.size(), 0);
            pv->locker.swap(L);

            // construct locker for records triggered by each member
            for(size_t i=0, J=0, N=info.members.size(); i<N; i++)
            {
                const GroupMemberInfo &mem = info.members[i];
                if(mem.pvname.empty()) continue;
                PDBGroupPV::Info& info = pv->members[J++];

                if(mem.triggers.empty()) continue;

                std::vector<dbCommon*> trig_records;
                trig_records.reserve(mem.triggers.size());

                FOREACH(GroupMemberInfo::triggers_t::const_iterator, it, end, mem.triggers) {
                    members_map_t::const_iterator imap(members_map.find(*it));
                    if(imap==members_map.end())
                        throw std::logic_error("trigger resolution missed map to non-dbChannel");

                    info.triggers.push_back(imap->second);
                    trig_records.push_back(records[imap->second]);
                }

                DBManyLock L(&trig_records[0], trig_records.size(), 0);
                info.locker.swap(L);
            }

            persist_pv_map[info.name] = pv;

        }catch(std::exception& e){
            fprintf(stderr, "%s: Error Group not created: %s\n", info.name.c_str(), e.what());
        }
    }
#else
    if(!proc.groups.empty()) {
        fprintf(stderr, "Group(s) were defined, but need Base >=3.16.0.2 to function.  Ignoring.\n");
    }
#endif // USE_MULTILOCK

    event_context = db_init_events();
    if(!event_context)
        throw std::runtime_error("Failed to create dbEvent context");
    int ret = db_start_events(event_context, "PDB-event", NULL, NULL, epicsThreadPriorityCAServerLow-1);
    if(ret!=DB_EVENT_OK)
        throw std::runtime_error("Failed to stsart dbEvent context");

    // setup group monitors
#ifdef USE_MULTILOCK
    for(persist_pv_map_t::iterator next = persist_pv_map.begin(),
                                    end = persist_pv_map.end(),
                                     it = next!=end ? next++ : end;
        it != end; it = next==end ? end : next++)
    {
        const PDBPV::shared_pointer& ppv = it->second;
        PDBGroupPV *pv = dynamic_cast<PDBGroupPV*>(ppv.get());
        if(!pv)
            continue;
        try {

            // prepare for monitor

            size_t i=0;
            FOREACH(PDBGroupPV::members_t::iterator, it2, end2, pv->members)
            {
                PDBGroupPV::Info& info = *it2;
                info.evt_VALUE.index = info.evt_PROPERTY.index = i++;
                info.evt_VALUE.self = info.evt_PROPERTY.self = pv;
                assert(info.chan);

                info.pvif.reset(info.builder->attach(pv->complete, info.attachment));

                // TODO: don't need evt_PROPERTY for PVIF plain
                dbChannel *pchan = info.chan2.chan ? info.chan2.chan : info.chan.chan;
                info.evt_PROPERTY.create(event_context, pchan, &pdb_group_event, DBE_PROPERTY);

                if(!info.triggers.empty()) {
                    info.evt_VALUE.create(event_context, info.chan, &pdb_group_event, DBE_VALUE|DBE_ALARM);
                }
            }
        }catch(std::exception& e){
            fprintf(stderr, "%s: Error during dbEvent setup : %s\n", pv->name.c_str(), e.what());
            persist_pv_map.erase(it);
        }
    }
#endif // USE_MULTILOCK
    epics::atomic::increment(num_instances);
}

PDBProvider::~PDBProvider()
{
    epics::atomic::decrement(num_instances);

    destroy();
}

void PDBProvider::destroy()
{
    dbEventCtx ctxt = NULL;

    persist_pv_map_t ppv;
    {
        epicsGuard<epicsMutex> G(transient_pv_map.mutex());
        persist_pv_map.swap(ppv);
        std::swap(ctxt, event_context);
    }
    ppv.clear(); // indirectly calls all db_cancel_events()
    if(ctxt) db_close_events(ctxt);
}

std::string PDBProvider::getProviderName() { return "QSRV"; }

namespace {
struct ChannelFindRequesterNOOP : public pva::ChannelFind
{
    const pva::ChannelProvider::weak_pointer provider;
    ChannelFindRequesterNOOP(const pva::ChannelProvider::shared_pointer& prov) : provider(prov) {}
    virtual ~ChannelFindRequesterNOOP() {}
    virtual void destroy() {}
    virtual std::tr1::shared_ptr<pva::ChannelProvider> getChannelProvider() { return provider.lock(); }
    virtual void cancel() {}
};
}

pva::ChannelFind::shared_pointer
PDBProvider::channelFind(const std::string &channelName, const pva::ChannelFindRequester::shared_pointer &requester)
{
    pva::ChannelFind::shared_pointer ret(new ChannelFindRequesterNOOP(shared_from_this()));

    bool found = false;
    {
        epicsGuard<epicsMutex> G(transient_pv_map.mutex());
        if(persist_pv_map.find(channelName)!=persist_pv_map.end()
                || transient_pv_map.find(channelName)
                || dbChannelTest(channelName.c_str())==0)
            found = true;
    }
    requester->channelFindResult(pvd::Status(), ret, found);
    return ret;
}

pva::ChannelFind::shared_pointer
PDBProvider::channelList(pva::ChannelListRequester::shared_pointer const & requester)
{
    pva::ChannelFind::shared_pointer ret;
    pvd::PVStringArray::svector names;
    for(pdbRecordIterator rec; !rec.done(); rec.next())
    {
        names.push_back(rec.name());
    }
    {
        epicsGuard<epicsMutex> G(transient_pv_map.mutex());

        for(persist_pv_map_t::const_iterator it=persist_pv_map.begin(), end=persist_pv_map.end();
            it != end; ++it)
        {
            names.push_back(it->first);
        }
    }
    // check for duplicates?
    requester->channelListResult(pvd::Status::Ok,
                                 shared_from_this(),
                                 pvd::freeze(names), false);
    return ret;
}

pva::Channel::shared_pointer
PDBProvider::createChannel(std::string const & channelName,
                           pva::ChannelRequester::shared_pointer const & channelRequester,
                           short priority)
{
    return createChannel(channelName, channelRequester, priority, "???");
}

pva::Channel::shared_pointer
PDBProvider::createChannel(std::string const & channelName,
                                                               pva::ChannelRequester::shared_pointer const & requester,
                                                               short priority, std::string const & address)
{
    pva::Channel::shared_pointer ret;
    PDBPV::shared_pointer pv;
    pvd::Status status;

    {
        epicsGuard<epicsMutex> G(transient_pv_map.mutex());

        pv = transient_pv_map.find(channelName);
        if(!pv) {
            persist_pv_map_t::const_iterator it=persist_pv_map.find(channelName);
            if(it!=persist_pv_map.end()) {
                pv = it->second;
            }
        }
        if(!pv) {
            dbChannel *pchan = dbChannelCreate(channelName.c_str());
            if(pchan) {
                DBCH chan(pchan);
                pv.reset(new PDBSinglePV(chan, shared_from_this()));
                transient_pv_map.insert(channelName, pv);
                PDBSinglePV::shared_pointer spv = std::tr1::static_pointer_cast<PDBSinglePV>(pv);
                spv->weakself = spv;
                spv->activate();
            }
        }
    }
    if(pv) {
        ret = pv->connect(shared_from_this(), requester);
    }
    if(!ret) {
        status = pvd::Status(pvd::Status::STATUSTYPE_ERROR, "not found");
    }
    requester->channelCreated(status, ret);
    return ret;
}

FieldName::FieldName(const std::string& pv)
{
    if(pv.empty())
        return;
    Splitter S(pv.c_str(), '.');
    std::string part;
    while(S.snip(part)) {
        if(part.empty())
            throw std::runtime_error("Empty field component in: "+pv);

        if(part[part.size()-1]==']') {
            const size_t open = part.find_last_of('['),
                         N = part.size();
            bool ok = open!=part.npos;
            epicsUInt32 index = 0;
            for(size_t i=open+1; ok && i<(N-1); i++) {
                ok &= part[i]>='0' && part[i]<='9';
                index = 10*index + part[i] - '0';
            }
            if(!ok)
                throw std::runtime_error("Invalid field array sub-script in : "+pv);

            parts.push_back(Component(part.substr(0, open), index));

        } else {
            parts.push_back(Component(part));
        }
    }
    if(parts.empty())
        throw std::runtime_error("Empty field name");
    if(parts.back().isArray())
        throw std::runtime_error("leaf field may not have sub-script : "+pv);
}

epics::pvData::PVFieldPtr
FieldName::lookup(const epics::pvData::PVStructurePtr& S, epics::pvData::PVField **ppsar) const
{
    if(ppsar)
        *ppsar = 0;

    pvd::PVFieldPtr ret = S;
    for(size_t i=0, N=parts.size(); i<N; i++) {
        pvd::PVStructure* parent = dynamic_cast<pvd::PVStructure*>(ret.get());
        if(!parent)
            throw std::runtime_error("mid-field is not structure");

        ret = parent->getSubFieldT(parts[i].name);

        if(parts[i].isArray()) {
            pvd::PVStructureArray* sarr = dynamic_cast<pvd::PVStructureArray*>(ret.get());
            if(!sarr)
                throw std::runtime_error("indexed field is not structure array");

            if(ppsar && !*ppsar)
                *ppsar = sarr;

            pvd::PVStructureArray::const_svector V(sarr->view());

            if(V.size()<=parts[i].index || !V[parts[i].index]) {
                // automatic re-size and ensure non-null
                V.clear(); // drop our extra ref so that reuse() might avoid a copy
                pvd::PVStructureArray::svector E(sarr->reuse());

                if(E.size()<=parts[i].index)
                    E.resize(parts[i].index+1);

                if(!E[parts[i].index])
                    E[parts[i].index] = pvd::getPVDataCreate()->createPVStructure(sarr->getStructureArray()->getStructure());

                ret = E[parts[i].index];

                sarr->replace(pvd::freeze(E));

            } else {
                ret = V[parts[i].index];
            }
        }
    }
    return ret;
}

void FieldName::show() const
{
    if(parts.empty()) {
        printf("/");
        return;
    }

    bool first = true;
    for(size_t i=0, N=parts.size(); i<N; i++)
    {
        if(!first) {
            printf(".");
        } else {
            first = false;
        }
        if(parts[i].isArray())
            printf("%s[%u]", parts[i].name.c_str(), (unsigned)parts[i].index);
        else
            printf("%s", parts[i].name.c_str());
    }
}

extern "C" {
epicsExportAddress(int, PDBProviderDebug);
}
