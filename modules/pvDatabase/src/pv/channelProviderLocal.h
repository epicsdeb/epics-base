/* channelProviderLocal.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#ifndef CHANNELPROVIDERLOCAL_H
#define CHANNELPROVIDERLOCAL_H

#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <set>

#include <pv/lock.h>
#include <pv/pvType.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/status.h>
#include <pv/serverContext.h>
#include <pv/pvStructureCopy.h>
#include <pv/pvDatabase.h>

#include <shareLib.h>
#include <asLib.h>

namespace epics { namespace pvDatabase {

class ChannelProviderLocal;
typedef std::tr1::shared_ptr<ChannelProviderLocal> ChannelProviderLocalPtr;
typedef std::tr1::weak_ptr<ChannelProviderLocal> ChannelProviderLocalWPtr;
class ChannelLocal;
typedef std::tr1::shared_ptr<ChannelLocal> ChannelLocalPtr;
typedef std::tr1::weak_ptr<ChannelLocal> ChannelLocalWPtr;


epicsShareFunc epics::pvData::MonitorPtr createMonitorLocal(
    PVRecordPtr const & pvRecord,
    epics::pvData::MonitorRequester::shared_pointer const & monitorRequester,
    epics::pvData::PVStructurePtr const & pvRequest);

epicsShareFunc ChannelProviderLocalPtr getChannelProviderLocal();


/**
 * @brief ChannelProvider for PVDatabase.
 *
 * An implementation of channelProvider that provides access to records in PVDatabase.
 */
class epicsShareClass ChannelProviderLocal :
    public epics::pvAccess::ChannelProvider,
    public epics::pvAccess::ChannelFind,
    public std::tr1::enable_shared_from_this<ChannelProviderLocal>
{
public:
    POINTER_DEFINITIONS(ChannelProviderLocal);
    /**
     * @brief Initialize access security configuration
     * @param filePath AS definition file path 
     * @param substitutions macro substitutions
     * @throws std::runtime_error in case of configuration problem 
     */
    static void initAs(const std::string& filePath, const std::string& substitutions="");
    /**
     * @brief Is access security active?
     * @return true is AS is active
     */
    static bool isAsActive();

    /**
     * @brief Constructor
     */
    ChannelProviderLocal();
    /**
     * @brief Destructor
     */
    virtual ~ChannelProviderLocal();
    /**
     * @brief Returns the channel provider name.
     *
     * @return <b>local</b>
     */
    virtual  std::string getProviderName();
    /**
     * @brief Returns either a null channelFind or a channelFind for records in the PVDatabase.
     *
     * @param channelName The name of the channel desired.
     * @param channelFindRequester The client callback.
     * @return shared pointer to ChannelFind.
     * This is null if the channelName is not the name of a record in the PVDatabase.
     * It is an implementation of SyncChannelFind if the channelName is the name
     * of a record in the PVDatabase.
     * The interface for SyncChannelFind is defined by pvAccessCPP.
     * The channelFindResult method of channelFindRequester is called before the
     * method returns.
     */
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(
        std::string const &channelName,
        epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);
    /**
     * @brief Calls method channelListRequester::channelListResult.
     *
     * This provides the caller with a list of the record names on the PVDatabase.
     * A record name is the same as a channel name.
     * @param channelListRequester The client callback.
     * @return shared pointer to ChannelFind.
     * The interface for SyncChannelFind is defined by pvAccessCPP.
     */
    virtual epics::pvAccess::ChannelFind::shared_pointer channelList(
        epics::pvAccess::ChannelListRequester::shared_pointer const & channelListRequester);
    /**
     * @brief Create a channel for a record.
     *
     * This method just calls the next method with a address of "".
     * @param channelName The name of the channel desired.
     * @param channelRequester The client callback.
     * @param priority The priority.
     * @return shared pointer to Channel.
     */
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        std::string const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority);
    /**
     * @brief Create a channel for a record.
     * @param channelName The name of the channel desired.
     * @param channelRequester The callback to call with the result.
     * @param priority The priority.
     * This is ignored.
     * @param address The address.
     * This is ignored.
     * @return shared pointer to Channel.
     * This is null if the channelName is not the name of a record in the PVDatabase.
     * Otherwise it is a newly created channel inteface.
     * ChannelRequester::channelCreated is called to give the result.
     */
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        std::string const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority,
        std::string const &address);
    /**
     * @brief get trace level (0,1,2) means (nothing,lifetime,process)
     * @return the level
     */
    int getTraceLevel() {return traceLevel;}
    /**
     * @brief set trace level (0,1,2) means (nothing,lifetime,process)
     * @param level The level
     */
    void setTraceLevel(int level) {traceLevel = level;}
    /**
     * @brief ChannelFind method.
     *
     * @return pointer to self.
     */
    virtual std::tr1::shared_ptr<ChannelProvider> getChannelProvider();
    /**
     * @brief ChannelFind method.
     *
     */
    virtual void cancel() {}
