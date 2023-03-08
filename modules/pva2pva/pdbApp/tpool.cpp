
#include <typeinfo>
#include <stdexcept>

#include <epicsEvent.h>
#include <epicsGuard.h>
#include <epicsThread.h>
#include <errlog.h>

#include <pv/sharedPtr.h>

#include "helper.h"
#include "tpool.h"

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

WorkQueue::WorkQueue(const std::string& name)
    :name(name)
    ,state(Idle)
{}

WorkQueue::~WorkQueue() { close(); }

void WorkQueue::start(unsigned nworkers, unsigned prio)
{
    Guard G(mutex);

    if(state!=Idle)
        throw std::logic_error("Already started");

    try {
        state = Active;

        for(unsigned i=0; i<nworkers; i++) {
            p2p::auto_ptr<epicsThread> worker(new epicsThread(*this, name.c_str(),
                                                              epicsThreadGetStackSize(epicsThreadStackSmall),
                                                              prio));

            worker->start();

            workers.push_back(worker.get());
            worker.release();
        }
    }catch(...){
        UnGuard U(G); // unlock as close() blocks to join any workers which were started
        close();
        throw;
    }
}

void WorkQueue::close()
{
    workers_t temp;

    {
        Guard G(mutex);
        if(state!=Active)
            return;

        temp.swap(workers);
        state = Stopping;
    }

    wakeup.signal();

    for(workers_t::iterator it(temp.begin()), end(temp.end()); it!=end; ++it)
    {
        (*it)->exitWait();
        delete *it;
    }

    {
        Guard G(mutex);
        state = Idle;
    }
}

void WorkQueue::add(const value_type& work)
{
    bool empty;

    {
        Guard G(mutex);
        if(state!=Active)
            return;

        empty = queue.empty();

        queue.push_back(work);
    }

    if(empty) {
        wakeup.signal();
    }
}

void WorkQueue::run()
{
    Guard G(mutex);

    std::tr1::shared_ptr<epicsThreadRunable> work;

    while(state==Active) {

        if(!queue.empty()) {
            work = queue.front().lock();
            queue.pop_front();
        }

        bool last = queue.empty();

        {
            UnGuard U(G);

            if(work) {
                try {
                    work->run();
                    work.reset();
                }catch(std::exception& e){
                    errlogPrintf("%s Unhandled exception from %s: %s\n",
                                 name.c_str(), typeid(work.get()).name(), e.what());
                    work.reset();
                }
            }

            if(last) {
                wakeup.wait();
            }
        }
    }

    // pass along the close() signal to next worker
    wakeup.signal();
}
