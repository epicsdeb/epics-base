/* ntndarray.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTNDARRAY_H
#define NTNDARRAY_H

#include <vector>
#include <string>

#ifdef epicsExportSharedSymbols
#   define ntndarrayEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDisplay.h>
#include <pv/pvControl.h>

#ifdef ntndarrayEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntndarrayEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTNDArray;
typedef std::tr1::shared_ptr<NTNDArray> NTNDArrayPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTNDArray.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author mse
     */
    class epicsShareClass NTNDArrayBuilder :
        public std::tr1::enable_shared_from_this<NTNDArrayBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTNDArrayBuilder);

        /**
         * Adds descriptor field to the NTNDArray.
         * @return this instance of <b>NTNDArrayBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTNDArray.
         * @return this instance of <b>NTNDArrayBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTNDArray.
         * @return this instance of <b>NTNDArrayBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Adds display field to the NTNDArray.
         * @return this instance of <b>NTNDArrayBuilder</b>.
         */
        shared_pointer addDisplay();

        /**
         * Creates a  <b>Structure</b> that represents NTNDArray.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTNDArray.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTNDArray</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTNDArray</b>
         */
        NTNDArrayPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of a <b>NTArrayBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTNDArrayBuilder();

        void reset();

        bool descriptor;
        bool timeStamp;
        bool alarm;
        bool display;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTNDArray;
    };

}

typedef std::tr1::shared_ptr<detail::NTNDArrayBuilder> NTNDArrayBuilderPtr;

/**
 * @brief Convenience Class for NTNDArray
 *
 * @author dgh
 */
class epicsShareClass NTNDArray
{
public:
    POINTER_DEFINITIONS(NTNDArray);

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
     * Creates an NTNDArray wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTNDArray or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTNDArray instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTNDArray.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTNDArray through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the pvStructure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTNDArray
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTNDArray.
     *
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTNDArray through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTNDArray
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTNDArray.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTNDArray through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTNDArray
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTNDArray.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTNDArray through the introspection interface.
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTNDArray
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTNDArray.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTNDArray
     */
    bool isValid();

    /**
     * Creates an NTNDArrayBuilder instance
     * @return builder instance.
     */
    static NTNDArrayBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTNDArray() {}

     /**
      * Attaches a PVTimeStamp to the wrapped PVStructure.
      * Does nothing if no timeStamp field.
      * @param pvTimeStamp the PVTimeStamp that will be attached.
      * @return true if the operation was successfull (i.e. this instance has a timeStamp field), otherwise false.
      */
    bool attachTimeStamp(epics::pvData::PVTimeStamp &pvTimeStamp) const;

     /**
      * Attaches a pvTimeStamp to dataTimeStamp field.
      * @param pvTimeStamp The pvTimeStamp that will be attached.
      * Does nothing if no timeStamp.
      * @return true if the operation was successfull (i.e. this instance has a timeStamp field), otherwise false.
      */
    bool attachDataTimeStamp(epics::pvData::PVTimeStamp &pvTimeStamp) const;

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
     * Returns the value field.
     * Returns the value field.
     */
    epics::pvData::PVUnionPtr getValue() const;

    /**
     * Returns the codec field.
     * @return the codec field.
     */
    epics::pvData::PVStructurePtr getCodec() const;

    /**
     * Returns the compressedDataSize field.
     * @return the compressedDataSize field.
     */
    epics::pvData::PVLongPtr getCompressedDataSize() const;

    /**
     * Returns the uncompressedDataSize field.
     * @return the uncompressedDataSize field.
     */
    epics::pvData::PVLongPtr getUncompressedDataSize() const;

    /**
     * Returns the dimension field.
     * @return the dimension field.
     */
    epics::pvData::PVStructureArrayPtr getDimension() const;

    /**
     * Returns the uniqueId field.
     * @return the uniqueId field.
     */
    epics::pvData::PVIntPtr getUniqueId() const;

    /**
     * Returns the dataTimeStamp field.
     * @return the dataTimeStamp field.
     */
    epics::pvData::PVStructurePtr getDataTimeStamp() const;

    /**
     * Returns the attribute field.
     * @return the attribute field.
     */
    epics::pvData::PVStructureArrayPtr getAttribute() const;

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
     * Attaches a PVDisplay to the wrapped PVStructure.
     * Does nothing if no display.
     * @param pvDisplay the PVDisplay that will be attached.
     * @return true if the operation was successfull (i.e. this instance has a display field), otherwise false.
     */
    bool attachDisplay(epics::pvData::PVDisplay &pvDisplay) const;

    /**
     * Returns the display field.
     * @return PVStructurePtr or null if no alarm field.
     */
    epics::pvData::PVStructurePtr getDisplay() const;

private:
    NTNDArray(epics::pvData::PVStructurePtr const & pvStructure);

    epics::pvData::int64 getExpectedUncompressedSize();
    epics::pvData::int64 getValueSize();
    epics::pvData::int64 getValueTypeSize();

    epics::pvData::PVStructurePtr pvNTNDArray;

    friend class detail::NTNDArrayBuilder;
};

}}
#endif  /* NTNDARRAY_H */
