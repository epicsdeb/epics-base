/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#ifndef PVDATABASE_H
#define PVDATABASE_H

#include <list>
#include <map>

#include <pv/pvData.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class PVRecord;
typedef std::tr1::shared_ptr<PVRecord> PVRecordPtr;
typedef std::tr1::weak_ptr<PVRecord> PVRecordWPtr;
typedef std::map<std::string,PVRecordPtr> PVRecordMap;

class PVRecordField;
typedef std::tr1::shared_ptr<PVRecordField> PVRecordFieldPtr;
typedef std::vector<PVRecordFieldPtr> PVRecordFieldPtrArray;
typedef std::tr1::shared_ptr<PVRecordFieldPtrArray> PVRecordFieldPtrArrayPtr;

class PVRecordStructure;
typedef std::tr1::shared_ptr<PVRecordStructure> PVRecordStructurePtr;
typedef std::tr1::weak_ptr<PVRecordStructure> PVRecordStructureWPtr;

class PVRecordClient;
typedef std::tr1::shared_ptr<PVRecordClient> PVRecordClientPtr;
typedef std::tr1::weak_ptr<PVRecordClient> PVRecordClientWPtr;

class PVListener;
typedef std::tr1::shared_ptr<PVListener> PVListenerPtr;
typedef std::tr1::weak_ptr<PVListener> PVListenerWPtr;

class PVDatabase;
typedef std::tr1::shared_ptr<PVDatabase> PVDatabasePtr;
typedef std::tr1::weak_ptr<PVDatabase> PVDatabaseWPtr;

/**
 * @brief Base interface for a PVRecord.
 *
 * It is also a complete implementation for <b>soft</b> records.
 * A soft record is a record where method <b>process</b> sets an
 * optional top level timeStamp field to the current time and does nothing else.
 * @author mrk
 * @date 2012.11.20
 */
