/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/*
 * testTimer.cpp
 *
 *  Created on: 2010.11
 *      Author: Marty Kraimer
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <iostream>
#include <exception>

#include <epicsUnitTest.h>
#include <testMain.h>
#include <epicsGuard.h>

#include <pv/timeStamp.h>
#include <pv/event.h>
#include <pv/timer.h>
#include <pv/thread.h>

using namespace epics::pvData;
using std::string;

static const double delays[3] = {1.0, 2.0, 4.0};
static const unsigned ntimes = 3;

namespace {

struct Marker : public TimerCallback
{
    POINTER_DEFINITIONS(Marker);

    Event wait, hold;
    virtual ~Marker() {}
    virtual void callback()
    {
        wait.signal();
        hold.wait();
    }
    virtual void timerStopped() {}
};

struct MyCallback : public TimerCallback {
    POINTER_DEFINITIONS(MyCallback);

    MyCallback(const string& name)
    :name(name)
    ,counter(0)
    ,first_gbl(0)
    {}
    virtual ~MyCallback() {}
    virtual void callback()
    {
        testDiag("expire %s",name.c_str());
        {
            epicsGuard<Mutex> G(gbl_mutex);
            counter++;
            if(first_gbl==0) {
                first_gbl = ++gbl_counter;
            }
        }
        wait.signal();
    }
    virtual void timerStopped()
    {
        testDiag("timerStopped %s",name.c_str());
    }
    void clear() {
        epicsGuard<Mutex> G(gbl_mutex);
        counter = gbl_counter = 0;
    }

    const string name;
    unsigned counter, first_gbl;
    Event wait;

    static unsigned gbl_counter;
    static Mutex gbl_mutex;
};

unsigned MyCallback::gbl_counter;
Mutex MyCallback::gbl_mutex;

typedef std::tr1::shared_ptr<MyCallback> MyCallbackPtr;

}// namespace

static void testBasic(unsigned oneOrd, unsigned twoOrd, unsigned threeOrd)
{
    testDiag("testBasic oneOrd %u twoOrd %u threeOrd %u",
             oneOrd,twoOrd,threeOrd);

    Timer timer("timer" ,middlePriority);

    Marker::shared_pointer marker(new Marker);
    MyCallbackPtr callbackOne(new MyCallback("one"));
    MyCallbackPtr callbackTwo(new MyCallback("two"));
    MyCallbackPtr callbackThree(new MyCallback("three"));

    for(unsigned n=0; n<ntimes; n++) {
        testDiag("iteration %u", n);

        testOk1(!timer.isScheduled(marker));
        timer.scheduleAfterDelay(marker, 0.01);
        marker->wait.wait();
        // timer worker is blocked

        testOk1(!timer.isScheduled(callbackOne));
        testOk1(!timer.isScheduled(callbackTwo));
        testOk1(!timer.isScheduled(callbackThree));

        callbackOne->clear();
        callbackTwo->clear();
        callbackThree->clear();

        timer.scheduleAfterDelay(callbackOne,delays[oneOrd]);
        timer.scheduleAfterDelay(callbackTwo,delays[twoOrd]);
        timer.scheduleAfterDelay(callbackThree,delays[threeOrd]);

        testOk1(timer.isScheduled(callbackOne));
        testOk1(timer.isScheduled(callbackTwo));
        testOk1(timer.isScheduled(callbackThree));

        marker->hold.signal(); // let the worker loose

        callbackOne->wait.wait();
        callbackTwo->wait.wait();
        callbackThree->wait.wait();

        testOk1(!timer.isScheduled(callbackOne));
        testOk1(!timer.isScheduled(callbackTwo));
        testOk1(!timer.isScheduled(callbackThree));

        {
            epicsGuard<Mutex> G(MyCallback::gbl_mutex);
            testOk(callbackOne->counter==1, "%s counter %u = 1", callbackOne->name.c_str(), callbackOne->counter);
            testOk(callbackTwo->counter==1, "%s counter %u = 1", callbackTwo->name.c_str(), callbackTwo->counter);
            testOk(callbackThree->counter==1, "%s counter %u = 1", callbackThree->name.c_str(), callbackThree->counter);

            testOk(callbackOne->first_gbl==oneOrd+1, "%s first_gbl %u = %u", callbackOne->name.c_str(), callbackOne->first_gbl, oneOrd+1);
            testOk(callbackTwo->first_gbl==twoOrd+1, "%s first_gbl %u = %u", callbackTwo->name.c_str(), callbackTwo->first_gbl, twoOrd+1);
            testOk(callbackThree->first_gbl==threeOrd+1, "%s first_gbl %u = %u", callbackThree->name.c_str(), callbackThree->first_gbl, threeOrd+1);
        }

    }
}

static void testCancel(unsigned oneOrd, unsigned twoOrd, unsigned threeOrd,
                       unsigned oneReal, unsigned threeReal)
{
    testDiag("testCancel oneOrd %u twoOrd %u threeOrd %u",
             oneOrd,twoOrd,threeOrd);

    Timer timer("timer" ,middlePriority);

    Marker::shared_pointer marker(new Marker);
    MyCallbackPtr callbackOne(new MyCallback("one"));
    MyCallbackPtr callbackTwo(new MyCallback("two"));
    MyCallbackPtr callbackThree(new MyCallback("three"));

    for(unsigned n=0; n<ntimes; n++) {
        testDiag("iteration %u", n);

        testOk1(!timer.isScheduled(marker));
        timer.scheduleAfterDelay(marker, 0.01);
        marker->wait.wait();
        // timer worker is blocked

        testOk1(!timer.isScheduled(callbackOne));
        testOk1(!timer.isScheduled(callbackTwo));
        testOk1(!timer.isScheduled(callbackThree));

        callbackOne->clear();
        callbackTwo->clear();
        callbackThree->clear();

        timer.scheduleAfterDelay(callbackOne,delays[oneOrd]);
        timer.scheduleAfterDelay(callbackTwo,delays[twoOrd]);
        timer.scheduleAfterDelay(callbackThree,delays[threeOrd]);

        testOk1(timer.isScheduled(callbackOne));
        testOk1(timer.isScheduled(callbackTwo));
        testOk1(timer.isScheduled(callbackThree));

        timer.cancel(callbackTwo);

        testOk1(timer.isScheduled(callbackOne));
        testOk1(!timer.isScheduled(callbackTwo));
        testOk1(timer.isScheduled(callbackThree));

        marker->hold.signal(); // let the worker loose

        callbackOne->wait.wait();
        callbackThree->wait.wait();

        testOk1(!timer.isScheduled(callbackOne));
        testOk1(!timer.isScheduled(callbackTwo));
        testOk1(!timer.isScheduled(callbackThree));

        {
            epicsGuard<Mutex> G(MyCallback::gbl_mutex);
            testOk(callbackOne->counter==1, "%s counter %u = 1", callbackOne->name.c_str(), callbackOne->counter);
            testOk(callbackTwo->counter==0, "%s counter %u = 0", callbackTwo->name.c_str(), callbackTwo->counter);
            testOk(callbackThree->counter==1, "%s counter %u = 1", callbackThree->name.c_str(), callbackThree->counter);

            testOk(callbackOne->first_gbl==oneReal+1, "%s first_gbl %u = %u", callbackOne->name.c_str(), callbackOne->first_gbl, oneOrd+1);
            testOk(callbackTwo->first_gbl==0, "%s first_gbl %u = %u", callbackTwo->name.c_str(), callbackTwo->first_gbl, 0);
            testOk(callbackThree->first_gbl==threeReal+1, "%s first_gbl %u = %u", callbackThree->name.c_str(), callbackThree->first_gbl, threeOrd+1);
        }

    }
}

MAIN(testTimer)
{
    testPlan(315);
    try {
        testDiag("Tests timer");

        testBasic(2, 1, 0);
        testCancel(2, 1, 0, 1, 0);

        testBasic(0, 1, 2);
        testCancel(0, 1, 2, 0, 1);

        testBasic(0, 2, 1);
        testCancel(0, 2, 1, 0, 1);

    }catch(std::exception& e) {
        testFail("Unhandled exception: %s", e.what());
    }

    return testDone();
}
