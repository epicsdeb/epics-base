/* ntnameValue.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTNAMEVALUE_H
#define NTNAMEVALUE_H

#ifdef epicsExportSharedSymbols
#   define ntnameValueEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDisplay.h>
#include <pv/pvControl.h>

#ifdef ntnameValueEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntnameValueEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTNameValue;
typedef std::tr1::shared_ptr<NTNameValue> NTNameValuePtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTNameValue.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author mse
     */
    class epicsShareClass NTNameValueBuilder :
        public std::tr1::enable_shared_from_this<NTNameValueBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTNameValueBuilder);

        /**
         * Sets the value array <b>Scalar</b> type.
         * @param scalarType the value field element ScalarType
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer value(epics::pvData::ScalarType scalarType);

        /**
         * Adds descriptor field to the NTNameValue.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTNameValue.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTNameValue.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTNameValue.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTNameValue.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>PVStructure</b>
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTNameValue</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>NTNameValue</b>
         */
        NTNameValuePtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTNameValueBuilder();

        void reset();

        bool valueTypeSet;
        epics::pvData::ScalarType valueType;

        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTNameValue;
    };

}

typedef std::tr1::shared_ptr<detail::NTNameValueBuilder> NTNameValueBuilderPtr;

/**
 * @brief Convenience Class for NTNameValue
 *
 * @author mrk
 */
class epicsShareClass NTNameValue
{
public:
    POINTER_DEFINITIONS(NTNameValue);

    static const std::string URI;

    /**
     * Creates an NTNameValue wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTNameValue
     * and if so returns an NTNameValue which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped.
     * @return NTNameValue instance wrapping pvStructure on success, null otherwise.
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTNameValue wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTNameValue or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped.
     * @return NTNameValue instance wrapping pvStructure.
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTNameValue.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTNameValue through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test.
     * @return (false,true) if the specified Structure (is not, is) a compatible NTNameValue.
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTNameValue.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTNameValue through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type
     * @param pvStructure The PVStructure to test.
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTNameValue.
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTNameValue.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTNameValue through the introspection interface.
     * @param structure the Structure to test.
     * @return (false,true) if the specified Structure (is not, is) a compatible NTNameValue.
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTNameValue.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTNameValue through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTNameValue
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTNameValue.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTNameValue
     */
    bool isValid();

    /**
     * Creates an NTNameValue builder instance.
     * @return builder instance.
     */
    static NTNameValueBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTNameValue() {}

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
     * Returns the name array field.
     * @return The PVStringArray for the name.
     */
    epics::pvData::PVStringArrayPtr getName() const;

    /**
     * Returns the value array field.
     * @return The PVField for the value.
     */
    epics::pvData::PVFieldPtr getValue() const;

    /**
     * Returns the value array field of a specified expected type (e.g. PVDoubleArray).
     *
     * @tparam PVT The expected type of the value field which should be
     *             be PVScalarArray or a derived class.
     * @return The PVT array for the value.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> getValue() const
    {
        epics::pvData::PVFieldPtr pvField = getValue();
        if (pvField.get())
            return std::tr1::dynamic_pointer_cast<PVT>(pvField);
        else
            return std::tr1::shared_ptr<PVT>();
    }

private:
    NTNameValue(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTNameValue;
    friend class detail::NTNameValueBuilder;
};

}}
#endif  /* NTNAMEVALUE_H */
