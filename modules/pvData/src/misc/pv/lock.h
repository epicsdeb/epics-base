/* lock.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#ifndef LOCK_H
#define LOCK_H

#include <stdexcept>

#include <epicsMutex.h>
#include <shareLib.h>

#include <pv/noDefaultMethods.h>


/* This is based on item 14 of 
 * Effective C++, Third Edition, Scott Meyers
 */

// TODO reference counting lock to allow recursions

namespace epics { namespace pvData { 

typedef epicsMutex Mutex;

/**
 * @brief A lock for multithreading
 *
 * This is based on item 14 of 
 *  * Effective C++, Third Edition, Scott Meyers
 */
class Lock {
    EPICS_NOT_COPYABLE(Lock)
public:
    /**
     * Constructor
     * @param m The mutex for the facility being locked.
     */
    explicit Lock(Mutex &m)
    : mutexPtr(m), locked(true)
    { mutexPtr.lock();}
    /**
     * Destructor
     * Note that destructor does an automatic unlock.
     */
    ~Lock(){unlock();}
    /**
     * Take the lock
     * Recursive locks are supported but each lock must be matched with an unlock.
     */
    void lock()
    {
        if(!locked) 
        {
            mutexPtr.lock();
            locked = true;
        }
    }
    /** 
     * release the lock.
     */
    void unlock()
    {
        if(locked)
        {
            mutexPtr.unlock();
            locked=false;
        }
    }
    /** 
     * If lock is not held take the lock.
     * @return (false,true) if caller (does not have, has) the lock.
     */
    bool tryLock()
    {
         if(locked) return true;
         if(mutexPtr.tryLock()) {
             locked = true;
             return true;
         }
         return false;
    }
    /** 
     * See if caller has the lock,
     * @return (false,true) if caller (does not have, has) the lock.
     */
    bool ownsLock() const{return locked;}
private:
    Mutex &mutexPtr;
    bool locked;
};


}}
#endif  /* LOCK_H */
