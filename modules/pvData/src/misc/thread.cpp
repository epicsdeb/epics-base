/* thread.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <epicsThread.h>
#define epicsExportSharedSymbols
#include <pv/thread.h>
#include <pv/reftrack.h>

namespace epics { namespace pvData {

namespace detail {
struct FuncRunner : public epicsThreadRunable
{
    typedef void (*fn_t)(void*);
    fn_t fn;
    void *arg;
    FuncRunner(fn_t f, void *a) :fn(f), arg(a) {}
    virtual ~FuncRunner(){}
    virtual void run()
    {
        (*fn)(arg);
    }
};
#if __cplusplus>=201103L
struct BindRunner : public epicsThreadRunable
{
    typedef std::function<void()> fn_t;
    fn_t fn;
    BindRunner(fn_t&& f) : fn(std::move(f)) {}
    virtual ~BindRunner() {}
    virtual void run()
    {
        fn();
    }
};
#endif
} // detail


Runnable& Thread::Config::x_getrunner()
{
    if(!this->p_runner)
        throw std::logic_error("Thread::Config missing run()");
    return *this->p_runner;
}

void Thread::Config::x_setdefault()
{
    this->p_prio = epicsThreadPriorityLow;
    this->p_autostart = true;
    this->p_runner = NULL;
    (*this).stack(epicsThreadStackBig);
}

size_t Thread::num_instances;

Thread::Config::Config() {this->x_setdefault();}

Thread::Config::Config(Runnable *r) {this->x_setdefault();this->run(r);}

Thread::Config::Config(void(*fn)(void*), void *ptr) {this->x_setdefault();this->run(fn, ptr);}

#if __cplusplus>=201103L
Thread::Config::Config(std::function<void()>&& fn) {this->x_setdefault();this->run(std::move(fn));}
#endif

Thread::Config& Thread::Config::name(const std::string& n)
{ this->p_strm.str(n); return *this; }

Thread::Config& Thread::Config::prio(unsigned int p)
{ this->p_prio = p; return *this; }

Thread::Config& Thread::Config::stack(epicsThreadStackSizeClass s)
{ this->p_stack = epicsThreadGetStackSize(s); return *this; }

Thread::Config& Thread::Config::autostart(bool a)
{ this->p_autostart = a; return *this; }

Thread::Config& Thread::Config::run(Runnable* r)
{ this->p_runner = r; return *this; }

Thread::Config& Thread::Config::run(void(*fn)(void*), void *ptr)
{
    this->p_owned_runner.reset(new detail::FuncRunner(fn, ptr));
    this->p_runner = this->p_owned_runner.get();
    return *this;
}

#if __cplusplus>=201103L
Thread::Config& Thread::Config::run(std::function<void()> &&fn)
{
    this->p_owned_runner.reset(new detail::BindRunner(std::move(fn)));
    this->p_runner = this->p_owned_runner.get();
    return *this;
}
#endif

Thread::Thread(std::string name,
               ThreadPriority priority,
               Runnable *runnable,
               epicsThreadStackSizeClass stkcls)
    :epicsThread(*runnable,
                 name.c_str(),
                 epicsThreadGetStackSize(stkcls),
                 priority)
{
    REFTRACE_INCREMENT(num_instances);
    this->start();
}

Thread::Thread(Runnable &runnable,
       std::string name,
       unsigned int stksize,
       unsigned int priority)
    :epicsThread(runnable,
                 name.c_str(),
                 stksize,
                 priority)
{
    REFTRACE_INCREMENT(num_instances);
    this->start();
}

Thread::Thread(Config& c)
    :epicsThread(c.x_getrunner(), c.p_strm.str().c_str(),
                 c.p_stack, c.p_prio)
{
    REFTRACE_INCREMENT(num_instances);
#if __cplusplus>=201103L
    p_owned = std::move(c.p_owned_runner);
#else
    p_owned = c.p_owned_runner;
#endif
    if(c.p_autostart)
        this->start();
}

Thread::~Thread()
{
    this->exitWait();
    REFTRACE_DECREMENT(num_instances);
}

}} // epics::pvData
