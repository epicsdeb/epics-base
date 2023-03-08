/* pvType.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */

/* Definitions for the primitive types for pvData.
 * It also defines the arrays of the primitive types
 */

#ifndef PVTYPE_H
#define PVTYPE_H

#if defined(_WIN32) && !defined(_MINGW)
#pragma warning( push )
#pragma warning(disable: 4251)
#endif

#include <string>
#include <vector>

#if defined(vxWorks) && \
    (_WRS_VXWORKS_MAJOR+0 <= 6) && (_WRS_VXWORKS_MINOR+0 < 9)
typedef int intptr_t;
typedef unsigned int uintptr_t;
#ifndef INT64_MAX
#define INT64_MAX (0x7fffffffffffffffLL)
#define UINT64_MAX (0xffffffffffffffffULL)
#endif
#elif _MSC_VER==1500
#include <epicsTypes.h>
typedef epicsUInt8 uint8_t;
typedef epicsInt8 int8_t;
typedef epicsUInt16 uint16_t;
typedef epicsInt16 int16_t;
typedef epicsUInt32 uint32_t;
typedef epicsInt32 int32_t;
typedef epicsUInt64 uint64_t;
typedef epicsInt64 int64_t;
#else
#include <stdint.h>
#endif

#include <pv/sharedPtr.h>

//! epics
namespace epics {
//! pvData
namespace pvData {

namespace detail {
    // Pick either type If or type Else to not be Cond
    template<typename Cond, typename If, typename Else>
    struct pick_type { typedef If type; };
    template<typename Cond, typename Else>
    struct pick_type<Cond,Cond,Else> { typedef Else type; };
}

/**
 * This is a set of typedefs used by pvData.
 */

/**
 * boolean, i.e. can only have the values @c false or @c true
 */
typedef detail::pick_type<int8_t, signed char,
                          detail::pick_type<uint8_t, char, unsigned char>::type
                          >::type boolean;
/**
 * A 8 bit signed integer
 */
typedef int8_t   int8;
/**
 * A 16 bit signed integer
 */
typedef int16_t  int16;
/**
 * A 32 bit signed integer
 */
typedef int32_t  int32;
/**
 * A 64 bit signed integer
 */
typedef int64_t  int64;
/**
 * A 8 bit unsigned integer
 */
typedef uint8_t   uint8;
/**
 * A 16 bit unsigned integer
 */
typedef uint16_t  uint16;
/**
 * A 32 bit unsigned integer
 */
typedef uint32_t uint32;
/**
 * A 64 bit unsigned integer
 */
typedef uint64_t uint64;

// float and double are types

/**
 * A string array.
 */
typedef std::vector<std::string> StringArray;
typedef std::tr1::shared_ptr<StringArray> StringArrayPtr;
inline std::string * get(StringArray &value)
{
    return &value[0];
}
inline std::string const * get(StringArray const &value)
{
    return static_cast<std::string const *>(&value[0]);
}
inline std::string * get(StringArrayPtr &value)
{
    return get(*value.get());
}
inline std::string const * get(StringArrayPtr const &value)
{
    return get(*value.get());
}
inline StringArray & getVector(StringArrayPtr &value)
{
     return *value.get();
}
inline StringArray const & getVector(StringArrayPtr const &value)
{
     return *value.get();
}
typedef std::vector<std::string>::iterator StringArray_iterator;
typedef std::vector<std::string>::const_iterator StringArray_const_iterator;

}}

#if defined(_WIN32) && !defined(_MINGW)
#pragma warning( pop )
#endif

#endif  /* PVTYPE_H */
