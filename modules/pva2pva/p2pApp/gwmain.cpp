
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>

#if !defined(_WIN32)
#include <signal.h>
#define USE_SIGNAL
#endif

#include <epicsStdlib.h>
#include <epicsGetopt.h>
#include <iocsh.h>
#include <epicsTimer.h>
#include <libComRegister.h>

#include <pv/json.h>

#include <pv/pvAccess.h>
#include <pv/clientFactory.h>
#include <pv/configuration.h>
#include <pv/serverContext.h>
#include <pv/reftrack.h>
#include <pv/iocreftrack.h>
#include <pv/iocshelper.h>
#include <pv/logger.h>

#include "server.h"
#include "pva2pva.h"

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

extern int p2pReadOnly;

namespace {

pvd::StructureConstPtr schema(pvd::getFieldCreate()->createFieldBuilder()
                              ->add("version", pvd::pvUInt)
                              ->add("readOnly", pvd::pvBoolean)
                              ->addNestedStructureArray("clients")
                                 ->add("name", pvd::pvString)
                                 ->add("provider", pvd::pvString)
                                 ->add("addrlist", pvd::pvString)
                                 ->add("autoaddrlist", pvd::pvBoolean)
                                 ->add("serverport", pvd::pvUShort)
                                 ->add("bcastport", pvd::pvUShort)
                              ->endNested()
                              ->addNestedStructureArray("servers")
                                 ->add("name", pvd::pvString)
                                 ->addArray("clients", pvd::pvString)
                                 ->add("interface", pvd::pvString)
                                 ->add("addrlist", pvd::pvString)
                                 ->add("autoaddrlist", pvd::pvBoolean)
                                 ->add("serverport", pvd::pvUShort)
                                 ->add("bcastport", pvd::pvUShort)
                                 ->add("control_prefix", pvd::pvString)
                              ->endNested()
                              ->createStructure());


void usage(const char *me)
{
    std::cerr<<"Usage: "<<me<<" [-vhiIC] <config file>\n";
}

void getargs(ServerConfig& arg, int argc, char *argv[])
{
    int opt;
    bool checkonly = false;

    while( (opt=getopt(argc, argv, "qvhiIC"))!=-1)
    {
        switch(opt) {
        case 'q':
            arg.debug--;
            break;
        case 'v':
            arg.debug++;
            break;
        case 'I':
            arg.interactive = true;
            break;
        case 'i':
            arg.interactive = false;
            break;
        case 'C':
            checkonly = true;
            break;
        default:
            std::cerr<<"Unknown argument -"<<char(opt)<<"\n";
        case 'h':
            usage(argv[0]);
            exit(1);
        }
    }

    if(optind!=argc-1) {
        std::cerr<<"Exactly one positional argument expected\n";
        usage(argv[0]);
        exit(1);
    }

    arg.conf = pvd::getPVDataCreate()->createPVStructure(schema);
    std::ifstream strm(argv[optind]);
    pvd::parseJSON(strm, arg.conf);

    p2pReadOnly = arg.conf->getSubFieldT<pvd::PVScalar>("readOnly")->getAs<pvd::boolean>();

    unsigned version = arg.conf->getSubFieldT<pvd::PVUInt>("version")->get();
    if(version==0) {
        std::cerr<<"Warning: config file missing \"version\" key.  Assuming 1\n";
    } else if(version!=1) {
        std::cerr<<"config file version mis-match. expect 1 found "<<version<<"\n";
        exit(1);
    }
    if(arg.conf->getSubFieldT<pvd::PVStructureArray>("clients")->view().empty()) {
        std::cerr<<"No clients configured\n";
        exit(1);
    }
    if(arg.conf->getSubFieldT<pvd::PVStructureArray>("servers")->view().empty()) {
        std::cerr<<"No servers configured\n";
        exit(1);
    }

    if(checkonly) {
        std::cerr<<"Config file OK\n";
        exit(0);
    }
}

GWServerChannelProvider::shared_pointer configure_client(ServerConfig& arg, const pvd::PVStructurePtr& conf)
{
    std::string name(conf->getSubFieldT<pvd::PVString>("name")->get());
    std::string provider(conf->getSubFieldT<pvd::PVString>("provider")->get());

    LOG(pva::logLevelInfo, "Configure client '%s' with provider '%s'", name.c_str(), provider.c_str());

    pva::Configuration::shared_pointer C(pva::ConfigurationBuilder()
                                         .add("EPICS_PVA_ADDR_LIST", conf->getSubFieldT<pvd::PVString>("addrlist")->get())
                                         .add("EPICS_PVA_AUTO_ADDR_LIST", conf->getSubFieldT<pvd::PVScalar>("autoaddrlist")->getAs<std::string>())
                                         .add("EPICS_PVA_SERVER_PORT", conf->getSubFieldT<pvd::PVScalar>("serverport")->getAs<pvd::uint16>())
                                         .add("EPICS_PVA_BROADCAST_PORT", conf->getSubFieldT<pvd::PVScalar>("bcastport")->getAs<pvd::uint16>())
                                         .add("EPICS_PVA_DEBUG", arg.debug>=5 ? 5 : 0)
                                         .push_map()
                                         .build());

    pva::ChannelProvider::shared_pointer base(pva::ChannelProviderRegistry::clients()->createProvider(provider, C));
    if(!base)
        throw std::runtime_error("Can't create ChannelProvider");

    GWServerChannelProvider::shared_pointer ret(new GWServerChannelProvider(base));
    return ret;
}

pva::ServerContext::shared_pointer configure_server(ServerConfig& arg, const pvd::PVStructurePtr& conf)
{
    std::string name(conf->getSubFieldT<pvd::PVString>("name")->get());

    LOG(pva::logLevelInfo, "Configure server '%s'", name.c_str());

    pva::Configuration::shared_pointer C(pva::ConfigurationBuilder()
                                         .add("EPICS_PVAS_INTF_ADDR_LIST", conf->getSubFieldT<pvd::PVString>("interface")->get())
                                         .add("EPICS_PVAS_BEACON_ADDR_LIST", conf->getSubFieldT<pvd::PVString>("addrlist")->get())
                                         .add("EPICS_PVAS_AUTO_BEACON_ADDR_LIST", conf->getSubFieldT<pvd::PVScalar>("autoaddrlist")->getAs<std::string>())
                                         .add("EPICS_PVAS_SERVER_PORT", conf->getSubFieldT<pvd::PVScalar>("serverport")->getAs<pvd::uint16>())
                                         .add("EPICS_PVAS_BROADCAST_PORT", conf->getSubFieldT<pvd::PVScalar>("bcastport")->getAs<pvd::uint16>())
                                         .add("EPICS_PVA_DEBUG", arg.debug>=5 ? 5 : 0)
                                         .push_map()
                                         .build());

    pvd::PVStringArray::shared_pointer clients(conf->getSubFieldT<pvd::PVStringArray>("clients"));
    pvd::PVStringArray::const_svector names(clients->view());
    std::vector<pva::ChannelProvider::shared_pointer> providers;

    for(pvd::PVStringArray::const_svector::const_iterator it(names.begin()), end(names.end()); it!=end; ++it)
    {
        ServerConfig::clients_t::const_iterator it2(arg.clients.find(*it));
        if(it2==arg.clients.end())
            throw std::runtime_error("Server references non-existant client");
        providers.push_back(it2->second);
    }

    pva::ServerContext::shared_pointer ret(pva::ServerContext::create(pva::ServerContext::Config()
                                                                      .config(C)
                                                                      .providers(providers)));
    return ret;
}

volatile int quit;
epicsEvent done;

#ifdef USE_SIGNAL
void sigdone(int num)
{
    (void)num;
    quit = 1;
    done.signal();
}
#endif

ServerConfig* volatile theserver;

void iocsh_drop(const char *client, const char *channel)
{
    if(!theserver)
        return;
    try {
        theserver->drop(client, channel);
    }catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
    }
}

