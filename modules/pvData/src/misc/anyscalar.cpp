
#include <epicsAssert.h>

#define epicsExportSharedSymbols
#include <shareLib.h>

#include "pv/anyscalar.h"

namespace epics {namespace pvData {

AnyScalar::AnyScalar(ScalarType type, const void *buf)
{
    if(type==pvString) {
        new (_wrap.blob) std::string(*static_cast<const std::string*>(buf));

    } else {
        memcpy(_wrap.blob, buf, ScalarTypeFunc::elementSize(type));
    }

    _stype = type;
}

AnyScalar::AnyScalar(const AnyScalar& o)
    :_stype(o._stype)
{
    if(o._stype==pvString) {
        new (_wrap.blob) std::string(o._as<std::string>());
    } else if(o._stype!=(ScalarType)-1) {
        memcpy(_wrap.blob, o._wrap.blob, sizeof(_largest_blob));
    }
}

#if __cplusplus>=201103L
AnyScalar::AnyScalar(AnyScalar&& o)
    :_stype(o._stype)
{
    typedef std::string string;
    if(o._stype==pvString) {
        new (_wrap.blob) std::string();
        _as<std::string>() = std::move(o._as<std::string>());
        o._as<string>().~string();
    } else if(o._stype!=(ScalarType)-1) {
        memcpy(_wrap.blob, o._wrap.blob, sizeof(_largest_blob));
    }
    o._stype = (ScalarType)-1;
}
#endif

void AnyScalar::clear() {
    if(_stype==pvString) {
        typedef std::string string;
        _as<string>().~string();
    }
    // other types need no cleanup
    _stype = (ScalarType)-1;
}

void AnyScalar::swap(AnyScalar& o) {
    typedef std::string string;
    switch((int)_stype) {
    case -1:
        switch((int)o._stype) {
        case -1:
            // nil <-> nil
            break;
        case pvString:
            // nil <-> string
            new (_wrap.blob) std::string();
            _as<std::string>().swap(o._as<std::string>());
            o._as<std::string>().~string();
            break;
        default:
            // nil <-> non-string
            memcpy(_wrap.blob, o._wrap.blob, sizeof(_largest_blob));
            break;
        }
        break;
    case pvString:
        switch((int)o._stype) {
        case -1:
            // string <-> nil
            new (o._wrap.blob) std::string();
            _as<std::string>().swap(o._as<std::string>());
            _as<std::string>().~string();
            break;
        case pvString:
            // string <-> string
            _as<std::string>().swap(o._as<std::string>());
            break;
        default: {
            // string <-> non-string
            std::string temp;
            temp.swap(_as<std::string>());

            _as<std::string>().~string();

            memcpy(_wrap.blob, o._wrap.blob, sizeof(_largest_blob));

            new (o._wrap.blob) std::string();
            temp.swap(o._as<std::string>());
        }
            break;
        }
        break;
    default:
        switch((int)o._stype) {
        case -1:
            // non-string <-> nil
            memcpy(o._wrap.blob, _wrap.blob, sizeof(_largest_blob));
            break;
        case pvString: {
            // non-string <-> string
            std::string temp;
            temp.swap(o._as<std::string>());

            o._as<std::string>().~string();

            memcpy(o._wrap.blob, _wrap.blob, sizeof(_largest_blob));

            new (_wrap.blob) std::string();
            temp.swap(_as<std::string>());
        }
            break;
        default:
            // non-string <-> non-string
            _largest_blob temp;
            memcpy(&temp, _wrap.blob, sizeof(_largest_blob));
            memcpy(_wrap.blob, o._wrap.blob, sizeof(_largest_blob));
            memcpy(o._wrap.blob, &temp, sizeof(_largest_blob));
            // std::swap(o._wrap.blob, _wrap.blob); // gcc <=4.3 doesn't like this
            break;
        }
        break;
    }
    std::swap(_stype, o._stype);
}
const void* AnyScalar::bufferUnsafe() const {
    if(_stype==pvString) {
        return as<std::string>().c_str();
    } else {
        return _wrap.blob;
    }
}

std::ostream& operator<<(std::ostream& strm, const AnyScalar& v)
{
    switch(v.type()) {
#define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE) case pv ## PVACODE: strm<<v._as<PVATYPE>(); break;
#define CASE_REAL_INT64
#define CASE_STRING
#include "pv/typemap.h"
#undef CASE
#undef CASE_REAL_INT64
#undef CASE_STRING
    default:
        strm<<"(nil)"; break;
    }
    return strm;
}

}} // namespace epics::pvData
