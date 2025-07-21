/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <limits.h>

#include <epicsVersion.h>

#include <epicsMath.h>
#include <epicsStdlib.h>
#include <epicsString.h>
#include <epicsConvert.h>

#define epicsExportSharedSymbols
#include "pv/typeCast.h"

using std::string;

// need to use "long long" when sizeof(int)==sizeof(long)
#if (ULONG_MAX == 0xfffffffful) || defined(_WIN32) || defined(__rtems__) || defined(__APPLE__)
#define NEED_LONGLONG
#endif

/* do we need long long? */
#ifdef NEED_LONGLONG
static int
epicsParseLongLong(const char *str, long long *to, int base, char **units)
{
    int c;
    char *endp;
    long long value;

    while ((c = *str) && isspace(c))
        ++str;

    errno = 0;
    value = strtoll(str, &endp, base);

    if (endp == str)
        return S_stdlib_noConversion;
    if (errno == EINVAL)    /* Not universally supported */
        return S_stdlib_badBase;
    if (errno == ERANGE)
        return S_stdlib_overflow;

    while ((c = *endp) && isspace(c))
        ++endp;
    if (c && !units)
        return S_stdlib_extraneous;

    *to = value;
    if (units)
        *units = endp;
    return 0;
}

static int
epicsParseULongLong(const char *str, unsigned long long *to, int base, char **units)
{
    int c;
    char *endp;
    unsigned long long value;

    while ((c = *str) && isspace(c))
        ++str;

    errno = 0;
    value = strtoull(str, &endp, base);

    if (endp == str)
        return S_stdlib_noConversion;
    if (errno == EINVAL)    /* Not universally supported */
        return S_stdlib_badBase;
    if (errno == ERANGE)
        return S_stdlib_overflow;

    while ((c = *endp) && isspace(c))
        ++endp;
    if (c && !units)
        return S_stdlib_extraneous;

    *to = value;
    if (units)
        *units = endp;
    return 0;
}
#endif

static
void handleParseError(int err)
{
    switch(err) {
    case 0: break;
    case S_stdlib_noConversion: throw std::runtime_error("parseToPOD: No digits to convert");
    case S_stdlib_extraneous: throw std::runtime_error("parseToPOD: Extraneous characters");
    case S_stdlib_underflow: throw std::runtime_error("parseToPOD: Too small to represent");
    case S_stdlib_overflow: throw std::runtime_error("parseToPOD: Too large to represent");
    case S_stdlib_badBase: throw std::runtime_error("parseToPOD: Number base not supported");
    default:
        throw std::runtime_error("parseToPOD: unknown error");
    }
}

namespace epics { namespace pvData { namespace detail {

void parseToPOD(const char* in, boolean *out)
{
    if(epicsStrCaseCmp(in,"true")==0)
        *out = 1;
    else if(epicsStrCaseCmp(in,"false")==0)
        *out = 0;
    else
        throw std::runtime_error("parseToPOD: string no match true/false");
}

#define INTFN(T, S) \
void parseToPOD(const char* in, T *out) { \
    epics ## S temp; \
    int err = epicsParse ## S (in, &temp, 0, NULL); \
    if(err)   handleParseError(err); \
    else      *out = temp; \
}

INTFN(int8, Int8);
INTFN(uint8, UInt8);
INTFN(int16_t, Int16);
INTFN(uint16_t, UInt16);
INTFN(int32_t, Int32);
INTFN(uint32_t, UInt32);

void parseToPOD(const char* in, int64_t *out) {
#ifdef NEED_LONGLONG
    int err = epicsParseLongLong(in, out, 0, NULL);
#else
    int err = epicsParseLong(in, out, 0, NULL);
#endif
    if(err)   handleParseError(err);
}

void parseToPOD(const char* in, uint64_t *out) {
#ifdef NEED_LONGLONG
    int err = epicsParseULongLong(in, out, 0, NULL);
#else
    int err = epicsParseULong(in, out, 0, NULL);
#endif
    if(err)   handleParseError(err);
}

void parseToPOD(const char* in, float *out) {
    int err = epicsParseFloat(in, out, NULL);
    if(err)   handleParseError(err);
}

void parseToPOD(const char* in, double *out) {
    int err = epicsParseDouble(in, out, NULL);
    if(err)   handleParseError(err);
#if defined(vxWorks)
    /* vxWorks strtod returns [-]epicsINF when it should return ERANGE error.
     * If [-]epicsINF is returned and the first char is a digit we translate
     * this into an ERANGE error
     */
    else if (*out == epicsINF || *out == -epicsINF) {
        const char* s = in;
        int c;

        /* skip spaces and the sign */
        do {
            c = *s++;
        } while (isspace(c));

        if (c == '-' || c == '+')
            c = *s++;

        if (isdigit(c))
            handleParseError(S_stdlib_overflow);
    }
#endif
}

}}}
