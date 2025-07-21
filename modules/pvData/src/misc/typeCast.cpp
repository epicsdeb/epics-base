/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Michael Davidsaver */
#include <algorithm>
#include <sstream>

#include <string.h>

#include <epicsConvert.h>

#define epicsExportSharedSymbols
#include "pv/typeCast.h"

using epics::pvData::castUnsafe;
using epics::pvData::ScalarType;
using epics::pvData::pvString;
using std::string;

namespace {

static void noconvert()
{
    throw std::runtime_error("castUnsafeV: Conversion not supported");
}

template<typename TO, typename FROM>
static void castVTyped(size_t count, void *draw, const void *sraw)
{
    TO *dest=(TO*)draw;
    const FROM *src=(FROM*)sraw;

    //std::transform(src, src+count, dest, castUnsafe<TO,FROM>);

    try {
        for(size_t i=0; i<count; i++) {
            dest[i] = castUnsafe<TO,FROM>(src[i]);
        }
    } catch (std::exception& ex) {
        // do not report index for scalars (or arrays with one element)
        if (count > 1)
        {
            std::ostringstream os;
            os << "failed to parse element at index " << (src - (FROM*)sraw);
            os << ": " << ex.what();
            throw std::runtime_error(os.str());
        }
        else
            throw;
    }
}

template<typename T>
static void copyV(size_t count, void *draw, const void *sraw)
{
    T *dest=(T*)draw;
    const T *src=(const T*)sraw;
    std::copy(src, src+count, dest);
}

template<int N>
static void copyMem(size_t count, void *draw, const void *sraw)
{
    memcpy(draw, sraw, count*N);
}

} // end namespace

namespace epics { namespace pvData {

void castUnsafeV(size_t count, ScalarType to, void *dest, ScalarType from, const void *src)
{
#define COPYMEM(N) copyMem<N>(count, dest, src)
#define CAST(TO, FROM) castVTyped<TO, FROM>(count, dest, src)

    switch(to) {
    case pvBoolean:
        switch(from) {
        case pvBoolean: COPYMEM(1); return;
        case pvString: CAST(boolean, std::string); return;
        default: noconvert(); return;
        }
        break;

    case pvByte:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:
        case pvUByte:   COPYMEM(1); return;
        case pvShort:   CAST(int8, int16); return;
        case pvUShort:  CAST(int8, uint16); return;
        case pvInt:     CAST(int8, int32); return;
        case pvUInt:    CAST(int8, uint32); return;
        case pvLong:    CAST(int8, int64); return;
        case pvULong:   CAST(int8, uint64); return;
        case pvFloat:   CAST(int8, float); return;
        case pvDouble:  CAST(int8, double); return;
        case pvString:  CAST(int8, std::string); return;
        }
        break;

    case pvUByte:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:
        case pvUByte:   COPYMEM(1); return;
        case pvShort:   CAST(uint8, int16); return;
        case pvUShort:  CAST(uint8, uint16); return;
        case pvInt:     CAST(uint8, int32); return;
        case pvUInt:    CAST(uint8, uint32); return;
        case pvLong:    CAST(uint8, int64); return;
        case pvULong:   CAST(uint8, uint64); return;
        case pvFloat:   CAST(uint8, float); return;
        case pvDouble:  CAST(uint8, double); return;
        case pvString:  CAST(uint8, std::string); return;
        }
        break;

    case pvShort:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(int16, int8); return;
        case pvUByte:   CAST(int16, uint8); return;
        case pvShort:
        case pvUShort:  COPYMEM(2); return;
        case pvInt:     CAST(int16, int32); return;
        case pvUInt:    CAST(int16, uint32); return;
        case pvLong:    CAST(int16, int64); return;
        case pvULong:   CAST(int16, uint64); return;
        case pvFloat:   CAST(int16, float); return;
        case pvDouble:  CAST(int16, double); return;
        case pvString:  CAST(int16, std::string); return;
        }
        break;

