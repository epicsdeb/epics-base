/* alarm.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef ALARM_H
#define ALARM_H

#include <string>

#include <pv/pvType.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief enum definition of AlarmSeverity
 * 
 * AlarmSeverity is:
 @code
enum AlarmSeverity {
 noAlarm,minorAlarm,majorAlarm,invalidAlarm,undefinedAlarm
};
 @endcode
 * 
 */
enum AlarmSeverity {
 noAlarm,minorAlarm,majorAlarm,invalidAlarm,undefinedAlarm
};

/** @brief enum definition of AlarmStatus
 * 
 * AlarmStatus is:
 @code
enum AlarmStatus {
    noStatus,deviceStatus,driverStatus,recordStatus,
    dbStatus,confStatus,undefinedStatus,clientStatus
};
 @endcode
 * 
 */
enum AlarmStatus {
    noStatus,deviceStatus,driverStatus,recordStatus,
    dbStatus,confStatus,undefinedStatus,clientStatus
};


/** @brief methods for AlarmSeverity
 *
 */
class epicsShareClass AlarmSeverityFunc {
public:
    /**
     * Get the severity.
     * @param value Get the alarm severity corresponding to the integer value.
     * @return The severity.
     * @throw if severity value is out of range.
     */
    static AlarmSeverity getSeverity(int value);
    /**
     * Get the array of severity names.
     * @return The array of severity names.
     */
    static StringArrayPtr getSeverityNames();
};

/** @brief methods for AlarmStatus
 *
 */
class epicsShareClass AlarmStatusFunc {
public:
    /**
     * Get the status.
     * @param value Get the alarm status corresponding to the integer value.
     * @return The status.
     * @throw if status value is out of range.
     */
    static AlarmStatus getStatus(int value);
    /**
     * Get the array of status names.
     * @return The array of status names.
     */
    static StringArrayPtr getStatusNames();
};

/** @brief Methods for manipulating alarm.
 * 
 * An alarm structure has the following fields:
 @code
structure
    int severity
    int status
    string message
 @endcode
 * This is a class that holds values corresponding to the fields in
 * an alarm structure.
 * It is meant to be used together with pvAlarm
 * which allows values to be copied between an alarm structure
 * and this class.
 * This class should not be extended.
 */
class epicsShareClass Alarm {
public:
    /**
     * Constructor
     */
    Alarm() : severity(0),status(0), message(std::string("")) {}
    //default constructors and destructor are OK
    /**
     * get the current message.
     * @return  The message.
     */
    std::string getMessage() const {return message;}
    /**
     * set the current message.
     * @param  value The new message value.
     */
    void setMessage(std::string const &value) {message = value;}
    /**
     * get the current severity.
     * @return The severity.
     */
    AlarmSeverity getSeverity() const;
    /**
     * set the current severity.
     * @param value The new severity.
     * @throw if severity value is out of range.
     */
    void setSeverity(AlarmSeverity value) {severity = value;}
    /**
     * get the current status.
     * @return The status.
     */
    AlarmStatus getStatus() const;
    /**
     * set the current status.
     * @param value The new status.
     * @throw if status value is out of range.
     */
    void setStatus(AlarmStatus value) { status = value;}
private:
    int32 severity;
    int32 status;
    std::string message;
};
    
}}
#endif  /* ALARM_H */
