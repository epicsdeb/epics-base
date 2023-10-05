/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#if __cplusplus>=201103L

#include <epicsMutex.h>
#include <epicsGuard.h>

#define epicsExportSharedSymbols
#include <pv/debugPtr.h>

namespace {
typedef epicsGuard<epicsMutex> Guard;
}

namespace epics {
namespace debug {

// joins together a group of ptr_base instances
// which all have the same dtor
struct tracker {
    epicsMutex mutex;
    ptr_base::ref_set_t refs;
};

void shared_ptr_base::track_new()
{
    if(track) {
        Guard G(track->mutex);
        track->refs.insert(this);
    }
    snap_stack();
}

// create new tracker if ptr!=nullptr, otherwise clear
void shared_ptr_base::track_new(void* ptr)
{
    track_clear();
    if(ptr){
        track.reset(new tracker);
        Guard G(track->mutex);
        track->refs.insert(this);
    }
    snap_stack();
}

void shared_ptr_base::track_assign(const shared_ptr_base &o)
{
    if(track!=o.track) {
        track_clear();
        track = o.track;
        if(track) {
            Guard G(track->mutex);
            track->refs.insert(this);
        }
        snap_stack();
    }
}

void shared_ptr_base::track_clear()
{
    if(track) {
        Guard G(track->mutex);
        track->refs.erase(this);
    }
    track.reset();
#ifndef EXCEPT_USE_NONE
    m_depth = 0;
#endif
}

void shared_ptr_base::swap(shared_ptr_base &o)
{
    // we cheat a bit here to avoid lock order, and to lock only twice
    if(track) {
        Guard G(track->mutex);
        track->refs.insert(&o);
        track->refs.erase(this);
    }
    track.swap(o.track);
    if(track) {
        Guard G(track->mutex);
        track->refs.insert(this);
        track->refs.erase(&o);
    }
    //TODO: keep original somehow???
    snap_stack();
    o.snap_stack();
}

void shared_ptr_base::snap_stack()
{
    if(!track) {
#ifndef EXCEPT_USE_NONE
        m_depth = 0;
#endif
        return;
    }
#if defined(EXCEPT_USE_BACKTRACE)
    {
        m_depth=backtrace(m_stack,EXCEPT_DEPTH);
    }
#else
    {}
#endif

}

void shared_ptr_base::show_stack(std::ostream& strm) const
{
    strm<<"ptr "<<this;
#ifndef EXCEPT_USE_NONE
    if(m_depth<=0) return;
#endif
#if 0 && defined(EXCEPT_USE_BACKTRACE)
    {

        char **symbols=backtrace_symbols(m_stack, m_depth);

        strm<<": ";
        for(int i=0; i<m_depth; i++) {
            strm<<symbols[i]<<", ";
        }

        std::free(symbols);
    }
#elif !defined(EXCEPT_USE_NONE)
    {
        strm<<": ";
        for(int i=0; i<m_depth; i++) {
            strm<<std::hex<<m_stack[i]<<" ";
        }
    }
#endif

}

void ptr_base::show_refs(std::ostream& strm, bool self, bool weak) const
{
    if(!track) {
        strm<<"# No refs\n";
    } else {
        Guard G(track->mutex);
        for(auto ref : track->refs) {
            if(!self && ref==this) continue;
            strm<<'#';
            ref->show_stack(strm);
            strm<<'\n';
        }
    }
}

void ptr_base::spy_refs(ref_set_t &refs) const
{
    if(track) {
        Guard G(track->mutex);
        refs.insert(track->refs.begin(), track->refs.end());
    }
}

}} // namespace epics::debug

#endif // __cplusplus>=201103L
