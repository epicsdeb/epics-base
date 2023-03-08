/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mes
 */

#include <sstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

#define epicsExportSharedSymbols
#include <pv/epicsException.h>

using std::string;

namespace epics{ namespace pvData {

void
ExceptionMixin::print(FILE *fp) const
{
    fprintf(fp, "On line %d of %s\n",m_line,m_file);

#if defined(EXCEPT_USE_BACKTRACE)
    if(m_depth>0) {
        fflush(fp); // must flush before using raw handle
        backtrace_symbols_fd(m_stack, m_depth, fileno(fp));
        fprintf(fp, "To translate run 'addr2line -e execname 0xXXXXXXX ...'\n"
                "  Note: Must be compiled with debug symbols\n");
    }
#endif
}

string
ExceptionMixin::show() const
{
    std::ostringstream out;

    out<<"On line "<<m_line<<" of "<<m_file<<"\n";

#if defined(EXCEPT_USE_BACKTRACE)
    if (m_depth>0) {

        char **symbols=backtrace_symbols(m_stack, m_depth);

        for(int i=0; i<m_depth; i++) {
            out<<symbols[i]<<"\n";
        }

        std::free(symbols);
    }

#endif
    return out.str();
}


const char*
BaseException::what() const throw()
{
    try{
        if (base_msg.size()==0) {
            const char *base=std::logic_error::what();
            string out, stack;

            const ExceptionMixin *info=dynamic_cast<const ExceptionMixin*>(this);
            if(info) {
                stack=info->show();
            }

            out.reserve(strlen(base)+1+stack.size()+1);

            out+=base;
            out+="\n";
            if(info) {
                out+=stack;
                out+="\n";
            }

            base_msg.swap(out);
        }
        return base_msg.c_str();
    } catch(std::bad_alloc&) {
        return "BaseException::what - Insufficient memory to construct message";
    } catch(...) {
        return "BaseException::what - Unknown error when constructing message";
    }
}

}}
