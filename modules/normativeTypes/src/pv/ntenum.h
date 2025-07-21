/* ntenum.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTENUM_H
#define NTENUM_H

#ifdef epicsExportSharedSymbols
#   define ntenumEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef ntenumEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntenumEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTEnum;
typedef std::tr1::shared_ptr<NTEnum> NTEnumPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTEnum.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTEnumBuilder :
        public std::tr1::enable_shared_from_this<NTEnumBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTEnumBuilder);

        /**
         * Adds descriptor field to the NTEnum.
         * @return this instance of <b>NTEnumBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTEnum.
         * @return this instance of <b>NTEnumBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTEnum.
         * @return this instance of <b>NTEnumBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTEnum.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTEnum.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTEnum</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTEnum</b>.
         */
        NTEnumPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTEnumBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTEnumBuilder();

        void reset();

        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTEnum;
    };

}

typedef std::tr1::shared_ptr<detail::NTEnumBuilder> NTEnumBuilderPtr;



/**
 * @brief Convenience Class for NTEnum
 *
 * @author dgh
 */
class epicsShareClass NTEnum
{
public:
    POINTER_DEFINITIONS(NTEnum);

    static const std::string URI;

    /**
     * Creates an NTEnum wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTEnum or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTEnum instance wrapping pvStructure
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTEnum wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTEnum or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTEnum instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTEnum.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTEnum through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test.
     * @return (false,true) if the specified Structure (is not, is) a compatible NTEnum.
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTEnum.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTEnum through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTEnum
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTEnum.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTEnum through the introspection interface.
     *
     * @param structure The Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTEnum
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTEnum.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTEnum through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTEnum
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTEnum.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTEnum
     */
    bool isValid();

    /**
     * Creates an NTEnum builder instance.
     * @return builder instance.
     */
    static NTEnumBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTEnum() {}

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
     * @return the timStamp field or or null if no such field.
     */
    epics::pvData::PVStructurePtr getTimeStamp() const;

    /**
     * Returns the alarm field.
     * @return the alarm field or or null if no such field.
     */
    epics::pvData::PVStructurePtr getAlarm() const;

    /**
     * Get the value field.
     * @return the value field.
     */
    epics::pvData::PVStructurePtr getValue() const;

private:
    NTEnum(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTEnum;
    epics::pvData::PVStructurePtr pvValue;

    friend class detail::NTEnumBuilder;
};

}}
#endif  /* NTENUM_H */