class epicsShareClass PVRecord :
     public epics::pvCopy::PVCopyTraverseMasterCallback,
     public std::tr1::enable_shared_from_this<PVRecord>
{
public:
    POINTER_DEFINITIONS(PVRecord);
    
    /**
     * The Destructor.
     */
    virtual ~PVRecord();
    /**
     * @brief Optional  initialization method.
     *
     * A derived method <b>Must</b> call initPVRecord.
     * @return <b>true</b> for success and <b>false</b> for failure.
     */
    virtual bool init() {initPVRecord(); return true;}
    /**
     *  @brief Optional method for derived class.
     *
     * It is called before record is added to database.
     */
    virtual void start() {}
    /**
     * @brief Optional method for derived class.
     *
     *  It is the method that makes a record smart.
     *  If it encounters errors it should raise alarms and/or
     *  call the <b>message</b> method provided by the base class.
     *  If the pvStructure has a top level timeStamp,
     *  the base class sets the timeStamp to the current time.
     */
    virtual void process();
    /**
     *  @brief remove record from database.
     *
     * Remove the PVRecord. Release any resources used and
     *  get rid of listeners and requesters.
     *  If derived class overrides this then it must call PVRecord::remove()
     *  after it has destroyed any resorces it uses.
     */
    virtual void remove();
    /**
     *  @brief Optional method for derived class.
     *
     * Return a service corresponding to the specified request PVStructure.
     * @param pvRequest The request PVStructure
     * @return The corresponding service
     */
    virtual epics::pvAccess::RPCServiceAsync::shared_pointer getService(
        epics::pvData::PVStructurePtr const & pvRequest)
    {
        return epics::pvAccess::RPCServiceAsync::shared_pointer();
    }
    /**
     * @brief Creates a <b>soft</b> record.
     *
     * @param recordName The name of the record, which is also the channelName.
     * @param pvStructure The top level structure.
     * @param asLevel AS level (default: ASL0)
     * @param asGroup AS group (default: DEFAULT)
     * @return A shared pointer to the newly created record.
     */
    static PVRecordPtr create(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure,
        int asLevel = 0, const std::string& asGroup = "DEFAULT");
    /**
     * @brief  Get the name of the record.
     *
     * @return The name.
     */
    std::string getRecordName() const { return recordName;}
    /**
     * @brief  Get the top level PVRecordStructure.
     *
     * @return The shared pointer.
     */
    PVRecordStructurePtr getPVRecordStructure() const { return pvRecordStructure;}
    /**
     * @brief Get the top level PVStructure.
     *
     * @return The top level PVStructure.
     */
    epics::pvData::PVStructurePtr getPVStructure() const { return pvStructure;}
    /**
     * @brief Find the PVRecordField for the PVField.
     *
     * This is called by the pvCopy facility.
     * @param pvField The PVField.
     * @return The shared pointer to the PVRecordField.
     */
    PVRecordFieldPtr findPVRecordField(
        epics::pvData::PVFieldPtr const & pvField);
    /**
     * @brief Lock the record.
     *
     * Any code must lock while accessing a record.
     */
    void lock();
    /**
     * @brief Unlock the record.
     *
     * The code that calls lock must unlock when done accessing the record.
     */
    void unlock();
    /**
     * @brief Try to lock the record.
     *
     * If <b>true</b> then just like <b>lock</b>.
     * If <b>false</b>client can not access record.
     * Code can try to simultaneously hold the lock for more than two records
     * by calling this method but must be willing to accept failure.
     * @return <b>true</b> if the record is locked.
     */
    bool tryLock();
    /**
     * @brief Lock another record.
     *
     * A client that holds the lock for one record can lock one other record.
     * A client <b>must</b> not call this if the client already has the lock for
     * more then one record.
     *
     * @param otherRecord The other record to lock.
     */
    void lockOtherRecord(PVRecordPtr const & otherRecord);
    /**
     * @brief Add a client that wants to access the record.
     *
     * Every client that accesses the record must call this so that the
     * client can be notified when the record is deleted.
     * @param pvRecordClient The client.
     * @return <b>true</b> if the client is added.
     */
    bool addPVRecordClient(PVRecordClientPtr const & pvRecordClient);
    /**
     * @brief Add a PVListener.
     *
     * This must be called before calling pvRecordField.addListener.
     * @param pvListener The listener.
     * @param pvCopy The pvStructure that has the client fields.
     * @return <b>true</b> if the listener was added.
     */
    bool addListener(
        PVListenerPtr const & pvListener,
        epics::pvCopy::PVCopyPtr const & pvCopy);
    /**
     *  @brief  PVCopyTraverseMasterCallback method
     *
     * @param pvField The next client field.
     */
    void nextMasterPVField(epics::pvData::PVFieldPtr const & pvField);
    /**
     * @brief Remove a listener.
     *
     * @param pvListener The listener.
     * @param pvCopy The pvStructure that has the client fields.
     * @return <b>true</b> if the listener was removed.
     */
    bool removeListener(
        PVListenerPtr const & pvListener,
        epics::pvCopy::PVCopyPtr const & pvCopy);


    /**
     * @brief Begins a group of puts.
     */
    void beginGroupPut();
    /**
     * @brief Ends a group of puts.
     */
    void endGroupPut();
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
     * @brief Get the ASlevel 
     *
     * @return The level.
     */
    int getAsLevel() const {return asLevel;}
    /**
     * @brief Get the AS group name 
     *
     * @return The name.
     */
    std::string getAsGroup() const {return asGroup;}
    /**
     * @brief set access security level.
     * @param level The level
     */
    void setAsLevel(int level) {asLevel=level;}
    /**
     * @brief set access security group
     * @param group The group name
     */
    void setAsGroup(const std::string& group) {asGroup = group;}
protected:
    /**
     * @brief Constructor
     * @param recordName The name of the record
     * @param pvStructure The top level PVStructutre
     * @param asLevel AS level (default: ASL0)
     * @param asGroup AS group (default: DEFAULT)
     */
    PVRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure,
        int asLevel = 0, const std::string& asGroup = "DEFAULT");
    /**
     * @brief Initializes the base class.
     *
     * Must be called by derived classes.
     */
    void initPVRecord();
