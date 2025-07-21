/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Michael Davidsaver */
#ifndef PVTYPECAST_H
#define PVTYPECAST_H

#include <stdexcept>
#include <sstream>

#include <epicsConvert.h>
#include <shareLib.h>

#include <pv/pvType.h>
#include <pv/pvIntrospect.h>
#include <pv/templateMeta.h>


namespace epics { namespace pvData {

namespace detail {
    // parseToPOD wraps the epicsParse*() functions in one name
    // and throws exceptions
    epicsShareExtern void parseToPOD(const char*, boolean *out);
    epicsShareExtern void parseToPOD(const char*, int8 *out);
    epicsShareExtern void parseToPOD(const char*, uint8 *out);
    epicsShareExtern void parseToPOD(const char*, int16_t *out);
    epicsShareExtern void parseToPOD(const char*, uint16_t *out);
    epicsShareExtern void parseToPOD(const char*, int32_t *out);
    epicsShareExtern void parseToPOD(const char*, uint32_t *out);
    epicsShareExtern void parseToPOD(const char*, int64_t *out);
    epicsShareExtern void parseToPOD(const char*, uint64_t *out);
    epicsShareExtern void parseToPOD(const char*, float *out);
    epicsShareExtern void parseToPOD(const char*, double *out);

    static inline void parseToPOD(const std::string& str, boolean *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, int8 *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, uint8 *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, int16_t *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, uint16_t *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, int32_t *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, uint32_t *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, int64_t *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, uint64_t *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, float *out) { return parseToPOD(str.c_str(), out); }
    static inline void parseToPOD(const std::string& str, double *out) { return parseToPOD(str.c_str(), out); }

    /* want to pass POD types by value,
     * and std::string by const reference
     */
    template<typename ARG>
    struct cast_arg { typedef ARG arg; };
    template<>
    struct cast_arg<std::string> { typedef const std::string& arg; };

    // Handle mangling of type/value when printing
    template<typename T>
    struct print_convolute {
        typedef T return_t;
        static FORCE_INLINE return_t op(const T& i) { return i; }
    };
    // trick std::ostream into treating chars as numbers
    // by promoting char to int
    template<>
    struct print_convolute<int8> {
        typedef signed int return_t;
        static FORCE_INLINE return_t op(int8 i) { return i; }
    };
    template<>
    struct print_convolute<uint8> {
        typedef unsigned int return_t;
        static FORCE_INLINE return_t op(uint8 i) { return i; }
    };
    // Turn boolean into a string
    template<>
    struct print_convolute<boolean> {
        typedef const char* return_t;
        static FORCE_INLINE return_t op(boolean i) { return i ? "true" : "false"; }
    };


    // default to C++ type casting
    template<typename TO, typename FROM, class Enable = void>
    struct cast_helper {
        static FORCE_INLINE TO op(FROM from) {
            return static_cast<TO>(from);
        }
    };

    // special handling when down-casting double to float
    template<>
    struct cast_helper<float, double> {
        static FORCE_INLINE float op(double from) {
            return epicsConvertDoubleToFloat(from);
        }
    };

    // print POD to string
    // when std::string!=FROM
    template<typename FROM>
    struct cast_helper<std::string, FROM, typename meta::not_same_type<std::string,FROM>::type> {
        static std::string op(FROM from) {
            std::ostringstream strm;
            strm << print_convolute<FROM>::op(from);
            if(strm.fail())
                throw std::runtime_error("Cast to string failed");
            return strm.str();
        }
    };

    // parse POD from string
    // TO!=std::string
    template<typename TO>
    struct cast_helper<TO, std::string, typename meta::not_same_type<TO,std::string>::type> {
        static FORCE_INLINE TO op(const std::string& from) {
            TO ret;
            parseToPOD(from, &ret);
            return ret;
        }
    };

