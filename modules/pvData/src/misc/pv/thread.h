/* thread.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef THREAD_H
#define THREAD_H

#include <memory>
#include <sstream>
#include <stdexcept>

#if __cplusplus>=201103L
#include <functional>
#endif

#include <epicsThread.h>
#include <shareLib.h>

#include <pv/noDefaultMethods.h>
#include <pv/pvType.h>


namespace epics { namespace pvData {

enum ThreadPriority {
    lowestPriority  =epicsThreadPriorityLow,
    lowerPriority   =epicsThreadPriorityLow + 15,
    lowPriority     =epicsThreadPriorityMedium - 15,
    middlePriority  =epicsThreadPriorityMedium,
    highPriority    =epicsThreadPriorityMedium + 15,
    higherPriority  =epicsThreadPriorityHigh - 15,
    highestPriority =epicsThreadPriorityHigh
};

class Thread;
typedef std::tr1::shared_ptr<Thread> ThreadPtr;
typedef std::tr1::shared_ptr<epicsThread> EpicsThreadPtr;

typedef epicsThreadRunable Runnable;

namespace detail {
template<typename C>
struct MethRunner : public epicsThreadRunable
{
    typedef void(C::*fn_t)();
    fn_t fn;
    C* inst;
    MethRunner(C* i, fn_t f) :fn(f), inst(i) {}
    virtual ~MethRunner() {}
    virtual void run()
    {
        (inst->*fn)();
    }
};
} // namespace detail

/**
 * @brief C++ wrapper for epicsThread from EPICS base.
 *
 */
class epicsShareClass Thread : public epicsThread {
    EPICS_NOT_COPYABLE(Thread)
public:
    /** @brief Holds all the configuration necessary to launch a @class Thread
     *
     * The defaults may be used except for the runnable, which must be given
     * either in the constructor, or the @method run() method.
     *
     * @note Instances of @class Config may not be reused.
     *
     * Defaults:
     *  name: ""
     *  priority: epicsThreadPriorityLow (aka epics::pvData::lowestPriority)
     *  stack size: epicsThreadStackSmall
     *  auto start: true
     *  runner: nil (must be set explictly)
     *
     @code
        stuct bar { void meth(); ... } X;
        // with a static thread name
        Thread foo(Thread::Config(&X, &bar::meth)
                    .name("example")
                    .prio(epicsThreadPriorityHigh));

        // with a constructed thread name
        Thread foo(Thread::Config(&X, &bar::meth)
                    .prio(epicsThreadPriorityHigh)
                    <<"example"<<1);
     @endcode
     */
    class epicsShareClass Config
    {
        unsigned int p_prio, p_stack;
        std::ostringstream p_strm;
        bool p_autostart;
        Runnable *p_runner;
        typedef epics::auto_ptr<Runnable> p_owned_runner_t;
        p_owned_runner_t p_owned_runner;
        friend class Thread;
        Runnable& x_getrunner();
        void x_setdefault();

    public:
        Config();
        Config(Runnable *r);
        Config(void(*fn)(void*), void *ptr);
        template<typename C>
        Config(C* inst, void(C::*meth)()) {this->x_setdefault();this->run(inst, meth);}
#if __cplusplus>=201103L
        Config(std::function<void()>&& fn);
#endif

        Config& name(const std::string& n);
        Config& prio(unsigned int p);
        Config& stack(epicsThreadStackSizeClass s);
        Config& autostart(bool a);

        //! Thread will execute Runnable::run()
        Config& run(Runnable* r);
        //! Thread will execute (*fn)(ptr)
        Config& run(void(*fn)(void*), void *ptr);
        //! Thread will execute (inst->*meth)()
        template<typename C>
        Config& run(C* inst, void(C::*meth)())
        {
            this->p_owned_runner.reset(new detail::MethRunner<C>(inst, meth));
            this->p_runner = this->p_owned_runner.get();
            return *this;
        }
#if __cplusplus>=201103L
        Config& run(std::function<void()>&& fn);
#endif

        //! Append to thread name string.  Argument must be understood by std::ostream::operator<<
        template<typename T>
        Config& operator<<(T x) { this->p_strm<<x; return *this; }
    };

    /**
     * 
     * Constructor
     * @param name thread name.
     * @param priority priority is one of:
     @code
     enum ThreadPriority {
        lowestPriority, lowerPriority, lowPriority,
        middlePriority,
        highPriority, higherPriority, highestPriority
     };
     @endcode
     * @param runnable this is a c function
     * @param stkcls stack size as specified by epicsThreadStackSizeClass
     */
    Thread(std::string name,
           ThreadPriority priority,
           Runnable *runnable,
           epicsThreadStackSizeClass stkcls=epicsThreadStackBig);

    /**
     * 
     * Constructor
     * @param runnable this is a c function
     * @name thread name.
     * @param stkcls stack size as specified by epicsThreadStackSizeClass
     * @param priority priority is one of:
     @code
     enum ThreadPriority {
        lowestPriority, lowerPriority, lowPriority,
        middlePriority,
        highPriority, higherPriority, highestPriority
     };
     @endcode
     */
    Thread(Runnable &runnable,
           std::string name,
           unsigned int stksize,
           unsigned int priority=lowestPriority);

    //! @brief Create a new thread using the given @class Config
    //! @throws std::logic_error for improper @class Config (ie. missing runner)
    Thread(Config& c);

    /**
     * Destructor
     */
    ~Thread();

    static size_t num_instances;
private:
    Config::p_owned_runner_t p_owned;
};


}}
#endif  /* THREAD_H */
