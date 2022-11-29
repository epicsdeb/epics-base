/* control.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef CONTROL_H
#define CONTROL_H

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief Methods for a control structure.
 * 
 * An control structure has the following fields:
 @code
structure
    double limitLow
    double limitHigh
    double minStep
 @endcode
 * This is a class that holds values corresponding to the fields in
 * a control structure.
 * It is meant to be used together with pvControl
 * which allows values to be copied between an control structure
 * and this class.
 * This class should not be extended.
 */
class epicsShareClass Control {
public:
    /**
     * Constructor
     */
    Control() : low(0.0), high(0.0), minStep(0.0) {}
    //default constructors and destructor are OK
    /**
     * get limitLow
     * @return the current value.
     */
    double getLow() const {return low;}
    /**
     * get limitHigh
     * @return the current value.
     */
    double getHigh() const {return high;}
    /**
     *  get minStep
     * @return the current value.
     */
    double getMinStep() const {return minStep;}
    /**
     *  set limitLow
     *  @param value The new value.
     */
    void setLow(double value) {low = value;}
    /**
     * set limitHigh
     *  @param value The new value.
     */
    void setHigh(double value) {high = value;}
    /**
     * set minStep
     *  @param value The new value.
     */
    void setMinStep(double value) {minStep = value;}
private:
    double low;
    double high;
    double minStep;
};
    
}}
#endif  /* CONTROL_H */
