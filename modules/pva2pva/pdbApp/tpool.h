#ifndef TPOOL_H
#define TPOOL_H

#include <stdexcept>
#include <deque>
#include <vector>
#include <string>

#include <errlog.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include <pv/sharedPtr.h>

struct WorkQueue : private epicsThreadRunable
{
    typedef std::tr1::weak_ptr<epicsThreadRunable> value_type;

private:
    const std::string name;

    epicsMutex mutex;

    enum state_t {
        Idle,
        Active,
        Stopping,
    } state;

    typedef std::deque<value_type> queue_t;
    queue_t queue;

    epicsEvent wakeup;

    typedef std::vector<epicsThread*> workers_t;
    workers_t workers;

public:
    WorkQueue(const std::string& name);
    virtual ~WorkQueue();

    void start(unsigned nworkers=1, unsigned prio = epicsThreadPriorityLow);
    void close();

    void add(const value_type& work);

private:
    virtual void run();
};

#endif // TPOOL_H
