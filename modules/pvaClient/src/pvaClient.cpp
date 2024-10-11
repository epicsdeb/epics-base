/* pvaClient.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.02
 */

#include <map>
#include <pv/createRequest.h>
#include <pv/clientFactory.h>
#include <pv/caProvider.h>

#define epicsExportSharedSymbols

#include <pv/pvaClient.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvAccess::ca;
using namespace std;

namespace epics { namespace pvaClient {


class epicsShareClass PvaClientChannelCache
{
public:
    PvaClientChannelCache(){}
    ~PvaClientChannelCache(){
         if(PvaClient::getDebug()) cout << "PvaClientChannelCache::~PvaClientChannelCache\n";
     }
    PvaClientChannelPtr getChannel(
        string const & channelName,
        string const & providerName);
    void addChannel(PvaClientChannelPtr const & pvaClientChannel);
    void showCache();
    size_t cacheSize();
private:
    map<string,PvaClientChannelPtr> pvaClientChannelMap;
};

PvaClientChannelPtr PvaClientChannelCache::getChannel(
    string const & channelName,
    string const & providerName)
{
    string name = channelName + providerName;
    map<string,PvaClientChannelPtr>::iterator iter = pvaClientChannelMap.find(name);
    if(iter!=pvaClientChannelMap.end()) return iter->second;
    return PvaClientChannelPtr();
}

void PvaClientChannelCache::addChannel(PvaClientChannelPtr const & pvaClientChannel)
{
     Channel::shared_pointer channel = pvaClientChannel->getChannel();
     string name = channel->getChannelName()
         + channel->getProvider()->getProviderName();
    map<string,PvaClientChannelPtr>::iterator iter = pvaClientChannelMap.find(name);
    if(iter!=pvaClientChannelMap.end()) {
        throw std::runtime_error("pvaClientChannelCache::addChannel channel already cached");
    }
    pvaClientChannelMap.insert(std::pair<string,PvaClientChannelPtr>(
         name,pvaClientChannel));
}

void PvaClientChannelCache::showCache()
{
    map<string,PvaClientChannelPtr>::iterator iter;
    for(iter = pvaClientChannelMap.begin(); iter != pvaClientChannelMap.end(); ++iter)
    {
         PvaClientChannelPtr pvaChannel = iter->second;
         Channel::shared_pointer channel = pvaChannel->getChannel();
         string channelName = channel->getChannelName();
         string providerName = channel->getProvider()->getProviderName();
         cout << "channel " << channelName << " provider " << providerName << endl;
         pvaChannel->showCache();
    }
}

size_t PvaClientChannelCache::cacheSize()
{
    return pvaClientChannelMap.size();

}

// MSVC doesn't like making this a class static data member:
static bool debug = 0;

void PvaClient::setDebug(bool value)
{
    debug = value;
}

bool PvaClient::getDebug()
{
    return debug;
}

PvaClientPtr PvaClient::get(std::string const & providerNames)
{
    static  PvaClientPtr master;
    static Mutex mutex;
    Lock xx(mutex);
    if(!master) {
        master = PvaClientPtr(new PvaClient(providerNames));
    }
    return master;
}

PvaClientPtr PvaClient::create() {return get();}

PvaClient::PvaClient(std::string const & providerNames)
:  pvaClientChannelCache(new PvaClientChannelCache()),
   pvaStarted(false),
   caStarted(false),
   channelRegistry(ChannelProviderRegistry::clients())
{
    stringstream ss(providerNames);
    string providerName;
    if(getDebug()) {
         cout<< "PvaClient::PvaClient()\n";
    }
    while (getline(ss, providerName, ' '))
    {
         if(providerName=="pva") {
             if(getDebug()) {
                  cout<< "calling ClientFactory::start()\n";
              }
             ClientFactory::start();
             pvaStarted = true;
         } else if(providerName=="ca") {
             if(getDebug()) {
                  cout<< "calling CAClientFactory::start()\n";
              }
             CAClientFactory::start();
             caStarted = true;
         } else {
             if(!channelRegistry->getProvider(providerName)) {
                 cerr << "PvaClient::get provider " << providerName  << " not known" << endl;
             }
         }
    }
}

PvaClient::~PvaClient() {
    if(getDebug()) {
        cout<< "PvaClient::~PvaClient()\n"
            << "pvaChannel cache:\n";
        showCache();
    }
    if(pvaStarted){
        if(getDebug()) cout<< "calling ClientFactory::stop()\n";
        ClientFactory::stop();
        if(getDebug()) cout<< "after calling ClientFactory::stop()\n";
    }
    if(caStarted) {
        if(getDebug()) cout<< "calling CAClientFactory::stop()\n";
        CAClientFactory::stop();
        if(getDebug()) cout<< "after calling CAClientFactory::stop()\n";
    }
channelRegistry.reset();
}

string PvaClient:: getRequesterName()
{
    static string name("pvaClient");
    RequesterPtr req = requester.lock();
    if(req) {
         return req->getRequesterName();
    }
    return name;
}

void  PvaClient::message(
        string const & message,
        MessageType messageType)
{
    RequesterPtr req = requester.lock();
    if(req) {
         req->message(message,messageType);
         return;
    }
    cout << getMessageTypeName(messageType) << " " << message << endl;
}

PvaClientChannelPtr PvaClient::channel(
        std::string const & channelName,
        std::string const & providerName,
        double timeOut)
{
    PvaClientChannelPtr pvaClientChannel =
        pvaClientChannelCache->getChannel(channelName,providerName);
    if(pvaClientChannel) return pvaClientChannel;
    pvaClientChannel = createChannel(channelName,providerName);
    pvaClientChannel->connect(timeOut);
    pvaClientChannelCache->addChannel(pvaClientChannel);
    return pvaClientChannel;
}

PvaClientChannelPtr PvaClient::createChannel(string const & channelName, string const & providerName)
{
     return PvaClientChannel::create(shared_from_this(),channelName,providerName);
}

void PvaClient::setRequester(RequesterPtr const & requester)
{
    this->requester = requester;
}

void PvaClient::clearRequester()
{
    requester.reset();
}

void PvaClient::showCache()
{
    if(pvaClientChannelCache->cacheSize()>=1) {
        pvaClientChannelCache->showCache();
    } else {
         cout << "pvaClientChannelCache is empty\n";
    }
}


size_t PvaClient::cacheSize()
{
    return pvaClientChannelCache->cacheSize();
}

}}