private:
    friend epicsShareFunc ChannelProviderLocalPtr getChannelProviderLocal();
    PVDatabaseWPtr pvDatabase;
    int traceLevel;
    friend class ChannelProviderLocalRun;
};


/**
 * @brief Channel for accessing a PVRecord.
 *
 * A Channel for accessing a record in the PVDatabase.
 * It is a complete implementation of Channel
 */
class epicsShareClass ChannelLocal :
  public epics::pvAccess::Channel,
  public PVRecordClient,
  public std::tr1::enable_shared_from_this<ChannelLocal>
{
public:
    POINTER_DEFINITIONS(ChannelLocal);
    /** Constructor
     * @param channelProvider The channel provider.
     * @param requester The client callback.
     * @param pvRecord The record the channel will access.
     */
    ChannelLocal(
        ChannelProviderLocalPtr const &channelProvider,
        epics::pvAccess::ChannelRequester::shared_pointer const & requester,
        PVRecordPtr const & pvRecord
    );
    /**
     * @brief Destructor
     */
    virtual ~ChannelLocal();
    /**
     * @brief Detach from the record.
     *
     * This is called when a record is being removed from the database.
     * @param pvRecord The record being removed.
     */
    virtual void detach(PVRecordPtr const &pvRecord);
    /**
     * @brief Get the requester name.
     * @return returns the name of the channel requester.
     */
    virtual std::string getRequesterName();
    /**
     * @brief Passes the message to the channel requester.
     * @param message The message.
     * @param messageType The message type.
     */
    virtual void message(
        std::string const & message,
        epics::pvData::MessageType messageType);
    /**
     * @brief Get the channel provider
     * @return The provider.
     */
    virtual epics::pvAccess::ChannelProvider::shared_pointer getProvider();
    /**
     * @brief Get the remote address
     * @return <b>local</b>
     */
    virtual std::string getRemoteAddress();
    /**
     * Get the connection state.
     * @return Channel::CONNECTED.
     */
    virtual epics::pvAccess::Channel::ConnectionState getConnectionState();
    /**
     * @brief Get the channel name.
     * @return the record name.
     */
    virtual std::string getChannelName();
    /**
     * @brief Get the channel requester
     * @return The channel requester.
     */
    virtual epics::pvAccess::ChannelRequester::shared_pointer getChannelRequester();
    /**
     * @brief Is the channel connected?
     * @return true.
     */
    virtual bool isConnected();
    /**
     * @brief Get the introspection interface for subField.
     *
     * The introspection interface is given via GetFieldRequester::getDone.
     * @param requester The client callback.
     * @param subField The subField of the record.
     * If an empty string then the interface for the top level structure of
     * the record is provided.
     */
    virtual void getField(
        epics::pvAccess::GetFieldRequester::shared_pointer const &requester,
        std::string const & subField);
    /**
     * Get the access rights for the record.
     * This throws an exception because it is assumed that access rights are
     * handled by a higher level.
     */
    virtual epics::pvAccess::AccessRights getAccessRights(
        epics::pvData::PVField::shared_pointer const &pvField);
    /**
     * @brief Create a channelProcess.
     *
     * @param requester The client callback.
     * @param pvRequest The options specified by the client.
     * @return A shared pointer to the newly created implementation.
     * The implementation is null if pvRequest has invalid options.
     */
    virtual epics::pvAccess::ChannelProcess::shared_pointer createChannelProcess(
        epics::pvAccess::ChannelProcessRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    /**
     * @brief Create a channelGet.
     *
     * @param requester The client callback.
     * @param pvRequest The options specified by the client.
     * @return A shared pointer to the newly created implementation.
     * The implementation is null if pvRequest has invalid options.
     */
    virtual epics::pvAccess::ChannelGet::shared_pointer createChannelGet(
        epics::pvAccess::ChannelGetRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    /**
     * @brief Create a channelPut.
     *
     * @param requester The client callback.
     * @param pvRequest The options specified by the client.
     * @return A shared pointer to the newly created implementation.
     * The implementation is null if pvRequest has invalid options.
     */
    virtual epics::pvAccess::ChannelPut::shared_pointer createChannelPut(
        epics::pvAccess::ChannelPutRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    /**
     * @brief Create a channelPutGet.
     *
     * @param requester The client callback.
     * @param pvRequest The options specified by the client.
     * @return A shared pointer to the newly created implementation.
     * The implementation is null if pvRequest has invalid options.
     */
    virtual epics::pvAccess::ChannelPutGet::shared_pointer createChannelPutGet(
        epics::pvAccess::ChannelPutGetRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    /**
     * @brief Create a channelRPC.
     *
     * The PVRecord must implement <b>getService</b> or an empty shared pointer is returned.
     * @param requester The client callback
     * @param pvRequest The options specified by the client.
     * @return null.
     */
    virtual epics::pvAccess::ChannelRPC::shared_pointer createChannelRPC(
        epics::pvAccess::ChannelRPCRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    /**
     * @brief Create a monitor.
     *
     * @param requester The client callback.
     * @param pvRequest The options specified by the client.
     * @return A shared pointer to the newly created implementation.
     * The implementation is null if pvRequest has invalid options.
     */
    virtual epics::pvData::Monitor::shared_pointer createMonitor(
        epics::pvData::MonitorRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    /**
     * @brief Create a channelArray.
     *
     * @param requester The client callback.
     * @param pvRequest The options specified by the client.
     * @return A shared pointer to the newly created implementation.
     * The implementation is null if pvRequest has invalid options.
     */
    virtual epics::pvAccess::ChannelArray::shared_pointer createChannelArray(
        epics::pvAccess::ChannelArrayRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    /**
     *  @brief calls printInfo(std::cout);
     */
    virtual void printInfo();
    /**
     * @brief displays a message
     *
     * @param out the stream on which the message is displayed.
     */
    virtual void printInfo(std::ostream& out);
    /**
     * @brief determines if client can write
     *
     * @return true if client can write
     */
    virtual bool canWrite();
    /**
     * @brief determines if client can read
     *
     * @return true if client can read
     */
    virtual bool canRead();
protected:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
private:
    epics::pvAccess::ChannelRequester::shared_pointer requester;
    ChannelProviderLocalWPtr provider;
    PVRecordWPtr pvRecord;
    epics::pvData::Mutex mutex;

    // AS-specific variables/methods
    std::vector<char> toCharArray(const std::string& s);
    std::vector<char> getAsGroup(const PVRecordPtr& pvRecord);
    std::vector<char> getAsUser(const epics::pvAccess::ChannelRequester::shared_pointer& requester);
    std::vector<char> getAsHost(const epics::pvAccess::ChannelRequester::shared_pointer& requester);

    int asLevel;
    std::vector<char> asGroup;
    std::vector<char> asUser;
    std::vector<char> asHost;
    ASMEMBERPVT asMemberPvt;
    ASCLIENTPVT asClientPvt;
};

}}
#endif  /* CHANNELPROVIDERLOCAL_H */
