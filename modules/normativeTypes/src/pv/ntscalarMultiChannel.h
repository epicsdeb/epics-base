/* ntscalarMultiChannel.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTSCALARMULTICHANNEL_H
#define NTSCALARMULTICHANNEL_H

#include <vector>
#include <string>

#ifdef epicsExportSharedSymbols
#   define ntscalarMultiChannelEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDisplay.h>
#include <pv/pvControl.h>

#ifdef ntscalarMultiChannelEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntscalarMultiChannelEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>


namespace epics { namespace nt { 


class NTScalarMultiChannel;
typedef std::tr1::shared_ptr<NTScalarMultiChannel> NTScalarMultiChannelPtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTScalarMultiChannel.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author dgh
     * @author mse
     */
    class epicsShareClass NTScalarMultiChannelBuilder :
        public std::tr1::enable_shared_from_this<NTScalarMultiChannelBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTScalarMultiChannelBuilder);

        /**
         * Sets the scalar type for the value field.
         * If this is not called then pvDouble is the default.
         * @param scalarType the value field element ScalarType.
         * @return this instance of  <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer value(epics::pvData::ScalarType scalarType);

        /**
         * Adds descriptor field to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Adds severity array to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addSeverity();

        /**
         * Adds status array to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addStatus();

        /**
         * Adds message array to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addMessage();

        /**
         * Adds secondsPastEpoch array to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addSecondsPastEpoch();

        /**
         * Adds nanoseconds array to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addNanoseconds();

        /**
         * Adds userTag array to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addUserTag();

        /**
         * Adds isConnected array to the NTScalarMultiChannel.
         * @return this instance of <b>NTScalarMultiChannelBuilder</b>.
         */
        shared_pointer addIsConnected();

        /**
         * Creates a <b>Structure</b> that represents NTScalarMultiChannel.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTScalarMultiChannel.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>PVStructure</b>
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTScalarMultiChannel</b> instance.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of a <b>NTScalarMultiChannel</b>
         */
        NTScalarMultiChannelPtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of a <b>NTScalarMultiChannelBuilder</b>
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTScalarMultiChannelBuilder();

        void reset();

        epics::pvData::ScalarType valueType;
        bool descriptor;
        bool alarm;
        bool timeStamp;
        bool severity;
        bool status;
        bool message;
        bool secondsPastEpoch;
        bool nanoseconds;
        bool userTag;
        bool isConnected;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTScalarMultiChannel;
    };

}

typedef std::tr1::shared_ptr<detail::NTScalarMultiChannelBuilder> NTScalarMultiChannelBuilderPtr;


/**
 * @brief Convenience Class for NTScalarMultiChannel
 * @author dgh
 * @author mrk
 *
 */
class epicsShareClass NTScalarMultiChannel
{
public:
    POINTER_DEFINITIONS(NTScalarMultiChannel);

    static const std::string URI;

    /**
     * Creates an NTScalarMultiChannel wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTScalarMultiChannel
     * and if so returns an NTScalarMultiChannel which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTScalarMultiChannel instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTScalarMultiChannel wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTScalarMultiChannel or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTScalarMultiChannel instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTScalarMultiChannel.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTScalarMultiChannel through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTScalarMultiChannel
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTScalarMultiChannel.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTScalarMultiChannel through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTScalarMultiChannel
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTScalarMultiChannel.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTScalarMultiChannel through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTScalarMultiChannel
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the wrapped PVStructure is a valid NTScalarMultiChannel.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if wrapped PVStructure (is not, is) a valid NTScalarMultiChannel
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the specified PVStructure is a valid NTScalarMultiChannel.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if wrapped PVStructure (is not, is) a valid NTScalarMultiChannel.
     */
    bool isValid();

    /**
     * Creates an NTScalarMultiChannelBuilder instance
     * @return builder instance.
     */
    static  NTScalarMultiChannelBuilderPtr createBuilder();