private:
    friend class PVDatabase;
    void unlistenClients();

    PVRecordFieldPtr findPVRecordField(
        PVRecordStructurePtr const & pvrs,
        epics::pvData::PVFieldPtr const & pvField);

    std::string recordName;
    epics::pvData::PVStructurePtr pvStructure;
    PVRecordStructurePtr pvRecordStructure;
    std::list<PVListenerWPtr> pvListenerList;
    std::list<PVRecordClientWPtr> clientList;
    epics::pvData::Mutex mutex;
    std::size_t depthGroupPut;
    int traceLevel;
    // following only valid while addListener or removeListener is active.
    bool isAddListener;
    PVListenerWPtr pvListener;

    epics::pvData::PVTimeStamp pvTimeStamp;
    epics::pvData::TimeStamp timeStamp;

    int asLevel;
    std::string asGroup;
};

epicsShareFunc std::ostream& operator<<(std::ostream& o, const PVRecord& record);

/**
 * @brief Interface for a field of a record.
 *
 * One exists for each field of the top level PVStructure.
 * @author mrk
 */
class epicsShareClass PVRecordField :
     public virtual epics::pvData::PostHandler,
     public std::tr1::enable_shared_from_this<PVRecordField>
{
public:
    POINTER_DEFINITIONS(PVRecordField);
    /**
     * @brief  Constructor.
     *
     * @param pvField The field from the top level structure.
     * @param parent The parent.
     * @param pvRecord The PVRecord.
     */
    PVRecordField(
        epics::pvData::PVFieldPtr const & pvField,
        PVRecordStructurePtr const &parent,
        PVRecordPtr const & pvRecord);
    /**
     *  @brief Destructor.
     */
    virtual ~PVRecordField() {}
    /**
     *  @brief Get the parent.
     *
     * @return The parent.
     */
    PVRecordStructurePtr getParent();
    /**
     * @brief Get the PVField.
     *
     * @return The shared pointer.
     */
    epics::pvData::PVFieldPtr getPVField();
    /**
     * @brief Get the full name of the field, i.e. field,field,..
     * @return The full name.
     */
    std::string getFullFieldName();
    /**
     * @brief Get the recordName plus the full name of the field, i.e. recordName.field,field,..
     * @return The name.
     */
    std::string getFullName();
    /**
     * @brief Return the PVRecord to which this field belongs.
     * @return The shared pointer,
     */
    PVRecordPtr getPVRecord();
    /**
     * @brief This is called by the code that implements the data interface.
     * It is called whenever the put method is called.
     */
    virtual void postPut();
protected:
    virtual void init();
    virtual void postParent(PVRecordFieldPtr const & subField);
    virtual void postSubField();
private:
    bool addListener(PVListenerPtr const & pvListener);
    virtual void removeListener(PVListenerPtr const & pvListener);
    void callListener();

    std::list<PVListenerWPtr> pvListenerList;
    epics::pvData::PVField::weak_pointer pvField;
    bool isStructure;
    PVRecordStructureWPtr master;
    PVRecordStructureWPtr parent;
    PVRecordWPtr pvRecord;
    std::string fullName;
    std::string fullFieldName;
    friend class PVRecordStructure;
    friend class PVRecord;
};

/**
 * @brief Interface for a field that is a structure.
 *
 * One exists for each structure field of the top level PVStructure.
 * @author mrk
 */
