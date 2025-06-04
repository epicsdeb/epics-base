/* pvStructureCopy.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#ifndef PVSTRUCTURECOPY_H
#define PVSTRUCTURECOPY_H

#include <string>
#include <stdexcept>
#include <memory>
#include <pv/pvData.h>
#include <pv/bitSet.h>

#include <shareLib.h>

namespace epics { namespace pvCopy{

class PVCopyTraverseMasterCallback;
typedef std::tr1::shared_ptr<PVCopyTraverseMasterCallback> PVCopyTraverseMasterCallbackPtr;

/**
 * @brief Callback for traversing master structure
 *
 * Must be implemented by code that creates pvCopy.
 *
 * This was originally name pvCopy.h and implemented in pvDataCPP
 * When it was moved to pvDatabaseCPP it was renamed to prevent conflicts with
 * the version in pvDataCPP.
 * Also the namespace was changed from epics::pvData to epics::pvCopy
 */
class epicsShareClass PVCopyTraverseMasterCallback
{
public:
    POINTER_DEFINITIONS(PVCopyTraverseMasterCallback);
    virtual ~PVCopyTraverseMasterCallback() {}
    /**
     * Called once for each field in master.
     * @param pvField The field in master.
     */
    virtual void nextMasterPVField(epics::pvData::PVFieldPtr const &pvField) = 0;
};


class PVCopy;
typedef std::tr1::shared_ptr<PVCopy> PVCopyPtr;

struct CopyNode;
typedef std::tr1::shared_ptr<CopyNode> CopyNodePtr;

struct CopyStructureNode;
typedef std::tr1::shared_ptr<CopyStructureNode> CopyStructureNodePtr;


/**
 * @brief Support for subset of fields in a pvStructure.
 *
 * Class that manages one or more PVStructures that holds an arbitrary subset of the fields
 * in another PVStructure called master.
 */
