
#include <epicsStdio.h>
#include <dbAccess.h>
#include <dbChannel.h>
#include <dbStaticLib.h>
#include <dbLock.h>
#include <dbEvent.h>
#include <epicsString.h>
#include <epicsVersion.h>

#include <pv/status.h>
#include <pv/bitSet.h>
#include <pv/pvData.h>
#include <pv/anyscalar.h>

#include "pvif.h"

namespace pvd = epics::pvData;

// note that we handle DBF_ENUM differently than in pvif.cpp
static
pvd::ScalarType DBR2PVD(short dbr)
{
    switch(dbr) {
#define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE) case DBR_##DBFTYPE: return pvd::pv##PVACODE;
#define CASE_SKIP_BOOL
#define CASE_REAL_INT64
#include "pv/typemap.h"
#undef CASE_SKIP_BOOL
#undef CASE_REAL_INT64
#undef CASE
    case DBF_ENUM: return pvd::pvUShort;
    case DBF_STRING: return pvd::pvString;
    }
    throw std::invalid_argument("Unsupported DBR code");
}

long copyPVD2DBF(const pvd::PVField::const_shared_pointer& inraw,
                 void *outbuf, short outdbf, long *outnReq)
{
    long nreq = outnReq ? *outnReq : 1;
    if(!inraw || nreq <= 0 || INVALID_DB_REQ(outdbf)) return S_db_errArg;

    pvd::ScalarType outpvd = DBR2PVD(outdbf);

    pvd::PVField::const_shared_pointer in(inraw);

    if(outdbf != DBF_STRING && in->getField()->getType() == pvd::structure) {
        // assume NTEnum.
        // index to string not requested, so attempt to treat .index as plain integer
        in = static_cast<const pvd::PVStructure*>(in.get())->getSubField("index");
        if(!in) return S_db_errArg;
    }

    if(in->getField()->getType() == pvd::structure) {
        assert(outdbf == DBF_STRING);
        char *outsbuf = (char*)outbuf;

        // maybe NTEnum
        // try index -> string
        const pvd::PVStructure* sin = static_cast<const pvd::PVStructure*>(in.get());

        pvd::PVScalar::const_shared_pointer index(sin->getSubField<pvd::PVScalar>("index"));
        if(!index) return S_db_badField; // Not NTEnum, don't know how to handle...

        // we will have an answer.
        if(outnReq)
            *outnReq = 1;

        pvd::uint16 ival = index->getAs<pvd::uint16>();


        pvd::PVStringArray::const_shared_pointer choices(sin->getSubField<pvd::PVStringArray>("choices"));

        if(choices) {
            pvd::PVStringArray::const_svector strs(choices->view());

            if(ival < strs.size()) {
                // found it!

                const std::string& sval = strs[ival];
                size_t slen = std::min(sval.size(), size_t(MAX_STRING_SIZE-1));
                memcpy(outbuf, sval.c_str(), slen);
                outsbuf[slen] = '\0';
                return 0;
            }
        }
        // didn't find it.  either no choices or index is out of range

        // print numeric index
        epicsSnprintf(outsbuf, MAX_STRING_SIZE, "%u", ival);
        return 0;

    } else if(in->getField()->getType() == pvd::scalarArray) {
        const pvd::PVScalarArray* sarr = static_cast<const pvd::PVScalarArray*>(in.get());
        pvd::shared_vector<const void> arr;
        sarr->getAs(arr);
        size_t elemsize = pvd::ScalarTypeFunc::elementSize(arr.original_type());

        arr.slice(0, nreq*elemsize);
        nreq = arr.size()/elemsize;

        if(outdbf == DBF_STRING) {
            char *outsbuf = (char*)outbuf;

            // allocate a temp buffer of string[], ick...
            pvd::shared_vector<std::string> strs(nreq); // alloc

            pvd::castUnsafeV(nreq, pvd::pvString, strs.data(), arr.original_type(), arr.data());

            for(long i =0; i<nreq; i++, outsbuf += MAX_STRING_SIZE) {
                size_t slen = std::min(strs[i].size(), size_t(MAX_STRING_SIZE-1));
                memcpy(outsbuf, strs[i].c_str(), slen);
                outsbuf[slen] = '\0';
            }

        } else {
            pvd::castUnsafeV(nreq, outpvd, outbuf, arr.original_type(), arr.data());
        }

        if(outnReq)
            *outnReq = nreq;
        return 0;

    } else if(in->getField()->getType() == pvd::scalar) {
        char *outsbuf = (char*)outbuf;
        const pvd::PVScalar* sval = static_cast<const pvd::PVScalar*>(in.get());
        pvd::AnyScalar val;
        sval->getAs(val);

        if(outdbf == DBF_STRING && val.type()==pvd::pvString) {
            // std::string to char*
            size_t len = std::min(val.as<std::string>().size(), size_t(MAX_STRING_SIZE-1));

            memcpy(outbuf, val.as<std::string>().c_str(), len);
            outsbuf[len] = '\0';

        } else if(outdbf == DBF_STRING) {
            // non-string to char*
            std::string temp;

            pvd::castUnsafeV(1, pvd::pvString, &temp, val.type(), val.unsafe());

            size_t len = std::min(temp.size(), size_t(MAX_STRING_SIZE-1));

            memcpy(outbuf, temp.c_str(), len);
            outsbuf[len] = '\0';

        } else {
            // non-string to any
            pvd::castUnsafeV(1, outpvd, outbuf, val.type(), val.unsafe());
        }

        if(outnReq)
            *outnReq = 1;
        return 0;

    } else {
        // struct array or other strangeness which I don't know how to handle
        return S_dbLib_badField;
    }
}

