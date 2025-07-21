/* ntcontinuum.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTCONTINUUM_H
#define NTCONTINUUM_H

#ifdef epicsExportSharedSymbols
#   define ntcontinuumEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef ntcontinuumEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntcontinuumEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>


namespace epics { namespace nt {

class NTContinuum;
typedef std::tr1::shared_ptr<NTContinuum> NTContinuumPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTContinuum.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTContinuumBuilder :
        public std::tr1::enable_shared_from_this<NTContinuumBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTContinuumBuilder);

        /**
         * Adds descriptor field to the NTContinuum.
         * @return this instance of <b>NTContinuumBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTContinuum.
         * @return this instance of <b>NTContinuumBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTContinuum.
         * @return this instance of <b>NTContinuumBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTContinuum.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTContinuum.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTContinuum</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>NTContinuum</b>.
         */
        NTContinuumPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTContinuumBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTContinuumBuilder();

        void reset();

        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTContinuum;
    };

}

typedef std::tr1::shared_ptr<detail::NTContinuumBuilder> NTContinuumBuilderPtr;



/**
 * @brief Convenience Class for NTContinuum
 *
 * @author dgh
 */
class epicsShareClass NTContinuum
{
public:
    POINTER_DEFINITIONS(NTContinuum);

    static const std::string URI;

    /**
     * Creates an NTContinuum wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied structure is compatible with NTContinuum
     * and if so returns an NTContinuum which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTContinuum instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTContinuum wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTContinuum or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTContinuum instance wrapping pvStructure.
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTContinuum.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTContinuum through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTContinuum
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTContinuum.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTContinuum through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type
     *
     * @param pvStructure The PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTContinuum
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTContinuum.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTContinuum through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTContinuum
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTContinuum.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTContinuum through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTContinuum
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped structure is valid with respect to this
     * version of NTContinuum.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTContinuum
     */
    bool isValid();

    /**
     * Creates an NTContinuum builder instance.
     * @return builder instance.
     */
    static NTContinuumBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTContinuum() {}

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
     * Returns the base field.
     * @return the base field.
     */
    epics::pvData::PVDoubleArrayPtr getBase() const;

    /**
     * Returns the value field.
     * @return the value field.
     */
    epics::pvData::PVDoubleArrayPtr getValue() const;

    /**
     * Returns the units field.
     * @return the units field.
     */
    epics::pvData::PVStringArrayPtr getUnits() const;   

private:
    NTContinuum(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTContinuum;
    epics::pvData::PVDoubleArrayPtr pvValue;

    friend class detail::NTContinuumBuilder;
};

}}
#endif  /* NTCONTINUUM_H */
