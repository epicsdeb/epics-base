/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <vector>
#include <sstream>

#define epicsExportSharedSymbols
#include <pv/pvdVersion.h>
#include <pv/pvData.h>
#include <pv/valueBuilder.h>
#include <pv/bitSet.h>
#include "pv/json.h"

namespace pvd = epics::pvData;
using pvd::yajl::integer_arg;
using pvd::yajl::size_arg;

namespace {
struct context {

    std::string msg;

    struct frame {
        pvd::PVFieldPtr fld;
        pvd::BitSet *assigned;
        frame(const pvd::PVFieldPtr& fld, pvd::BitSet *assigned)
            :fld(fld), assigned(assigned)
        {}
    };

    typedef std::vector<frame> stack_t;
    stack_t stack;

    context(const pvd::PVFieldPtr& root, pvd::BitSet *assigned)
    {
        stack.push_back(frame(root, assigned));
    }
};

#define TRY context *self = (context*)ctx; assert(!self->stack.empty()); try

#define CATCH() catch(std::exception& e) { if(self->msg.empty()) self->msg = e.what(); return 0; }

int jtree_null(void * ctx)
{
    TRY {
        self->msg = "NULL value not permitted";
        return 0;
    }CATCH()
}

template<typename PVScalarT, typename PVArrayT>
void valueAssign(context *self, typename PVScalarT::value_type val)
{
    assert(!self->stack.empty());
    context::frame& back = self->stack.back();
    pvd::Type type(back.fld->getField()->getType());
    if(type==pvd::scalar) {
        pvd::PVScalar* fld(static_cast<pvd::PVScalar*>(back.fld.get()));

        fld->putFrom(val);
        if(back.assigned)
            back.assigned->set(fld->getFieldOffset());
        self->stack.pop_back();
        // structure at the top of the stack

    } else if(type==pvd::scalarArray) {
        pvd::PVScalarArray *fld(static_cast<pvd::PVScalarArray*>(back.fld.get()));

        pvd::shared_vector<const void> carr;
        fld->getAs(carr);

        switch(carr.original_type())
        {
#define CASE_STRING
#define CASE_REAL_INT64
#define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE) case epics::pvData::pv##PVACODE: { \
            pvd::shared_vector<const PVATYPE> arr(pvd::static_shared_vector_cast<const PVATYPE>(carr)); \
            pvd::shared_vector<PVATYPE> tarr(pvd::thaw(arr)); \
            tarr.push_back(pvd::castUnsafe<PVATYPE>(val)); \
            carr = pvd::static_shared_vector_cast<const void>(pvd::freeze(tarr)); \
            } break;
#include <pv/typemap.h>
#undef CASE
#undef CASE_REAL_INT64
#undef CASE_STRING
        }

        fld->putFrom(carr);

        // leave array field at top of stack

    } else if(type==pvd::union_) {
        pvd::PVUnion* fld(static_cast<pvd::PVUnion*>(back.fld.get()));
        pvd::UnionConstPtr utype(fld->getUnion());

        if(utype->isVariant()) {
            typename PVScalarT::shared_pointer elem(pvd::getPVDataCreate()->createPVScalar<PVScalarT>());

            elem->put(val);

            fld->set(elem);

        } else {
            // attempt automagic assignment

            const pvd::StringArray& names = utype->getFieldNames();
            const pvd::FieldConstPtrArray types = utype->getFields();
            assert(names.size()==types.size());

            bool assigned = false;
            for(size_t i=0, N=names.size(); i<N; i++) {
                if(types[i]->getType()!=pvd::scalar) continue;

                pvd::PVScalarPtr ufld(fld->select<pvd::PVScalar>(i));
                try{
                    ufld->putFrom(val);
                    assigned = true;
                }catch(std::runtime_error&){
                    if(i==N-1)
                        throw;
                    continue;
                }

                break;
            }
            if(!assigned)
                throw std::runtime_error("Unable to select union member");
        }
        if(back.assigned)
            back.assigned->set(fld->getFieldOffset());
        self->stack.pop_back();
        // structure back at the top of the stack

    } else {
        throw std::invalid_argument("Can't assign value");
    }
}

int jtree_boolean(void * ctx, int boolVal)
{
    TRY {
        valueAssign<pvd::PVBoolean, pvd::PVBooleanArray>(self, !!boolVal);
        return 1;
    }CATCH()
}

int jtree_integer(void * ctx, integer_arg integerVal)
{
    TRY {
        valueAssign<pvd::PVLong, pvd::PVLongArray>(self, integerVal);
        return 1;
    }CATCH()
}

int jtree_double(void * ctx, double doubleVal)
{
    TRY {
        valueAssign<pvd::PVDouble, pvd::PVDoubleArray>(self, doubleVal);
        return 1;
    }CATCH()
}

int jtree_string(void * ctx, const unsigned char * stringVal,
                    size_arg stringLen)
{
    TRY {
        std::string val((const char*)stringVal, stringLen);
        valueAssign<pvd::PVString, pvd::PVStringArray>(self, val);
        return 1;
    }CATCH()
}