long copyDBF2PVD(const pvd::shared_vector<const void> &inbuf,
                 const pvd::PVField::shared_pointer& outraw,
                 pvd::BitSet& changed,
                 const pvd::PVStringArray::const_svector &choices)
{

    pvd::ScalarType inpvd = inbuf.original_type();
    size_t incnt = inbuf.size()/pvd::ScalarTypeFunc::elementSize(inpvd);

    if(!outraw) return S_db_errArg;

    pvd::PVField::shared_pointer out(outraw);

    if(inpvd != pvd::pvString && out->getField()->getType() == pvd::structure) {
        // assume NTEnum.
        // string to index not requested, so attempt to treat .index as plain integer
        out = static_cast<pvd::PVStructure*>(out.get())->getSubField("index");
        if(!out) return S_db_errArg;
    }

    if(out->getField()->getType() == pvd::structure) {
        assert(inpvd == pvd::pvString);

        if(incnt==0)
            return S_db_errArg; // Need at least one string

        const pvd::shared_vector<const std::string> insbuf(pvd::static_shared_vector_cast<const std::string>(inbuf));
        const std::string& instr(insbuf[0]);

        // assume NTEnum
        // try string to index, then parse
        pvd::PVStructure* sout = static_cast<pvd::PVStructure*>(out.get());

        pvd::PVScalar::shared_pointer index(sout->getSubField<pvd::PVScalar>("index"));
        if(!index) return S_db_badField; // Not NTEnum, don't know how to handle...

        pvd::uint16 result = pvd::uint16(-1);
        bool match = false;

        for(size_t i=0, N=std::min(size_t(0xffff), choices.size()); i<N; i++) {
            if(choices[i] == instr) {
                match = true;
                result = pvd::uint16(i);
            }
        }

        if(!match) {
            // no choice string matched, so try to parse as integer

            try{
                result = pvd::castUnsafe<pvd::uint16>(instr);
            }catch(std::exception&){
                return S_db_errArg;
            }
        }

        index->putFrom(result);

        out = index;

    } else if(out->getField()->getType() == pvd::scalarArray) {
        pvd::PVScalarArray* sarr = static_cast<pvd::PVScalarArray*>(out.get());

        sarr->putFrom(inbuf);

    } else if(out->getField()->getType() == pvd::scalar) {
        pvd::PVScalar* sval = static_cast<pvd::PVScalar*>(out.get());

        if(incnt==0) return S_db_errArg;

        pvd::AnyScalar val(inpvd, inbuf.data());

        sval->putFrom(val);

    } else {
        // struct array or other strangeness which I don't know how to handle
        return S_db_badField;
    }

    changed.set(out->getFieldOffset());
    return 0;
}