class epicsShareClass PVCopy :
    public std::tr1::enable_shared_from_this<PVCopy>
{
public:
    POINTER_DEFINITIONS(PVCopy);
    /**
     * Create a new pvCopy
     * @param pvMaster The top-level structure for which a copy of
     * an arbitrary subset of the fields in master will be created and managed.
     * @param pvRequest Selects the set of subfields desired and options for each field.
     * @param structureName The name for the top level of any PVStructure created.
     */
    static PVCopyPtr create(
        epics::pvData::PVStructurePtr const &pvMaster,
        epics::pvData::PVStructurePtr const &pvRequest,
        std::string const & structureName);
    virtual ~PVCopy(){}
    /**
     * Get the top-level structure of master
     * @returns The master top-level structure.
     * This should not be modified.
     */
    epics::pvData::PVStructurePtr getPVMaster();
    /**
     * Traverse all the fields in master.
     * @param callback This is called for each field on master.
     */
    void traverseMaster(PVCopyTraverseMasterCallbackPtr const & callback);
    /**
     * Get the introspection interface for a PVStructure for e copy.
     */
    epics::pvData::StructureConstPtr getStructure();
    /**
     * Create a copy instance. Monitors keep a queue of monitor elements.
     * Since each element needs a PVStructure, multiple top-level structures will be created.
     */
    epics::pvData::PVStructurePtr createPVStructure();
    /**
     * Given a field in pvMaster. return the offset in copy for the same field.
     * A value of std::string::npos means that the copy does not have this field.
     * @param masterPVField The field in master.
     */
    std::size_t getCopyOffset(epics::pvData::PVFieldPtr const  &masterPVField);
    /**
     * Given a field in pvMaster. return the offset in copy for the same field.
     * A value of std::string::npos means that the copy does not have this field.
     * @param masterPVStructure A structure in master that has masterPVField.
     * @param masterPVField The field in master.
     */
    std::size_t getCopyOffset(
        epics::pvData::PVStructurePtr const  &masterPVStructure,
        epics::pvData::PVFieldPtr const  &masterPVField);
    /**
     * Given an offset in the copy get the corresponding field in pvMaster.
     * @param structureOffset The offset in the copy.
     */
    epics::pvData::PVFieldPtr getMasterPVField(std::size_t structureOffset);
    /**
     * Initialize the fields in copyPVStructure by giving each field
     * the value from the corresponding field in pvMaster.
     * bitSet will be set to show that all fields are changed.
     * @param copyPVStructure A copy top-level structure.
     * @param bitSet A bitSet for copyPVStructure.
     */
    void initCopy(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet);
    /**
     * Set all fields in copyPVStructure to the value of the corresponding field in pvMaster.
     * Each field that is changed has it's corresponding bit set in bitSet.
     * @param copyPVStructure A copy top-level structure.
     * @param bitSet A bitSet for copyPVStructure.
     * @returns (false,true) if client (should not,should) receive changes.
     */
    bool updateCopySetBitSet(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet);
    /**
     * For each set bit in bitSet
     * set the field in copyPVStructure to the value of the corresponding field in pvMaster.
     * @param copyPVStructure A copy top-level structure.
     * @param bitSet A bitSet for copyPVStructure.
     * @returns (false,true) if client (should not,should) receive changes.
     */
    bool updateCopyFromBitSet(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet);
    /**
     * For each set bit in bitSet
     * set the field in pvMaster to the value of the corresponding field in copyPVStructure
     * @param copyPVStructure A copy top-level structure.
     * @param bitSet A bitSet for copyPVStructure.
     */
    void updateMaster(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet);
    /**
     * Get the options for the field at the specified offset.
     * @param fieldOffset the offset in copy.
     * @returns A NULL is returned if no options were specified for the field.
     * If options were specified,PVStructurePtr is a structures
     *  with a set of PVString subfields that specify name,value pairs.s
     *  name is the subField name and value is the subField value.
     */
    epics::pvData::PVStructurePtr getOptions(std::size_t fieldOffset);
    /**
     * Is master field requested?
     */
    bool isMasterFieldRequested() const {return requestHasMasterField;}
    /**
     * For debugging.
     */
    std::string dump();
private:

    PVCopyPtr getPtrSelf()
    {
        return shared_from_this();
    }

    epics::pvData::PVStructurePtr pvMaster;
    epics::pvData::StructureConstPtr structure;
    CopyNodePtr headNode;
    epics::pvData::PVStructurePtr cacheInitStructure;
    epics::pvData::BitSetPtr ignorechangeBitSet;
    bool requestHasMasterField;

    void traverseMaster(
        CopyNodePtr const &node,
        PVCopyTraverseMasterCallbackPtr const & callback);
    void updateCopySetBitSet(
        epics::pvData::PVFieldPtr const &pvCopy,
        epics::pvData::PVFieldPtr const &pvMaster,
        epics::pvData::BitSetPtr const &bitSet);
    void updateCopySetBitSet(
        epics::pvData::PVFieldPtr const &pvCopy,
        CopyNodePtr const &node,
        epics::pvData::BitSetPtr const &bitSet);
    void updateCopyFromBitSet(
        epics::pvData::PVFieldPtr const &pvCopy,
        CopyNodePtr const &node,
        epics::pvData::BitSetPtr const &bitSet);
    void updateMasterField(
        CopyNodePtr const & node,
        epics::pvData::PVFieldPtr const & pvCopy,
        epics::pvData::PVFieldPtr const &pvMaster,
        epics::pvData::BitSetPtr const &bitSet);
    void updateMasterCheckBitSet(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet,
        size_t nextSet);
    CopyNodePtr getCopyNode(std::size_t fieldOffset);

    PVCopy(epics::pvData::PVStructurePtr const &pvMaster);
    bool init(epics::pvData::PVStructurePtr const &pvRequest);
    epics::pvData::StructureConstPtr createStructure(
        epics::pvData::PVStructurePtr const &pvMaster,
        epics::pvData::PVStructurePtr const &pvFromRequest);
    CopyNodePtr createStructureNodes(
        epics::pvData::PVStructurePtr const &pvMasterStructure,
        epics::pvData::PVStructurePtr const &pvFromRequest,
        epics::pvData::PVStructurePtr const &pvFromField);
    void initPlugin(
        CopyNodePtr const & node,
        epics::pvData::PVStructurePtr const & pvOptions,
        epics::pvData::PVFieldPtr const & pvMasterField);
    void traverseMasterInitPlugin();
    void traverseMasterInitPlugin(CopyNodePtr const & node);

    CopyNodePtr getCopyOffset(
        CopyStructureNodePtr const &structureNode,
        epics::pvData::PVFieldPtr const &masterPVField);
    bool checkIgnore(
        epics::pvData::PVStructurePtr const & copyPVStructure,
        epics::pvData::BitSetPtr const & bitSet);
    void setIgnore(CopyNodePtr const & node);
    CopyNodePtr getMasterNode(
        CopyStructureNodePtr const &structureNode,
        std::size_t structureOffset);
    void dump(
        std::string *builder,
        CopyNodePtr const &node,
        int indentLevel);
};

}}

#endif  /* PVSTRUCTURECOPY_H */
