/*TypeFunc.cpp*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstdio>

#define epicsExportSharedSymbols
#include <pv/pvIntrospect.h>
#include <pv/epicsException.h>
#include <pv/sharedVector.h>

#include "dbDefs.h" // for NELEMENTS

using std::string;

namespace epics { namespace pvData {

namespace TypeFunc {
    static const char* names[] = {
        "scalar", "scalarArray", "structure", "structureArray", "union", "unionArray"
    };
    const char* name(Type t) {
        if (t<int(scalar) || t>int(unionArray))
            THROW_EXCEPTION2(std::invalid_argument, "logic error unknown Type");
        return names[t];
    }
} // namespace TypeFunc

std::ostream& operator<<(std::ostream& o, const Type& type)
{
    return o << TypeFunc::name(type);
}


namespace ScalarTypeFunc {
    bool isInteger(ScalarType type) {
        if(type>=pvByte && type<=pvULong) return true;
        return false;
    }

    bool isUInteger(ScalarType type) {
        if(type>=pvUByte && type<=pvULong) return true;
        return false;
    }

    bool isNumeric(ScalarType type) {
        if(type>=pvByte && type<=pvDouble) return true;
        return false;
    }

    bool isPrimitive(ScalarType type) {
        if(type>=pvBoolean && type<=pvDouble) return true;
        return false;
    }

    static const char* names[] = {
        "boolean",
        "byte", "short", "int", "long",
        "ubyte", "ushort", "uint", "ulong",
        "float", "double", "string",
    };
    ScalarType getScalarType(const string& pvalue) {
        for(size_t i=0; i<NELEMENTS(names); i++)
            if(pvalue==names[i])
                return ScalarType(i);
        THROW_EXCEPTION2(std::invalid_argument, "error unknown ScalarType");
    }

    const char* name(ScalarType t) {
        if (t<pvBoolean || t>pvString)
            THROW_EXCEPTION2(std::invalid_argument, "error unknown ScalarType");
        return names[t];
    }

    size_t elementSize(ScalarType id)
    {
        switch(id) {
#define OP(ENUM, TYPE) case ENUM: return sizeof(TYPE)
            OP(pvBoolean, boolean);
            OP(pvUByte, uint8);
            OP(pvByte, int8);
            OP(pvUShort, uint16);
            OP(pvShort, int16);
            OP(pvUInt, uint32);
            OP(pvInt, int32);
            OP(pvULong, uint64);
            OP(pvLong, int64);
            OP(pvFloat, float);
            OP(pvDouble, double);
            OP(pvString, string);
#undef OP
        default:
            THROW_EXCEPTION2(std::invalid_argument, "error unknown ScalarType");
        }
    }

    shared_vector<void> allocArray(ScalarType id, size_t len)
    {
        switch(id) {
#define OP(ENUM, TYPE) case ENUM: return static_shared_vector_cast<void>(shared_vector<TYPE>(len))
        OP(pvBoolean, boolean);
        OP(pvUByte, uint8);
        OP(pvByte, int8);
        OP(pvUShort, uint16);
        OP(pvShort, int16);
        OP(pvUInt, uint32);
        OP(pvInt, int32);
        OP(pvULong, uint64);
        OP(pvLong, int64);
        OP(pvFloat, float);
        OP(pvDouble, double);
        OP(pvString, string);
#undef OP
        default:
            throw std::bad_alloc();
        }
    }

} // namespace ScalarTypeFunc

std::ostream& operator<<(std::ostream& o, const ScalarType& scalarType)
{
    return o << ScalarTypeFunc::name(scalarType);
}

}}
