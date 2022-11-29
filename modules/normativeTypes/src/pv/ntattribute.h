/* ntattribute.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTATTRIBUTE_H
#define NTATTRIBUTE_H

#ifdef epicsExportSharedSymbols
#   define ntattributeEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef ntattributeEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntattributeEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTAttribute;
typedef std::tr1::shared_ptr<NTAttribute> NTAttributePtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTAttribute.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTAttributeBuilder :
        public std::tr1::enable_shared_from_this<NTAttributeBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTAttributeBuilder);

        /**
         * Adds tags field to the NTAttribute.
         * @return this instance of <b>NTAttributeBuilder</b>.
         */
        shared_pointer addTags();

        /**
         * Adds descriptor field to the NTAttribute.
         * @return this instance of <b>NTAttributeBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTAttribute.
         * @return this instance of <b>NTAttributeBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTAttribute.
         * @return this instance of <b>NTAttributeBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTAttribute.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTAttribute.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTAttribute</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTAttribute</b>.
         */
        NTAttributePtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTAttributeBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    protected://private:
        NTAttributeBuilder();

        void reset();

        bool tags;
        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTAttribute;
    };

}

typedef std::tr1::shared_ptr<detail::NTAttributeBuilder> NTAttributeBuilderPtr;



/**
 * @brief Convenience Class for NTAttribute
 *
 * @author dgh
 */
class epicsShareClass NTAttribute
{
public:
    POINTER_DEFINITIONS(NTAttribute);

    static const std::string URI;

    /**
     * Creates an NTAttribute wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTAttribute
     * and if so returns an NTAttribute which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTAttribute instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTAttribute wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTAttribute or is non-null.
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
     * Returns whether the specified Structure is compatible with NTAttribute.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTAttribute through the introspection interface.
     * 
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTAttribute
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTAttribute.
     * <p>
     * Checks if the specified tructure is compatible with this version
     * of NTAttribute through the introspection interface.

     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTAttribute
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTAttribute.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTAttribute
     */
    bool isValid();

    /**
     * Creates an NTAttribute builder instance.
     * @return builder instance.
     */
    static NTAttributeBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTAttribute() {}

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
     * @return the descriptor field or null if no such field.
     */
    epics::pvData::PVStringPtr getDescriptor() const;

    /**
     * Returns the timeStamp field.
     * @return the timStamp field or null if no such field.
     */
    epics::pvData::PVStructurePtr getTimeStamp() const;

    /**
     * Returns the alarm field.
     * @return the alarm field or null if no such field.
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

private:
    NTAttribute(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTAttribute;
    epics::pvData::PVUnionPtr pvValue;

    friend class detail::NTAttributeBuilder;
};

}}
#endif  /* NTATTRIBUTE_H */
