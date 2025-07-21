/* alarm.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <string>
#include <stdexcept>

#include <epicsMutex.h>

#define epicsExportSharedSymbols
#include <pv/lock.h>
#include <pv/pvType.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/alarm.h>

using std::string;

namespace epics { namespace pvData {

AlarmSeverity AlarmSeverityFunc::getSeverity(int value)
{
    if(value<0 || value>4) {
         throw std::logic_error(string("getSeverity value is illegal"));
    }
    switch (value) {
    case 0: return noAlarm;
    case 1: return minorAlarm;
    case 2: return majorAlarm;
    case 3: return invalidAlarm;
    case 4: return undefinedAlarm;
    }
    throw std::logic_error(string("should never get here"));
}

StringArrayPtr AlarmSeverityFunc::getSeverityNames()
{
    static size_t severityCount = 5;
    static StringArrayPtr severityNames;
    static Mutex mutex;
    Lock xx(mutex);
    if(severityNames.get()==NULL) {
        severityNames = StringArrayPtr(new StringArray());
        severityNames->reserve(severityCount);
        severityNames->push_back("NONE");
        severityNames->push_back("MINOR");
        severityNames->push_back("MAJOR");
        severityNames->push_back("INVALID");
        severityNames->push_back("UNDEFINED");
    }
    return severityNames;
}

AlarmSeverity Alarm::getSeverity() const
{
    switch(severity) {
    case 0: return noAlarm;
    case 1: return minorAlarm;
    case 2: return majorAlarm;
    case 3: return invalidAlarm;
    case 4: return undefinedAlarm;
    }
    throw std::logic_error(string("should never get here"));
}

AlarmStatus AlarmStatusFunc::getStatus(int value)
{
    if(value<0 || value>7) {
         throw std::logic_error(string("getStatus value is illegal"));
    }
    switch (value) {
    case 0: return noStatus;
    case 1: return deviceStatus;
    case 2: return driverStatus;
    case 3: return recordStatus;
    case 4: return dbStatus;
    case 5: return confStatus;
    case 6: return undefinedStatus;
    case 7: return clientStatus;
    }
    throw std::logic_error(string("should never get here"));
}

StringArrayPtr AlarmStatusFunc::getStatusNames()
{
    static size_t statusCount = 8;
    static StringArrayPtr statusNames;
    static Mutex mutex;
    Lock xx(mutex);
    if(statusNames.get()==NULL) {
        statusNames = StringArrayPtr(new StringArray());
        statusNames->reserve(statusCount);
        statusNames->push_back("NONE");
        statusNames->push_back("DEVICE");
        statusNames->push_back("DRIVER");
        statusNames->push_back("RECORD");
        statusNames->push_back("DB");
        statusNames->push_back("CONF");
        statusNames->push_back("UNDEFINED");
        statusNames->push_back("CLIENT");
    }
    return statusNames;
}

AlarmStatus Alarm::getStatus() const
{
    switch(status) {
    case 0: return noStatus;
    case 1: return deviceStatus;
    case 2: return driverStatus;
    case 3: return recordStatus;
    case 4: return dbStatus;
    case 5: return confStatus;
    case 6: return undefinedStatus;
    case 7: return clientStatus;
    }
    throw std::logic_error(string("should never get here"));
}

}}