    case pvUShort:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(uint16, int8); return;
        case pvUByte:   CAST(uint16, uint8); return;
        case pvShort:
        case pvUShort:  COPYMEM(2); return;
        case pvInt:     CAST(uint16, int32); return;
        case pvUInt:    CAST(uint16, uint32); return;
        case pvLong:    CAST(uint16, int64); return;
        case pvULong:   CAST(uint16, uint64); return;
        case pvFloat:   CAST(uint16, float); return;
        case pvDouble:  CAST(uint16, double); return;
        case pvString:  CAST(uint16, std::string); return;
        }
        break;

    case pvInt:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(int32, int8); return;
        case pvUByte:   CAST(int32, uint8); return;
        case pvShort:   CAST(int32, int16); return;
        case pvUShort:  CAST(int32, uint16); return;
        case pvInt:
        case pvUInt:    COPYMEM(4); return;
        case pvLong:    CAST(int32, int64); return;
        case pvULong:   CAST(int32, uint64); return;
        case pvFloat:   CAST(int32, float); return;
        case pvDouble:  CAST(int32, double); return;
        case pvString:  CAST(int32, std::string); return;
        }
        break;

    case pvUInt:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(uint32, int8); return;
        case pvUByte:   CAST(uint32, uint8); return;
        case pvShort:   CAST(uint32, int16); return;
        case pvUShort:  CAST(uint32, uint16); return;
        case pvInt:
        case pvUInt:    COPYMEM(4); return;
        case pvLong:    CAST(uint32, int64); return;
        case pvULong:   CAST(uint32, uint64); return;
        case pvFloat:   CAST(uint32, float); return;
        case pvDouble:  CAST(uint32, double); return;
        case pvString:  CAST(uint32, std::string); return;
        }
        break;

    case pvLong:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(int64, int8); return;
        case pvUByte:   CAST(int64, uint8); return;
        case pvShort:   CAST(int64, int16); return;
        case pvUShort:  CAST(int64, uint16); return;
        case pvInt:     CAST(int64, int32); return;
        case pvUInt:    CAST(int64, uint32); return;
        case pvLong:
        case pvULong:   COPYMEM(8); return;
        case pvFloat:   CAST(int64, float); return;
        case pvDouble:  CAST(int64, double); return;
        case pvString:  CAST(int64, std::string); return;
        }
        break;

    case pvULong:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(uint64, int8); return;
        case pvUByte:   CAST(uint64, uint8); return;
        case pvShort:   CAST(uint64, int16); return;
        case pvUShort:  CAST(uint64, uint16); return;
        case pvInt:     CAST(uint64, int32); return;
        case pvUInt:    CAST(uint64, uint32); return;
        case pvLong:
        case pvULong:   COPYMEM(8); return;
        case pvFloat:   CAST(uint64, float); return;
        case pvDouble:  CAST(uint64, double); return;
        case pvString:  CAST(uint64, std::string); return;
        }
        break;

    case pvFloat:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(float, int8); return;
        case pvUByte:   CAST(float, uint8); return;
        case pvShort:   CAST(float, int16); return;
        case pvUShort:  CAST(float, uint16); return;
        case pvInt:     CAST(float, int32); return;
        case pvUInt:    CAST(float, uint32); return;
        case pvLong:    CAST(float, int64); return;
        case pvULong:   CAST(float, uint64); return;
        case pvFloat:   COPYMEM(4); return;
        case pvDouble:  CAST(float, double); return;
        case pvString:  CAST(float, std::string); return;
        }
        break;

    case pvDouble:
        switch(from) {
        case pvBoolean: noconvert(); return;
        case pvByte:    CAST(double, int8); return;
        case pvUByte:   CAST(double, uint8); return;
        case pvShort:   CAST(double, int16); return;
        case pvUShort:  CAST(double, uint16); return;
        case pvInt:     CAST(double, int32); return;
        case pvUInt:    CAST(double, uint32); return;
        case pvLong:    CAST(double, int64); return;
        case pvULong:   CAST(double, uint64); return;
        case pvFloat:   CAST(double, float); return;
        case pvDouble:  COPYMEM(8); return;
        case pvString:  CAST(double, std::string); return;
        }
        break;

    case pvString:
        switch(from) {
        case pvBoolean: CAST(std::string, boolean); return;
        case pvByte:    CAST(std::string, int8); return;
        case pvUByte:   CAST(std::string, uint8); return;
        case pvShort:   CAST(std::string, int16); return;
        case pvUShort:  CAST(std::string, uint16); return;
        case pvInt:     CAST(std::string, int32); return;
        case pvUInt:    CAST(std::string, uint32); return;
        case pvLong:    CAST(std::string, int64); return;
        case pvULong:   CAST(std::string, uint64); return;
        case pvFloat:   CAST(std::string, float); return;
        case pvDouble:  CAST(std::string, double); return;
        case pvString:  copyV<std::string>(count, dest, src); return;
        }
        break;
    }

    THROW_EXCEPTION2(std::logic_error, "Undefined cast");
}

}}
