#ifndef WEAKSET_H
#define WEAKSET_H

#include <set>
#include <vector>
#include <stdexcept>

#include <pv/sharedPtr.h>
#include <epicsMutex.h>
#include <epicsGuard.h>

/** @brief a std::set-ish container where entries are removed when ref. counts fall to zero
 *
 * A container of ref. counted (by shared_ptr) entries
 * where an entry may be present zero or one times in the set.
 *
 * Meant to be used in situations where an object must hold some weak references
 * of entries which it can iterate.
 *
 * Note that the insert() method replaces the reference pass to it
 * with a "wrapped" reference which removes from the set then releases the original ref.
 * The reference passed in *must* be unique() or std::invalid_argument is thrown.
 * While this can't be enforced, no weak_ptr to this object should exist.
 *
 * A reference loop will exist if the object owning the weak_set also
 * holds strong references to entries in this set.
 *
 * @note With the exception of swap() all methods are thread-safe
 *
 * @warning Use caution when storing types deriving from enabled_shared_from_this<>
 *          As the implict weak reference they contain will not be wrapped.
 @code
   struct Owner;
   struct Entry {
     shared_ptr<Owner> O;
   };
   struct Owner {
     weak_set<Entry> S;
   };
   shared_ptr<Entry> build(const shared_ptr<Owner>& own) {
     shared_ptr<Owner> N(new Entry);
     N.O = own;
     own.S.insert(N); // modifies 'N'
     return N;
   }
   void example()
   {
     shared_ptr<Owner> O(new Owner);
     shared_ptr<Entry> E(build(O));
     assert(!O.S.empty());
     E.reset(); // Entry is removed from the set and free'd
     assert(O.S.empty());
   }
 @endcode
 */
template<typename T>
class weak_set
{
public:
    typedef T value_type;
    typedef std::tr1::shared_ptr<T> value_pointer;
    typedef std::tr1::weak_ptr<T> value_weak_pointer;
    typedef std::set<value_pointer> set_type;
    typedef std::vector<value_pointer> vector_type;

    typedef epicsMutex mutex_type;
    typedef epicsGuard<epicsMutex> guard_type;
    typedef epicsGuardRelease<epicsMutex> release_type;
private:
    struct weak_less {
        bool operator()(const value_weak_pointer& lhs,
                        const value_weak_pointer& rhs) const
        {
            value_pointer LHS(lhs.lock()), RHS(rhs.lock());
            return LHS && RHS && LHS.get() < RHS.get();
        }
        bool operator()(const value_pointer& lhs,
                        const value_weak_pointer& rhs) const
        {
            value_pointer RHS(rhs.lock());
            return RHS && lhs.get() < RHS.get();
        }
        bool operator()(const value_weak_pointer& lhs,
                        const value_pointer& rhs) const
        {
            value_pointer LHS(lhs.lock());
            return LHS && LHS.get() < rhs.get();
        }
    };

    typedef std::set<value_weak_pointer, weak_less> store_t;

    struct data {
        mutex_type mutex;
        store_t store;
    };
    std::tr1::shared_ptr<data> _data;

    //! Destroyer for a chained shared_ptr
    //! which holds the unique() real strong
    //! refrence to the object
    struct dtor {
        std::tr1::weak_ptr<data> container;
        value_pointer realself;
        dtor(const std::tr1::weak_ptr<data>& d,
             const value_pointer& w)
            :container(d), realself(w)
        {}
        void operator()(value_type *)
        {
            value_pointer R;
            R.swap(realself);
            assert(R.unique());
            std::tr1::shared_ptr<data> C(container.lock());
            if(C) {
                guard_type G(C->mutex);
                C->store.erase(R);
            }

            /* A subtle gotcha may exist since this struct
             * may not be destructed until the *weak*
             * count of the enclosing shared_ptr goes
             * to zero.  Which won't happen
             * as long as we hold a weak ref to the
             * container holding a weak ref to us.
             * It is *essential* that we break this
             * "weak ref. loop" explicitly
             */
            container.reset();
        }
    };
public:
    //! Construct a new empty set
    weak_set() :_data(new data) {}

private:
    //! Not copyable
    weak_set(const weak_set& O);
    //! Not copyable
    weak_set& operator=(const weak_set& O);
public:

