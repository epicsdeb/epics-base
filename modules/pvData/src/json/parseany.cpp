/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <sstream>

#define epicsExportSharedSymbols
#include <pv/pvdVersion.h>
#include <pv/pvData.h>
#include <pv/valueBuilder.h>

#include "pv/json.h"

namespace pvd = epics::pvData;
using pvd::yajl::integer_arg;
using pvd::yajl::size_arg;

namespace {

struct context {

    unsigned depth;

    enum state_t {
        Undefined,
        Key,
        Array,
    } state;

    pvd::shared_vector<void> arr;

    pvd::ValueBuilder root,
                     *cur;

    std::string msg,
                key;

    context() :depth(0u), state(Undefined), cur(&root) {}
};

#define TRY context *self = (context*)ctx; try

#define CATCH() catch(std::exception& e) { self->msg = e.what(); return 0; }

int jtree_null(void * ctx)
{
    TRY {
        self->msg = "NULL value not permitted";
        return 0;
    }CATCH()
}

int jtree_boolean(void * ctx, int boolVal)
{
    TRY {
        if(self->depth==0) throw std::runtime_error("Bare value not supported");
        switch(self->state) {
        case context::Key:
            self->cur = &self->cur->add<pvd::pvBoolean>(self->key, boolVal);
            self->key.clear();
            self->state = context::Undefined;
            break;
        case context::Array:
        {
            if(self->arr.size()>0 && self->arr.original_type()!=pvd::pvBoolean)
                throw std::runtime_error("Mixed type array not supported");
            pvd::shared_vector<pvd::boolean> arr(pvd::static_shared_vector_cast<pvd::boolean>(self->arr));
            arr.push_back(boolVal);
            self->arr = pvd::static_shared_vector_cast<void>(arr);
            break;
        }
        default:
            throw std::logic_error("boolean in bad state");
        }
        return 1;
    }CATCH()
}

int jtree_integer(void * ctx, integer_arg integerVal)
{
    TRY {
        if(self->depth==0) throw std::runtime_error("Bare value not supported");
        switch(self->state) {
        case context::Key:
            self->cur = &self->cur->add<pvd::pvLong>(self->key, integerVal);
            self->key.clear();
            self->state = context::Undefined;
            break;
        case context::Array:
        {
            if(self->arr.size()>0 && self->arr.original_type()!=pvd::pvLong)
                throw std::runtime_error("Mixed type array not supported");
            pvd::shared_vector<pvd::int64> arr(pvd::static_shared_vector_cast<pvd::int64>(self->arr));
            arr.push_back(integerVal);
            self->arr = pvd::static_shared_vector_cast<void>(arr);
            break;
        }
        default:
            throw std::logic_error("int64 in bad state");
        }
        return 1;
    }CATCH()
}

int jtree_double(void * ctx, double doubleVal)
{
    TRY {
        if(self->depth==0) throw std::runtime_error("Bare value not supported");
        switch(self->state) {
        case context::Key:
            self->cur = &self->cur->add<pvd::pvDouble>(self->key, doubleVal);
            self->key.clear();
            self->state = context::Undefined;
            break;
        case context::Array:
        {
            if(self->arr.size()>0 && self->arr.original_type()!=pvd::pvDouble)
                throw std::runtime_error("Mixed type array not supported");
            pvd::shared_vector<double> arr(pvd::static_shared_vector_cast<double>(self->arr));
            arr.push_back(doubleVal);
            self->arr = pvd::static_shared_vector_cast<void>(arr);
            break;
        }
        default:
            throw std::logic_error("double in bad state");
        }
        return 1;
    }CATCH()
}

int jtree_string(void * ctx, const unsigned char * stringVal,
                    size_arg stringLen)
{
    TRY {
        if(self->depth==0) throw std::runtime_error("Bare value not supported");
        std::string sval((const char*)stringVal, stringLen);
        switch(self->state) {
        case context::Key:
            self->cur = &self->cur->add<pvd::pvString>(self->key, sval);
            self->key.clear();
            self->state = context::Undefined;
            break;
        case context::Array:
        {
            if(self->arr.size()>0 && self->arr.original_type()!=pvd::pvString)
                throw std::runtime_error("Mixed type array not supported");
            pvd::shared_vector<std::string> arr(pvd::static_shared_vector_cast<std::string>(self->arr));
            arr.push_back(sval);
            self->arr = pvd::static_shared_vector_cast<void>(arr);
            break;
        }
        default:
            throw std::logic_error("double in bad state");
        }
        return 1;
    }CATCH()
}

int jtree_start_map(void * ctx)
{
    TRY {
        if(self->depth>0) {
            if(self->key.empty())
                throw std::logic_error("anonymous dict not top level?");
            self->cur = &self->cur->addNested(self->key);
            self->key.clear();
        }
        self->depth++;
        return 1;
    }CATCH()
}

int jtree_map_key(void * ctx, const unsigned char * key,
                     size_arg stringLen)
{
    TRY {
        if(!self->key.empty())
            throw std::logic_error("double key?");
        if(stringLen==0)
            throw std::runtime_error("empty key not allowed");
        self->key = std::string((const char*)key, stringLen);
        self->state = context::Key;
        return 1;
    }CATCH()
}

int jtree_end_map(void * ctx)
{
    TRY {
        if(self->depth>1)
            self->cur = &self->cur->endNested();
        else if(self->depth==0)
            throw std::logic_error("Unbalenced dict");
        self->depth--;
        return 1;
    }CATCH()
}

int jtree_start_array(void * ctx)
{
    TRY {
        if(self->depth==0) throw std::runtime_error("Bare array not supported");
        if(self->state!=context::Key)
            throw std::logic_error("bare array not supported");
        self->state = context::Array;
        return 1;
    }CATCH()
}
int jtree_end_array(void * ctx)
{
    TRY {
        if(self->state!=context::Array)
            throw std::logic_error("Bad array parse");
        self->cur = &self->cur->add(self->key, pvd::freeze(self->arr));
        self->key.clear();
        self->state = context::Undefined;
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

} // namespace

namespace epics{namespace pvData{

epics::pvData::PVStructure::shared_pointer
parseJSON(std::istream& strm)
{
#ifndef EPICS_YAJL_VERSION
    yajl_parser_config conf;
    memset(&conf, 0, sizeof(conf));
    conf.allowComments = 1;
    conf.checkUTF8 = 1;
#endif

    context ctxt;

#ifndef EPICS_YAJL_VERSION
    handler handle(yajl_alloc(&jtree_cbs, &conf, NULL, &ctxt));
#else
    handler handle(yajl_alloc(&jtree_cbs, NULL, &ctxt));

    yajl_config(handle, yajl_allow_comments, 1);
#endif

    if(!yajl_parse_helper(strm, handle))
        throw std::runtime_error(ctxt.msg);

    return ctxt.cur->buildPVStructure();
}

}} // namespace epics::pvData
