/* display.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>

#include <pv/pvType.h>
#include <pv/pvData.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief Methods for a display structure.
 * 
 * An display structure has the following fields:
 @code
structure
    double limitLow
    double limitHigh
    string description
    string format
    string units
 @endcode
 * This is a class that holds values corresponding to the fields in
 * a display structure.
 * It is meant to be used together with pvDisplay
 * which allows values to be copied between an display structure
 * and this class.
 * This class should not be extended.
 */
class epicsShareClass Display {
public:
    /**
     * Constructor
     */
    Display()
    : description(std::string("")),format(std::string("")),units(std::string("")),
        low(0.0),high(0.0) {}
    //default constructors and destructor are OK
    /**
     * Get the current value of limitLow.
     * @return The current value.
     */
    double getLow() const {return low;}
    /**
     * Get the current value of limitHigh.
     * @return The current value.
     */
    double getHigh() const{ return high;}
    /**
     * Set limitLow to a new value.
     * @param value The value.
     */
    void setLow(double value){low = value;}
    /**
     * Set limitHigh to a new value.
     * @param value The value.
     */
    void setHigh(double value){high = value;}
    /**
     * Get the current value of description.
     * @return The current value.
     */
    std::string getDescription() const {return description;}
    /**
     * Set description to a new value.
     * @param value The value.
     */
    void setDescription(std::string const & value) {description = value;}
    /**
     * Get the current value of format.
     * @return The current value.
     */
    std::string getFormat() const {return format;}
    /**
     * Set format to a new value.
     * @param value The value.
     * The rules for a valid syntax has not been specified.
     */
    void setFormat(std::string const & value) {format = value;}
    /**
     * Get the current value of units.
     * @return The current value.
     */
    std::string getUnits() const {return units;}
    /**
     * Set units to a new value.
     * @param value The value.
     */
    void setUnits(std::string const & value) {units = value;}
private:
    std::string description;
    std::string format;
    std::string units;
    double low;
    double high;
};
    
}}
#endif  /* DISPLAY_H */
