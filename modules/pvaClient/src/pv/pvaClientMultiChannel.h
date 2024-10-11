/* pvaClientMultiChannel.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.02
 */
#ifndef PVACLIENTMULTICHANNEL_H
#define PVACLIENTMULTICHANNEL_H

#ifdef epicsExportSharedSymbols
#   define pvaClientMultiChannelEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/ntmultiChannel.h>
#include <pv/createRequest.h>

#ifdef pvaClientMultiChannelEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#   undef pvaClientMultiChannelEpicsExportSharedSymbols
#endif


#include <pv/pvaClient.h>


namespace epics { namespace pvaClient {


class PvaClientMultiChannel;
typedef std::tr1::shared_ptr<PvaClientMultiChannel> PvaClientMultiChannelPtr;
class PvaClientMultiGetDouble;
typedef std::tr1::shared_ptr<PvaClientMultiGetDouble> PvaClientMultiGetDoublePtr;
class PvaClientMultiPutDouble;
typedef std::tr1::shared_ptr<PvaClientMultiPutDouble> PvaClientMultiPutDoublePtr;
class PvaClientMultiMonitorDouble;
typedef std::tr1::shared_ptr<PvaClientMultiMonitorDouble> PvaClientMultiMonitorDoublePtr;

class PvaClientNTMultiGet;
typedef std::tr1::shared_ptr<PvaClientNTMultiGet> PvaClientNTMultiGetPtr;
class PvaClientNTMultiPut;
typedef std::tr1::shared_ptr<PvaClientNTMultiPut> PvaClientNTMultiPutPtr;
class PvaClientNTMultiMonitor;
typedef std::tr1::shared_ptr<PvaClientNTMultiMonitor> PvaClientNTMultiMonitorPtr;
class PvaClientNTMultiData;
typedef std::tr1::shared_ptr<PvaClientNTMultiData> PvaClientNTMultiDataPtr;


typedef epics::pvData::shared_vector<PvaClientChannelPtr> PvaClientChannelArray;

/**
 * @brief PvaMultiChannel is a synchronous interface for accessing multiple channels.
 *
 * @author mrk
 */
class epicsShareClass PvaClientMultiChannel :
    public std::tr1::enable_shared_from_this<PvaClientMultiChannel>
{
public:
    POINTER_DEFINITIONS(PvaClientMultiChannel);
    /** @brief Create a PvaClientMultiChannel.
     * @param pvaClient The interface to pvaClient.
     * @param channelNames The names of the channel..
     * @param providerName The name of the provider.
     * This is also used for the provider for all channels
     * with providerNames.size less than channelNames.size()
     * @param maxNotConnected The maximum number of channels that can be disconnected.
     * @param providerNames The providerName for each Channells
     * @return The interface to the PvaClientMultiChannel
     */
    static PvaClientMultiChannelPtr create(
         PvaClientPtr const &pvaClient,
         epics::pvData::shared_vector<const std::string> const & channelNames,
         std::string const & providerName = "pva",
         size_t maxNotConnected=0,
         epics::pvData::shared_vector<const std::string> const & providerNames
             = epics::pvData::shared_vector<const std::string>()
     );
     /**
     * @brief Destructor
     */
    ~PvaClientMultiChannel();
    /** @brief Get the channelNames.
     * @return The names.
     */
    epics::pvData::shared_vector<const std::string> getChannelNames();
    /** @brief Connect to the channels.
     *
     * This calls issueConnect and waitConnect.
     * An exception is thrown if connect fails.
     * @param timeout The time to wait for connecting to the channel.
     * @return status of request
     */
    epics::pvData::Status connect(double timeout=5);
    /** @brief Are all channels connected?
     * @return if all are connected.
     */
    bool allConnected();
    /** @brief Has a connection state change occured?
     * @return (true, false) if (at least one, no) channel has changed state.
     */
    bool connectionChange();
    /** @brief Get the connection state of each channel.
     * @return The state of each channel.
     */
    epics::pvData::shared_vector<epics::pvData::boolean> getIsConnected();
    /** @brief Get the pvaClientChannelArray.
     * @return The shared pointer.
     */
    PvaClientChannelArray getPvaClientChannelArray();
    /** @brief Get pvaClient.
     * @return The shared pointer.
     */
    PvaClientPtr getPvaClient();
    /**
     * @brief create a pvaClientMultiGetDouble
     * @return The interface.
     */
    PvaClientMultiGetDoublePtr createGet();
    /**
     * @brief Create a pvaClientMultiPutDouble.
     * @return The interface.
     */
    PvaClientMultiPutDoublePtr createPut();
    /**
     * @brief Create a pvaClientMultiMonitorDouble.
     * @return The interface.
     */
    PvaClientMultiMonitorDoublePtr createMonitor();
    /**
     * @brief Create a pvaClientNTMultiPut.
     * @return The interface.
     */
    PvaClientNTMultiPutPtr createNTPut();
    /**
     * @brief Create a pvaClientNTMultiGet;
     * @param request The request for each channel.
     * @return The interface.
     */
    PvaClientNTMultiGetPtr createNTGet(
        std::string const &request = "field(value,alarm,timeStamp)");
    /**
     * @brief Create a pvaClientNTMultiPut.
     * @param request The request for each channel.
     * @return The interface.
     */
    PvaClientNTMultiMonitorPtr createNTMonitor(
        std::string const &request= "field(value,alarm,timeStamp)");
private:
    PvaClientMultiChannel(
        PvaClientPtr const &pvaClient,
        epics::pvData::shared_vector<const std::string> const & channelNames,
        std::string const & providerName,
        size_t maxNotConnected,
        epics::pvData::shared_vector<const std::string> const & providerNames);
    void checkConnected();

    PvaClientPtr pvaClient;
    epics::pvData::shared_vector<const std::string> channelNames;
    std::string providerName;
    size_t maxNotConnected;
    epics::pvData::shared_vector<const std::string> const & providerNames;

    size_t numChannel;
    size_t numProviderNames;
    epics::pvData::Mutex mutex;

    size_t numConnected;
    bool firstConnect;
    PvaClientChannelArray pvaClientChannelArray;
    epics::pvData::shared_vector<epics::pvData::boolean> isConnected;
    epics::pvData::CreateRequest::shared_pointer createRequest;
};

/**
 *  @brief Provides channelGet to multiple channels where each channel has a numeric scalar value field.
 */
class epicsShareClass PvaClientMultiGetDouble :
    public std::tr1::enable_shared_from_this<PvaClientMultiGetDouble>
{

public:
    POINTER_DEFINITIONS(PvaClientMultiGetDouble);
protected:
    static PvaClientMultiGetDoublePtr create(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);
    friend class PvaClientMultiChannel;
public:
    /**
     * @brief Destructor
     */
    ~PvaClientMultiGetDouble();
     /**
      * @brief Create a channelGet for each channel.
      */
    void connect();
    /**
     * @brief Get the data.
     * @return The double[] where each element is the value field of the corresponding channel.
     */
    epics::pvData::shared_vector<double> get();
private:
    PvaClientMultiGetDouble(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);

    PvaClientMultiChannelPtr pvaClientMultiChannel;
    PvaClientChannelArray pvaClientChannelArray;
    size_t nchannel;
    epics::pvData::Mutex mutex;

    epics::pvData::shared_vector<double> doubleValue;
    std::vector<PvaClientGetPtr> pvaClientGet;
    bool isGetConnected;
};

/**
 * @brief Provides channelPut to multiple channels where each channel has a numeric scalar value field.
 */
class epicsShareClass PvaClientMultiPutDouble :
    public std::tr1::enable_shared_from_this<PvaClientMultiPutDouble>
{

public:
    POINTER_DEFINITIONS(PvaClientMultiPutDouble);
protected:
    static PvaClientMultiPutDoublePtr create(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);
    friend class PvaClientMultiChannel;
public:
   /**
     * @brief Destructor
     */
    ~PvaClientMultiPutDouble();
     /**
     * @brief Create a channelPut for each channel.
     */
    void connect();
    /** @brief put data to each channel as a double
     * @param data The array of data for each channel.
     */
    void put(epics::pvData::shared_vector<double> const &data);

private:
    PvaClientMultiPutDouble(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);

    PvaClientMultiChannelPtr pvaClientMultiChannel;
    PvaClientChannelArray pvaClientChannelArray;
    size_t nchannel;
    epics::pvData::Mutex mutex;

    std::vector<PvaClientPutPtr> pvaClientPut;
    bool isPutConnected;
};

/**
 * @brief Provides a monitor to multiple channels where each channel has a numeric scalar value field.
 */
class epicsShareClass PvaClientMultiMonitorDouble :
    public std::tr1::enable_shared_from_this<PvaClientMultiMonitorDouble>
{

public:
    POINTER_DEFINITIONS(PvaClientMultiMonitorDouble);
protected:
    static PvaClientMultiMonitorDoublePtr create(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);
    friend class PvaClientMultiChannel;
public:
   /**
     * @brief Destructor
     */
    ~PvaClientMultiMonitorDouble();
     /**
      * @brief Connect a channel monitor for each channel.
      */
    void connect();
     /**
     * @brief Poll each channel.
     *
     * If any has new data it is used to update the double[].
     * @return (false,true) if (no, at least one) value was updated.
     */
    bool poll();
    /**
     * @brief Wait until poll returns true.
     * @param secondsToWait The time to keep trying.
     * A thread sleep of .1 seconds occurs between each call to poll.
     * @return (false,true) if (timeOut, poll returned true).
     */
    bool waitEvent(double secondsToWait);
    /**
     * get the data.
     *  @return The double[] where each element is the value field of the corresponding channel.
     */
    epics::pvData::shared_vector<double> get();

private:
    PvaClientMultiMonitorDouble(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);

    PvaClientMultiChannelPtr pvaClientMultiChannel;
    PvaClientChannelArray pvaClientChannelArray;
    size_t nchannel;
    epics::pvData::Mutex mutex;

    epics::pvData::shared_vector<double> doubleValue;
    std::vector<PvaClientMonitorPtr> pvaClientMonitor;
    bool isMonitorConnected;
};

/**
 *  @brief Provides channelGet to multiple channels where the value field of each channel is presented as a union.
 */
class epicsShareClass PvaClientNTMultiGet :
    public std::tr1::enable_shared_from_this<PvaClientNTMultiGet>
{

public:
    POINTER_DEFINITIONS(PvaClientNTMultiGet);
protected:
    static PvaClientNTMultiGetPtr create(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         epics::pvData::PVStructurePtr const &  pvRequest);
    friend class PvaClientMultiChannel;
public:
   /**
     * @brief Destructor
     */
    ~PvaClientNTMultiGet();
     /**
      * @brief Connect a channelGet for each channel.
      */
    void connect();
    /**
     * @brief Get each channel.
     *
     * @param valueOnly use only value for union.
     */
    void get(bool valueOnly = true);
    /**
     * @brief Get the data from the last get.
     * @return the pvaClientNTMultiData.
     */
    PvaClientNTMultiDataPtr getData();

private:
    PvaClientNTMultiGet(
         epics::pvData::UnionConstPtr const & u,
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         epics::pvData::PVStructurePtr const &  pvRequest);

    PvaClientMultiChannelPtr pvaClientMultiChannel;
    PvaClientChannelArray pvaClientChannelArray;
    epics::pvData::PVStructurePtr pvRequest;
    size_t nchannel;
    epics::pvData::Mutex mutex;

    PvaClientNTMultiDataPtr pvaClientNTMultiData;
    std::vector<PvaClientGetPtr> pvaClientGet;
    bool isConnected;
};

/**
 *  @brief Provides channelPut to multiple channels where the value field of each channel is presented as a union.
 */
class epicsShareClass PvaClientNTMultiPut :
    public std::tr1::enable_shared_from_this<PvaClientNTMultiPut>
{

public:
    POINTER_DEFINITIONS(PvaClientNTMultiPut);
protected:
    static PvaClientNTMultiPutPtr create(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);
    friend class PvaClientMultiChannel;
public:
   /**
     * @brief Destructor
     */
    ~PvaClientNTMultiPut();
     /**
     * @brief Connect a channelPut for each channel.
     */
    void connect();
    /**
     * @brief Get the value field of each channel as a union.
     * @return A shared vector of union.
     */
    epics::pvData::shared_vector<epics::pvData::PVUnionPtr> getValues();
    /**
     * @brief Issue a put for each channel.
     */
    void put();

private:
    PvaClientNTMultiPut(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray);

    PvaClientMultiChannelPtr pvaClientMultiChannel;
    PvaClientChannelArray pvaClientChannelArray;
    size_t nchannel;
    epics::pvData::Mutex mutex;

    epics::pvData::shared_vector<epics::pvData::PVUnionPtr> unionValue;
    epics::pvData::shared_vector<epics::pvData::PVFieldPtr> value;
    std::vector<PvaClientPutPtr> pvaClientPut;
    bool isConnected;
};

/**
 *  @brief Provides channel monitor to multiple channels where the value field of each channel is presented as a union.
 */
class epicsShareClass PvaClientNTMultiMonitor :
    public std::tr1::enable_shared_from_this<PvaClientNTMultiMonitor>
{

public:
    POINTER_DEFINITIONS(PvaClientNTMultiMonitor);
protected:    
    static PvaClientNTMultiMonitorPtr create(
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         epics::pvData::PVStructurePtr const &  pvRequest);
    friend class PvaClientMultiChannel;
public:
   /**
     * @brief Destructor
     */         
    ~PvaClientNTMultiMonitor();
     /**
     * @brief Connect to a channel monitor for each channel.
     */
    void connect();
     /**
     * @brief Poll each channel.
     *
     * @param valueOnly use only value for union.
     * If any has new data it is used to update the double[].
     * @return (false,true) if (no, at least one) value was updated.
     */
    bool poll(bool valueOnly = true);
    /**
     * @brief Wait until poll returns true.
     * @param secondsToWait The time to keep trying.
     * A thread sleep of .1 seconds occurs between each call to poll.
     * @return (false,true) if (timeOut, poll returned true).
     */
    bool waitEvent(double secondsToWait);
     /**
     * @brief Get the data for the last successfull poll.
     * @return the pvaClientNTMultiData.
     */
    PvaClientNTMultiDataPtr getData();

private:
    PvaClientNTMultiMonitor(
         epics::pvData::UnionConstPtr const & u,
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         epics::pvData::PVStructurePtr const &  pvRequest);

    PvaClientMultiChannelPtr pvaClientMultiChannel;
    PvaClientChannelArray pvaClientChannelArray;
    epics::pvData::PVStructurePtr pvRequest;
    size_t nchannel;
    epics::pvData::Mutex mutex;
    PvaClientNTMultiDataPtr pvaClientNTMultiData;
    std::vector<PvaClientMonitorPtr> pvaClientMonitor;
    bool isConnected;
};

/**
 *  @brief Provides NTMultiChannel data for both PvaClientNTMultiGet and PvaClientNTMultiMonitor.
 */
class epicsShareClass PvaClientNTMultiData :
    public std::tr1::enable_shared_from_this<PvaClientNTMultiData>
{

public:
    POINTER_DEFINITIONS(PvaClientNTMultiData);
    ~PvaClientNTMultiData();
        /**
     * @brief Get the number of channels.
     * @return The number of channels.
     */
    size_t getNumber();
        /**
     * @brief Get the time when the last get was made.
     * @return The timeStamp.
     */
    epics::pvData::TimeStamp getTimeStamp();
    /**
     * @brief Get the  NTMultiChannel.
     * @return The value.
     */
    epics::nt::NTMultiChannelPtr getNTMultiChannel();
     /**
     * @brief Get channel change flags.
     * @return Array of boolean fields that are set to true if corresponding channel changed
     */
    epics::pvData::shared_vector<epics::pvData::boolean> getChannelChangeFlags();
protected: 
    static PvaClientNTMultiDataPtr create(
         epics::pvData::UnionConstPtr const & u,
         PvaClientMultiChannelPtr const &pvaClientMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         epics::pvData::PVStructurePtr const &  pvRequest);
    void startDeltaTime();
    void endDeltaTime(bool valueOnly = true);
    friend class PvaClientNTMultiGet;
    friend class PvaClientNTMultiMonitor;
private:
    PvaClientNTMultiData(
         epics::pvData::UnionConstPtr const & u,
         PvaClientMultiChannelPtr const &pvaNTMultiChannel,
         PvaClientChannelArray const &pvaClientChannelArray,
         epics::pvData::PVStructurePtr const &  pvRequest);
    void setPVStructure(
        epics::pvData::PVStructurePtr const &pvStructure,size_t index);

    PvaClientMultiChannelPtr pvaClientMultiChannel;
    PvaClientChannelArray pvaClientChannelArray;
    size_t nchannel;
    epics::pvData::Mutex mutex;

    std::vector<epics::pvData::PVStructurePtr> topPVStructure;
    bool gotAlarm;
    bool gotTimeStamp;
    
    epics::pvData::shared_vector<epics::pvData::boolean> changeFlags;
    epics::pvData::StructureConstPtr ntMultiChannelStructure;
    epics::pvData::shared_vector<epics::pvData::PVUnionPtr> unionValue;
    epics::pvData::shared_vector<epics::pvData::int32> severity;
    epics::pvData::shared_vector<epics::pvData::int32> status;
    epics::pvData::shared_vector<std::string> message;
    epics::pvData::shared_vector<epics::pvData::int64> secondsPastEpoch;
    epics::pvData::shared_vector<epics::pvData::int32> nanoseconds;
    epics::pvData::shared_vector<epics::pvData::int32> userTag;
    epics::pvData::Alarm alarm;
    epics::pvData::TimeStamp timeStamp;;
    epics::pvData::PVTimeStamp pvTimeStamp;
};


}}

#endif  /* PVACLIENTMULTICHANNEL_H */