int jtree_start_map(void * ctx)
{
    TRY {
        assert(!self->stack.empty());

        context::frame& back = self->stack.back();
        pvd::Type type = back.fld->getField()->getType();
        if(type==pvd::structure) {
            // will fill in
        } else if(type==pvd::structureArray) {
            // starting new element in structure array
            pvd::PVStructureArray* sarr(static_cast<pvd::PVStructureArray*>(back.fld.get()));

            pvd::PVStructurePtr elem(pvd::getPVDataCreate()->createPVStructure(sarr->getStructureArray()->getStructure()));

            self->stack.push_back(context::frame(elem, 0));
        } else {
            throw std::runtime_error("Can't map (sub)structure");
        }

        assert(self->stack.back().fld->getField()->getType()==pvd::structure);
        return 1;
    }CATCH()
}

int jtree_map_key(void * ctx, const unsigned char * key,
                     size_arg stringLen)
{
    TRY {
        assert(!self->stack.empty());
        std::string name((const char*)key, stringLen);

        // start_map() ensures we have a structure at the top of the stack
        pvd::PVStructure *fld = static_cast<pvd::PVStructure*>(self->stack.back().fld.get());

        try {
            self->stack.push_back(context::frame(fld->getSubFieldT(name), self->stack.back().assigned));
        }catch(std::runtime_error& e){
            std::ostringstream strm;
            strm<<"At "<<fld->getFullName()<<" : "<<e.what()<<"\n";
            throw std::runtime_error(strm.str());
        }

        return 1;
    }CATCH()
}

int jtree_end_map(void * ctx)
{
    TRY {
        assert(!self->stack.empty());
        assert(self->stack.back().fld->getField()->getType()==pvd::structure);

        context::frame elem(self->stack.back());
        self->stack.pop_back();

        if(!self->stack.empty() && self->stack.back().fld->getField()->getType()==pvd::structureArray) {
            // append element to struct array
            pvd::PVStructureArray *sarr = static_cast<pvd::PVStructureArray*>(self->stack.back().fld.get());

            pvd::PVStructureArray::const_svector cval;
            sarr->swap(cval);

            pvd::PVStructureArray::svector val(pvd::thaw(cval));

            val.push_back(std::tr1::static_pointer_cast<pvd::PVStructure>(elem.fld));

            sarr->replace(pvd::freeze(val));
        }

        return 1;
    }CATCH()
}

int jtree_start_array(void * ctx)
{
    TRY {
        assert(!self->stack.empty());
        pvd::PVFieldPtr& back(self->stack.back().fld);
        pvd::Type type = back->getField()->getType();
        if(type!=pvd::structureArray && type!=pvd::scalarArray)
            throw std::runtime_error("Can't assign array");

        return 1;
    }CATCH()
}
int jtree_end_array(void * ctx)
{
    TRY {
        assert(!self->stack.empty());

        if(self->stack.back().assigned)
            self->stack.back().assigned->set(self->stack.back().fld->getFieldOffset());
        self->stack.pop_back();
        return 1;
    }CATCH()
}


yajl_callbacks jtree_cbs = {
    &jtree_null,
    &jtree_boolean,
    &jtree_integer,
    &jtree_double,
    NULL, // number
    &jtree_string,
    &jtree_start_map,
    &jtree_map_key,
    &jtree_end_map,
    &jtree_start_array,
    &jtree_end_array,
};

struct handler {
    yajl_handle handle;
    handler(yajl_handle handle) :handle(handle)
    {
        if(!handle)
            throw std::runtime_error("Failed to allocate yajl handle");
    }
    ~handler() {
        yajl_free(handle);
    }
    operator yajl_handle() { return handle; }
};

struct noop {
    void operator()(pvd::PVField*) {}
};

} // namespace

namespace epics{namespace pvData{

epicsShareFunc
void parseJSON(std::istream& strm,
               PVField& dest,
               BitSet *assigned)
{
#ifndef EPICS_YAJL_VERSION
    yajl_parser_config conf;
    memset(&conf, 0, sizeof(conf));
    conf.allowComments = 1;
    conf.checkUTF8 = 1;
#endif

    // we won't create refs to 'dest' which presist beyond this call.
    // however, it is convienent to treat 'dest' in the same manner as
    // any union/structureArray memebers it may contain.
    PVFieldPtr fakedest(&dest, noop());

    context ctxt(fakedest, assigned);

#ifndef EPICS_YAJL_VERSION
    handler handle(yajl_alloc(&jtree_cbs, &conf, NULL, &ctxt));
#else
    handler handle(yajl_alloc(&jtree_cbs, NULL, &ctxt));

    yajl_config(handle, yajl_allow_comments, 1);
#endif


    if(!yajl_parse_helper(strm, handle))
        throw std::runtime_error(ctxt.msg);

    if(!ctxt.stack.empty())
        throw std::logic_error("field stack not empty");
    assert(fakedest.use_count()==1);
}

}} // namespace epics::pvData
