/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvAccessCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include <stdexcept>
#include <vector>
#include <utility>

#define epicsExportSharedSymbols
#include <pv/rpcServer.h>
#include <pv/serverContextImpl.h>
#include <pv/wildcard.h>

using namespace epics::pvData;
using std::string;

namespace epics {
namespace pvAccess {


class ChannelRPCServiceImpl :
    public ChannelRPC,
    public RPCResponseCallback,
    public std::tr1::enable_shared_from_this<ChannelRPCServiceImpl>
{
private:
    Channel::shared_pointer m_channel;
    ChannelRPCRequester::shared_pointer m_channelRPCRequester;
    RPCServiceAsync::shared_pointer m_rpcService;
    AtomicBoolean m_lastRequest;

public:
    ChannelRPCServiceImpl(
        Channel::shared_pointer const & channel,
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        RPCServiceAsync::shared_pointer const & rpcService) :
        m_channel(channel),
        m_channelRPCRequester(channelRPCRequester),
        m_rpcService(rpcService),
        m_lastRequest()
    {
    }

    virtual ~ChannelRPCServiceImpl()
    {
        destroy();
    }

    virtual void requestDone(
        epics::pvData::Status const & status,
        epics::pvData::PVStructure::shared_pointer const & result
    )
    {
        m_channelRPCRequester->requestDone(status, shared_from_this(), result);

        if (m_lastRequest.get())
            destroy();
    }

    virtual void request(epics::pvData::PVStructure::shared_pointer const & pvArgument)
    {
        try
        {
            m_rpcService->request(pvArgument, shared_from_this());
        }
        catch (std::exception& ex)
        {
            // handle user unexpected errors
            Status errorStatus(Status::STATUSTYPE_FATAL, ex.what());

            m_channelRPCRequester->requestDone(errorStatus, shared_from_this(), PVStructure::shared_pointer());

            if (m_lastRequest.get())
                destroy();
        }
        catch (...)
        {
            // handle user unexpected errors
            Status errorStatus(Status::STATUSTYPE_FATAL,
                               "Unexpected exception caught while calling RPCServiceAsync.request(PVStructure, RPCResponseCallback).");

            m_channelRPCRequester->requestDone(errorStatus, shared_from_this(), PVStructure::shared_pointer());

            if (m_lastRequest.get())
                destroy();
        }

        // we wait for callback to be called
    }

    void lastRequest()
    {
        m_lastRequest.set();
    }

    virtual Channel::shared_pointer getChannel()
    {
        return m_channel;
    }

    virtual void cancel()
    {
        // noop
    }

    virtual void destroy()
    {
        // noop
    }
};




class RPCChannel :
    public Channel,
    public std::tr1::enable_shared_from_this<RPCChannel>
{
private:

    AtomicBoolean m_destroyed;

    ChannelProvider::shared_pointer m_provider;
    string m_channelName;
    ChannelRequester::shared_pointer m_channelRequester;

    RPCServiceAsync::shared_pointer m_rpcService;

public:
    POINTER_DEFINITIONS(RPCChannel);

    RPCChannel(
        ChannelProvider::shared_pointer const & provider,
        string const & channelName,
        ChannelRequester::shared_pointer const & channelRequester,
        RPCServiceAsync::shared_pointer const & rpcService) :
        m_provider(provider),
        m_channelName(channelName),
        m_channelRequester(channelRequester),
        m_rpcService(rpcService)
    {
    }

    virtual ~RPCChannel()
    {
        destroy();
    }

    virtual std::tr1::shared_ptr<ChannelProvider> getProvider()
    {
        return m_provider;
    }

    virtual std::string getRemoteAddress()
    {
        // local
        return getChannelName();
    }

    virtual ConnectionState getConnectionState()
    {
        return (!m_destroyed.get()) ?
               Channel::CONNECTED :
               Channel::DESTROYED;
    }

    virtual std::string getChannelName()
    {
        return m_channelName;
    }

    virtual std::tr1::shared_ptr<ChannelRequester> getChannelRequester()
    {
        return m_channelRequester;
    }

    virtual AccessRights getAccessRights(epics::pvData::PVField::shared_pointer const & /*pvField*/)
    {
        return none;
    }

    virtual ChannelRPC::shared_pointer createChannelRPC(
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        epics::pvData::PVStructure::shared_pointer const & /*pvRequest*/)
    {
        // nothing expected to be in pvRequest

        if (channelRPCRequester.get() == 0)
            throw std::invalid_argument("channelRPCRequester == null");

        if (m_destroyed.get())
        {
            ChannelRPC::shared_pointer nullPtr;
            channelRPCRequester->channelRPCConnect(epics::pvData::Status(epics::pvData::Status::STATUSTYPE_ERROR, "channel destroyed"), nullPtr);
            return nullPtr;
        }

        // TODO use std::make_shared
        std::tr1::shared_ptr<ChannelRPCServiceImpl> tp(
            new ChannelRPCServiceImpl(shared_from_this(), channelRPCRequester, m_rpcService)
        );
        ChannelRPC::shared_pointer channelRPCImpl = tp;
        channelRPCRequester->channelRPCConnect(Status::Ok, channelRPCImpl);
        return channelRPCImpl;
    }

    virtual void printInfo(std::ostream& out)
    {
        out << "RPCChannel: ";
        out << getChannelName();
        out << " [";
        out << Channel::ConnectionStateNames[getConnectionState()];
        out << "]";
    }

    virtual string getRequesterName()
    {
        return getChannelName();
    }

    virtual void destroy()
    {
        m_destroyed.set();
    }
};

Channel::shared_pointer createRPCChannel(ChannelProvider::shared_pointer const & provider,
        std::string const & channelName,
        ChannelRequester::shared_pointer const & channelRequester,
        RPCServiceAsync::shared_pointer const & rpcService)
{
    // TODO use std::make_shared
    std::tr1::shared_ptr<RPCChannel> tp(
        new RPCChannel(provider, channelName, channelRequester, rpcService)
    );
    Channel::shared_pointer channel = tp;
    return channel;
}


class RPCChannelProvider :
    public virtual ChannelProvider,
    public virtual ChannelFind,
    public std::tr1::enable_shared_from_this<RPCChannelProvider> {

public:
    POINTER_DEFINITIONS(RPCChannelProvider);

    static const string PROVIDER_NAME;

    static const Status noSuchChannelStatus;

    // TODO thread pool support

    RPCChannelProvider() {
    }

    virtual string getProviderName() {
        return PROVIDER_NAME;
    }

    virtual std::tr1::shared_ptr<ChannelProvider> getChannelProvider()
    {
        return shared_from_this();
    }

    virtual void cancel() {}

    virtual void destroy() {}

    virtual ChannelFind::shared_pointer channelFind(std::string const & channelName,
            ChannelFindRequester::shared_pointer const & channelFindRequester)
    {
        bool found;
        {
            Lock guard(m_mutex);
            found = (m_services.find(channelName) != m_services.end()) ||
                    findWildService(channelName);
        }
        ChannelFind::shared_pointer thisPtr(shared_from_this());
        channelFindRequester->channelFindResult(Status::Ok, thisPtr, found);
        return thisPtr;
    }


    virtual ChannelFind::shared_pointer channelList(
        ChannelListRequester::shared_pointer const & channelListRequester)
    {
        if (!channelListRequester.get())
            throw std::runtime_error("null requester");

        PVStringArray::svector channelNames;
        {
            Lock guard(m_mutex);
            channelNames.reserve(m_services.size());
            for (RPCServiceMap::const_iterator iter = m_services.begin();
                    iter != m_services.end();
                    iter++)
                channelNames.push_back(iter->first);
        }

        ChannelFind::shared_pointer thisPtr(shared_from_this());
        channelListRequester->channelListResult(Status::Ok, thisPtr, freeze(channelNames), false);
        return thisPtr;
    }

    virtual Channel::shared_pointer createChannel(
        std::string const & channelName,
        ChannelRequester::shared_pointer const & channelRequester,
        short /*priority*/)
    {
        RPCServiceAsync::shared_pointer service;

        RPCServiceMap::const_iterator iter;
        {
            Lock guard(m_mutex);
            iter = m_services.find(channelName);
        }
        if (iter != m_services.end())
            service = iter->second;

        // check for wild services
        if (!service)
            service = findWildService(channelName);

        if (!service)
        {
            Channel::shared_pointer nullChannel;
            channelRequester->channelCreated(noSuchChannelStatus, nullChannel);
            return nullChannel;
        }

        // TODO use std::make_shared
        std::tr1::shared_ptr<RPCChannel> tp(
            new RPCChannel(
                shared_from_this(),
                channelName,
                channelRequester,
                service));
        Channel::shared_pointer rpcChannel = tp;
        channelRequester->channelCreated(Status::Ok, rpcChannel);
        return rpcChannel;
    }

    virtual Channel::shared_pointer createChannel(
        std::string const & /*channelName*/,
        ChannelRequester::shared_pointer const & /*channelRequester*/,
        short /*priority*/,
        std::string const & /*address*/)
    {
        // this will never get called by the pvAccess server
        throw std::runtime_error("not supported");
    }

    void registerService(std::string const & serviceName, RPCServiceAsync::shared_pointer const & service)
    {
        Lock guard(m_mutex);
        m_services[serviceName] = service;

        if (isWildcardPattern(serviceName))
            m_wildServices.push_back(std::make_pair(serviceName, service));
    }

    void unregisterService(std::string const & serviceName)
    {
        Lock guard(m_mutex);
        m_services.erase(serviceName);

        if (isWildcardPattern(serviceName))
        {
            for (RPCWildServiceList::iterator iter = m_wildServices.begin();
                    iter != m_wildServices.end();
                    iter++)
                if (iter->first == serviceName)
                {
                    m_wildServices.erase(iter);
                    break;
                }
        }
    }

private:
    // assumes sync on services
    RPCServiceAsync::shared_pointer findWildService(string const & wildcard)
    {
        if (!m_wildServices.empty())
            for (RPCWildServiceList::iterator iter = m_wildServices.begin();
                    iter != m_wildServices.end();
                    iter++)
                if (Wildcard::wildcardfit(iter->first.c_str(), wildcard.c_str()))
                    return iter->second;

        return RPCServiceAsync::shared_pointer();
    }

    // (too) simple check
    bool isWildcardPattern(string const & pattern)
    {
        return
            (pattern.find('*') != string::npos ||
             pattern.find('?') != string::npos ||
             (pattern.find('[') != string::npos && pattern.find(']') != string::npos));
    }

    typedef std::map<string, RPCServiceAsync::shared_pointer> RPCServiceMap;
    RPCServiceMap m_services;

    typedef std::vector<std::pair<string, RPCServiceAsync::shared_pointer> > RPCWildServiceList;
    RPCWildServiceList m_wildServices;

    epics::pvData::Mutex m_mutex;
};

const string RPCChannelProvider::PROVIDER_NAME("rpcService");
const Status RPCChannelProvider::noSuchChannelStatus(Status::STATUSTYPE_ERROR, "no such channel");


RPCServer::RPCServer(const Configuration::const_shared_pointer &conf)
    :m_channelProviderImpl(new RPCChannelProvider)
{
    m_serverContext = ServerContext::create(ServerContext::Config()
                                            .config(conf)
                                            .provider(m_channelProviderImpl));
}

RPCServer::~RPCServer()
{
    // multiple destroy call is OK
    destroy();
}

void RPCServer::printInfo()
{
    std::cout << m_serverContext->getVersion().getVersionString() << std::endl;
    m_serverContext->printInfo();
}

void RPCServer::run(int seconds)
{
    m_serverContext->run(seconds);
}

struct ThreadRunnerParam {
    RPCServer::shared_pointer server;
    int timeToRun;
};

static void threadRunner(void* usr)
{
    ThreadRunnerParam* pusr = static_cast<ThreadRunnerParam*>(usr);
    ThreadRunnerParam param = *pusr;
    delete pusr;

    param.server->run(param.timeToRun);
}

/// Method requires usage of std::tr1::shared_ptr<RPCServer>. This instance must be
/// owned by a shared_ptr instance.
void RPCServer::runInNewThread(int seconds)
{
    epics::auto_ptr<ThreadRunnerParam> param(new ThreadRunnerParam());
    param->server = shared_from_this();
    param->timeToRun = seconds;

    epicsThreadCreate("RPCServer thread",
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackBig),
                      threadRunner, param.get());

    // let the thread delete 'param'
    param.release();
}

void RPCServer::destroy()
{
    m_serverContext->shutdown();
}

void RPCServer::registerService(std::string const & serviceName, RPCServiceAsync::shared_pointer const & service)
{
    m_channelProviderImpl->registerService(serviceName, service);
}

void RPCServer::unregisterService(std::string const & serviceName)
{
    m_channelProviderImpl->unregisterService(serviceName);
}

}
}