    /**
     * Destructor
     */
    ~NTScalarMultiChannel() {}
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
    epics::pvData::PVStructurePtr getPVStructure() const
    {return pvNTScalarMultiChannel;}

    /**
     * Return the descriptor field.
     * @return the descriptor field or null if no descriptor field.
     */
    epics::pvData::PVStringPtr getDescriptor() const 
    {return pvDescriptor;}

    /**
     * Returns the timeStamp field.
     * @return the timeStamp field or null if no such field.
     */
    epics::pvData::PVStructurePtr getTimeStamp() const
    {return pvTimeStamp;}

    /**
     * Returns the alarm field.
     * @return the alarm field or null if no such field.
     */
    epics::pvData::PVStructurePtr getAlarm() const
     {return pvAlarm;}

    /**
     * Returns the field with the value of each channel.
     * @return the value field.
     */
    epics::pvData::PVScalarArrayPtr getValue() const 
    {return pvValue;}

    /**
     * Returns the value of each channel of a specified expected type
     * (for example, PVDoubleArray).
     * @tparam PVT the expected type of the value field which should be
     *             be PVScalarArray or a derived class.
     * @return the value field or null if it is not of the expected type.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> getValue() const
    {
        return std::tr1::dynamic_pointer_cast<PVT>(pvValue);
    }

    /**
     * Returns the field with the channelName of each channel.
     * @return the channelName field 
     */
    epics::pvData::PVStringArrayPtr getChannelName() const 
    { return pvChannelName;};

    /**
     * Returns the field with the connection state of each channel.
     * @return the isConnected field or null if no such field
     */
    epics::pvData::PVBooleanArrayPtr getIsConnected() const 
    { return pvIsConnected;};

    /**
     * Returns the field with the severity of each channel.
     * @return the severity field or null if no such field
     */
    epics::pvData::PVIntArrayPtr getSeverity() const 
    {return pvSeverity;}

    /**
     * Returns the field with the status of each channel.
     * @return the status field or null if no such field
     */
    epics::pvData::PVIntArrayPtr getStatus() const 
    {return pvStatus;}

    /**
     * Returns the field with the message of each channel.
     * @return message field or null if no such field.
     */
    epics::pvData::PVStringArrayPtr getMessage() const 
    {return pvMessage;}

    /**
     * Returns the field with the secondsPastEpoch of each channel.
     * @return the secondsPastEpoch  field or null if no such field.
     */
    epics::pvData::PVLongArrayPtr getSecondsPastEpoch() const 
    {return pvSecondsPastEpoch;}

    /**
     * Returns the field with the nanoseconds of each channel.
     * @return nanoseconds field or null if no such field.
     */
    epics::pvData::PVIntArrayPtr getNanoseconds() const 
    {return pvNanoseconds;}

    /**
     * Returns the field with the userTag of each channel.
     * @return the  userTag field or null if no such field.
     */
    epics::pvData::PVIntArrayPtr getUserTag() const 
    {return pvUserTag;}

private:
    NTScalarMultiChannel(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTScalarMultiChannel;
    epics::pvData::PVStructurePtr pvTimeStamp;
    epics::pvData::PVStructurePtr pvAlarm;
    epics::pvData::PVScalarArrayPtr pvValue;
    epics::pvData::PVStringArrayPtr pvChannelName;
    epics::pvData::PVBooleanArrayPtr pvIsConnected;
    epics::pvData::PVIntArrayPtr pvSeverity;
    epics::pvData::PVIntArrayPtr pvStatus;
    epics::pvData::PVStringArrayPtr pvMessage;
    epics::pvData::PVLongArrayPtr pvSecondsPastEpoch;
    epics::pvData::PVIntArrayPtr pvNanoseconds;
    epics::pvData::PVIntArrayPtr pvUserTag;
    epics::pvData::PVStringPtr pvDescriptor;
    friend class detail::NTScalarMultiChannelBuilder;
};

}}
#endif  /* NTSCALARMULTICHANNEL_H */
