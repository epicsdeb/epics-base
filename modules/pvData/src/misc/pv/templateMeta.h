/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/** C++ Template meta programming helpers
 */
#ifndef TEMPLATEMETA_H
#define TEMPLATEMETA_H

// gently nudge the compiler to inline our wrappers
// Warning: Only use this when the template body is *small*.
//          You have been warned!
#if defined(__MINGW32__)
# define FORCE_INLINE inline
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 402)
# define FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
# define FORCE_INLINE __forceinline
#else
# define FORCE_INLINE inline
#endif

namespace epics { namespace pvData {
namespace meta {

/** If needed, add the 'const' qualifier to the provided type.
 *
 * Avoids adding the const qualifier twice (aka 'const const int')
 @code
   assert(typeid(decorate_const<int>::type)==typeid(const int));
   assert(typeid(decorate_const<const int>::type)==typeid(const int));
 @endcode
 */
template<typename T> struct decorate_const { typedef const T type; };
template<typename T> struct decorate_const<const T> { typedef const T type; };

/** Remove the 'const' qualifier if present
 @code
   assert(typeid(strip_const<int>::type)==typeid(int));
   assert(typeid(strip_const<const int>::type)==typeid(int));
 @endcode
 */
template<typename T> struct strip_const { typedef T type; };
template<typename T> struct strip_const<const T> { typedef T type; };

/** test to allow specialization only when A!=B
 *
 @code
   template<typename A, typename B, class Enable = void>
   struct myTemp {...};

   // specialization when A==B
   template<typename T>
   struct myTemp<T,T> {...};

   // specialization for A is 'int',
   // enabler needed to remove ambiguity when B is 'int'.
   template<typename B>
   struct myTemp<int, B, typename meta::not_same_type<int,B>::type>
   {...};
 @endcode
 */
template<typename A, typename B, typename R = void>
struct not_same_type {typedef R type;};
template<typename A>
struct not_same_type<A,A> {};

//! Select if both A and B have the same root type (excluding const qualifier)
template<typename A, typename B, class R = void> struct same_root {};
template<typename T, class R> struct same_root<T,T,R> { typedef R type; };
template<typename T, class R> struct same_root<const T,T,R> { typedef R type; };
template<typename T, class R> struct same_root<T,const T,R> { typedef R type; };

namespace detail {
    struct _const_yes {};
    struct _const_no {};
    template<typename T> struct _has_const { typedef _const_no type; };
    template<typename T> struct _has_const<const T> { typedef _const_yes type; };

    template<typename A, typename B, class R = void>  struct _same_type {};
    template<typename T, class R> struct _same_type<T,T,R> { typedef R type; };
} // namespace detail

//! Check if both A and B are either const or non-const.
template<typename A, typename B, class R = void>
struct same_const :
        public detail::_same_type<typename detail::_has_const<A>::type,
                                  typename detail::_has_const<B>::type,
                                  R>
{};

/** test if provided type is 'void' or 'const void'
 *
 * Avoid having to explicitly specialize for both
 @code
   template<typename A, class Enable = void>
   struct myTemp {...};

   // specialization when A is 'void' or 'const void'
   template<typename A>
   struct myTemp<A, typename meta::is_void<A>::type> {...};
 @endcode
 */
template<typename T, class R = void> struct is_void {};
template<class R> struct is_void<void,R> { typedef R type; };
template<class R> struct is_void<const void,R> { typedef R type; };

//! Inverse of is_void<T>
template<typename T, class R = void> struct is_not_void { typedef R type; };
template<> struct is_not_void<void> {};
template<> struct is_not_void<const void> {};

//! Enabler to ensure that both conditions A and B are true
template<typename A, typename B, class EnableA = void, class EnableB = void, class R = void>
struct _and {};
template<typename A, typename B, class R>
struct _and<A,B, typename A::type, typename B::type, R> { typedef R type; };

/** Mangle type to best pass as an argument.
 *
 * POD types passed by value.
 * All others by const reference.
 */
template<typename T>
struct arg_type {typedef const T& type;};
#define SIMPLE_ARG_TYPE(TYPE) template<> struct arg_type<TYPE> { typedef TYPE type; };
SIMPLE_ARG_TYPE(bool)
SIMPLE_ARG_TYPE(char)
SIMPLE_ARG_TYPE(signed char)
SIMPLE_ARG_TYPE(unsigned char)
SIMPLE_ARG_TYPE(short)
SIMPLE_ARG_TYPE(unsigned short)
SIMPLE_ARG_TYPE(int)
SIMPLE_ARG_TYPE(unsigned int)
SIMPLE_ARG_TYPE(long)
SIMPLE_ARG_TYPE(unsigned long)
SIMPLE_ARG_TYPE(long long)
SIMPLE_ARG_TYPE(unsigned long long)
SIMPLE_ARG_TYPE(float)
SIMPLE_ARG_TYPE(double)
SIMPLE_ARG_TYPE(long double)
#undef SIMPLE_ARG_TYPE

}}}

#endif // TEMPLATEMETA_H
