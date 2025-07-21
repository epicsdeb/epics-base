/* ntunion.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTUNION_H
#define NTUNION_H

#ifdef epicsExportSharedSymbols
#   define ntunionEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef ntunionEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntunionEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTUnion;
typedef std::tr1::shared_ptr<NTUnion> NTUnionPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTUnion.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTUnionBuilder :
        public std::tr1::enable_shared_from_this<NTUnionBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTUnionBuilder);

        /**
         * Specifies the union for the value field.
         * If this is not called then a variant union is the default.
         * @param unionType  the introspection object for the union value field
         * @return this      instance of  NTUnionBuilder
         */
        shared_pointer value(epics::pvData::UnionConstPtr unionType);

        /**
         * Adds descriptor field to the NTUnion.
         * @return this instance of <b>NTUnionBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTUnion.
         * @return this instance of <b>NTUnionBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTUnion.
         * @return this instance of <b>NTUnionBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTUnion.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTUnion.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTUnion</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTUnion</b>.
         */
        NTUnionPtr create();
        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTUnionBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTUnionBuilder();

        epics::pvData::UnionConstPtr valueType;

        void reset();

        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTUnion;
    };

}

typedef std::tr1::shared_ptr<detail::NTUnionBuilder> NTUnionBuilderPtr;



/**
 * @brief Convenience Class for NTUnion
 *
 * @author dgh
 */
class epicsShareClass NTUnion
{
public:
    POINTER_DEFINITIONS(NTUnion);

    static const std::string URI;

    /**
     * Creates an NTUnion wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTUnion
     * and if so returns an NTUnion which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTUnion instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTUnion wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTUnion or is non-null.
     * @param pvStructure the PVStructure to be wrapped
     * @return NTUnion instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTUnion.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTUnion through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTUnion
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTUnion.
     *
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTUnion through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTUnion
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTUnion.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTUnion through the introspection interface.
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTUnion
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTUnion.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTUnion through the introspection interface
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTUnion
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is a valid NTUnion.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if wrapped PVStructure (is not, is) a valid NTUnion
     */
    bool isValid();

    /**
     * Creates an NTUnion builder instance.
     * @return builder instance.
     */
    static NTUnionBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTUnion() {}

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
     * @return the descriptor field or null if no descriptor field.
     */
    epics::pvData::PVStringPtr getDescriptor() const;

    /**
     * Returns the timeStamp field.
     * @return the timStamp field or null if no timeStamp field.
     */
    epics::pvData::PVStructurePtr getTimeStamp() const;

    /**
     * Returns the alarm field.
     * @return the alarm field or null if no alarm field.
     */
    epics::pvData::PVStructurePtr getAlarm() const;

    /**
     * Returns the value field.
     * @return the value field.
     */
    epics::pvData::PVUnionPtr getValue() const;

private:
    NTUnion(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTUnion;
    epics::pvData::PVUnionPtr pvValue;

    friend class detail::NTUnionBuilder;
};

}}
#endif  /* NTUNION_H */
