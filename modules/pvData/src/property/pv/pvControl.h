/* pvControl.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef PVCONTROL_H
#define PVCONTROL_H

#include <pv/pvData.h>
#include <pv/control.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief Methods for accessing an control structure.
 * 
 * An control structure has the following fields:
 @code
structure
    double limitLow
    double limitHigh
    double minStep
 @endcode
 * This class can be attached to an control structure field of any
 * PVData object.
 * The methods provide access to the fields in the attached structure,
 * via an instance of class Control.
 * This class should not be extended.
 */
class epicsShareClass PVControl {
public:
    /**
     * 
     * Constructor
     */
    PVControl(){}
    //default constructors and destructor are OK
    //returns (false,true) if pvField(is not, is) a valid control structure
    //An automatic detach is issued if already attached.
    /*
     * Attach to a field of a PVData object.
     * @param pvField The pvField.
     * @return (false,true) if the pvField (is not, is) an control structure.
     */
    bool attach(PVFieldPtr const & pvField);
    /**
     * Detach for pvField.
     */
    void detach();
    /**
     * Is the PVControl attached to a pvField?
     * @return (false,true) (is not,is) attached to a pvField.
     */
    bool isAttached();
    // each of the following throws logic_error if not attached to PVField
    // set returns false if field is immutable
    /**
     * copy the control structure  values to Control
     * @param control An instance of class Control
     * @throw If not attached to a pvField.
     */
    void get(Control & control) const;
    /**
     * copy the values from Control to the control structure.
     * @param control An instance of class Control
     * @return (false,true) if pvField (immutable, mutable)
     * @throw If not attached to a pvField.
     */
    bool set(Control const & control);
private:
    PVDoublePtr pvLow;
    PVDoublePtr pvHigh;
    PVDoublePtr pvMinStep;
    static std::string noControlFound;
    static std::string notAttached;
};
    
}}
#endif  /* PVCONTROL_H */
