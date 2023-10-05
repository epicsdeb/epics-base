/* pvAlarm.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef PVALARM_H
#define PVALARM_H

#include <string>

#include <pv/pvType.h>
#include <pv/alarm.h>
#include <pv/pvData.h>

#include <shareLib.h>

namespace epics { namespace pvData {

/** @brief Methods for accessing an alarm structure.
 * 
 * An alarm structure has the following fields:
 @code
structure
    int severity
    int status
    string message
 @endcode
 * This class can be attached to an alarm structure field of any
 * PVData object.
 * The methods provide access to the fields in the attached structure,
 * via an instance of class Alarm.
 * This class should not be extended.
 */
class epicsShareClass PVAlarm {
public:
    /**
     * 
     * Constructor
     */
    PVAlarm() {}
    //default constructors and destructor are OK
    //returns (false,true) if pvField(is not, is) a valid alarm structure
    //An automatic detach is issued if already attached.
    /*
     * Attach to a field of a PVData object.
     * @param pvField The pvField.
     * @return (false,true) if the pvField (is not, is) an alarm structure.
     */
    /*
     * Attach to a field of a PVData object.
     * @param pvField The pvField.
     * @return (false,true) if the pvField (is not, is) an alarm structure.
     */
    bool attach(PVFieldPtr const & pvField);
    /**
     * Detach for pvField.
     */
    void detach();
    /**
     * Is the PVAlarm attached to a pvField?
     * @return (false,true) (is not,is) attached to a pvField.
     */
    bool isAttached();
    // each of the following throws logic_error is not attached to PVField
    // set returns false if field is immutable
    /**
     * copy the alarm structure  values to Alarm
     * @param alarm An instance of class Alarm
     * @throw If not attached to a pvField.
     */
    void get(Alarm & alarm) const;
    /**
     * copy the values from Alarm to the alarm structure.
     * @param alarm An instance of class Alarm
     * @return (false,true) if pvField (immutable, mutable)
     * @throw If not attached to a pvField.
     */
    bool set(Alarm const & alarm);
private:
    PVIntPtr pvSeverity;
    PVIntPtr pvStatus;
    PVStringPtr pvMessage;
    static std::string noAlarmFound;
    static std::string notAttached;
};
    
}}
#endif  /* PVALARM_H */