    //! exchange the two sets.
    //! @warning Not thread safe (exchanges mutexes as well)
    void swap(weak_set& O) {
        _data.swap(O._data);
    }

    //! Remove all (weak) entries from the set
    //! @note Thread safe
    void clear() {
        guard_type G(_data->mutex);
        return _data->store.clear();
    }

    //! Test if set is empty
    //! @note Thread safe
    //! @warning see size()
    bool empty() const {
        guard_type G(_data->mutex);
        return _data->store.empty();
    }

    //! number of entries in the set at this moment
    //! @note Thread safe
    //! @warning May be momentarily inaccurate (larger) due to dead refs.
    //!          which have not yet been removed.
    size_t size() const {
        guard_type G(_data->mutex);
        return _data->store.size();
    }

    //! Insert a new entry into the set
    //! The callers shared_ptr must be unique()
    //! and is (transparently) replaced with another
    void insert(value_pointer&);

    //! Remove any (weak) ref to this object from the set
    //! @returns the number of objects removed (0 or 1)
    size_t erase(value_pointer& v) {
        guard_type G(_data->mutex);
        return _data->store.erase(v);
    }

    //! Return a set of strong references to all entries
    //! @note that this allocates a new std::set and copies all entries
    set_type lock_set() const;

    //! Return a vector of strong references to all entries
    //! Useful for iteration
    //! @note that this allocates a new std::set and copies all entries
    vector_type lock_vector() const;

    void lock_vector(vector_type&) const;

    //! Access to the weak_set internal lock
    //! for use with batch operations.
    //! @warning Use caution when swap()ing while holding this lock!
    inline epicsMutex& mutex() const {
        return _data->mutex;
    }

    //! an iterator-ish object which also locks the set during iteration
    struct XIterator {
        weak_set& set;
        epicsGuard<epicsMutex> guard;
        typename store_t::iterator it, end;
        XIterator(weak_set& S) :set(S), guard(S.mutex()), it(S._data->store.begin()), end(S._data->store.end()) {}
        //! yield the next live entry
        value_pointer next() {
            value_pointer ret;
            while(it!=end) {
                ret = (it++)->lock();
                if(ret) break;
            }
            return ret;
        }
    private:
        XIterator(const XIterator&);
        XIterator& operator=(const XIterator&);
    };

    typedef XIterator iterator;
};

template<typename T>
void weak_set<T>::insert(value_pointer &v)
{
    if(!v.unique())
        throw std::invalid_argument("Only unique() references may be inserted");

    guard_type G(_data->mutex);
    typename store_t::const_iterator it = _data->store.find(v);
    if(it==_data->store.end()) { // new object

        // wrapped strong ref. which removes from our map
        value_pointer chainptr(v.get(), dtor(_data, v));

        _data->store.insert(chainptr);

        v.swap(chainptr); // we only keep the chained pointer
    } else {
        // already stored, no-op

        // paranoia, if already inserted then this should be a wrapped ref.
        // but not sure how to check this so update arg. with known wrapped ref.
        v = value_pointer(*it); // could throw bad_weak_ptr, but really never should
    }
}

template<typename T>
typename weak_set<T>::set_type
weak_set<T>::lock_set() const
{
    set_type ret;
    guard_type G(_data->mutex);
    for(typename store_t::const_iterator it=_data->store.begin(),
        end=_data->store.end(); it!=end; ++it)
    {
        value_pointer P(it->lock());
        if(P) ret.insert(P);
    }
    return ret;
}

template<typename T>
typename weak_set<T>::vector_type
weak_set<T>::lock_vector() const
{
    vector_type ret;
    lock_vector(ret);
    return ret;
}

template<typename T>
void weak_set<T>::lock_vector(vector_type& ret) const
{
    guard_type G(_data->mutex);
    ret.reserve(_data->store.size());
    for(typename store_t::const_iterator it=_data->store.begin(),
        end=_data->store.end(); it!=end; ++it)
    {
        value_pointer P(it->lock());
        if(P) ret.push_back(P);
    }
}

#endif // WEAKSET_H
