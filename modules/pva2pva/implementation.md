PV Access to PV Access protocol gateway (aka. proxy)


Theory of Operation

The GW maintains a Channel Cache, which is a dictionary of client side channels
(shared_ptr<epics::pvAccess::Channel> instances)
in the NEVER_CONNECTED or CONNECTED states.

Each entry also has an activity flag and reference count.

The activity flag is set each time the server side receives a search request for a PV.

The reference count is incremented for each active server side channel.

Periodically the cache is iterated and any client channels with !activity and count==0 are dropped.
In addition the activity flag is unconditionally cleared.


Name search handling

The server side listens for name search requests.
When a request is received the channel cache is searched.
If no entry exists, then one is created and no further action is taken.
If an entry exists, but the client channel is not connected, then it's activiy flag is set and no further action is taken.
If a connected entry exists, then an affirmative response is sent to the requester.


When a channel create request is received, the channel cache is checked.
If no connected entry exists, then the request is failed.


Structure associations and ownership

```c

struct ServerContextImpl {
  vector<shared_ptr<ChannelProvider> > providers; // GWServerChannelProvider
};

struct GWServerChannelProvider : public pva::ChannelProvider {
  ChannelCache cache;
};

struct ChannelCache {
  weak_pointer<ChannelProvider> server;
  map<string, shared_ptr<ChannelCacheEntry> > entries;

  epicsMutex cacheLock; // guards entries
};

struct ChannelCacheEntry {
  ChannelCache * const cache;
  shared_ptr<Channel> channel; // InternalChannelImpl
  set<GWChannel*> interested;
};

struct InternalChannelImpl { // PVA client channel
  shared_ptr<ChannelRequester> requester; // ChannelCacheEntry::CRequester
};

struct ChannelCacheEntry::CRequester {
  weak_ptr<ChannelCacheEntry> chan;
};

struct GWChannel {
  shared_ptr<ChannelCacheEntry> entry;
  shared_ptr<ChannelRequester> requester; // pva::ServerChannelRequesterImpl
};

struct pva::ServerChannelImpl : public pva::ServerChannel
{
    shared_ptr<Channel> channel; // GWChannel
};
```

Threading and Locking

ServerContextImpl
  BeaconServerStatusProvider ?

  2x BlockingUDPTransport (bcast and mcast, one thread each)
    calls ChannelProvider::channelFind with no locks held

  BlockingTCPAcceptor
    BlockingServerTCPTransportCodec -> BlockingAbstractCodec (send and recv threads)
      ServerResponseHandler
        calls ChannelProvider::channelFind
        calls ChannelProvider::createChannel
        calls Channel::create*

InternalClientContextImpl

  2x BlockingUDPTransport (bcast listener and ucast tx/rx, one thread each)

  BlockingTCPConnector
    BlockingClientTCPTransportCodec -> BlockingSocketAbstractCodec (send and recv threads)
      ClientResponseHandler
        calls MonitorRequester::monitorEvent with MonitorStrategyQueue::m_mutex locked

TODO

ServerChannelRequesterImpl::channelStateChange() - placeholder, needs implementation


the send queue in BlockingAbstractCodec has no upper bound



Monitor

ServerChannelImpl calls GWChannel::createMonitor with a MonitorRequester which is ServerMonitorRequesterImpl

The MonitorRequester is given a Monitor which is GWMonitor

GWChannel calls InternalChannelImpl::createMonitor with a GWMonitorRequester

GWMonitorRequester is given a Monitor which is ChannelMonitorImpl

Updates originate from the client side, entering as an argument when GWMonitorRequester::monitorEvent is called,
and exiting to the server when passed as an argument of a call to ServerMonitorRequesterImpl::monitorEvent.

When an update is added to the monitor queue ServerMonitorRequesterImpl::monitorEvent is
called, as notification that the queue is not empty, which enqueues itself for transmission.
The associated TCP sender thread later calls ServerMonitorRequesterImpl::send(),
which calls GWMonitor::poll() to de-queue an event, which it encodes to the senders bytebuffer.
It then reschedules itself.

