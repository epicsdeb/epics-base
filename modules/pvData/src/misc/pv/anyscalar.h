#ifndef PV_ANYSCALAR_H
#define PV_ANYSCALAR_H

#if __cplusplus>=201103L
#  include <type_traits>
#endif

#include <ostream>
#include <exception>
#include <map>

#include <epicsAssert.h>

#include <pv/templateMeta.h>
#include <pv/typeCast.h>
#include <pv/pvIntrospect.h> /* for ScalarType enum */

namespace epics{namespace pvData{
namespace detail {

// special mangling for AnyScalar ctor to map from argument type to storage type.
// allow construction from constants.
template <typename T>
struct any_storage_type { typedef T type; };
template<> struct any_storage_type<int> { typedef int32 type; };
template<> struct any_storage_type<unsigned> { typedef uint32 type; };
template<> struct any_storage_type<char*> { typedef std::string type; };
template<> struct any_storage_type<const char*> { typedef std::string type; };

#if __cplusplus>=201103L
// std::max() isn't constexpr until c++14 :(
constexpr size_t cmax(size_t A, size_t B) {
    return A>B ? A : B;
}
#endif

}// namespace detail

/** A type-safe variant union capable of holding
 *  any of the PVD scalar types (POD or string)
 *
 @code
 AnyScalar A(5);
 assert(A.type()==pvInt);
 assert(A.ref<int32>()==5);
 assert(A.as<int32>()==5);
 assert(A.as<double>()==5.0);
 assert(A.ref<double>()==5.0); // throws AnyScalar::bad_cast
 @endcode
 */
class epicsShareClass AnyScalar {
public:
    struct bad_cast : public std::exception {
#if __cplusplus>=201103L
        bad_cast() noexcept {}
        virtual ~bad_cast() noexcept {}
        virtual const char* what() const noexcept
#else
        bad_cast() throw() {}
        virtual ~bad_cast() throw() {}
        virtual const char* what() const throw()
#endif
        { return "bad_cast() type mis-match"; }
    };

private:
    ScalarType _stype;

    // always reserve enough storage for std::string or double (assumed worst case)
#if __cplusplus>=201103L

    struct wrap_t {
        typename std::aligned_storage<detail::cmax(sizeof(std::string), sizeof(double)),
                                      detail::cmax(alignof(std::string), alignof(double))
        >::type blob[1];
    } _wrap;
#else
    struct wrap_t {
        union blob_t {
            char data[sizeof(std::string)];
            double align_f; // assume std::string alignment <= 8
        } blob[1];
    } _wrap;
#endif

    // assumed largest non-string type
    typedef double _largest_blob;

    template<typename T>
    inline T& _as() {
        return *reinterpret_cast<T*>(_wrap.blob);
    }
    template<typename T>
    inline const T& _as() const {
        return *reinterpret_cast<const T*>(_wrap.blob);
    }
public:
    //! Construct empty
    //! @post empty()==true
    AnyScalar() : _stype((ScalarType)-1) {}

    //! Construct from provided value.
    template<typename T>
    explicit AnyScalar(T v)
    {
        typedef typename meta::strip_const<T>::type T2;
        typedef typename detail::any_storage_type<T2>::type TT;

        STATIC_ASSERT(sizeof(TT)<=sizeof(_wrap.blob));

        new (_wrap.blob) TT(v);

        // this line fails to compile when type T can't be mapped to one of
        // the PVD scalar types.
        _stype = (ScalarType)ScalarTypeID<TT>::value;
    }

    //! Construct from un-typed pointer.
    //! Caller is responsible to ensure that buf actually points to the provided type
    //! @version Added after 7.0.0
    AnyScalar(ScalarType type, const void *buf);

    AnyScalar(const AnyScalar& o);

#if __cplusplus>=201103L
    AnyScalar(AnyScalar&& o);
#endif

    inline ~AnyScalar() {clear();}

    inline AnyScalar& operator=(const AnyScalar& o) {
        AnyScalar(o).swap(*this);
        return *this;
    }

    template<typename T>
    inline AnyScalar& operator=(T v) {
        AnyScalar(v).swap(*this);
        return *this;
    }

#if __cplusplus>=201103L
    inline AnyScalar& operator=(AnyScalar&& o) {
        clear();
        swap(o);
        return *this;
    }
#endif

    //! Reset internal state.
    //! @version Added after 7.0.0
    //! @post empty()==true
    void clear();

    void swap(AnyScalar& o);

    //! Type code of contained value.  Or (ScalarType)-1 is empty.
    inline ScalarType type() const {
        return _stype;
    }

    inline       void* unsafe()       { return _wrap.blob; }
    inline const void* unsafe() const { return _wrap.blob; }

    inline bool empty() const { return _stype==(ScalarType)-1; }

#if __cplusplus>=201103L
    explicit operator bool() const { return !empty(); }
#else
private:
    typedef void (AnyScalar::*bool_type)(AnyScalar&);
public:
    operator bool_type() const { return !empty() ? &AnyScalar::swap : 0; }
#endif

    //! Provide read-only access to underlying buffer.
    //! For a string this is std::string::c_str().
    //! @version Added after 7.0.0
    const void* bufferUnsafe() const;

    /** Return typed reference to wrapped value.  Non-const reference allows value modification
     *
     * @throws bad_cast when the requested type does not match the stored type
     @code
       AnyScalar v(42);
       v.ref<uint32>() = 43;
       assert(v.ref<uint32>() == 43);
     @endcode
     */
    template<typename T>
    inline
    // T -> strip_const -> map to storage type -> add reference
    typename detail::any_storage_type<typename meta::strip_const<T>::type>::type&
    ref() {
        typedef typename meta::strip_const<T>::type T2;
        typedef typename detail::any_storage_type<T2>::type TT;

        if(_stype!=(ScalarType)ScalarTypeID<TT>::value)
            throw bad_cast();
        return reinterpret_cast<TT&>(_wrap.blob);
    }

    /** Return typed reference to wrapped value.  Const reference does not allow modification.
     *
     * @throws bad_cast when the requested type does not match the stored type
     @code
       const AnyScalar v(42);
       assert(v.ref<uint32>() == 42);
     @endcode
     */
    template<typename T>
    inline
    // T -> strip_const -> map to storage type -> add const reference
    typename meta::decorate_const<typename detail::any_storage_type<typename meta::strip_const<T>::type>::type>::type&
    ref() const {
        typedef typename meta::strip_const<T>::type T2;
        typedef typename detail::any_storage_type<T2>::type TT;

        if(_stype!=(ScalarType)ScalarTypeID<TT>::value)
            throw bad_cast();
        return reinterpret_cast<typename meta::decorate_const<TT>::type&>(_wrap.blob);
    }

    /** copy out wrapped value, with a value conversion.
     *
     * @throws bad_cast when empty()==true
     */
    template<typename T>
    inline
    T as() const {
        typedef typename meta::strip_const<T>::type T2;
        typedef typename detail::any_storage_type<T2>::type TT;

        if(_stype==(ScalarType)-1)
            throw bad_cast();
        TT ret;
        castUnsafeV(1, (ScalarType)ScalarTypeID<T2>::value, &ret,
                                   _stype, _wrap.blob);
        return ret;
    }

private:
    friend epicsShareFunc std::ostream& operator<<(std::ostream& strm, const AnyScalar& v);
};

epicsShareExtern
std::ostream& operator<<(std::ostream& strm, const AnyScalar& v);

}} // namespace epics::pvData

#endif // PV_ANYSCALAR_H
