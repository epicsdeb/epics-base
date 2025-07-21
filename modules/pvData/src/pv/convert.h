/* convert.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef CONVERT_H
#define CONVERT_H

#include <string>
#include <stdexcept>
#include <vector>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

class Convert;
typedef std::tr1::shared_ptr<Convert> ConvertPtr;

/**
 * @brief Conversion and Copy facility for pvData.
 *
 * Convert between numeric types,  convert any field to a string,
 *  or convert from a string to a scalar field.
 * <p>Numeric conversions are between scalar numeric types or between arrays of
 * numeric types. It is not possible to convert between a scalar
 * and an array.
 * Numeric conversions are between types:
 * pvByte, pvShort, pvInt, pvLong,
 * pvUByte, pvUShort, pvUInt, pvULong,
 * pvFloat, or pvDouble.</p>
 * 
 * <p>getString converts any supported type to a std::string.</p>
 *
 * <p>fromString converts a std::string to a scalar.
 * fromStringArray converts an array of std::strings
 * to a pvArray, which must have a scaler element type.
 * A scalar field is a numeric field or pvBoolean or pvString.</p>
 * <p>All from methods put data into a PVField, e.g. from means where the PVField gets it's data.</p>
 */
class epicsShareClass Convert {
public:
    static ConvertPtr getConvert();

    /**
     * Copy from a PVField to another PVField.
     * This calls one on copyScalar, copyArray, copyStructure.
     * The two arguments must be compatible.
     * @param from The source.
     * @param to The destination
     * @throws std::invalid_argument if the arguments are not compatible.
     * @deprecated use "to->copy[Unchecked](*from)" instead
     */
    void copy(PVFieldPtr const & from, PVFieldPtr const & to) {
        to->copy(*from);
    }

    /**
     * Convert a PVField to a string.
     * If a PVField is a structure or array be prepared for a very long string.
     * @param buf string that will hold pvField converted to a string,
     * @param pvField The PVField to convert to a string.
     */
    inline void getString(std::string * buf,PVFieldPtr const & pvField)
    {getString(buf, pvField.get(), 0);}
    /**
     * Convert a PVField to a string.
     * If a PVField is a structure or array be prepared for a very long string.
     * @param buf string that will hold pvField converted to a string,
     * @param pvField The PVField to convert to a string.
     * @param indentLevel indentation level
     */
    void getString(std::string * buf,PVField const * pvField,int indentLevel);
     /**
      * Convert from an array of std::string to a PVStructure
      * @param pv The PV.
      * @param from The array of std::string value to convert and put into a PV.
      * @param fromStartIndex The first element if the array of strings.
      * @return The total number of fields that have been changed.
      * @throws std::logic_error if the array of std::string does not have a valid values.
      */
    std::size_t fromString(
        PVStructurePtr const &pv,
        StringArray const & from,
        std::size_t fromStartIndex = 0);
     /**
     * Convert from a std::string to a PVScalar
     * @param pv The PV.
     * @param from The std::string value to convert and put into a PV.
     * @throws std::logic_error if the std::string does not have a valid value.
     */
    void fromString(PVScalarPtr const & pv, std::string const & from)
    {
        pv->putFrom<std::string>(from);
    }

