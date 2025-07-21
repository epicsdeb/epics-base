/* #define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE)
 *
 * Helper for switch() on type code
 *
 * BASETYPE is suffix of type from epicsTypes.h
 *    epics ## BASETYPE -> eg. epicsUInt8
 * PVATYPE is storage type
 *    PVATYPE -> eg epics::pvData::uint8
 * DBFTYPE is suffix of DBF_*
 *    DBF_ ## DBFTYPE -> eg. DBF_UCHAR
 * PVACODE is suffix of ScalarType enum
 *    epics::pvData::pv ## PVACODE -> epics::pvData::pvUByte
 */
CASE(UInt8, epics::pvData::uint8, UCHAR, UByte)
CASE(Int8, epics::pvData::int8, CHAR, Byte)
CASE(UInt16, epics::pvData::uint16, USHORT, UShort)
CASE(Int16, epics::pvData::int16, SHORT, Short)
CASE(UInt32, epics::pvData::uint32, ULONG, UInt)
CASE(Int32, epics::pvData::int32, LONG, Int)
CASE(Float32, float, FLOAT, Float)
CASE(Float64, double, DOUBLE, Double)
#ifndef CASE_SKIP_BOOL
CASE(UInt8, epics::pvData::boolean, UCHAR, Boolean)
#endif
#ifdef CASE_REAL_INT64
CASE(UInt64, epics::pvData::uint64, UINT64, ULong)
CASE(Int64, epics::pvData::int64, INT64, Long)
#elif defined(CASE_SQUEEZE_INT64)
CASE(UInt32, epics::pvData::uint64, ULONG, ULong)
CASE(Int32, epics::pvData::int64, LONG, Long)
#else
/* nothing */
#endif
#ifdef CASE_ENUM
// yes really, Base uses SHORT (16-bit) while PVD uses Int (32-bit)
CASE(Enum16, epics::pvData::int32, ENUM, Int)
#endif
#ifdef CASE_STRING
CASE(Int8, std::string, CHAR, String)
#endif
/* #undef CASE */