class epicsShareClass PVRecordStructure : public PVRecordField {
public:
    POINTER_DEFINITIONS(PVRecordStructure);
    /**
     * @brief Constructor.
     * @param pvStructure The data.
     * @param parent The parent
     * @param pvRecord The record that has this field.
     */
    PVRecordStructure(
        epics::pvData::PVStructurePtr const &pvStructure,
        PVRecordStructurePtr const &parent,
        PVRecordPtr const & pvRecord);
    /**
     * @brief Destructor.
     */
    virtual ~PVRecordStructure() {}
    /**
     * @brief Get the sub fields.
     * @return the array of PVRecordFieldPtr.
     */
    PVRecordFieldPtrArrayPtr getPVRecordFields();
    /**
     * @brief Get the data structure/
     * @return The shared pointer.
     */
    epics::pvData::PVStructurePtr getPVStructure();
protected:
    /**
     * @brief Called by implementation code of PVRecord.
     */
    virtual void init();
private:
    epics::pvData::PVStructure::weak_pointer pvStructure;
    PVRecordFieldPtrArrayPtr pvRecordFields;
    friend class PVRecord;
};

/**
 * @brief An interface implemented by code that accesses the record.
 *
 * @author mrk
 */
class epicsShareClass PVRecordClient {
public:
    POINTER_DEFINITIONS(PVRecordClient);
    /**
     * @brief Destructor.
     */
    virtual ~PVRecordClient() {}
    /**
     * @brief Detach from the record because it is being removed.
     * @param pvRecord The record.
     */
    virtual void detach(PVRecordPtr const & pvRecord) = 0;
};

/**
 * @brief Listener for PVRecord::message.
 *
 * An interface that is implemented by code that traps calls to PVRecord::message.
 * @author mrk
 */
class epicsShareClass PVListener :
    virtual public PVRecordClient
{
public:
    POINTER_DEFINITIONS(PVListener);
    /**
     * @brief Destructor.
     */
    virtual ~PVListener() {}
    /**
     * @brief pvField has been modified.
     *
     * This is called if the listener has called PVRecordField::addListener for pvRecordField.
     * @param pvRecordField The modified field.
     */
    virtual void dataPut(PVRecordFieldPtr const & pvRecordField) = 0;
    /**
     * @brief A subfield has been modified.
     *
     * @param requested The structure that was requested.
     * @param pvRecordField The field that was modified.
     */
    virtual void dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField) = 0;
    /**
     * @brief Begin a set of puts.
     * @param pvRecord The record.
     */
    virtual void beginGroupPut(PVRecordPtr const & pvRecord) = 0;
    /**
     * @brief End a set of puts.
     * @param pvRecord The record.
     */
    virtual void endGroupPut(PVRecordPtr const & pvRecord) = 0;
    /**
     * @brief Connection to record is being terminated.
     * @param pvRecord The record.
     */
    virtual void unlisten(PVRecordPtr const & pvRecord) = 0;
};

/**
 * @brief The interface for a database of PVRecords.
 *
 * @author mrk
 */
class epicsShareClass PVDatabase {
public:
    POINTER_DEFINITIONS(PVDatabase);
    /**
     * @brief Get the master database.
     * @return The shared pointer.
     */
    static PVDatabasePtr getMaster();
    /**
     * @brief Destructor
     */
    virtual ~PVDatabase();
    /**
     * Find a record.
     * An empty pointer is returned if the record is not in the database.
     * @param recordName The record to find.
     * @return The shared pointer.
     */
    PVRecordPtr findRecord(std::string const& recordName);
    /**
     * @brief Add a record.
     *
     * @param record The record to add.
     * @return <b>true</b> if record was added.
     */
    bool addRecord(PVRecordPtr const & record);
    /**
     * @brief Remove a record.
     * @param record The record to remove.
     *
     * @return <b>true</b> if record was removed.
     */
    bool removeRecord(PVRecordPtr const & record);
    /**
     * @brief Get the names of all the records in the database.
     * @return The names.
     */
    epics::pvData::PVStringArrayPtr getRecordNames();
private:
    friend class PVRecord;

    PVRecordWPtr removeFromMap(PVRecordPtr const & record);
    PVDatabase();
    void lock();
    void unlock();
    PVRecordMap  recordMap;
    epics::pvData::Mutex mutex;
    static bool getMasterFirstCall;
};

}}

#endif  /* PVDATABASE_H */
