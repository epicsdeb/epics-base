/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef REFTRACK_H
#define REFTRACK_H

/** @page pvd_reftrack RefTrack
 *
 * reftrack.h is a utility for listing, finding, and reading global atomic counters.
 * By convention used to expose object instance counters as a way of detecting (slow)
 * reference/resource leaks before they cause problems.
 *
 * cf. the IOC shell commands "refshow", "refsave", and "refdiff".
 *
 * Example usage:
 *
 * @code
 *   // my header.h
 *   struct MyClass {
 *      MyClass();
 *      ~MyClass();
 *      static size_t num_instances;
 *      ...
 *   };
 *   ...
 *   // my src.cpp
 *   size_t MyClass::num_instances;
 *   MyClass::MyClass() {
 *      REFTRACE_INCREMENT(num_instances);
 *   }
 *   MyClass::~MyClass() {
 *      REFTRACE_DECREMENT(num_instances);
 *   }
 *   // in some IOC registrar or global ctor
 *   registerRefCounter("MyClass", &MyClass::num_instances);
 * @endcode
 */

#ifdef __cplusplus

#include <map>
#include <string>
#include <ostream>

#include <stdlib.h>

#include <epicsVersion.h>
#include <epicsAtomic.h>

#define REFTRACE_INCREMENT(counter) ::epics::atomic::increment(counter)
#define REFTRACE_DECREMENT(counter) ::epics::atomic::decrement(counter)

#include <shareLib.h>

namespace epics {

//! Register new global reference counter
epicsShareFunc
void registerRefCounter(const char *name, const size_t* counter);

//! Remove registration of global reference counter (if dynamically allocated)
epicsShareFunc
void unregisterRefCounter(const char *name, const size_t* counter);

//! Fetch current value of single reference counter
epicsShareFunc
size_t readRefCounter(const char *name);

//! Represent a snapshot of many reference counters
class epicsShareClass RefSnapshot
{
public:
    //! A single count
    struct Count {
        size_t current;
        long delta; //!< current - previous
        Count() :current(0u), delta(0) {}
        explicit Count(size_t c, long d) :current(c), delta(d) {}
        bool operator==(const Count& o) const
        { return current==o.current && delta==o.delta; }
    };

private:
    typedef std::map<std::string, Count> cnt_map_t;
    cnt_map_t counts;
public:
    typedef cnt_map_t::const_iterator iterator;
    typedef cnt_map_t::const_iterator const_iterator;

    /** Fetch values of all reference counters.
     *
     * This involves many atomic reads, not a single operation.
     */
    void update();

    const Count& operator[](const std::string& name) const;

    iterator begin() const { return counts.begin(); }
    iterator end() const { return counts.end(); }
    size_t size() const { return counts.size(); }

    inline void swap(RefSnapshot& o)
    {
        counts.swap(o.counts);
    }

    /** Compute the difference lhs - rhs
     *
     * Returned RefSnapshot has Count::current=lhs.current
     * and Count::delta= lhs.current - rhs.current
     */
    RefSnapshot operator-(const RefSnapshot& rhs) const;
};

//! Print all counters with a non-zero delta
epicsShareFunc
std::ostream& operator<<(std::ostream& strm, const RefSnapshot& snap);

//! Helper to run a thread which periodically prints (via show() )
//! global reference counter deltas.
class epicsShareClass RefMonitor
{
    struct Impl;
    Impl *impl;
public:
    RefMonitor();
    virtual ~RefMonitor();

    void start(double period=10.0);
    void stop();
    bool running() const;

    //! call show() with current snapshot
    void current();
protected:
    //! Default prints to stderr
    //! @param complete when false show only non-zero delta, when true show non-zero count or delta
    virtual void show(const RefSnapshot& snap, bool complete=false);
};

} // namespace epics

extern "C" {
#endif /* __cplusplus */

/** Fetch and print current snapshot
 * @return NULL or a char* which must be free()'d
 */
char* epicsRefSnapshotCurrent();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REFTRACK_H