void gwsr(int lvl, const char *server)
{
    if(!theserver)
        return;
    try {
        theserver->status_server(lvl, server);
    }catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
    }
}

void gwcr(int lvl, const char *client, const char *channel)
{
    if(!theserver)
        return;
    try {
        theserver->status_client(lvl, client, channel);
    }catch(std::exception& e){
        std::cout<<"Error: "<<e.what()<<"\n";
    }
}

}// namespace

int main(int argc, char *argv[])
{
    try {
        pva::refTrackRegistrar();

        epics::iocshRegister<const char*, const char*, &iocsh_drop>("drop", "client", "channel");
        epics::iocshRegister<int, const char*, &gwsr>("gwsr", "level", "channel");
        epics::iocshRegister<int, const char*, const char*, &gwcr>("gwcr", "level", "client", "channel");

        libComRegister();
        registerReadOnly();
        epics::registerRefCounter("ChannelCacheEntry", &ChannelCacheEntry::num_instances);
        epics::registerRefCounter("ChannelCacheEntry::CRequester", &ChannelCacheEntry::CRequester::num_instances);
        epics::registerRefCounter("GWChannel", &GWChannel::num_instances);
        epics::registerRefCounter("MonitorCacheEntry", &MonitorCacheEntry::num_instances);
        epics::registerRefCounter("MonitorUser", &MonitorUser::num_instances);

        ServerConfig arg;
        theserver = &arg;
        getargs(arg, argc, argv);

        if(arg.debug>0)
            std::cout<<"Notice: This p2p gateway prototype has been superceded by the p4p.gw gateway\n"
                       "        which has exciting new features including granular access control,\n"
                       "        and status PVs including bandwidth usage reports.\n"
                       "        p2p is considered deprecated by its author, and will receive\n"
                       "        minimal maintainance effort going forward.\n"
                       "        Users are encouraged to migrate to p4p.gw.\n"
                       "\n"
                       "        https://mdavidsaver.github.io/p4p/gw.html\n"
                       "\n";

        pva::pvAccessLogLevel lvl;
        if(arg.debug<0)
            lvl = pva::logLevelError;
        else if(arg.debug==0)
            lvl = pva::logLevelWarn;
        else if(arg.debug==1)
            lvl = pva::logLevelInfo;
        else if(arg.debug==2)
            lvl = pva::logLevelDebug;
        else if(arg.debug==3)
            lvl = pva::logLevelTrace;
        else if(arg.debug>=4)
            lvl = pva::logLevelAll;
        SET_LOG_LEVEL(lvl);

        pva::ClientFactory::start();

        pvd::PVStructureArray::const_svector arr;

        arr = arg.conf->getSubFieldT<pvd::PVStructureArray>("clients")->view();

        for(size_t i=0; i<arr.size(); i++) {
            if(!arr[i]) continue;
            const pvd::PVStructurePtr& client = arr[i];

            std::string name(client->getSubFieldT<pvd::PVString>("name")->get());
            if(name.empty())
                throw std::runtime_error("Client with empty name not allowed");

            ServerConfig::clients_t::const_iterator it(arg.clients.find(name));
            if(it!=arg.clients.end())
                throw std::runtime_error(std::string("Duplicate client name not allowed : ")+name);

            arg.clients[name] = configure_client(arg, client);
        }

        arr = arg.conf->getSubFieldT<pvd::PVStructureArray>("servers")->view();

        for(size_t i=0; i<arr.size(); i++) {
            if(!arr[i]) continue;
            const pvd::PVStructurePtr& server = arr[i];

            std::string name(server->getSubFieldT<pvd::PVString>("name")->get());
            if(name.empty())
                throw std::runtime_error("Server with empty name not allowed");

            ServerConfig::servers_t::const_iterator it(arg.servers.find(name));
            if(it!=arg.servers.end())
                throw std::runtime_error(std::string("Duplicate server name not allowed : ")+name);

            arg.servers[name] = configure_server(arg, server);
        }

        int ret = 0;
        if(arg.interactive) {
            ret = iocsh(NULL);
        } else {
#ifdef USE_SIGNAL
            signal(SIGINT, sigdone);
            signal(SIGTERM, sigdone);
            signal(SIGQUIT, sigdone);
#endif

            while(!quit) {
                done.wait();
            }
        }

        theserver = 0;

        return ret;
    }catch(std::exception& e){
        std::cerr<<"Fatal Error : "<<e.what()<<"\n";
        return 1;
    }
}
