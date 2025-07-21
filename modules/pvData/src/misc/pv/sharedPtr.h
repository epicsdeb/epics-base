/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author Michael Davidsaver
 */

#ifndef SHAREDPTR_H
#define SHAREDPTR_H

#include <memory> /* for auto_ptr */

/** @file sharedPtr.h
 * Pulls in the std::tr1 namespace with the following names
 *
 * class shared_ptr
 * class weak_ptr
 * class bad_weak_ptr
 * function static_pointer_cast;
 * function dynamic_pointer_cast
 * function const_pointer_cast
 * function swap
 * function get_deleter
 * function enable_shared_from_this
 */

/* where should we look? (In decending order of preference)
 *
 * # manual (per source file) selection
 * # c++11 version of <memory>, then alias into tr1
 * # <tr1/memory>
 * # boost version of tr1/memory
 */

/* Debugging shared_ptr with debugPtr.h requires >= c++11
 *
 * Define DEBUG_SHARED_PTR globally to cause epics::debug::shared_ptr
 * to be injected as std::tr1::shared_ptr and the macro
 * HAVE_SHOW_REFS will be defined.
 *
 * epics::debug::shared_ptr wraps std::shared_ptr with additional
 * tracking of backwards references.
 * std::shared_ptr::use_count() gives the number of shared_ptr
 * (strong refs) to the referenced object.
 *
 * If use_count()==5 then epics::debug::shared_ptr::show_refs() will print
 * 5 lines of the format
 *
 *   # <addr>: <IP0> <IP1> ...
 *
 * Given the numberic address of each shared_ptr as well as the call stack
 * at the point where it was initialized.
 * Use the 'addr2line' utility to interpret the stack addresses.
 *
 * On linux w/ ASLR it is necessary to turn on static linking to meaningfully
 * interpret call stack addresses.
 * Append "STATIC_BUILD=YES" to configure/CONFIG_SITE
 */
//#define DEBUG_SHARED_PTR

#if defined(SHARED_FROM_MANUAL)
// define SHARED_FROM_MANUAL if from some reason it is desirable to manually select
// which shared_ptr implementation to use
#elif __cplusplus>=201103L || (defined(_MSC_VER) && (_MSC_VER>=1600)) || defined(_LIBCPP_VERSION)
// MSVC has been bad about incrementing __cplusplus, even when new features are added.  shared_ptr from MSVC 2010
// the llvm libc++ doesn't bother with tr1, and puts shared_ptr in std:: even with -std=c++98
#  define SHARED_FROM_STD

#elif defined(__GNUC__) && __GNUC__>=4 && !defined(vxWorks)
   // GCC >=4.0.0
#  define SHARED_FROM_TR1

#elif defined(_MSC_VER) && _MSC_VER==1500
// MSVC 2009  (eg. Visual C++ for Python 2.7)
// Dinkumware _CPPLIB_VER=505
// Has std::tr1::shared_ptr in <memory>
#  define SHARED_TR1_FROM_STD

#elif defined(_MSC_VER) && (_MSC_VER>1500 || defined(_HAS_TR1))
   // MSVC > 2008, or 2008 w/ SP1
#  define SHARED_FROM_TR1

#else
#  define SHARED_FROM_BOOST
#endif

// go and get it

#if defined(SHARED_FROM_MANUAL)
// no-op
#elif defined(SHARED_FROM_STD)

#include <memory>

#ifndef DEBUG_SHARED_PTR

namespace std {
    namespace tr1 {
        using ::std::shared_ptr;
        using ::std::weak_ptr;
        using ::std::static_pointer_cast;
        using ::std::dynamic_pointer_cast;
        using ::std::const_pointer_cast;
        using ::std::enable_shared_from_this;
        using ::std::bad_weak_ptr;
    }
}

#else // DEBUG_SHARED_PTR

#include "debugPtr.h"

namespace std {
    namespace tr1 {
        using ::epics::debug::shared_ptr;
        using ::epics::debug::weak_ptr;
        using ::epics::debug::static_pointer_cast;
        using ::epics::debug::dynamic_pointer_cast;
        using ::epics::debug::const_pointer_cast;
        using ::epics::debug::enable_shared_from_this;
        using ::std::bad_weak_ptr;
    }
}


#endif // DEBUG_SHARED_PTR

#elif defined(SHARED_TR1_FROM_STD)
#  include <memory>

#elif defined(SHARED_FROM_TR1)
#  include <tr1/memory>

#elif defined(SHARED_FROM_BOOST)

#if defined(__GNUC__) && __GNUC__ < 3
#define BOOST_EXCEPTION_DISABLE
#define BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#endif

#  include <boost/tr1/memory.hpp>

#else
#  error No shared_ptr selection
#endif

// cleanup

#ifdef SHARED_FROM_STD
#  undef SHARED_FROM_STD
#endif

#ifdef SHARED_FROM_TR1
#  undef SHARED_FROM_TR1
#endif

#ifdef SHARED_FROM_BOOST
#  undef SHARED_FROM_BOOST
#endif

namespace detail {
template<typename T>
struct ref_shower {
    const std::tr1::shared_ptr<T>& ptr;
    bool self, weak;
    ref_shower(const std::tr1::shared_ptr<T>& ptr, bool self, bool weak) :ptr(ptr),self(self),weak(weak) {}
};
}

/** Print a list (one per line) of shared_ptr which refer to the same object
 *
 * @param ptr Use the object pointed to by this shared_ptr
 * @param self include or omit a line for this shared_ptr
 * @param weak include a line for each weak_ptr (not implemented)
 @code
   shared_ptr<int> x;
   std::cout << show_referrers(x);
 @endcode
 */
template<typename T>
inline ::detail::ref_shower<T> show_referrers(const std::tr1::shared_ptr<T>& ptr, bool self=true, bool weak=false)
{
    return ::detail::ref_shower<T>(ptr, self, weak);
}

namespace std{
template<typename T>
inline std::ostream& operator<<(std::ostream& strm, const ::detail::ref_shower<T>& refs)
{
#ifdef HAVE_SHOW_REFS
    refs.ptr.show_refs(strm, refs.self, refs.weak);
#endif // HAVE_SHOW_REFS
    return strm;
}
}//namespace std

#define POINTER_DEFINITIONS(clazz) \
    typedef std::tr1::shared_ptr<clazz> shared_pointer; \
    typedef std::tr1::shared_ptr<const clazz> const_shared_pointer; \
    typedef std::tr1::weak_ptr<clazz> weak_pointer; \
    typedef std::tr1::weak_ptr<const clazz> const_weak_pointer

namespace epics{
/** A semi-hack to help with migration from std::auto_ptr to std::unique_ptr,
 * and avoid copious deprecation warning spam
 * which may be hiding legitimate issues.
 *
 * Provides epics::auto_ptr<T> and epics::swap()
 *
 * epics::auto_ptr<T> is std::auto_ptr<T> for c++98
 * and std::unique_ptr<T> for >= c++11.
 *
 * epics::swap() is the only supported operation.
 * copy/assignment/return are not supported
 * (use auto_ptr or unique_ptr explicitly).
 */
#if __cplusplus>=201103L
template<typename T>
using auto_ptr = std::unique_ptr<T>;
template<typename T>
static inline void swap(auto_ptr<T>& lhs, auto_ptr<T>& rhs) {
    lhs.swap(rhs);
}
#else
using std::auto_ptr;
template<typename T>
static inline void swap(auto_ptr<T>& lhs, auto_ptr<T>& rhs) {
    auto_ptr<T> temp(lhs);
    lhs = rhs;
    rhs = temp;
}
#endif
}

#endif // SHAREDPTR_H
