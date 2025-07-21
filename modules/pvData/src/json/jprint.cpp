/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <vector>
#include <sstream>

#include <errlog.h>
#include <yajl_gen.h>

#define epicsExportSharedSymbols
#include <pv/pvdVersion.h>
#include <pv/pvData.h>
#include <pv/valueBuilder.h>
#include <pv/bitSet.h>
#include "pv/json.h"

namespace pvd = epics::pvData;

namespace {
using namespace pvd::yajl;

void yg(yajl_gen_status sts) {
    const char *msg = "<\?\?\?>";
    switch(sts) {
    case yajl_gen_status_ok:
    case yajl_gen_generation_complete:
        return;
#define CASE(STS) case STS: msg = #STS; break
    CASE(yajl_gen_keys_must_be_strings);
    CASE(yajl_gen_in_error_state);
    CASE(yajl_gen_no_buf);
    CASE(yajl_gen_invalid_number);
    CASE(yajl_max_depth_exceeded);
#ifdef EPICS_YAJL_VERSION
    CASE(yajl_gen_invalid_string);
#endif
#undef CASE
    }
    throw std::runtime_error(msg);
}

static
void stream_printer(void * ctx,
                    const char * str,
                    size_arg len)
{
    std::ostream *strm = (std::ostream*)ctx;
    strm->write(str, len);
}

struct args {
    yajl_gen handle;
    const pvd::JSONPrintOptions& opts;

    std::string indent;

