/* nthistogram.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTHISTOGRAM_H
#define NTHISTOGRAM_H

#ifdef epicsExportSharedSymbols
#   define nthistogramEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef nthistogramEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef nthistogramEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>


namespace epics { namespace nt {

class NTHistogram;
typedef std::tr1::shared_ptr<NTHistogram> NTHistogramPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTHistogram.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTHistogramBuilder :
        public std::tr1::enable_shared_from_this<NTHistogramBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTHistogramBuilder);

        /**
         * Sets the scalar type of the value field array.
         * @param scalarType the scalar type of the value field array.
         * @return this instance of <b>NTHistogramBuilder</b>.
         */
        shared_pointer value(epics::pvData::ScalarType scalarType);

        /**
         * Adds descriptor field to the NTHistogram.
         * @return this instance of <b>NTHistogramBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTHistogram.
         * @return this instance of <b>NTHistogramBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTHistogram.
         * @return this instance of <b>NTHistogramBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTHistogram.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTHistogram.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTHistogram</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>NTHistogram</b>.
         */
        NTHistogramPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTHistogramBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTHistogramBuilder();

        void reset();

        bool valueTypeSet;
        epics::pvData::ScalarType valueType;

        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTHistogram;
    };

}

typedef std::tr1::shared_ptr<detail::NTHistogramBuilder> NTHistogramBuilderPtr;



/**
 * @brief Convenience Class for NTHistogram
 *
 * @author dgh
 */
class epicsShareClass NTHistogram
{
public:
    POINTER_DEFINITIONS(NTHistogram);

    static const std::string URI;

    /**
     * Creates an NTHistogram wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTHistogram
     * and if so returns an NTHistogram which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTHistogram instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTHistogram wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTHistogram or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped.
     * @return NTHistogram instance wrapping pvStructure.
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTHistogram.
     * <p>
     * Checks whether the specified Structure reports compatibility with this
     * version of NTHistogram through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test.
     * @return (false,true) if the specified Structure (is not, is) a compatible NTHistogram
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTHistogram.
     * <p>
     * Checks whether the specified PVStructure reports compatibility with this
     * version of NTHistogram through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTHistogram
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTHistogram.
     * <p>
     * Checks whether the specified Structure is compatible with this version
     * of NTHistogram through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTHistogram
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTHistogram.
     * <p>
     * Checks whether the specified PVStructure is compatible with this version
     * of NTHistogram through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTHistogram
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped structure is valid with respect to this
     * version of NTHistogram.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTHistogram
     */
    bool isValid();

    /**
     * Creates an NTHistogram builder instance.
     * @return builder instance.
     */
    static NTHistogramBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTHistogram() {}

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
     * Get the PVStructure wrapped by this instance.
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
     * Returns the ranges field.
     * @return the ranges field.
     */
    epics::pvData::PVDoubleArrayPtr getRanges() const;

    /**
     * Returns the value field.
     * @return the value field.
     */
    epics::pvData::PVScalarArrayPtr getValue() const;

    /**
     * Returns the value field of a specified type (e.g. PVIntArray).
     * @tparam PVT the expected type of the value field which should be
     *             be PVShortArray, PVIntArray pr PVLongArray.
     * @return the value field or null if it is not of the expected type.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> getValue() const
    {
        return std::tr1::dynamic_pointer_cast<PVT>(pvValue);
    }
  

private:
    NTHistogram(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTHistogram;
    epics::pvData::PVScalarArrayPtr pvValue;

    friend class detail::NTHistogramBuilder;
};

}}
#endif  /* NTHISTOGRAM_H */
