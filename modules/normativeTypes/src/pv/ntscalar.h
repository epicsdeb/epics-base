/* ntscalar.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTSCALAR_H
#define NTSCALAR_H

#ifdef epicsExportSharedSymbols
#   define ntscalarEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDisplay.h>
#include <pv/pvControl.h>

#ifdef ntscalarEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntscalarEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTScalar;
typedef std::tr1::shared_ptr<NTScalar> NTScalarPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTScalar.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author mse
     */
    class epicsShareClass NTScalarBuilder :
        public std::tr1::enable_shared_from_this<NTScalarBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTScalarBuilder);

        /**
         * Sets the value type of an NTScalar.
         * @param scalarType the value type.
         * @return this instance of <b>NTScalarBuilder</b>.
         */
        shared_pointer value(epics::pvData::ScalarType scalarType);

        /**
         * Adds descriptor field to the NTScalar.
         * @return this instance of <b>NTScalarBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTScalar.
         * @return this instance of <b>NTScalarBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTScalar.
         * @return this instance of <b>NTScalarBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Adds display field to the NTScalar.
         * @return this instance of <b>NTScalarBuilder</b>.
         */
        shared_pointer addDisplay();

        /**
         * Adds control field to the NTScalar.
         * @return this instance of <b>NTScalarBuilder</b>.
         */
        shared_pointer addControl();

        /**
         * Creates a <b>Structure</b> that represents NTScalar.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTScalar.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTScalar</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTScalar</b>.
         */
        NTScalarPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTScalarBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTScalarBuilder();

        void reset();

        bool valueTypeSet;
        epics::pvData::ScalarType valueType;

        bool descriptor;
        bool alarm;
        bool timeStamp;
        bool display;
        bool control;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTScalar;
    };

}

typedef std::tr1::shared_ptr<detail::NTScalarBuilder> NTScalarBuilderPtr;



/**
 * @brief Convenience Class for NTScalar
 *
 * @author mrk
 */
class epicsShareClass NTScalar
{
public:
    POINTER_DEFINITIONS(NTScalar);

    static const std::string URI;

    /**
     * Creates an NTScalar wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTScalar
     * and if so returns an NTScalar which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTScalar instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTScalar wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTScalar or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTScalar instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTScalar.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTScalar through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTScalar
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTScalar.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTScalar through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTScalar
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTScalar.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTScalar through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTScalar
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTScalar.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTScalar through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTScalar
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTScalar.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if wrapped PVStructure (is not, is) a valid NTScalar
     */
    bool isValid();

    /**
     * Creates an NTScalar builder instance.
     * @return builder instance.
     */
    static NTScalarBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTScalar() {}

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
     * Attaches a PVDisplay to the wrapped PVStructure.
     * Does nothing if no display field.
     * @param pvDisplay the PVDisplay that will be attached.
     * @return true if the operation was successfull (i.e. this instance has a display field), otherwise false.
     */
    bool attachDisplay(epics::pvData::PVDisplay &pvDisplay) const;

    /**
     * Attaches an PVControl to the wrapped PVStructure.
     * Does nothing if no control field.
     * @param pvControl The PVControl that will be attached.
     * @return true if the operation was successfull (i.e. this instance has a control field), otherwise false.
     */
    bool attachControl(epics::pvData::PVControl &pvControl) const;

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
     * Returns the display.
     * @return PVStructurePtr which may be null.
     */
    epics::pvData::PVStructurePtr getDisplay() const;

    /**
     * Returns the control.
     * @return PVStructurePtr which may be null.
     */
    epics::pvData::PVStructurePtr getControl() const;

    /**
     * Returns the value field.
     * @return The PVField for the values.
     */
    epics::pvData::PVFieldPtr getValue() const;

    /**
     * Returns the value field of a specified type (for example, PVDouble).
     * @tparam PVT the expected type of the value field which should be
     *             be PVScalar or a derived class.
     * @return the value field or null if it is not of the expected type.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> getValue() const
    {
        return std::tr1::dynamic_pointer_cast<PVT>(pvValue);
    }

private:
    NTScalar(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTScalar;
    epics::pvData::PVFieldPtr pvValue;

    friend class detail::NTScalarBuilder;
};

}}
#endif  /* NTSCALAR_H */
