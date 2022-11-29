/* ntmatrix.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTMATRIX_H
#define NTMATRIX_H

#ifdef epicsExportSharedSymbols
#   define ntmatrixEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDisplay.h>

#ifdef ntmatrixEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntmatrixEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>


namespace epics { namespace nt {

class NTMatrix;
typedef std::tr1::shared_ptr<NTMatrix> NTMatrixPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTMatrix.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTMatrixBuilder :
        public std::tr1::enable_shared_from_this<NTMatrixBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTMatrixBuilder);

        /**
         * Adds dimension field to the NTMatrix.
         * @return this instance of <b>NTMatrixBuilder</b>.
         */
        shared_pointer addDim();

        /**
         * Adds descriptor field to the NTMatrix.
         * @return this instance of <b>NTMatrixBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTMatrix.
         * @return this instance of <b>NTMatrixBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTMatrix.
         * @return this instance of <b>NTMatrixBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Adds display field to the NTMatrix.
         * @return this instance of <b>NTMatrixBuilder</b>.
         */
        shared_pointer addDisplay();

        /**
         * Creates a <b>Structure</b> that represents NTMatrix.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTMatrix.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTMatrix</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>NTMatrix</b>.
         */
        NTMatrixPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTMatrixBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTMatrixBuilder();

        void reset();

        bool dim;
        bool descriptor;
        bool alarm;
        bool timeStamp;
        bool display;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTMatrix;
    };

}

typedef std::tr1::shared_ptr<detail::NTMatrixBuilder> NTMatrixBuilderPtr;



/**
 * @brief Convenience Class for NTMatrix
 *
 * @author dgh
 */
class epicsShareClass NTMatrix
{
public:
    POINTER_DEFINITIONS(NTMatrix);

    static const std::string URI;

    /**
     * Creates an NTMatrix wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTMatrix
     * and if so returns an NTMatrix which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTMatrix instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTMatrix wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTMatrix or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTMatrix instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTMatrix.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTMatrix through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTMatrix
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTMatrix.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTMatrix through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTMatrix
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTMatrix.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTMatrix through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTMatrix
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTMatrix.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTMatrix through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTMatrix
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTMatrix.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if the wrapped PVStructure (is not, is) a valid NTMatrix
     */
    bool isValid();

    /**
     * Creates an NTMatrix builder instance.
     * @return builder instance.
     */
    static NTMatrixBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTMatrix() {}

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
     * Does nothing if no display.
     * @param pvDisplay the PVDisplay that will be attached.
     * @return true if the operation was successfull (i.e. this instance has a display field), otherwise false.
     */
    bool attachDisplay(epics::pvData::PVDisplay &pvDisplay) const;

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
     * Returns the display field.
     * @return the display field or null if no such field.
     */
    epics::pvData::PVStructurePtr getDisplay() const;

    /**
     * Returns the value field.
     * @return the value field.
     */
    epics::pvData::PVDoubleArrayPtr getValue() const;

    /**
     * Returns the dim field.
     * @return the dim field or or null if no such field.
     */
    epics::pvData::PVIntArrayPtr getDim() const;   

private:
    NTMatrix(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTMatrix;
    epics::pvData::PVDoubleArrayPtr pvValue;

    friend class detail::NTMatrixBuilder;
};

}}
#endif  /* NTMATRIX_H */