    args(std::ostream& strm,
         const pvd::JSONPrintOptions& opts)
        :opts(opts)
        ,indent(opts.indent, ' ')
    {
#ifndef EPICS_YAJL_VERSION
        yajl_gen_config conf;
        conf.beautify = opts.multiLine;
        conf.indentString = indent.c_str();
        if(!(handle = yajl_gen_alloc2(stream_printer, NULL, NULL, &strm)))
            throw std::bad_alloc();

        if(opts.json5) {
            static bool warned;
            if(!warned) {
                warned = true;
                errlogPrintf("Warning: Ignoring request to print JSON5.  Update Base >= 7.0.6.1");
            }
        }
#else
        if(!(handle = yajl_gen_alloc(NULL)))
            throw std::bad_alloc();
        if(opts.multiLine) {
            yajl_gen_config(handle, yajl_gen_beautify, 1);
            yajl_gen_config(handle, yajl_gen_indent_string, indent.c_str());
        } else {
            yajl_gen_config(handle, yajl_gen_beautify, 0);
        }
#  if EPICS_VERSION_INT>=VERSION_INT(7,0,6,1)
        yajl_gen_config(handle, yajl_gen_json5, (int)opts.json5);
#  else
    if(opts.json5) {
        static bool warned;
        if(!warned) {
            warned = true;
            errlogPrintf("Warning: Ignoring request to print JSON5.  Update Base >= 7.0.6.1");
        }
    }
#  endif
        yajl_gen_config(handle, yajl_gen_print_callback, stream_printer, &strm);
#endif
    }
    ~args() {
        yajl_gen_free(handle);
    }
};

void yg_string(yajl_gen handle, const std::string& s) {
    yg(yajl_gen_string(handle, (const unsigned char*)s.c_str(), s.size()));
}

void show_field(args& A, const pvd::PVField* fld, const pvd::BitSet *mask);

void show_struct(args& A, const pvd::PVStructure* fld, const pvd::BitSet *mask)
{
    const pvd::StructureConstPtr& type = fld->getStructure();
    const pvd::PVFieldPtrArray& children = fld->getPVFields();

    const pvd::StringArray& names = type->getFieldNames();

    yg(yajl_gen_map_open(A.handle));

    for(size_t i=0, N=names.size(); i<N; i++)
    {
        if(mask && !mask->get(children[i]->getFieldOffset())) continue;

        yg_string(A.handle, names[i]);
        show_field(A, children[i].get(), mask);
    }

    yg(yajl_gen_map_close(A.handle));
}

void show_field(args& A, const pvd::PVField* fld, const pvd::BitSet *mask)
{
    switch(fld->getField()->getType())
    {
    case pvd::scalar:
    {
        const pvd::PVScalar *scalar=static_cast<const pvd::PVScalar*>(fld);
        switch(scalar->getScalar()->getScalarType()) {
        case pvd::pvString: yg_string(A.handle, scalar->getAs<std::string>()); break;
        case pvd::pvBoolean: yg(yajl_gen_bool(A.handle, scalar->getAs<pvd::boolean>())); break;
        case pvd::pvDouble:
        case pvd::pvFloat: yg(yajl_gen_double(A.handle, scalar->getAs<double>())); break;
        // case pvd::pvULong: // can't always be exactly represented...
        default:
            yg(yajl_gen_integer(A.handle, scalar->getAs<pvd::int64>())); break;
        }
    }
        return;
    case pvd::scalarArray:
    {
        const pvd::PVScalarArray *scalar=static_cast<const pvd::PVScalarArray*>(fld);

        pvd::shared_vector<const void> arr;
        scalar->getAs<void>(arr);

        yg(yajl_gen_array_open(A.handle));

        switch(arr.original_type()) {
        case pvd::pvString: {
            pvd::shared_vector<const std::string> sarr(pvd::shared_vector_convert<const std::string>(arr));
            for(size_t i=0, N=sarr.size(); i<N; i++) {
                yg_string(A.handle, sarr[i]);
            }
            break;
        }
        case pvd::pvBoolean: {
            pvd::shared_vector<const pvd::boolean> sarr(pvd::shared_vector_convert<const pvd::boolean>(arr));
            for(size_t i=0, N=sarr.size(); i<N; i++) {
                yg(yajl_gen_bool(A.handle, sarr[i]));
            }
            break;
        }
        case pvd::pvDouble:
        case pvd::pvFloat: {
            pvd::shared_vector<const double> sarr(pvd::shared_vector_convert<const double>(arr));
            for(size_t i=0, N=sarr.size(); i<N; i++) {
                yg(yajl_gen_double(A.handle, sarr[i]));
            }
            break;
        }
        default: {
            pvd::shared_vector<const pvd::int64> sarr(pvd::shared_vector_convert<const pvd::int64>(arr));
            for(size_t i=0, N=sarr.size(); i<N; i++) {
                yg(yajl_gen_integer(A.handle, sarr[i]));
            }
            break;
        }
        }

        yg(yajl_gen_array_close(A.handle));
    }
        return;
    case pvd::structure:
        show_struct(A, static_cast<const pvd::PVStructure*>(fld), mask);
        return;
    case pvd::structureArray:
    {
        pvd::PVStructureArray::const_svector arr(static_cast<const pvd::PVStructureArray*>(fld)->view());
        yg(yajl_gen_array_open(A.handle));

        for(size_t i=0, N=arr.size(); i<N; i++) {
            if(arr[i])
                show_struct(A, arr[i].get(), 0);
            else
                yg(yajl_gen_null(A.handle));
        }

        yg(yajl_gen_array_close(A.handle));
    }
        return;
    case pvd::union_:
    {
        const pvd::PVUnion *U=static_cast<const pvd::PVUnion*>(fld);
        const pvd::PVField::const_shared_pointer& C(U->get());

        if(!C) {
            yg(yajl_gen_null(A.handle));
        } else {
            show_field(A, C.get(), 0);
        }
    }
        return;
    case pvd::unionArray: {
        const pvd::PVUnionArray *U=static_cast<const pvd::PVUnionArray*>(fld);
        pvd::PVUnionArray::const_svector arr(U->view());

        yg(yajl_gen_array_open(A.handle));

        for(size_t i=0, N=arr.size(); i<N; i++) {
            if(arr[i])
                show_field(A, arr[i].get(), 0);
            else
                yg(yajl_gen_null(A.handle));
        }

        yg(yajl_gen_array_close(A.handle));
    }
        return;
    }
    // should not be reached
    if(A.opts.ignoreUnprintable)
        yg(yajl_gen_null(A.handle));
    else
        throw std::runtime_error("Encountered unprintable field type");
}

void expandBS(const pvd::PVStructure& top, pvd::BitSet& mask, bool parents) {
    if(mask.get(0)) { // special handling because getSubField(0) not allowed
        // wildcard
        for(size_t idx=1, N=top.getNumberFields(); idx<N; idx++) {
            mask.set(idx);
        }

    } else {
        for(pvd::int32 idx = mask.nextSetBit(0), N=top.getNumberFields(); idx>=0 && idx<N; idx=mask.nextSetBit(idx+1)) {
            pvd::PVField::const_shared_pointer fld = top.getSubFieldT(idx);

            // look forward and mark all children
            for(size_t i=idx+1, N=fld->getNextFieldOffset(); i<N; i++)
                mask.set(i);

            if(parents) {
                // look back and mark all parents
                // we've already stepped past all parents so siblings will not be automatically marked
                for(const pvd::PVStructure *parent = fld->getParent(); parent; parent = parent->getParent()) {
                    mask.set(parent->getFieldOffset());
                }
            }
        }
    }
}

} // namespace

namespace epics{namespace pvData{

JSONPrintOptions::JSONPrintOptions()
    :multiLine(true)
    ,ignoreUnprintable(true)
    ,indent(0)
    ,json5(false)
{}

void printJSON(std::ostream& strm,
               const PVStructure& val,
               const BitSet& mask,
               const JSONPrintOptions& opts)
{
    args A(strm, opts);
    pvd::BitSet emask(mask);
    expandBS(val, emask, true);
    if(!emask.get(0)) return;
    show_struct(A, &val, &emask);
}

void printJSON(std::ostream& strm,
               const PVField& val,
               const JSONPrintOptions& opts)
{
    args A(strm, opts);
    show_field(A, &val, 0);
}

}} // namespace epics::pvData
