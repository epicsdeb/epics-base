/* timeStamp.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>

#include <epicsTime.h>

#define epicsExportSharedSymbols
#include <pv/noDefaultMethods.h>
#include <pv/pvType.h>
#include <pv/timeStamp.h>

namespace epics { namespace pvData {

const int32 milliSecPerSec = 1000;
const int32 microSecPerSec = 1000000;
const int32 nanoSecPerSec = 1000000000;
const int64 posixEpochAtEpicsEpoch = POSIX_TIME_AT_EPICS_EPOCH;

TimeStamp::TimeStamp(int64 secondsPastEpoch,int32 nanoseconds,int32 userTag)
: secondsPastEpoch(secondsPastEpoch),nanoseconds(nanoseconds),userTag(userTag)
{
    normalize();
}

void TimeStamp::normalize()
{
    if(nanoseconds>=0 && nanoseconds<nanoSecPerSec) return;
    while(nanoseconds>=nanoSecPerSec) {
        nanoseconds -= nanoSecPerSec;
        secondsPastEpoch++;
    }
    while(nanoseconds<0) {
        nanoseconds += nanoSecPerSec;
        secondsPastEpoch--;
    }
}

void TimeStamp::fromTime_t(const time_t & tt)
{
    epicsTimeStamp epicsTime;
    epicsTimeFromTime_t(&epicsTime,tt);
    secondsPastEpoch = epicsTime.secPastEpoch + posixEpochAtEpicsEpoch;
    nanoseconds = epicsTime.nsec;
}

void TimeStamp::toTime_t(time_t  &tt) const
{
    epicsTimeStamp epicsTime;
    epicsTime.secPastEpoch = static_cast<epicsUInt32>(secondsPastEpoch-posixEpochAtEpicsEpoch);
    epicsTime.nsec = nanoseconds;
    epicsTimeToTime_t(&tt,&epicsTime);
}

void TimeStamp::put(int64 milliseconds)
{
    secondsPastEpoch = milliseconds/1000;
    nanoseconds = (milliseconds%1000)*1000000;
}

void TimeStamp::getCurrent()
{
    epicsTimeStamp epicsTime;
    epicsTimeGetCurrent(&epicsTime);
    secondsPastEpoch = epicsTime.secPastEpoch;
    secondsPastEpoch += posixEpochAtEpicsEpoch;
    nanoseconds = epicsTime.nsec;
}

double TimeStamp::toSeconds() const
{
    double value = static_cast<double>(secondsPastEpoch);
    double nano = nanoseconds;
    value += nano/1e9;
    return value;
}

int64 TimeStamp::diffInt(TimeStamp const & left,TimeStamp  const&right )
{
    int64 sl = left.secondsPastEpoch;
    int32 nl = left.nanoseconds;
    int64 sr = right.secondsPastEpoch;
    int32 nr = right.nanoseconds;
    int64 sdiff = sl - sr;
    sdiff *= nanoSecPerSec;
    sdiff += nl - nr;
    return sdiff;
}

bool TimeStamp::operator==(TimeStamp const &right) const
{
    int64 sdiff = diffInt(*this,right);
    if(sdiff==0) return true;
    return false;
}

bool TimeStamp::operator!=(TimeStamp const &right) const
{
    int64 sdiff = diffInt(*this,right);
    if(sdiff!=0) return true;
    return false;
}

bool TimeStamp::operator<=(TimeStamp const &right) const
{
    int64 sdiff = diffInt(*this,right);
    if(sdiff<=0) return true;
    return false;
}

bool TimeStamp::operator< (TimeStamp const &right) const
{
    int64 sdiff = diffInt(*this,right);
    if(sdiff<0) return true;
    return false;
}

bool TimeStamp::operator>=(TimeStamp const &right) const
{
    int64 sdiff = diffInt(*this,right);
    if(sdiff>=0) return true;
    return false;
}

bool TimeStamp::operator>(TimeStamp const &right) const
{
    int64 sdiff = diffInt(*this,right);
    if(sdiff>0) return true;
    return false;
}

double TimeStamp::diff(TimeStamp const & a,TimeStamp const & b)
{
    double result = static_cast<double>(a.secondsPastEpoch - b.secondsPastEpoch);
    result += (a.nanoseconds - b.nanoseconds)/1e9;
    return result;
}


TimeStamp & TimeStamp::operator+=(int64 seconds)
{
    secondsPastEpoch += seconds;
    return *this;
}

TimeStamp & TimeStamp::operator-=(int64 seconds)
{
    secondsPastEpoch -= seconds;
    return *this;
}

TimeStamp & TimeStamp::operator+=(double seconds)
{
    int64 secs = static_cast<int64>(seconds);
    int64 nano = static_cast<int64>((seconds - secs)*1e9);
    nanoseconds += static_cast<int32>(nano);
    if(nanoseconds>nanoSecPerSec) {
        nanoseconds -= nanoSecPerSec;
        secondsPastEpoch += 1;
    } else if(nanoseconds<-nanoSecPerSec) {
        nanoseconds += -nanoSecPerSec;
        secondsPastEpoch -= 1;
    }
    secondsPastEpoch += secs;
    return *this;
}

TimeStamp & TimeStamp::operator-=(double seconds)
{
   return operator+=(-seconds);
}

int64 TimeStamp::getMilliseconds()
{
    return secondsPastEpoch*1000 + nanoseconds/1000000;
}

}}
