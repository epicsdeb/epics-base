/* nttable.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTTABLE_H
#define NTTABLE_H

#include <vector>
#include <string>

#ifdef epicsExportSharedSymbols
#   define nttableEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDisplay.h>
#include <pv/pvControl.h>

#ifdef nttableEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef nttableEpicsExportSharedSymbols
#endif

#include <pv/ntfield.h>

#include <shareLib.h>

namespace epics { namespace nt {

class NTTable;
typedef std::tr1::shared_ptr<NTTable> NTTablePtr;

namespace detail {

    /**
     * @brief Interface for in-line creating of NTTable.
     *
     * One instance can be used to create multiple instances.
     * An instance of this object must not be used concurrently (an object has a state).
     * @author mse
     */
    class epicsShareClass NTTableBuilder :
        public std::tr1::enable_shared_from_this<NTTableBuilder>
    {
    public:
        POINTER_DEFINITIONS(NTTableBuilder);

        /**
         * Adds a column of given <b>Scalar</b> type.
         * @param name name of the column.
         * @param elementType column type, a scalar array.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer addColumn(std::string const & name, epics::pvData::ScalarType elementType);

        /**
         * Adds descriptor field to the NTTable.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer addDescriptor();

        /**
         * Adds alarm field to the NTTable.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer addAlarm();

        /**
         * Adds timeStamp field to the NTTable.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer addTimeStamp();

        /**
         * Creates a <b>Structure</b> that represents NTTable.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>Structure</b>.
         */
        epics::pvData::StructureConstPtr createStructure();

        /**
         * Creates a <b>PVStructure</b> that represents NTTable.
         * The returned PVStructure will have labels equal to the column names.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>PVStructure</b>.
         */
        epics::pvData::PVStructurePtr createPVStructure();

        /**
         * Creates a <b>NTTable</b> instance.
         * The returned NTTable will wrap a PVStructure which will have
         * labels equal to the column names.
         * This resets this instance state and allows new instance to be created.
         * @return a new instance of <b>NTTable</b>.
         */
        NTTablePtr create();

        /**
         * Adds extra <b>Field</b> to the type.
         * @param name the name of the field.
         * @param field the field to be added.
         * @return this instance of <b>NTTableBuilder</b>.
         */
        shared_pointer add(std::string const & name, epics::pvData::FieldConstPtr const & field);

    private:
        NTTableBuilder();

        void reset();

        std::vector<std::string> columnNames;
        std::vector<epics::pvData::ScalarType> types;

        bool descriptor;
        bool alarm;
        bool timeStamp;

        // NOTE: this preserves order, however it does not handle duplicates
        epics::pvData::StringArray extraFieldNames;
        epics::pvData::FieldConstPtrArray extraFields;

        friend class ::epics::nt::NTTable;
    };

}

typedef std::tr1::shared_ptr<detail::NTTableBuilder> NTTableBuilderPtr;



/**
 * @brief Convenience Class for NTTable
 *
 * @author mrk
 */
class epicsShareClass NTTable
{
public:
    POINTER_DEFINITIONS(NTTable);

    static const std::string URI;

    /**
     * Creates an NTTable wrapping the specified PVStructure if the latter is compatible.
     * <p>
     * Checks the supplied PVStructure is compatible with NTTable
     * and if so returns an NTTable which wraps it.
     * This method will return null if the structure is is not compatible
     * or is null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTTable instance wrapping pvStructure on success, null otherwise
     */
    static shared_pointer wrap(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Creates an NTTable wrapping the specified PVStructure, regardless of the latter's compatibility.
     * <p>
     * No checks are made as to whether the specified PVStructure
     * is compatible with NTTable or is non-null.
     *
     * @param pvStructure the PVStructure to be wrapped
     * @return NTTable instance wrapping pvStructure
     */
    static shared_pointer wrapUnsafe(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure reports to be a compatible NTTable.
     * <p>
     * Checks if the specified Structure reports compatibility with this
     * version of NTTable through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTTable
     */
    static bool is_a(epics::pvData::StructureConstPtr const & structure);

    /**
     * Returns whether the specified PVStructure reports to be a compatible NTTable.
     * <p>
     * Checks if the specified PVStructure reports compatibility with this
     * version of NTTable through its type ID, including checking version numbers.
     * The return value does not depend on whether the structure is actually
     * compatible in terms of its introspection type.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTTable
     */
    static bool is_a(epics::pvData::PVStructurePtr const & pvStructure);

    /**
     * Returns whether the specified Structure is compatible with NTTable.
     * <p>
     * Checks if the specified Structure is compatible with this version
     * of NTTable through the introspection interface.
     *
     * @param structure the Structure to test
     * @return (false,true) if the specified Structure (is not, is) a compatible NTTable
     */
    static bool isCompatible(
        epics::pvData::StructureConstPtr const &structure);

    /**
     * Returns whether the specified PVStructure is compatible with NTTable.
     *
     * Checks if the specified PVStructure is compatible with this version
     * of NTTable through the introspection interface.
     *
     * @param pvStructure the PVStructure to test
     * @return (false,true) if the specified PVStructure (is not, is) a compatible NTTable
     */
    static bool isCompatible(
        epics::pvData::PVStructurePtr const &pvStructure);

    /**
     * Returns whether the specified structure is a valid NTTable.
     * <p>
     * Unlike isCompatible(), isValid() may perform checks on the value
     * data as well as the introspection data.
     *
     * @return (false,true) if wrapped PVStructure (is not, is) a valid NTTable
     */
    bool isValid();

    /**
     * Creates an NTTable builder instance.
     * @return builder instance.
     */
    static NTTableBuilderPtr createBuilder();

    /**
     * Destructor.
     */
    ~NTTable() {}

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
     * @return the timStamp field or null if no such field.
     */
    epics::pvData::PVStructurePtr getTimeStamp() const;

    /**
     * Returns the alarm field.
     * @return the alarm field or null if no such field.
     */
    epics::pvData::PVStructurePtr getAlarm() const;

    /**
     * Returns the labels field.
     * @return the labels field.
     */
    epics::pvData::PVStringArrayPtr getLabels() const;

    /**
     * Returns the column names for the table.
     * For each name, calling getColumn should return the column, which should not be null.
     * @return the column names.
     */
    epics::pvData::StringArray const & getColumnNames() const;

    /**
     * Returns the PVField for the column with the specified colum name.
     * @param columnName the name of the column.
     * @return the field for the column or null if column does not exist.
     */
    epics::pvData::PVFieldPtr getColumn(std::string const & columnName) const;

    /**
     * Returns the column with the specified column name and of a specified
     * expected type (for example, PVDoubleArray).
     * @tparam PVT the expected type of the column which should be
     *             be PVScalarArray or a derived class.
     * @param columnName the name of the column.
     * @return the field for the column or null if column does not exist
     *         or is not of the specified type.
     */
    template<typename PVT>
    std::tr1::shared_ptr<PVT> getColumn(std::string const & columnName) const
    {
        epics::pvData::PVFieldPtr pvField = getColumn(columnName);
        if (pvField.get())
            return std::tr1::dynamic_pointer_cast<PVT>(pvField);
        else
            return std::tr1::shared_ptr<PVT>();
    }

private:
    NTTable(epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStructurePtr pvNTTable;
    epics::pvData::PVStructurePtr pvValue;
    friend class detail::NTTableBuilder;
};

}}
#endif  /* NTTABLE_H */
