/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <stdexcept>
#include <sstream>

#define epicsExportSharedSymbols
#include <pv/pvdVersion.h>

#include "pv/json.h"

namespace {

void check_trailing(const std::string& line)
{
    size_t idx = line.find_first_not_of(" \t\n\r");
    if(idx==line.npos) return;
    // TODO: detect the end of potentially multi-line comments...
    // for now trailing comments not allowed
    throw std::runtime_error("Trailing junk");
}

} // namespace

namespace epics{namespace pvData{

bool yajl_parse_helper(std::istream& src,
                       yajl_handle handle)
{
    unsigned linenum=0;
#ifndef EPICS_YAJL_VERSION
    bool done = false;
#endif

    std::string line;
    while(std::getline(src, line)) {
        linenum++;

#ifndef EPICS_YAJL_VERSION
        if(done) {
            check_trailing(line);
            continue;
        }
#endif

        yajl_status sts = yajl_parse(handle, (const unsigned char*)line.c_str(), line.size());

        switch(sts) {
        case yajl_status_ok: {
            size_t consumed = yajl_get_bytes_consumed(handle);

            if(consumed<line.size()) {
                check_trailing(line.substr(consumed));
            }

#ifndef EPICS_YAJL_VERSION
            done = true;
#endif
            break;
        }
        case yajl_status_client_canceled:
            return false;
#ifndef EPICS_YAJL_VERSION
        case yajl_status_insufficient_data:
            // continue with next line
            break;
#endif
        case yajl_status_error:
        {
            std::ostringstream msg;
            unsigned char *raw = yajl_get_error(handle, 1, (const unsigned char*)line.c_str(), line.size());
            if(!raw) {
                msg<<"Unknown error on line "<<linenum;
            } else {
                try {
                    msg<<"Error on line "<<linenum<<" : "<<(const char*)raw;
                }catch(...){
                    yajl_free_error(handle, raw);
                    throw;
                }
                yajl_free_error(handle, raw);
            }
            throw std::runtime_error(msg.str());
        }
        }
    }

    if(!src.eof() || src.bad()) {
        std::ostringstream msg;
        msg<<"I/O error after line "<<linenum;
        throw std::runtime_error(msg.str());

#ifndef EPICS_YAJL_VERSION
    } else if(!done) {
        switch(yajl_parse_complete(handle)) {
#else
    } else {
        switch(yajl_complete_parse(handle)) {
#endif
        case yajl_status_ok:
            break;
        case yajl_status_client_canceled:
            return false;
#ifndef EPICS_YAJL_VERSION
        case yajl_status_insufficient_data:
            throw std::runtime_error("unexpected end of input");
#endif
        case yajl_status_error:
            throw std::runtime_error("Error while completing parsing");
        }
    }

    return true;
}

}} // namespace epics::pvData
