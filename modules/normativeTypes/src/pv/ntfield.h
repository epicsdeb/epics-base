/* ntfield.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTFIELD_H
#define NTFIELD_H

#include <cstdarg>

#ifdef epicsExportSharedSymbols
#   define ntfieldEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>
#include <pv/sharedVector.h>

#ifdef ntfieldEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef ntfieldEpicsExportSharedSymbols
#endif

#include <shareLib.h>

namespace epics { namespace nt {

struct Result;

class NTField;
typedef std::tr1::shared_ptr<NTField> NTFieldPtr;

class PVNTField;
typedef std::tr1::shared_ptr<PVNTField> PVNTFieldPtr;

/**
 * @brief Convenience Class for introspection fields of a Normative Type
 *
 * @author mrk
 * 
 */
class epicsShareClass NTField {
public:
    POINTER_DEFINITIONS(NTField);
    /**
     * Gets the single implementation of this class.
     * @return the implementation
     */
    static NTFieldPtr get();
    /**
     * destructor
     */
    ~NTField() {}

    /**
     * Is field an enumerated structure.
     * @param field the field to test.
     * @return (false,true) if field (is not,is) an enumerated structure.
     */
    bool isEnumerated(epics::pvData::FieldConstPtr const & field);

    /**
     * Is field a timeStamp structure.
     * @param field the field to test.
     * @return (false,true) if field (is not,is) a timeStamp structure.
     */
    bool isTimeStamp(epics::pvData::FieldConstPtr const & field);

    /**
     * Is field an alarm structure.
     * @param field the field to test.
     * @return (false,true) if field (is not,is) an alarm structure.
     */
    bool isAlarm(epics::pvData::FieldConstPtr const & field);

    /**
     * Is field a display structure.
     * @param field the field to test.
     * @return (false,true) if field (is not,is) a display structure.
     */
    bool isDisplay(epics::pvData::FieldConstPtr const & field);

    /**
     * Is field an alarmLimit structure.
     * @param field the field to test.
     * @return (false,true) if field (is not,is) an alarmLimit structure.
     */
    bool isAlarmLimit(epics::pvData::FieldConstPtr const & field);

    /**
     * Is field a control structure.
     * @param field the field to test.
     * @return (false,true) if field (is not,is) a control structure.
     */
    bool isControl(epics::pvData::FieldConstPtr const & field);

    /**
     * Creates an enumerated structure.
     * @return an enumerated structure.
     */
    epics::pvData::StructureConstPtr createEnumerated();

    /**
     * Creates a timeStamp structure.
     * @return a timeStamp structure.
     */
    epics::pvData::StructureConstPtr createTimeStamp();

    /**
     * Creates an alarm structure.
     * @return an alarm structure.
     */
    epics::pvData::StructureConstPtr createAlarm();

    /**
     * Creates a display structure.
     * @return a displayalarm structure.
     */
    epics::pvData::StructureConstPtr createDisplay();

    /**
     * Creates a control structure.
     * @return a control structure.
     */
    epics::pvData::StructureConstPtr createControl();

    /**
     * Creates an array of enumerated structures.
     * @return an array of enumerated structures.
     */
    epics::pvData::StructureArrayConstPtr createEnumeratedArray();

    /**
     * Creates an array of timeStamp structures.
     * @return an array of timeStamp structures.
     */
    epics::pvData::StructureArrayConstPtr createTimeStampArray();

    /**
     * Creates an array of alarm structures.
     * @return an array of alarm structures.
     */
    epics::pvData::StructureArrayConstPtr createAlarmArray();

private:
    NTField();
    epics::pvData::FieldCreatePtr fieldCreate;
    epics::pvData::StandardFieldPtr standardField;

    // These won't be public just yet
    static Result& isEnumerated(Result&);
    static Result& isTimeStamp(Result&);
    static Result& isAlarm(Result&);
    static Result& isDisplay(Result&);
    static Result& isAlarmLimit(Result&);
    static Result& isControl(Result&);

    friend class NTAggregate;
    friend class NTAttribute;
    friend class NTContinuum;
    friend class NTEnum;
    friend class NTHistogram;
    friend class NTMatrix;
    friend class NTMultiChannel;
    friend class NTNameValue;
    friend class NTNDArray;
    friend class NTNDArrayAttribute;
    friend class NTScalar;
    friend class NTScalarArray;
    friend class NTScalarMultiChannel;
    friend class NTTable;
    friend class NTUnion;
};

/**
 * @brief Convenience Class for data fields of a Normative Type
 *
 * @author mrk
 * 
 */
class epicsShareClass PVNTField {
public:
    POINTER_DEFINITIONS(PVNTField);
    /**
     * Returns the single implementation of this class.
     * @return the implementation
     */
    static PVNTFieldPtr get();

    /**
     * destructor
     */
    ~PVNTField() {}

    /**
     * Creates an enumerated PVStructure.
     * @param choices The array of choices.
     * @return an enumerated PVStructure.
     */
    epics::pvData::PVStructurePtr createEnumerated(
        epics::pvData::StringArray const & choices);

    /**
     * Creates a timeStamp PVStructure.
     * @return a timeStamp PVStructure.
     */
    epics::pvData::PVStructurePtr createTimeStamp();

    /**
     * Creates an alarm PVStructure.
     * @return an alarm PVStructure.
     */
    epics::pvData::PVStructurePtr createAlarm();

    /**
     * Creates a display PVStructure.
     * @return a display PVStructure.
     */
    epics::pvData::PVStructurePtr createDisplay();

    /**
     * Creates a control PVStructure.
     * @return a control PVStructure.
     */
    epics::pvData::PVStructurePtr createControl();

    /**
     * Creates an enumerated PVStructureArray.
     * @return an enumerated PVStructureArray.
     */
    epics::pvData::PVStructureArrayPtr createEnumeratedArray();

    /**
     * Creates a timeStamp PVStructureArray.
     * @return a timeStamp PVStructureArray.
     */
    epics::pvData::PVStructureArrayPtr createTimeStampArray();

    /**
     * Creates an alarm PVStructureArray.
     * @return an alarm PVStructureArray.
     */
    epics::pvData::PVStructureArrayPtr createAlarmArray();

private:
    PVNTField();
    epics::pvData::PVDataCreatePtr pvDataCreate;
    epics::pvData::StandardFieldPtr standardField;
    epics::pvData::StandardPVFieldPtr standardPVField;
    NTFieldPtr ntstructureField;
};

}}

#endif  /* NTFIELD_H */
