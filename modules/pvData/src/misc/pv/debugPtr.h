/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Michael Davidsaver */
/* wrapper around shared_ptr which tracks backwards references.
 * Can help to find ref. leaks, loops, and other exciting bugs.
 * See comments in sharedPtr.h
 */
#ifndef DEBUGPTR_H
#define DEBUGPTR_H

#if __cplusplus<201103L
#  error c++11 required
#endif

#include <ostream>
#include <memory>
#include <set>

#include <pv/epicsException.h>

#include <shareLib.h>

//! User code should test this macro
//! before calling epics::debug::shared_ptr::show_refs()
#define HAVE_SHOW_REFS

namespace epics {
namespace debug {

struct tracker;
class shared_ptr_base;

class epicsShareClass ptr_base {
    friend class shared_ptr_base;
    template<typename A>
    friend class shared_ptr;
    template<typename A>
    friend class weak_ptr;
protected:
    typedef std::shared_ptr<tracker> track_t;
    track_t track;

    ptr_base() noexcept : track() {}
    ptr_base(const track_t& track) :track(track) {}
    ptr_base(const ptr_base&) = delete;
    ptr_base(ptr_base&&) = delete;

    ptr_base& operator=(const ptr_base&) = delete;

public:
    typedef std::set<const shared_ptr_base *> ref_set_t;
    void show_refs(std::ostream&, bool self=true, bool weak=false) const;
    void spy_refs(ref_set_t&) const;
};

class epicsShareClass weak_ptr_base : public ptr_base {
protected:
    weak_ptr_base() {}
    weak_ptr_base(const track_t& track) :ptr_base(track) {}
};

class epicsShareClass shared_ptr_base : public ptr_base {
protected:
    shared_ptr_base() noexcept
#ifndef EXCEPT_USE_NONE
        :m_stack(), m_depth(0)
#endif
    {}
    shared_ptr_base(const track_t& track) :ptr_base(track)
  #ifndef EXCEPT_USE_NONE
      ,m_stack(), m_depth(0)
  #endif
    {}
    ~shared_ptr_base() {track_clear();}

    // add ourselves to tracker
    void track_new();
    // create new tracker if ptr!=nullptr, otherwise clear
    void track_new(void* ptr);
    // copy tracker and add ourself
    void track_assign(const shared_ptr_base& o);
    void track_clear();
    void swap(shared_ptr_base& o);
    void snap_stack();

#ifndef EXCEPT_USE_NONE
    void *m_stack[EXCEPT_DEPTH];
    int m_depth; // always <= EXCEPT_DEPTH
#endif

public:
    void show_stack(std::ostream&) const;
};



template<typename T>
class shared_ptr;
template<typename T>
class weak_ptr;
template<class Base>
class enable_shared_from_this;

template<typename Store, typename Actual>
inline void
do_enable_shared_from_this(const shared_ptr<Store>& dest,
                            enable_shared_from_this<Actual>* self
                            );

template<typename T>
inline void
do_enable_shared_from_this(const shared_ptr<T>&, ...) {}

template<typename T>
class shared_ptr : public shared_ptr_base {
    typedef ::std::shared_ptr<T> real_type;

    real_type real;

    template<typename A>
    friend class shared_ptr;
    template<typename A>
    friend class weak_ptr;

    // ctor for casts
    shared_ptr(const real_type& r, const ptr_base::track_t& t)
        :shared_ptr_base(t), real(r)
    {track_new();}
public:
    typedef typename real_type::element_type element_type;
    typedef weak_ptr<T> weak_type;

    // new NULL
    shared_ptr() noexcept {}
    // copy existing same type
    shared_ptr(const shared_ptr& o) :shared_ptr_base(o.track), real(o.real) {track_new();}
    // copy existing of implicitly castable type
    template<typename A>
    shared_ptr(const shared_ptr<A>& o) :shared_ptr_base(o.track), real(o.real) {track_new();}

    // construct around new pointer
    template<typename A, class ... Args>
    explicit shared_ptr(A* a, Args ... args) : shared_ptr_base(), real(a, args...) {
        track_new(a);
        do_enable_shared_from_this(*this, a);
    }

    // make strong ref from weak
    template<typename A>
    shared_ptr(const weak_ptr<A>& o) :shared_ptr_base(o.track), real(o.real) {track_new();}

    // takeover from unique_ptr
    template<typename A>
    shared_ptr(std::unique_ptr<A>&& a) : shared_ptr_base(), real(a.release()) {track_new();}

    ~shared_ptr() {}

    shared_ptr& operator=(const shared_ptr& o) {
        if(this!=&o) {
            real = o.real;
            track_assign(o);
        }
        return *this;
    }
    template<typename A>
    shared_ptr& operator=(const shared_ptr<A>& o) {
        if(get()!=o.get()) {
            real = o.real;
            track_assign(o);
        }
        return *this;
    }