    /**
     * Convert  from a std::string to a PVScalarArray.
     * The std::string must be a comma separated set of values optionally enclosed in []
     * @param pv The PV.
     * @param from The std::string value to convert and put into a PV.
     * @return The number of elements converted.
     * @throws std::invalid_argument if the element Type is not a scalar.
     * @throws std::logic_error if the std::string does not have a valid array values.
     */
    std::size_t fromString(PVScalarArrayPtr const & pv, std::string from);
    /**
     * Convert a PVScalarArray from a std::string array.
     * The array element type must be a scalar.
     * @param pv The PV.
     * @param offset Starting element in a PV.
     * @param length The number of elements to transfer.
     * @param from The array of values to put into the PV.
     * @param fromOffset Starting element in the source array.
     * @return The number of elements converted.
     * @throws std::invalid_argument if the element Type is not a scalar.
     * @throws std::logic_error if the std::string does not have a valid value.
     */
    std::size_t fromStringArray(
        PVScalarArrayPtr const & pv,
        std::size_t offset, std::size_t length,
        StringArray const & from,
        std::size_t fromOffset);
    /**
     * Convert a PVScalarArray to a std::string array.
     * @param pv The PV.
     * @param offset Starting element in the PV array.
     * @param length Number of elements to convert to the string array.
     * @param to std::string array to receive the converted PV data.
     * @param toOffset Starting element in the string array.
     * @return Number of elements converted.
     */
    std::size_t toStringArray(PVScalarArrayPtr const & pv,
        std::size_t offset,
        std::size_t length,
        StringArray & to,
        std::size_t toOffset);
    /**
     * Convert a PV to a byte.
     * @param pv a PV
     * @return converted value
     */
    inline int8 toByte(PVScalarPtr const & pv) { return pv->getAs<int8>();}
    /**
     * Convert a PV to a short.
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline int16 toShort(PVScalarPtr const & pv) { return pv->getAs<int16>();}
    /**
     * Convert a PV to a int.
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline int32 toInt(PVScalarPtr const & pv) { return pv->getAs<int32>();}
    /**
     * Convert a PV to an long
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline int64 toLong(PVScalarPtr const & pv) { return pv->getAs<int32>();}
    /**
     * Convert a PV to a ubyte.
     * @param pv a PV
     * @return converted value
     */
    inline uint8 toUByte(PVScalarPtr const & pv) { return pv->getAs<uint8>();}
    /**
     * Convert a PV to a ushort.
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline uint16 toUShort(PVScalarPtr const & pv) { return pv->getAs<uint16>();}
    /**
     * Convert a PV to a uint.
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline uint32 toUInt(PVScalarPtr const & pv) { return pv->getAs<uint32>();}
    /**
     * Convert a PV to an ulong
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline uint64 toULong(PVScalarPtr const & pv) { return pv->getAs<uint64>();}
    /**
     * Convert a PV to a float
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline float toFloat(PVScalarPtr const & pv) { return pv->getAs<float>();}
    /**
     * Convert a PV to a double
     * @param pv a PV
     * @return converted value
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline double toDouble(PVScalarPtr const & pv) { return pv->getAs<double>();}
    /**
     * Convert a PV to a std::string
     * @param pv a PV
     * @return converted value
     */
    inline std::string toString(PVScalarPtr const & pv) { return pv->getAs<std::string>();}
    /**
     * Convert a PV from a byte
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromByte(PVScalarPtr const & pv,int8 from) { pv->putFrom<int8>(from); }
    /**
     * Convert a PV from a short
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromShort(PVScalarPtr const & pv,int16 from) { pv->putFrom<int16>(from); }
    /**
     * Convert a PV from an int
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromInt(PVScalarPtr const & pv, int32 from) { pv->putFrom<int32>(from); }
    /**
     * Convert a PV from a long
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromLong(PVScalarPtr const & pv, int64 from) { pv->putFrom<int64>(from); }
    /**
     * Convert a PV from a ubyte
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromUByte(PVScalarPtr const & pv,uint8 from) { pv->putFrom<uint8>(from); }
    /**
     * Convert a PV from a ushort
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromUShort(PVScalarPtr const & pv,uint16 from) { pv->putFrom<uint16>(from); }
    /**
     * Convert a PV from an uint
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromUInt(PVScalarPtr const & pv, uint32 from) { pv->putFrom<uint32>(from); }
    /**
     * Convert a PV from a ulong
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromULong(PVScalarPtr const & pv, uint64 from) { pv->putFrom<uint64>(from); }
    /**
     * Convert a PV from a float
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromFloat(PVScalarPtr const & pv, float from) { pv->putFrom<float>(from); }
    /**
     * Convert a PV from a double
     * @param pv a PV
     * @param from value to put into PV
     * @throws std::invalid_argument if the Type is not a numeric scalar
     */
    inline void fromDouble(PVScalarPtr const & pv, double from) { pv->putFrom<double>(from); }

};

static inline ConvertPtr getConvert() { return Convert::getConvert(); }
    
}}
#endif  /* CONVERT_H */
