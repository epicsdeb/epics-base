/* pvTimeStamp.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef PVTIMESTAMP_H
#define PVTIMESTAMP_H

#include <string>
#include <stdexcept>

#include <pv/pvType.h>
#include <pv/pvData.h>
#include <pv/timeStamp.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief Methods for accessing a timeStamp structure.
 * 
 * A timeStamp structure has the following fields:
 @code
structure
    long secondsPastEpoch
    int nanoseconds
    int userTag
 @endcode
 * This class can be attached to a timeStamp structure field of any
 * PVData object.
 * The methods provide access to the fields in the attached structure,
 * via an instance of class TimeStamp.
 * This class should not be extended.
 */
class epicsShareClass PVTimeStamp {
public:
    /**
     * 
     * Constructor
     */
    PVTimeStamp(){}
    //default constructors and destructor are OK
    //This class should not be extended
    
    //returns (false,true) if pvField(is not, is) a valid timeStamp structure
    /*
     * Attach to a field of a PVData object.
     * @param pvField The pvField.
     * @return (false,true) if the pvField (is not, is) an timeStamp structure.
     */
    bool attach(PVFieldPtr const & pvField);
    /**
     * Detach for pvField.
     */
    void detach();
    /**
     * Is the PVTimeStamp attached to a pvField?
     * @return (false,true) (is not,is) attached to a pvField.
     */
    bool isAttached();
    // following throw logic_error if not attached to PVField
    // set returns false if field is immutable
    /**
     * copy the timeStamp structure  values to TimeStamp
     * @param timeStamp An instance of class TimeStamp
     * @throw If not attached to a pvField.
     */
    void get(TimeStamp & timeStamp) const;
    /**
     * copy the values from TimeStamp to the timeStamp structure.
     * @param timeStamp An instance of class TimeStamp
     * @return (false,true) if pvField (immutable, mutable)
     * @throw If not attached to a pvField.
     */
    bool set(TimeStamp const & timeStamp);
private:
    static std::string noTimeStamp;
    static std::string notAttached;
    PVLongPtr pvSecs;
    PVIntPtr pvUserTag;
    PVIntPtr pvNano;
};
    
}}
#endif  /* PVTIMESTAMP_H */
