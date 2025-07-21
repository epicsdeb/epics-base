/* timeStamp.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <ctime>

#include <epicsTime.h>
#include <shareLib.h>

#include <pv/pvType.h>


namespace epics { namespace pvData { 

epicsShareExtern const int32 milliSecPerSec;
epicsShareExtern const int32 microSecPerSec;
epicsShareExtern const int32 nanoSecPerSec;
epicsShareExtern const int64 posixEpochAtEpicsEpoch;

/** @brief Methods for manipulating timeStamp.
 * 
 * A timeStamp structure has the following fields:
 @code
structure
    long secondsPastEpoch
    int nanoseconds
    int userTag
 @endcode
 * This is a class that holds values corresponding to the fields in
 * a timeStamp structure.
 * It is meant to be used together with pvTimeStamp
 * which allows values to be copied between an timeStamp structure
 * and this class.
 * This class should not be extended.
 */
class epicsShareClass TimeStamp {
public:
    /**
     * Default constructor
     */
    TimeStamp() 
    :secondsPastEpoch(0), nanoseconds(0), userTag(0) {}
    /**
     * Constructor
     * @param secondsPastEpoch seconds since 1970 UTC
     * @param nanoseconds nanoseconds since secondsPastEpoch
     * @param userTag application specific
     */
    TimeStamp(int64 secondsPastEpoch,int32 nanoseconds = 0,int32 userTag = 0);
    //default constructors and destructor are OK
    //This class should not be extended
    /**
     * adjust secondsPastEpoch and nanoseconds so that
     * 0 <= nanoseconds < nanoSecPerSec
     */
    void normalize();
    /**
     * Set timeStamp from standard C time
     * @param time time as returned by std::time
     */
    void fromTime_t(const time_t & time);
    /**
     * Set time from timeStamp
     * @param time time as defined by std::time
     */
    void toTime_t(time_t &time) const;
    /**
     * Get secondsPastEpoch.
     * @return The secondsPastEpoch.
     */
    int64 getSecondsPastEpoch() const {return secondsPastEpoch;}
    /**
     * Get secondsPastEpoch for EPICS V3.
     * This is seconds since 1990 UTC.
     * @return The epics V3 secondsPastEpoch.
     */
    int64 getEpicsSecondsPastEpoch() const {
        return secondsPastEpoch - posixEpochAtEpicsEpoch;
    }
    /**
     * Get nanoseconds.
     * @return nanoseconds within timeStamp.
     */
    int32 getNanoseconds() const  {return nanoseconds;}
    /**
     * Get userTag.
     * @return userTag.
     */
    int32 getUserTag() const {return userTag;}
    /**
     * Set userTag.
     * @param userTag application specific.
     */
    void setUserTag(int32 userTag) {this->userTag = userTag;}
    /**
     * Set time fields in timeStamp.
     * Result will be normalized.
     * @param secondsPastEpoch seconds part of timeStamp.
     * @param nanoseconds nanoseconds part of timeStamp.
     */
    void put(int64 secondsPastEpoch,int32 nanoseconds = 0) {
        this->secondsPastEpoch = secondsPastEpoch;
        this->nanoseconds = nanoseconds;
        normalize();
    }
    /**
     * Set time fields in timeStamp.
     * @param milliseconds The number of milliseconds since the epoch.
     */
    void put(int64 milliseconds);
    /**
     * Set the timeStamp to the current time.
     */
    void getCurrent();
    /**
     * Convert the timeStamp to a double value that is seconds past epoch.
     * @return seconds past 1970 UTC
     */
    double toSeconds() const ;
    /**
     * Standard C++ operator.
     */
    bool operator==(TimeStamp const &) const;
    /**
     * Standard C++ operator.
     */
    bool operator!=(TimeStamp const &) const;
    /**
     * Standard C++ operator.
     */
    bool operator<=(TimeStamp const &) const;
    /**
     * Standard C++ operator.
     */
    bool operator< (TimeStamp const &) const;
    /**
     * Standard C++ operator.
     */
    bool operator>=(TimeStamp const &) const;
    /**
     * Standard C++ operator.
     */
    bool operator> (TimeStamp const &) const;
    /**
     * Return a-b as a  double value with units of seconds.
     * @param a first timeStamp
     * @param b second timeStamp
     * @return time difference in seconds.
     */
    static double diff(TimeStamp const & a,TimeStamp const & b);
    /**
     * Standard C++ operator.
     */
    TimeStamp & operator+=(int64 seconds);
    /**
     * Standard C++ operator.
     */
    TimeStamp & operator-=(int64 seconds);
    /**
     * Standard C++ operator.
     */
    TimeStamp & operator+=(double seconds);
    /**
     * Standard C++ operator.
     */
    TimeStamp & operator-=(double seconds);
    /**
     * Get number of milliseconds past epoch.
     * @return milliseconds past epoch.
     */
    int64 getMilliseconds();
private:
    static int64 diffInt(TimeStamp const &left,TimeStamp const  &right );
    int64 secondsPastEpoch;
    int32 nanoseconds;
    int32 userTag;
};
  

}}
#endif  /* TIMESTAMP_H */
