/* ntscalarArray.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTSCALARARRAY_H
#define NTSCALARARRAY_H

#ifdef epicsExportSharedSymbols
#   define ntscalarArrayEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDisplay.h>
#include <pv/pvControl.h>

#ifdef ntscalarArrayEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntscalarArrayEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>


namespace epics { namespace nt {

class NTScalarArray;
typedef std::tr1::shared_ptr<NTScalarArray> NTScalarArrayPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTScalarArray.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author mse
     */
    class epicsShareClass NTScalarArrayBuilder :
        public std::tr1::enable_shared_from_this<NTScalarArrayBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTScalarArrayBuilder);

        /**
         * Sets the value type of the NTScalarArray.
         * @param elementType the value field element ScalarType.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         */
        shared_pointer value(epics::pvData::ScalarType elementType);

        /**
         * Sets the value type of the NTScalarArray.
         * @param elementType the value field element ScalarType.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         * @deprecated use value instead.
         */
        shared_pointer arrayValue(epics::pvData::ScalarType elementType);

        /**
         * Adds descriptor field to the NTScalarArray.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTScalarArray.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTScalarArray.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Adds display field to the NTScalarArray.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         */
        shared_pointer addDisplay();

        /**
         * Adds control field to the NTScalarArray.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         */
        shared_pointer addControl();

        /**
         * Creates a <b>Structure</b> that represents NTScalarArray.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTScalarArray.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTScalarArray</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>NTScalarArray</b>.
         */
        NTScalarArrayPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTScalarArrayBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTScalarArrayBuilder();

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

        friend class ::epics::nt::NTScalarArray;
    };

}

typedef std::tr1::shared_ptr<detail::NTScalarArrayBuilder> NTScalarArrayBuilderPtr;



/**
 * @brief Convenience Class for NTScalarArray
 *
 * @author mrk
 */
class epicsShareClass NTScalarArray
{
public:
    POINTER_DEFINITIONS(NTScalarArray);

    static const std::string URI;

    /**
     * Creates an NTScalarArray wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTScalarArray
     * and if so returns an NTScalarArray which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     * 
     * @param pvStructure the PVStructure to be wrapped
     * @return NTScalarArray instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTScalarArray wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTScalarArray or is non-null.
     * 
     * @param pvStructure the PVStructure to be wrapped
     * @return NTScalarArray instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTScalarArray.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTScalarArray through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     * 
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTScalarArray
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTScalarArray.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTScalarArray through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     * 
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTScalarArray
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTScalarArray.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTScalarArray through the introspection interface.
     * 
     * @param structure the Structure to test.
     * @return (false,true) if the specified Structure (is not, is) a compatible NTScalarArray
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTScalarArray.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTScalarArray through the introspection interface.
     * 
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTScalarArray
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is a valid NTScalarArray.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTScalarArray.
     */
    bool isValid();

    /**
     * Creates an NTScalarArray builder instance.
     * @return builder instance.
     */
    static NTScalarArrayBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTScalarArray() {}

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
     * Attaches an pvControl.
     * @param pvControl The pvControl that will be attached.
     * Does nothing if no control field.
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
     * Returns the value field of a specified type (e.g. PVDoubleArray).
     * @tparam PVT the expected type of the value field which should be
     *             be PVScalarArray or a derived class.
     * @return the value field or null if it is not of the expected type.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> getValue() const
    {
        return std::tr1::dynamic_pointer_cast<PVT>(pvValue);
    }

private:
    NTScalarArray(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTScalarArray;
    epics::pvData::PVFieldPtr pvValue;

    friend class detail::NTScalarArrayBuilder;
};

}}
#endif  /* NTSCALARARRAY_H */
