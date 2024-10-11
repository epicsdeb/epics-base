#ifndef WEAKMAP_H
#define WEAKMAP_H

#include <map>
#include <set>
#include <vector>
#include <stdexcept>

#include <pv/sharedPtr.h>
#include <epicsMutex.h>
#include <epicsGuard.h>

/** @brief An associative map where a weak_ptr to the value is stored.
 *
 * Acts like std::map<K, weak_ptr<V> > where entries are automatically
 * removed when no longer referenced.
 *
 * Meant to be used in situations where an object must hold some weak references
 * of entries which it can iterate.
 *
 * Note that insert() and operator[] w/ assignment replaces the reference pass in
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
     weak_map<string, Entry> M;
   };
   shared_ptr<Entry> build(const shared_ptr<Owner>& own, const std::string& k) {
     shared_ptr<Owner> N(new Entry);
     N.O = own;
     own.M[k] = N; // modifies 'N'
     return N;
   }
   void example()
   {
     shared_ptr<Owner> O(new Owner);
     shared_ptr<Entry> E(build(O, "test"));
     assert(!O.M.empty());
     assert(O.M["test"]==E);
     E.reset(); // Entry is removed from the set and free'd
     assert(O.M.empty());
   }
 @endcode
 */
template<typename K, typename V, typename C = std::less<K> >
class weak_value_map
{
public:
    typedef K key_type;
    typedef V value_type;
    typedef std::tr1::shared_ptr<V> value_pointer;
    typedef std::tr1::weak_ptr<V> value_weak_pointer;
    typedef std::set<value_pointer> set_type;

    typedef epicsMutex mutex_type;
    typedef epicsGuard<epicsMutex> guard_type;
    typedef epicsGuardRelease<epicsMutex> release_type;
private:
    typedef std::map<K, value_weak_pointer, C> store_t;

    struct data {
        mutex_type mutex;
        store_t store;
    };
    std::tr1::shared_ptr<data> _data;

    struct dtor {
        std::tr1::weak_ptr<data> container;
        K key;
        value_pointer realself;
        dtor(const std::tr1::weak_ptr<data>& d,
             const K& k,
             const value_pointer& w)
            :container(d), key(k), realself(w)
        {}
        void operator()(value_type *)
        {
            value_pointer R;
            R.swap(realself);
            std::tr1::shared_ptr<data> cont(container.lock());
            if(cont) {
                guard_type G(cont->mutex);
                cont->store.erase(key);
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
    weak_value_map() :_data(new data) {}

private:
    //! Not copyable
    weak_value_map(const weak_value_map& O);
    //! Not copyable
    weak_value_map& operator=(const weak_value_map& O);
public:

    //! exchange the two sets.
    //! @warning Not thread safe (exchanges mutexes as well)
    void swap(weak_value_map& O) {
        _data.swap(O._data);
    }

    //! Remove all (weak) entries from the set
    //! @note Thread safe
    void clear() {
        guard_type G(_data->mutex);
        return _data->store.clear();
    }

    //! Test if set is empty at this moment
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

    //! proxy class for lookup of non-const
    //! Supports assignment and deref.
    //! implicitly castable to value_pointer (aka shared_ptr<V>)
    class element_proxy {
        weak_value_map& M;
        const key_type& k;
        friend class weak_value_map;
        element_proxy(weak_value_map& m, const key_type& k)
            :M(m), k(k) {}
    public:
        ~element_proxy() {}
        //! Support: map[k] = v
        //! The value_pointer passed in will be replaced with a wrapped reference
        //! @returns the argument
        value_pointer& operator=(value_pointer& v)
        {
            if(!v.unique())
                throw std::invalid_argument("Only unique() references may be inserted");
            value_pointer chainptr(v.get(), dtor(M._data, k, v));
            M._data->store[k] = chainptr;
            v.swap(chainptr);
            return v;
        }
        //! Support: *map[k]
        inline V& operator*() const {
            return *value_pointer(*this);
        }
        //! Support: map[k]->mem
        inline V* operator->() const {
            return value_pointer(*this).get();
        }
        //! Support: value_pointer V(map[k])
        operator value_pointer() const
        {
            value_pointer ret = M.find(k);
            if(!ret)
                throw std::runtime_error("Bad key");
            return ret;
        }
        bool operator==(const value_pointer& v) const
        {
            return M.find(k)==v;
        }
        bool operator!=(const value_pointer& v) const
        {
            return !(*this==v);
        }
    };

    inline element_proxy operator[](const K& k)
    {
        return element_proxy(*this, k);
    }

    value_pointer operator[](const K& k) const
    {
        value_pointer ret = find(k);
        if(!ret)
            throw std::runtime_error("Bad key");
    }

    //! Lookup key 'k'
    //! @returns a strong reference or nullptr if 'k' is not present
    value_pointer find(const K& k) const
    {
        value_pointer ret;
        guard_type G(_data->mutex);
        typename store_t::const_iterator it(_data->store.find(k));
        if(it!=_data->store.end()) {
            // may be nullptr if we race destruction
            // as ref. count falls to zero before we can remove it
            ret = it->second.lock();
        }
        return ret;
    }

    //! Insert or replace
    //! @returns previous value of key k (may be nullptr).
    value_pointer insert(const K& k, value_pointer& v)
    {
        value_pointer ret;
        guard_type G(_data->mutex);
        typename store_t::const_iterator it = _data->store.find(k);
        if(it!=_data->store.end())
            ret = it->second.lock();
        (*this)[k] = v;
        return ret;
    }

    typedef std::map<K, value_pointer, C> lock_map_type;
    //! Return an equivalent map with strong value references
    lock_map_type lock_map() const
    {
        lock_map_type ret;
        guard_type G(_data->mutex);
        for(typename store_t::const_iterator it = _data->store.begin(),
            end = _data->store.end(); it!=end; ++it)
        {
            value_pointer P(it->second.lock);
            if(P) ret[it->first] = P;
        }
        return ret;
    }

    typedef std::vector<std::pair<K, value_pointer> > lock_vector_type;
    //! Return a vector of pairs of keys and strong value references.
    //! useful for iteration
    lock_vector_type lock_vector() const
    {
        lock_vector_type ret;
        guard_type G(_data->mutex);
        ret.reserve(_data->store.size());
        for(typename store_t::const_iterator it = _data->store.begin(),
            end = _data->store.end(); it!=end; ++it)
        {
            value_pointer P(it->second.lock());
            if(P) ret.push_back(std::make_pair(it->first, P));
        }
        return ret;
    }

    //! Access to the weak_set internal lock
    //! for use with batch operations.
    //! @warning Use caution when swap()ing while holding this lock!
    inline epicsMutex& mutex() const {
        return _data->mutex;
    }
};

#endif // WEAKMAP_H