    // parse POD from C string
    // TO!=const char*
    template<typename TO>
    struct cast_helper<TO, const char*,
            typename meta::_and<
                typename meta::not_same_type<TO,const char*>,
                typename meta::not_same_type<TO,std::string>
            >::type> {
        static FORCE_INLINE TO op(const char* from) {
            TO ret;
            parseToPOD(from, &ret);
            return ret;
        }
    };

} // end detail

/** @brief Casting/converting between supported scalar types.
 *
 * Supported types: uint8_t, int8_t, uint16_t, int16_t,
 *                  uint32_t, int32_t, uint64_t, int64_t,
 *                  float, double, std::string, const char* (only FROM)
 *
 * As defined in pvType.h
 *
 @throws std::runtime_error when the cast is not possible.
 @throws std::bad_alloc when the cast is not possible.
 *
 @section convertg Conversion Guarantees
 *
 * Conversions which always produce a correct result.
 *
 * - signed integer -> larger signed integer
 * - unsigned integer -> larger unsigned integer
 * - integer -> float or double (where sizeof(integer)<sizeof(floating))
 * - float -> double
 *
 * Conversions where out of range inputs always produce
 * a defined result, but may not be reversible.
 *
 * - double -> float.  When abs(value) is outside the range
 *    [FLT_MIN, FLT_MAX] the value is clipped to FLT_MIN or FLT_MAX
 *    with the sign preserved.
 *
 * Conversions where invalid or out of range inputs result
 * in an exception.
 *
 * - non-std::string -> std::string
 * - std::string -> non-std::string
 * - std::string -> std::string (throws only std::bad_alloc)
 *
 * Conversions where out of range inputs produce undefined
 * results.
 *
 * - signed integer -> smaller signed integer
 * - unsigned integer -> smaller unsigned integer
 * - signed integer <-> unsigned integer
 * - integer -> float or double (where sizeof(integer)>=sizeof(floating))
 * - float or double -> integer.  The floating point value
 *    is rounded towards zero.  However, the result for values
 *    too large to be represented by the integer type
 *    is not defined.
 *
 @section stringf std::string formats
 *
 * - Numbers beginning with 1-9 are parsed as base-10.
 * - Numbers beginning with '0x' are parsed as base-16
 * - Numbers beginning with '0' are parsed as base-8.
 * - Hex numbers are case insensitive.
 * - Exponential numbers may use either 'e' or 'E'.
 */
template<typename TO, typename FROM>
static FORCE_INLINE TO castUnsafe(const FROM& from)
{
    return detail::cast_helper<TO,FROM>::op(from);
}

epicsShareExtern void castUnsafeV(size_t count, ScalarType to, void *dest, ScalarType from, const void *src);

//! Cast value to printable type
//! A no-op except for int8 and uint8, which are cast to int
//! so that they are printed as numbers std::ostream operators,
//! and boolean which is transformed into a const char*
template<typename T>
static FORCE_INLINE
typename detail::print_convolute<T>::return_t
print_cast(const T& v) { return detail::print_convolute<T>::op(v); }

struct escape;

epicsShareFunc
std::ostream& operator<<(std::ostream& strm, const escape& Q);

//! Helper to print a string with escaping
//! @code strm<<'"'<<escape(astring)<<'"' @endcode
struct epicsShareClass escape
{
    enum style_t {
        C,   // default
        CSV, // a la RFC4180
    };
private:
    const std::string& orig;
    style_t S;
public:
    escape(const std::string& orig) :orig(orig), S(C) {}
    ~escape();
    //! Change escaping style
    inline escape& style(style_t s) { S = s; return *this; }
    //! print to string and return.  (alloc and copy)
    std::string str() const;

    friend
    epicsShareFunc
    std::ostream& operator<<(std::ostream& strm, const escape& Q);
};

struct maybeQuote {
    const std::string& s;
    maybeQuote(const std::string& s) :s(s) {}
};

epicsShareExtern
std::ostream& operator<<(std::ostream& strm, const maybeQuote& q);

}} // end namespace

#endif // PVTYPECAST_H
