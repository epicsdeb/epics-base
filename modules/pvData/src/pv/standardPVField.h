/* standardPVField.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef STANDARDPVFIELD_H
#define STANDARDPVFIELD_H

#include <string>
#include <stdexcept>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/standardField.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

class StandardPVField;
typedef std::tr1::shared_ptr<StandardPVField> StandardPVFieldPtr;

/**
 * @brief StandardPVField is a class or creating standard data fields.
 *
 * Like class StandardField it has two forms of the methods which create a fields:
 *      one without properties and one with properties.
 * The properties are some combination of alarm, timeStamp, control, display, and valueAlarm.
 * Just like StandardField there are methods to create the standard properties.
 *
 * StandardPVField is a singleton class. The class is accessed via the statement: {@code
    StandardPVField *standardPVField = getStandardPVField();
 * }
 */
class epicsShareClass StandardPVField {
    EPICS_NOT_COPYABLE(StandardPVField)
public:
    /**
     * getStandardPVField returns the singleton.
     * @return Shared pointer to StandardPVField.
     */
    static StandardPVFieldPtr getStandardPVField();
    ~StandardPVField();
    /**
     * Create a structure that has a scalar value field.
     * @param type The type.
     * @param properties A comma separated list of properties.
     * This is some combination of "alarm,timeStamp,display,control,valueAlarm".
     * @return The const shared pointer to the structure.
     */
    PVStructurePtr scalar(ScalarType type,std::string const & properties);
    /**
     * Create a structure that has a scalar array value field.
     * @param elementType The element scalar type.
     * @param properties A comma separated list of properties.
     * This is some combination of "alarm,timeStamp,display,control,valueAlarm".
     * @return The const shared pointer to the structure.
     */
    PVStructurePtr scalarArray(ScalarType elementType, std::string const & properties);
    /**
     * Create a structure that has a structure array value field.
     * @param structure The Structure introspection object for elements of the value field.
     * @param properties A comma separated list of properties.
     * This is some combination of "alarm,timeStamp,display,control,valueAlarm".
     * @return The const shared pointer to the structure.
     */
    PVStructurePtr structureArray(StructureConstPtr const &structure,std::string const & properties);
    /**
     * Create a structure that has a union array value field.
     * @param punion The Union introspection object for elements of the value field.
     * @param properties A comma separated list of properties.
     * This is some combination of "alarm,timeStamp,display,control,valueAlarm".
     * @return The const shared pointer to the structure.
     */
    PVStructurePtr unionArray(UnionConstPtr const &punion,std::string const & properties);
    /**
     * Create a structure that has an enumerated structure value field.
     * The id for the structure is "enum_t".
     * @param choices This is a StringArray of choices.
     * @return The const shared pointer to the structure.
     */
    PVStructurePtr enumerated(StringArray const &choices);
    /**
     * Create a structure that has an enumerated structure value field.
     * The id for the structure is "epics:nt/NTEnum:1.0".
     * @param choices This is a StringArray of choices.
     * @param properties A comma separated list of properties.
     * @return The const shared pointer to the structure.
     */
    PVStructurePtr enumerated(StringArray const &choices, std::string const & properties);
private:
    StandardPVField();
    StandardFieldPtr standardField;
    FieldCreatePtr fieldCreate;
    PVDataCreatePtr pvDataCreate;
    std::string notImplemented;
};

epicsShareExtern StandardPVFieldPtr getStandardPVField();
    
}}
#endif  /* STANDARDPVFIELD_H */