    void reset() noexcept { real.reset(); track_clear(); }
    template<typename A, class ... Args>
    void reset(A* a, Args ... args)
    {
        real.reset(a, args...);
        track_new(a);
        do_enable_shared_from_this(*this, a);
    }
    void swap(shared_ptr &o) noexcept
    {
        if(this!=&o) {
            real.swap(o.real);
            shared_ptr_base::swap(o);
        }
    }

    // proxy remaining to underlying shared_ptr

    T* get() const noexcept { return real.get(); }
    typename std::add_lvalue_reference<T>::type operator*() const noexcept { return *real; }
    T* operator->() const noexcept { return real.get(); }
    long use_count() const noexcept { return real.use_count(); }
    bool unique() const noexcept { return real.unique(); }
    explicit operator bool() const noexcept { return bool(real); }

    bool operator==(const shared_ptr<T>& o) const { return real==o.real; }
    bool operator!=(const shared_ptr<T>& o) const { return real!=o.real; }
    bool operator<(const shared_ptr<T>& o) const { return real<o.real; }

    template<typename A>
    bool owner_before(const shared_ptr<A>& o) { return real.owner_before(o); }
    template<typename A>
    bool owner_before(const weak_ptr<A>& o) { return real.owner_before(o); }

    template<typename TO, typename FROM>
    friend
    shared_ptr<TO> static_pointer_cast(const shared_ptr<FROM>& src);
    template<typename TO, typename FROM>
    friend
    shared_ptr<TO> const_pointer_cast(const shared_ptr<FROM>& src);
    template<typename TO, typename FROM>
    friend
    shared_ptr<TO> dynamic_pointer_cast(const shared_ptr<FROM>& src);
    template<typename Store, typename Actual>
    friend void
    do_enable_shared_from_this(const shared_ptr<Store>& dest,
                                enable_shared_from_this<Actual>* self
                                );
};

template<typename TO, typename FROM>
shared_ptr<TO> static_pointer_cast(const shared_ptr<FROM>& src) {
    return shared_ptr<TO>(std::static_pointer_cast<TO>(src.real), src.track);
}

template<typename TO, typename FROM>
shared_ptr<TO> const_pointer_cast(const shared_ptr<FROM>& src) {
    return shared_ptr<TO>(std::const_pointer_cast<TO>(src.real), src.track);
}

template<typename TO, typename FROM>
shared_ptr<TO> dynamic_pointer_cast(const shared_ptr<FROM>& src) {
    return shared_ptr<TO>(std::dynamic_pointer_cast<TO>(src.real), src.track);
}

template<typename T>
class weak_ptr : public weak_ptr_base {
    typedef ::std::weak_ptr<T> real_type;

    real_type real;

    template<typename A>
    friend class shared_ptr;
    template<typename A>
    friend class weak_ptr;

public:
    typedef typename real_type::element_type element_type;
    typedef weak_ptr<T> weak_type;

    // new NULL
    weak_ptr() noexcept {}
    // copy existing same type
    weak_ptr(const weak_ptr& o) :weak_ptr_base(o.track), real(o.real) {}
    // copy existing of similar type
    template<typename A>
    weak_ptr(const weak_ptr<A>& o) :weak_ptr_base(o.track), real(o.real) {}

    // create week ref from strong ref
    template<typename A>
    weak_ptr(const shared_ptr<A>& o) :weak_ptr_base(o.track), real(o.real) {}

    ~weak_ptr() {}

    weak_ptr& operator=(const weak_ptr& o) {
        if(this!=&o) {
            real = o.real;
            track = o.track;
        }
        return *this;
    }
    template<typename A>
    weak_ptr& operator=(const shared_ptr<A>& o) {
        real = o.real;
        track = o.track;
        return *this;
    }

    shared_ptr<T> lock() const noexcept { return shared_ptr<T>(real.lock(), track); }
    void reset() noexcept { track.reset(); real.reset(); }

    long use_count() const noexcept { return real.use_count(); }
    bool unique() const noexcept { return real.unique(); }
};

template<class Base>
class enable_shared_from_this {
    mutable weak_ptr<Base> xxInternalSelf;

    template<typename Store, typename Actual>
    friend
    void
    do_enable_shared_from_this(const shared_ptr<Store>& dest,
                                enable_shared_from_this<Actual>* self
                                );
public:
    shared_ptr<Base> shared_from_this() const {
        return shared_ptr<Base>(xxInternalSelf);
    }
};

template<typename Store, typename Actual>
inline void
do_enable_shared_from_this(const shared_ptr<Store>& dest,
                            enable_shared_from_this<Actual>* self
                            )
{
    shared_ptr<Actual> actual(dynamic_pointer_cast<Actual>(dest));
    if(!actual)
        throw std::logic_error("epics::debug::enabled_shared_from_this fails");
    self->xxInternalSelf = actual;
}

}} // namespace epics::debug

template<typename T>
inline std::ostream& operator<<(std::ostream& strm, const epics::debug::shared_ptr<T>& ptr)
{
    strm<<ptr.get();
    return strm;
}

#endif // DEBUGPTR_H
