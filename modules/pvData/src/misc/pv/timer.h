/* timer.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef TIMER_H
#define TIMER_H
#include <memory>
#include <list>

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <pv/pvType.h>
#include <pv/thread.h>
#include <pv/timeStamp.h>
#include <pv/event.h>
#include <pv/lock.h>
#include <pv/sharedPtr.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

class TimerCallback;
class Timer;
typedef std::tr1::shared_ptr<TimerCallback> TimerCallbackPtr;
typedef std::tr1::shared_ptr<Timer> TimerPtr;

/**
 * @brief Class that must be implemented by code that makes Timer requests.
 *
 */
class epicsShareClass TimerCallback {
public:
    POINTER_DEFINITIONS(TimerCallback);
    /**
     * Constructor
     */
    TimerCallback();
    /**
     * Destructor
     */
    virtual ~TimerCallback(){}
    /**
     * The method that is called when a timer expires.
     */
    virtual void callback() = 0;
    /**
     * The timer has stopped.
     */
    virtual void timerStopped() = 0;
private:
    epicsTime timeToRun;
    double period;
    bool onList;
    friend class Timer;
    struct IncreasingTime;
};

/**
 * @brief Support for delayed or periodic callback execution.
 *
 */
class epicsShareClass Timer : private Runnable {
public:
    POINTER_DEFINITIONS(Timer);
    /** Create a new timer queue
     * @param threadName name for the timer thread.
     * @param priority thread priority
     */
    Timer(std::string threadName, ThreadPriority priority);
    virtual ~Timer();
    //! Prevent new callbacks from being scheduled, and cancel pending callbacks
    void close();
    /**
     * schedule a callback after a delay.
     * @param timerCallback the timerCallback instance.
     * @param delay number of seconds before calling callback.
     */
    void scheduleAfterDelay(
        TimerCallbackPtr const &timerCallback,
        double delay);
    /**
     * schedule a periodic callback.`
     * @param timerCallback the timerCallback instance.
     * @param delay number of seconds before first callback.
     * @param period time in seconds between each callback.
     */
    void schedulePeriodic(
        TimerCallbackPtr const &timerCallback,
        double delay,
        double period);
    /**
     * cancel a callback.
     * @param timerCallback the timerCallback to cancel.
     * @returns true if the timer was queued, and now is cancelled
     */
    bool cancel(TimerCallbackPtr const &timerCallback);
    /**
     * Is the callback scheduled to be called?
     * @param timerCallback the timerCallback.
     * @return (false,true) if (not, is) scheduled.
     */
    bool isScheduled(TimerCallbackPtr const &timerCallback) const;
    /**
     * show the elements in the timer queue.
     * @param o The output stream for the output
     */
    void dump(std::ostream& o) const;

private:
    virtual void run();

    // call with mutex held
    void addElement(TimerCallbackPtr const &timerCallback);

    typedef std::list<TimerCallbackPtr> queue_t;

    mutable Mutex mutex;
    queue_t queue;
    Event waitForWork;
    bool waiting;
    bool alive;
    Thread thread;
};

epicsShareExtern std::ostream& operator<<(std::ostream& o, const Timer& timer);

}}
#endif  /* TIMER_H */
