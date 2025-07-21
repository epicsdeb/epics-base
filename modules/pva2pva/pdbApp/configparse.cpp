
#include <sstream>

#include <dbAccess.h>

#include <dbAccess.h>
#include <dbChannel.h>
#include <dbStaticLib.h>
#include <dbEvent.h>
#include <dbLock.h>

#include <pv/pvIntrospect.h>
#include <pv/pvAccess.h>
#include <pv/configuration.h>
#include <pv/json.h>

#include "pdbgroup.h"

namespace {

namespace pvd = epics::pvData;
using pvd::yajl::integer_arg;
using pvd::yajl::size_arg;

typedef std::map<std::string, pvd::AnyScalar> options_t;
typedef std::map<std::string, options_t> config_t;

struct context {
    const std::string chanprefix;
    std::string msg;
    std::string group, field, key;
    unsigned depth; // number of '{'s
    // depth 0 - invalid
    // depth 1 - top Object
    // depth 2 - Group
    // depth 3 - field

    context(const std::string& chanprefix, GroupConfig& conf)
        :chanprefix(chanprefix)
        ,depth(0u)
        ,conf(conf)
    {}

    GroupConfig& conf;

    void can_assign()
    {
        if(depth<2 || depth>3)
            throw std::runtime_error("Can't assign value in this context");
    }

    void assign(const pvd::AnyScalar& value) {
        can_assign();
        GroupConfig::Group& grp = conf.groups[group];

        if(depth==2) {
            if(field=="+atomic") {
                grp.atomic = value.as<pvd::boolean>();
                grp.atomic_set = true;

            } else if(field=="+id") {
                grp.id = value.as<std::string>();

            } else {
                conf.warning += "Unknown group option ";
                conf.warning += field;
            }
            field.clear();

        } else if(depth==3) {
            GroupConfig::Field& fld = grp.fields[field];

            if(key=="+type") {
                fld.type = value.ref<std::string>();

            } else if(key=="+channel") {
                fld.channel = chanprefix + value.ref<std::string>();

            } else if(key=="+id") {
                fld.id = value.ref<std::string>();

            } else if(key=="+trigger") {
                fld.trigger = value.ref<std::string>();

            } else if(key=="+putorder") {
                fld.putorder = value.as<pvd::int32>();

            } else {
                conf.warning += "Unknown group field option ";
                conf.warning += field;
            }
            key.clear();
        }
    }
};

#define TRY context *self = (context*)ctx; try

#define CATCH() catch(std::exception& e) { if(self->msg.empty()) self->msg = e.what(); return 0; }

int conf_null(void * ctx)
{
    TRY {
        self->assign(pvd::AnyScalar());
        return 1;
    }CATCH()
}


int conf_boolean(void * ctx, int boolVal)
{
    TRY {
        self->assign(pvd::AnyScalar(pvd::boolean(boolVal)));
        return 1;
    }CATCH()
}

int conf_integer(void * ctx, integer_arg integerVal)
{
    TRY {
        self->assign(pvd::AnyScalar(pvd::int64(integerVal)));
        return 1;
    }CATCH()
}

int conf_double(void * ctx, double doubleVal)
{
    TRY {
        self->assign(pvd::AnyScalar(doubleVal));
        return 1;
    }CATCH()
}

int conf_string(void * ctx, const unsigned char * stringVal,
                    size_arg stringLen)
{
    TRY {
        std::string val((const char*)stringVal, stringLen);
        self->assign(pvd::AnyScalar(val));
        return 1;
    }CATCH()
}

int conf_start_map(void * ctx)
{
    TRY {
        self->depth++;
        if(self->depth>3)
            throw std::runtime_error("Group field def. can't contain Object (too deep)");
        return 1;
    }CATCH()
}

int conf_map_key(void * ctx, const unsigned char * key,
                     size_arg stringLen)
{
    TRY {
        if(stringLen==0 && self->depth!=2)
            throw std::runtime_error("empty group or key name not allowed");

        std::string name((const char*)key, stringLen);

        if(self->depth==1)
            self->group.swap(name);
        else if(self->depth==2)
            self->field.swap(name);
        else if(self->depth==3)
            self->key.swap(name);
        else
            throw std::logic_error("Too deep!!");

        return 1;
    }CATCH()
}

int conf_end_map(void * ctx)
{
    TRY {
        assert(self->key.empty()); // cleared in assign()

        if(self->depth==3)
            self->key.clear();
        else if(self->depth==2)
            self->field.clear();
        else if(self->depth==1)
            self->group.clear();
        else
            throw std::logic_error("Invalid depth");
        self->depth--;

        return 1;
    }CATCH()
}

yajl_callbacks conf_cbs = {
    &conf_null,
    &conf_boolean,
    &conf_integer,
    &conf_double,
    NULL, // number
    &conf_string,
    &conf_start_map,
    &conf_map_key,
    &conf_end_map,
    NULL, // start_array,
    NULL, // end_array,
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

}// namespace

void GroupConfig::parse(const char *txt,
                        const char *recname,
                        GroupConfig& result)
{
#ifndef EPICS_YAJL_VERSION
    yajl_parser_config conf;
    memset(&conf, 0, sizeof(conf));
    conf.allowComments = 1;
    conf.checkUTF8 = 1;
#endif

    std::istringstream strm(txt);

    std::string chanprefix;
    if(recname) {
        chanprefix = recname;
        chanprefix += '.';
    }
    context ctxt(chanprefix, result);

#ifndef EPICS_YAJL_VERSION
    handler handle(yajl_alloc(&conf_cbs, &conf, NULL, &ctxt));
#else
    handler handle(yajl_alloc(&conf_cbs, NULL, &ctxt));

    yajl_config(handle, yajl_allow_comments, 1);
#endif

    if(!pvd::yajl_parse_helper(strm, handle))
        throw std::runtime_error(ctxt.msg);
}
