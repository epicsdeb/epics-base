/* ntaggregate.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTAGGREGATE_H
#define NTAGGREGATE_H

#ifdef epicsExportSharedSymbols
#   define ntaggregateEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#ifdef ntaggregateEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntaggregateEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTAggregate;
typedef std::tr1::shared_ptr<NTAggregate> NTAggregatePtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTAggregate.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     */
    class epicsShareClass NTAggregateBuilder :
        public std::tr1::enable_shared_from_this<NTAggregateBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTAggregateBuilder);

        /**
         * Adds dispersion field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addDispersion();

        /**
         * Adds first field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addFirst();

        /**
         * Adds firstTimeStamp field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addFirstTimeStamp();

        /**
         * Adds last field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addLast();

        /**
         * Adds lastTimeStamp field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addLastTimeStamp();

        /**
         * Adds max field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addMax();

        /**
         * Adds min field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addMin();

        /**
         * Adds descriptor field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTAggregate.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTAggregate.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTAggregate.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTAggregate</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTAggregate</b>.
         */
        NTAggregatePtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTAggregateBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTAggregateBuilder();

        void reset();

        bool dispersion;
        bool first;
        bool firstTimeStamp;
        bool last;
        bool lastTimeStamp;
        bool max; 
        bool min;

        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTAggregate;
    };

}

typedef std::tr1::shared_ptr<detail::NTAggregateBuilder> NTAggregateBuilderPtr;


/**
 * @brief Convenience Class for NTAggregate
 *
 * @author dgh
 */
class epicsShareClass NTAggregate
{
public:
    POINTER_DEFINITIONS(NTAggregate);

    static const std::string URI;

    /**
     * Creates an NTAggregate wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTAggregate
     * and if so returns an NTAggregate which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return the NTAggregate instance on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTAggregate wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTAggregate or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return the NTAggregate instance
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTAggregate.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTAggregate through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTAggregate
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTAggregate.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTAggregate through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test.
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTAggregate.
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTAggregate.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTAggregate through introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTAggregate
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTAggregate.
     * <p>
     * Checks if the specified PVStructure is compatible with this version
     * of NTAggregate through introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTAggregate
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the wrapped PVStructure is valid with respect to this
     * version of NTAggregate.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value data as
     * well as the introspection data.
     *
     * @return (false,true) if wrapped PVStructure a valid NTAggregate
     */
    bool isValid();

    /**
     * Creates an NTAggregate builder instance.
     * @return builder instance.
     */
    static NTAggregateBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTAggregate() {}

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
     * Returns the value field.
     * @return the value field.
     */
    epics::pvData::PVDoublePtr getValue() const;

    /**
     * Returns the N field.
     * @return the N field.
     */
    epics::pvData::PVLongPtr getN() const;

    /**
     * Returns the dispersion field.
     * @return the dispersion or null if no such field.
     */
    epics::pvData::PVDoublePtr getDispersion() const;

    /**
     * Returns the first field.
     * @return the first field or null if no such field.
     */
    epics::pvData::PVDoublePtr getFirst() const;

    /**
     * Returns the firstTimeStamp field.
     * @return the firstTimeStamp field or null if no such field.
     */
    epics::pvData::PVStructurePtr getFirstTimeStamp() const;

    /**
     * Returns the last field.
     * @return the last field or null if no such field.
     */
    epics::pvData::PVDoublePtr getLast() const;

    /**
     * Returns the lastTimeStamp field.
     * @return the lastTimeStamp field or null if no such field.
     */
    epics::pvData::PVStructurePtr getLastTimeStamp() const;

    /**
     * Returns the max field.
     * @return the max field or null if no such field.
     */
    epics::pvData::PVDoublePtr getMax() const;

    /**
     * Returns the min field.
     * @return the min field or null if no such field.
     */
    epics::pvData::PVDoublePtr getMin() const;

private:
    NTAggregate(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTAggregate;
    epics::pvData::PVDoublePtr pvValue;

    friend class detail::NTAggregateBuilder;
};

}}
#endif  /* NTAGGREGATE_H */
