#ifndef thread_h
#define thread_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: thread.h,v 1.7 2002-04-15 12:04:54 kristofer Exp $
________________________________________________________________________

*/

#include "gendefs.h"

#ifndef __win__
#define __pthread__ 1
#endif

#ifdef __pthread__
#include <pthread.h>
#endif

/*!\brief interface to threads that should be portable.

As usual, other thread systems are available but they are as far as we know
simply too big and dependent.

*/


namespace Threads
{

/*!\brief
Is a lock that allows a thread to have exlusive rights to something. It is
guaranteed that once locked, noone else will be able to lock it before it is
unlocked. If a thread tries to lock it, it will be postponed until the thread
that has locked it will unlock it.
*/

class Mutex
{
public:
			Mutex();
    virtual		~Mutex();	

    int			lock();
    int			unlock();

protected:

#ifdef __pthread__
    pthread_mutex_t 	mutex;
    pthread_mutexattr_t	attr;
#endif

};


/*!\brief
Is an object that is conveniant to use when a mutex should be locked and
be unlocked automaticly when returning.

Example:

int function()
{
    MutexLocker lock( myMutex );
    //Do whatever you want to do
}
*/
class MutexLocker
{
public:
		MutexLocker( Mutex& mutex_ ) : mutex( mutex_ ) { mutex.lock(); }
		~MutexLocker() { mutex.unlock(); }
protected:
    Mutex&	mutex;
};


/*!\brief
Is an object that is faciliates many threads to wait for something to happen.

Usage:

From the working thread
1. lock()
   You will now be the only one allowed to check weather condition is true
   (e.g. if new work has arrived).

2. Check condition. If false, call wait(). You will now sleep until someone
   calls signal(); If you are awakened, check the condition again and go back
   to sleep if it is false.

3. If condition is true, unlock() and start working. When finished working
   go back to 1.

It is wise to put an exit flag in the loop, so it's possible to say that we
are about to quit.

From the manager:
When you want to change the condition:
1. lock
2. set condition (e.g. add more work)
3. unlock
4. signal

*/


class ConditionVar : public Mutex
{
public:
				ConditionVar();
				~ConditionVar();

    int				wait();
    int 			signal(bool all);
    				/*!< If all is true, all threads that have
				     called wait() will be Notified about the
				     signal. If all is false, only one thread
				     will respond.
				*/

protected:

#ifdef __pthread__
    pthread_cond_t		cond;
    pthread_condattr_t		condattr;
#endif
};

/*!\brief
is the base class for all threads. Start it by creating it and give it the
function to run. The function running in the thread must not return. Instead
it should call threadExit to terminate itself.

The process that has created the thread must call
*/

class Thread
{
public:
				Thread(void (*)(void*), void* arg);

    static void			threadExit( void* retval=0 );
				/*!< Should only be called by the 
				     running thread */

    void			destroy(bool wait, void** retval);
    				/*!< Delete the thread with this function.
				    If wait is true, it will wait for the
				    thread to call threadExit. retval is
				    only set if wait is true
				*/

protected:
#ifdef __pthread__
    pthread_t			id;
#endif

private:
    friend			Threads::Mutex;
    				//< Only to avoid stupid compiler msg
    
    virtual			~Thread() {}
};
				    

};

#endif
