/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef PV_JSON_H
#define PV_JSON_H

#include <istream>
#include <ostream>
#include <string>
#include <map>

#include <pv/pvdVersion.h>
#include <pv/pvData.h>

#ifdef epicsExportSharedSymbols
#   define pvjson_epicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <yajl_parse.h>

#ifdef pvjson_epicsExportSharedSymbols
#   define epicsExportSharedSymbols
#   include "shareLib.h"
#endif

#include <shareLib.h>

namespace epics{namespace pvData{

class BitSet;

/** @defgroup pvjson JSON print/parse
 *
 * Printing PVField as JSON and parsing JSON into PVField.
 *
 * @{
 */

//! Options used during printing
struct epicsShareClass JSONPrintOptions
{
    bool multiLine;        //!< include new lines
    bool ignoreUnprintable;//!< ignore union/union array when encountered
    unsigned indent;       //!< Initial indentation (# of spaces)
    bool json5;            //!< Output extended JSON (eg. NaN).  @since 8.1.0
    JSONPrintOptions();
};

/** Print PVStructure as JSON
 *
 * 'mask' selects those fields which will be printed.
 * @version Overload added after 7.0.0
 */
epicsShareFunc
void printJSON(std::ostream& strm,
               const PVStructure& val,
               const BitSet& mask,
               const JSONPrintOptions& opts = JSONPrintOptions());

/** Print PVField as JSON
 * @version Overload added after 7.0.0
 */
epicsShareFunc
void printJSON(std::ostream& strm,
               const PVField& val,
               const JSONPrintOptions& opts = JSONPrintOptions());

// To be deprecated in favor of previous form
FORCE_INLINE
void printJSON(std::ostream& strm,
               const PVField::const_shared_pointer& val,
               const JSONPrintOptions& opts = JSONPrintOptions())
{
    printJSON(strm, *val, opts);
}

/** Parse JSON text into a PVStructure
 *
 * Restrictions:
 *
 * - Top level must be {}  dict/object
 * - field values must be number, string, array, or dict/object
 * - array values must be number or string
 */
epicsShareFunc
PVStructure::shared_pointer parseJSON(std::istream& strm);

/** Parse JSON and store into the provided PVStructure.
 *
 * Restrictions:
 *
 * - array of union not supported
 * - Only scalar value assigned to union
 *
 * @param strm Read JSON text from stream
 * @param dest Store in fields of this structure
 * @param assigned Which fields of _dest_ were assigned. (Optional)
 * @throws std::runtime_error on failure.  dest and assigned may be modified.
 * @version Overload added after 7.0.0
 */
epicsShareFunc
void parseJSON(std::istream& strm,
               PVField& dest,
               BitSet *assigned=0);

// To be deprecated in favor of previous form
FORCE_INLINE
void parseJSON(std::istream& strm,
               const PVField::shared_pointer& dest,
               BitSet *assigned=0)
{
    parseJSON(strm, *dest, assigned);
}


/** Wrapper around yajl_parse()
 *
 * Parse entire input stream.
 * Errors if extranious non-whitespace found after the point were parsing completes.
 *
 * @param src The stream from which input charactors are read
 * @param handle A parser handle previously allocated with yajl_alloc().  Not free'd on success or failure.
 *
 * @returns true if parsing completes successfully.  false if parsing cancelled by callback.  throws other errors
 *
 * @note The form of this call depends on EPICS_YAJL_VERSION
 */
epicsShareFunc
bool yajl_parse_helper(std::istream& src,
                       yajl_handle handle);

namespace yajl {
// undef implies API version 0
#ifndef EPICS_YAJL_VERSION
typedef long integer_arg;
typedef unsigned size_arg;
#else
typedef long long integer_arg;
typedef size_t size_arg;
#endif
} // namespace epics::pvData::yajl

/** @} */

}} // namespace epics::pvData

#endif // PV_JSON_H
