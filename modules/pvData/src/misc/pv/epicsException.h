/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/*
 * epicsException.h
 *
 *  Created on: Oct 20, 2010
 *      Author: Matej Sekoranja
 */

/*
 * Throwing exceptions w/ file+line# and, when possibly, a stack trace
 *
 * THROW_EXCEPTION1( std::bad_alloc );
 *
 * THROW_EXCEPTION2( std::logic_error, "my message" );
 *
 * THROW_EXCEPTION( mySpecialException("my message", 42, "hello", ...) );
 *
 * Catching exceptions
 *
 * catch(std::logic_error& e) {
 *   fprintf(stderr, "%s happened\n", e.what());
 *   PRINT_EXCEPTION2(e, stderr);
 *   cout<<SHOW_EXCEPTION(e);
 * }
 *
 * If the exception was not thrown with the above THROW_EXCEPTION*
 * the nothing will be printed.
 */

#ifndef EPICSEXCEPTION_H_
#define EPICSEXCEPTION_H_

#include <stdexcept>
#include <string>
#include <cstdio>

#include <shareLib.h>

// Users may redefine this for a large size if desired
#ifndef EXCEPT_DEPTH
#  define EXCEPT_DEPTH 20
#endif

#if defined(__GLIBC__) || (defined(__APPLE__) && defined(__MACH__)) /* and possibly some BSDs */
#  include <execinfo.h>
#  include <cxxabi.h>
#  define EXCEPT_USE_BACKTRACE
#elif defined(_WIN32) && !defined(_MINGW) && !defined(SKIP_DBGHELP)
#  define _WINSOCKAPI_
#  include <windows.h>
#  include <dbghelp.h>
#  define EXCEPT_USE_CAPTURE
#else
#  define EXCEPT_USE_NONE
#endif

#if defined(_WIN32) && !defined(_MINGW)
#pragma warning( push )
#pragma warning(disable: 4275) // non dll-interface class used as base for dll-interface class (std::logic_error)
#pragma warning(disable: 4251) // class std::string needs to have dll-interface to be used by clients
#endif

namespace epics { namespace pvData {


/* Stores file and line number given, and when possible the call stack
 * at the point where it was constructed
 */
class epicsShareClass ExceptionMixin {
    const char *m_file;
    int m_line;
#ifndef EXCEPT_USE_NONE
    void *m_stack[EXCEPT_DEPTH];
    int m_depth; // always <= EXCEPT_DEPTH
#endif
public:
    // allow the ctor to be inlined if possible
    ExceptionMixin(const char* file, int line)
        :m_file(file)
        ,m_line(line)
#if defined(EXCEPT_USE_BACKTRACE)
    {
        m_depth=backtrace(m_stack,EXCEPT_DEPTH);
    }
#elif defined(EXCEPT_USE_CAPTURE)
    {
        m_depth=CaptureStackBackTrace(0,EXCEPT_DEPTH,m_stack,0);
    }
#else
    {}
#endif

    void print(FILE *fp=stderr) const;

    std::string show() const;
};

#ifndef THROW_EXCEPTION_COMPAT

namespace detail {
    /* Combines user exception type with Mixin
     *
     * Takes advantage of the requirement that all exception classes
     * must be copy constructable.  Of course this also requires
     * that an extra copy be constructed...
     */
    template<typename E>
    class ExceptionMixed : public E, public ExceptionMixin {
    public:
        // construct from copy of E
        ExceptionMixed(const E& self,const char* file, int line)
            :E(self), ExceptionMixin(file,line)
        {}
        // construct for E w/o arguments
        ExceptionMixed(const char* file, int line)
            :E(), ExceptionMixin(file,line)
        {}
        // construct for E one argument
        template<typename A1>
        ExceptionMixed(A1 arg1,const char* file, int line)
            :E(arg1), ExceptionMixin(file,line)
        {}
        // construct for E two arguments
        template<typename A1, typename A2>
        ExceptionMixed(A1 arg1, A2 arg2,const char* file, int line)
            :E(arg1,arg2), ExceptionMixin(file,line)
        {}
    };

    // function template to deduce E from argument
    template<typename E>
    static inline
    ExceptionMixed<E>
    makeException(const E& self,const char* file, int line)
    {
        return ExceptionMixed<E>(self,file,line);
    }

    template<typename E>
    static inline
    std::string
    showException(const E& ex)
    {
        const ExceptionMixin *mx=dynamic_cast<const ExceptionMixin*>(&ex);
        if(!mx) return std::string();
        return mx->show();
    }
}

// Throw an exception of a mixed sub-class of the type of E
// The instance E is copied and discarded
#define THROW_EXCEPTION(E) \
do { \
     throw ::epics::pvData::detail::makeException(E, __FILE__, __LINE__); \
} while(0)

// Throw an exception of a mixed sub-class of E, passing MSG as an argument
#define THROW_EXCEPTION1(TYPE) \
do { \
     throw ::epics::pvData::detail::ExceptionMixed<TYPE>(__FILE__, __LINE__); \
 }while(0)

// Throw an exception of a mixed sub-class of E, passing MSG as an argument
#define THROW_EXCEPTION2(TYPE,MSG) \
do { \
    throw ::epics::pvData::detail::ExceptionMixed<TYPE>(MSG, __FILE__, __LINE__); \
}while(0)

#define PRINT_EXCEPTION2(EI, FP) \
do { \
    ::epics::pvData::ExceptionMixin *_em_p=dynamic_cast< ::epics::pvData::ExceptionMixin*>(&EI); \
    if (_em_p) {_em_p->print(FP);} \
}while(0)

#define PRINT_EXCEPTION(EI) PRINT_EXCEPTION2(EI,stderr)

#if !defined(__GNUC__) || __GNUC__ < 4
#  define SHOW_EXCEPTION(EI) ::epics::pvData::detail::showException(EI)
#else
#  define SHOW_EXCEPTION(EI) \
    ({ ::epics::pvData::ExceptionMixin *_mx=dynamic_cast< ::epics::pvData::ExceptionMixin*>(&(EI)); \
                           _mx ? _mx->show() : std::string(); \
                       })
#endif

#else // THROW_EXCEPTION_COMPAT
/* For older compilers which have a problem with the above */

#define PRINT_EXCEPTION(EI) do{}while(0)
#define PRINT_EXCEPTION2(EI,FP) do{}while(0)
#define SHOW_EXCEPTION(EI) std::string()

#define THROW_EXCEPTION(E) do{throw (E);}while(0)
#define THROW_EXCEPTION1(E) do{throw (E)();}while(0)
#define THROW_EXCEPTION2(E,A) do{throw (E)(A);}while(0)

#endif // THROW_EXCEPTION_COMPAT
/**
 * @brief Base for pvData exceptions.
 *
 */
class epicsShareClass BaseException : public std::logic_error {
public:
    /**
     * Constructor.
     */
    explicit BaseException(const std::string& msg) : std::logic_error(msg) {}

    /**
     * Destructor.
     */
    virtual ~BaseException() throw(){};

    /**
     * 
     * Reason for excepton.
     */
    virtual const char* what() const throw();

private:
    mutable std::string base_msg;
};

#if defined(_WIN32) && !defined(_MINGW)
#pragma warning( pop )
#endif

#define THROW_BASE_EXCEPTION(msg) THROW_EXCEPTION2(::epics::pvData::BaseException, msg)

    }
}


#endif /* EPICSEXCEPTION_H_ */
