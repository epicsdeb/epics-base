/* ntndarrayAttribute.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTNDARRAYATTRIBUTE_H
#define NTNDARRAYATTRIBUTE_H

#ifdef epicsExportSharedSymbols
#   define ntndarrayAttributeEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef ntndarrayAttributeEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntndarrayAttributeEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

struct Result;
class NTNDArrayAttribute;
typedef std::tr1::shared_ptr<NTNDArrayAttribute> NTNDArrayAttributePtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTAttribute extended as required by NTNDArray 
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTNDArrayAttributeBuilder :
        public std::tr1::enable_shared_from_this<NTNDArrayAttributeBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTNDArrayAttributeBuilder);

        /**
         * Adds tags field to the NTNDArrayAttribute.
         * @return this instance of <b>NTNDArrayAttributeBuilder</b>.
         */
        shared_pointer addTags();

        /**
         * Adds descriptor field to the NTNDArrayAttribute.
         * @return this instance of <b>NTNDArrayAttributeBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTNDArrayAttribute.
         * @return this instance of <b>NTNDArrayAttributeBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTNDArrayAttribute.
         * @return this instance of <b>NTNDArrayAttributeBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTNDArrayAttribute.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTNDArrayAttribute.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTNDArrayAttribute</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTNDArrayAttribute</b>.
         */
        NTNDArrayAttributePtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTNDArrayAttributeBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    protected://private:
        NTNDArrayAttributeBuilder();

        void reset();

        bool tags;
        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTNDArrayAttribute;
    };

}

typedef std::tr1::shared_ptr<detail::NTNDArrayAttributeBuilder> NTNDArrayAttributeBuilderPtr;



/**
 * @brief Convenience Class for NTNDArrayAttribute
 *
 * @author dgh
 */
class epicsShareClass NTNDArrayAttribute
{
public:
    POINTER_DEFINITIONS(NTNDArrayAttribute);

    static const std::string URI;

    /**
     * Creates an NTNDArrayAttribute wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTAttribute
     * extended as required by NTNDArray and if so returns an
     * NTNDArrayAttribute which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTAttribute instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTNDArrayAttribute wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTAttribute extended as required by NTNDArray
     * or is non-null.
     * 
     * @param pvStructure the PVStructure to be wrapped
     * @return NTAttribute instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTAttribute.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTAttribute through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTAttribute
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTAttribute.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTAttribute through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTAttribute
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTAttribute 
     * extended as required by NTNDArray.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTAttribute extended as required by this version of NTNDArray
     * through the introspection interface.
     * 
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTAttribute
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTAttribute
     * extended as required by NTNDArray.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTAttribute extended as required by this version of NTNDArray
     * through the introspection interface.

     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTAttribute
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTAttribute extended as per this version of NTNDArray.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTNDArrayAttribute
     */
    bool isValid();

    /**
     * Creates an NTNDArrayAttribute builder instance.
     * @return builder instance.
     */
    static NTNDArrayAttributeBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTNDArrayAttribute() {}

     /**
      * Attaches a PVTimeStamp to the wrapped PVStructure.
      * Does nothing if no timeStamp field.
      * @param pvTimeStamp the PVTimeStamp that will be attached.
      * @return true if the operation was successfull (i.e. this instance has a timeStamp field), otherwise false.
      */
    bool attachTimeStamp(epics::pvData::PVTimeStamp &pvTimeStamp) const;

    /**
     * Attaches a PVAlarm to the wrapped PVStructure.
     * Does nothing if no alarm field.
     * @param pvAlarm the PVAlarm that will be attached.
     * @return true if the operation was successfull (i.e. this instance has an alarm field), otherwise false.
     */
    bool attachAlarm(epics::pvData::PVAlarm &pvAlarm) const;

    /**
     * Returns the PVStructure wrapped by this instance.
     * @return the PVStructure wrapped by this instance.
     */
    epics::pvData::PVStructurePtr getPVStructure() const;

    /**
     * Returns the descriptor field.
     * @return the descriptor field.
     */
    epics::pvData::PVStringPtr getDescriptor() const;

    /**
     * Returns the timeStamp field.
     * @return the timStamp field or null if no such field.
     */
    epics::pvData::PVStructurePtr getTimeStamp() const;

    /**
     * Returns the alarm field.
     * @return the alarm field or null if no alarm field.
     */
    epics::pvData::PVStructurePtr getAlarm() const;

    /**
     * Returns the name field.
     * @return the name field.
     */
    epics::pvData::PVStringPtr getName() const;

    /**
     * Returns the value field.
     * @return the value field.
     */
    epics::pvData::PVUnionPtr getValue() const;

    /**
     * Returns the tags field.
     * @return the tags field or null if no such field.
     */
    epics::pvData::PVStringArrayPtr getTags() const;

    /**
     * Returns the sourceType field.
     * @return the sourceType field.
     */
    epics::pvData::PVIntPtr getSourceType() const;

    /**
     * Returns the source field.
     * @return the source field.
     */
    epics::pvData::PVStringPtr getSource() const;

private:
    NTNDArrayAttribute(epics::pvData::PVStructurePtr const & pvStructure);
    static Result& isAttribute(Result& result);

    epics::pvData::PVStructurePtr pvNTNDArrayAttribute;
    epics::pvData::PVUnionPtr pvValue;

    friend class detail::NTNDArrayAttributeBuilder;
    friend class NTNDArray;
};

}}
#endif  /* NTNDARRAYATTRIBUTE_H */
