/* noDefaultMethods.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef NO_DEFAULT_METHODS_H
#define NO_DEFAULT_METHODS_H

#include <shareLib.h>

/** @macro EPICS_NOT_COPYABLE(CLASS)
 *  @brief Disable implicit copyable
 *
 * Prevent the default copy constructor and assignment
 * operator from being usable.
 *
 * For >= C++11 explicitly disable.  Attempts to copy/assign will
 * fail to compile.
 *
 * For C++98 make these private, and don't implement them.
 * User code will fail to compile, implementation code will fail to link.
 @code
    struct MyClass {
        EPICS_NOT_COPYABLE(MyClass)
    public:
        ...
    };
 @endcode
 *
 * @note This macro contains 'private:'.
 */
#if __cplusplus>=201103L
#  define EPICS_NOT_COPYABLE(CLASS) private: CLASS(const CLASS&) = delete; CLASS& operator=(const CLASS&) = delete;
#else
#  define EPICS_NOT_COPYABLE(CLASS) private: CLASS(const CLASS&); CLASS& operator=(const CLASS&);
#endif

namespace epics { namespace pvData {

/**
 * @brief Base class for not allowing default methods.
 *
 * Note that copy constructor a copy methods are declared private.
 *
 * @deprecated Deprecated in favor of EPICS_NOT_COPYABLE() pvDataCPP 7.0.0
 */
class NoDefaultMethods {
public:
    NoDefaultMethods() {}
private:
#if __cplusplus>=201103L
    NoDefaultMethods(const NoDefaultMethods&) = delete;
    NoDefaultMethods & operator=(const NoDefaultMethods &) = delete;
#else
    // do not implement
    NoDefaultMethods(const NoDefaultMethods&);
    NoDefaultMethods & operator=(const NoDefaultMethods &);
#endif
};

}}
#endif  /* NO_DEFAULT_METHODS_H */
