/* pvEnumerated.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef PVENUMERATED_H
#define PVENUMERATED_H

#include <string>

#include <pv/pvType.h>
#include <pv/pvData.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief Methods for accessing an enumerated structure.
 * 
 * An enumerated structure has the following fields:
 @code
structure
    int index
    string[] choices
 @endcode
 * This class can be attached to an enumerated structure field of any
 * PVData object.
 * The methods provide access to the fields in the attached structure.
 * This class should not be extended.
 */
class epicsShareClass PVEnumerated {
public:
    /*
     * Constructor.
     */
    PVEnumerated() {}
    //default constructors and destructor are OK
    //This class should not be extended
    //returns (false,true) if pvField(is not, is) a valid enumerated structure
    //An automatic detach is issued if already attached.
    /*
     * Attach to a field of a PVData object.
     * @param pvField The pvField.
     * @return (false,true) if the pvField (is not, is) an enumerated structure.
     */
    bool attach(PVFieldPtr const & pvField);
    /**
     * Detach for pvField.
     */
    void detach();
    /**
     * Is the PVEnumerated attached to a pvField?
     * @return (false,true) (is not,is) attached to a pvField.
     */
    bool isAttached();
    // each of the following throws logic_error is not attached to PVField
    // a set returns false if field is immutable
    /**
     * Set the index.
     * @param index The new index.
     * @throw if not attached.
     * The index will be changed even if it is out of range of size of choices.
     */
    bool setIndex(int32 index);
    /**
     * Get the index.
     * @return The current index.
     * @throw if not attached.
     */
    int32 getIndex();
    /**
     * Get the choice corresponding to current index.
     * @return The choice. If index is out of range a null string is returned.
     * @throw if not attached.
     */
    std::string getChoice();
    /**
     * Can choices be changed?
     * @return (false,true) if choices (can not, can) be changed.
     * @throw if not attached.
     */
    bool choicesMutable();
    /**
     * Get the choices.
     * @return The current index.
     * @throw if not attached.
     */
    inline PVStringArray::const_svector getChoices(){return pvChoices->view();}
    /**
     * Get the size of the choices array.
     * @return The size.
     * @throw if not attached.
     */
    int32 getNumberChoices();
    /**
     * Get the choices.
     * @param choices The new value for choices.`
     * @return (false,true) if choices (was not was) replaced.
     * @throw if not attached.
     */
    bool setChoices(const StringArray & choices);
private:
    static std::string notFound;
    static std::string notAttached;
    PVIntPtr pvIndex;
    PVStringArrayPtr pvChoices;
};
    
}}
#endif  /* PVENUMERATED_H */
