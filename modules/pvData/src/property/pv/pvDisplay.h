/* pvDisplay.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef PVDISPLAY_H
#define PVDISPLAY_H

#include <string>

#include <pv/pvType.h>
#include <pv/pvData.h>
#include <pv/display.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief Methods for accessing an display structure.
 * 
 * A display structure has the following fields:
 @code
structure
    double limitLow
    double limitHigh
    string description
    string format
    string units
 @endcode
 * This class can be attached to a display structure field of any
 * PVData object.
 * The methods provide access to the fields in the attached structure,
 * via an instance of class Display.
 * This class should not be extended.
 */
class epicsShareClass PVDisplay {
public:
    /**
     * 
     * Constructor
     */
    PVDisplay() {}
    //default constructors and destructor are OK
    //An automatic detach is issued if already attached.
    /*
     * Attach to a field of a PVData object.
     * @param pvField The pvField.
     * @return (false,true) if the pvField (is not, is) an display structure.
     */
    bool attach(PVFieldPtr const & pvField);
    /**
     * Detach for pvField.
     */
    void detach();
    /**
     * Is the PVDisplay attached to a pvField?
     * @return (false,true) (is not,is) attached to a pvField.
     */
    bool isAttached();
    // each of the following throws logic_error if not attached to PVField
    // a set returns false if field is immutable
    /**
     * copy the display structure  values to Display
     * @param display An instance of class Display
     * @throw If not attached to a pvField.
     */
    void get(Display & display) const;
    /**
     * copy the values from Display to the display structure.
     * @param display An instance of class Display
     * @return (false,true) if pvField (immutable, mutable)
     * @throw If not attached to a pvField.
     */
    bool set(Display const & display);
private:
    static std::string noDisplayFound;
    static std::string notAttached;
    PVStringPtr pvDescription;
    PVStringPtr pvFormat;
    PVStringPtr pvUnits;
    PVDoublePtr pvLow;
    PVDoublePtr pvHigh;
};
    
}}
#endif  /* PVDISPLAY_H */
