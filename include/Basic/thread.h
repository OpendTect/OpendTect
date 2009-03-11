#ifndef thread_h
#define thread_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: thread.h,v 1.35 2009-03-11 14:15:12 cvskris Exp $
________________________________________________________________________

*/

#include "callback.h"
#ifdef __win__
# include "pthreadwin.h"
#else
# include <pthread.h>
#endif



/*!\brief interface to threads that should be portable.

As usual, other thread systems are available but they are as far as we know
simply too big and dependent.

*/


namespace Threads
{

/*!\brief Is a lock that allows a thread to have exlusive rights to something.

It is guaranteed that once locked, noone else will be able to lock it before
it is unlocked. If a thread tries to lock it, it will be postponed until
the thread that has locked it will unlock it.
*/

mClass Mutex
{
public:
			Mutex( bool deadlockdetection=false );
			Mutex(const Mutex&);
    virtual		~Mutex();	

    int			lock();
    int			unLock();

    bool		tryLock();
    			/*!< Returns true if mutex is locked.
			     If it is locked, it you must unLock it when
			     you are finished. If it returns false, 
			     carry on with your life.
			 */

protected:

    pthread_mutex_t 	mutex_;
    pthread_mutexattr_t	attr_;
};


/*!\brief Is an object that is convenient to use when a mutex should be
  locked and unlocked automaticly when returning.

Example:

int function()
{
    MutexLocker lock( myMutex );
    //Do whatever you want to do
}
*/


mClass MutexLocker
{
public:
		MutexLocker( Mutex& mutex, bool wait=true );
		~MutexLocker();

    bool	isLocked() const;

    void	unLock();
		/*!<Use at own risk! To be safe, it should only be called
		    by the process that created the lock. */
    void	lock();
		/*!<Use at own risk! To be safe, it should only be called
		    by the process that created the lock, and have
		    called the unLock(). */

protected:

    Mutex&	mutex_;
    bool	islocked_;
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

3. If condition is true, unLock() and start working. When finished working
   go back to 1.

It is wise to put an exit flag in the loop, so it's possible to say that we
are about to quit.

From the manager:
When you want to change the condition:
1. lock
2. set condition (e.g. add more work)
3. signal
4. unLock

*/


mClass ConditionVar : public Mutex
{
public:
				ConditionVar();
				ConditionVar(const ConditionVar&);
				~ConditionVar();

    int				wait();
    int 			signal(bool all);
    				/*!< If all is true, all threads that have
				     called wait() will be Notified about the
				     signal. If all is false, only one thread
				     will respond.
				*/

protected:

    pthread_cond_t		cond_;
    pthread_condattr_t		condattr_;

};


/*! Lock that permits multiple readers to lock the object at the same time,
but it will not allow any readers when writelocked, and no writelock is allowed
when readlocked. */


mClass ReadWriteLock
{
public:
    			ReadWriteLock();
    			ReadWriteLock(const ReadWriteLock&);
    virtual		~ReadWriteLock();

    void		readLock();
    			//!<No writers will be active.
    void		writeLock();
    			//!<No readers will be active.
    void		permissiveWriteLock();
    			/*!<Same as readlock, but I'm guaranteed to convert to
			    writelock without giving up my lock. Only one
			    thread may have the permissive write lock
			    at any given time. */
    void		readUnLock();
    void		writeUnLock();
    void		permissiveWriteUnLock();

    bool		convReadToWriteLock();
    			/*!<Lock MUST be readLocked when calling. Object Will
			    always be in write-lock status on return.
			    \returns false if it had to release the readlock
			             when switching to writelock.*/
    void		convWriteToReadLock();
    			//!<Lock MUST be writeLocked when calling.

    void		convPermissiveToWriteLock();
    void		convWriteToPermissive();

protected:
    int			nrreaders_;
    char		status_;
    			//0 not writelocked, -2 write lock, -1 permissive lock
			//>0, number of readers
    ConditionVar	statuscond_;
};



/*!\brief
is the base class for all threads. Start it by creating it and give it the
function or CallBack to execute. The function running in the thread must not
return. Instead it should call threadExit to terminate itself.

The process that has created the thread must call destroy() or detach().


*/

mClass Thread
{
public:

				Thread(void (*)(void*));
				Thread(const CallBack&);

    static void			threadExit();
				/*!< Should only be called by the 
				     running thread */

    void			stop();
    				/*!< Delete the thread with this function.
				    Will wait for the thread to call threadExit.
				*/

    void			detach();
    				/*!< Will make sure the threads resouces are
				     released once the thread calls threadExit.
				     Will return immidiately.
				*/

    unsigned long int		ID() const
				{ return (unsigned long int)id_; }
    				//!< debugging purposes

protected:

    pthread_t			id_;

private:

    friend class		Threads::Mutex;
    				//< Only to avoid a stupid compiler msg
    
    virtual			~Thread()	{}
    CallBack			cb;

};

/*! Fetches number of processors from operating system, unless:
  * On windows we may not be able to handle more than one processor
  * DTECT_USE_MULTIPROC is set to 'n' or 'N'
  * The user settings contain a 'Nr Processors' entry.
  */

mGlobal int getNrProcessors();


#define mThreadDeclareMutexedVar(T,var) \
    T			var; \
    Threads::Mutex	var##mutex

#define mThreadMutexedSet(var,newval) \
    var##mutex.lock(); \
    var = newval; \
    var##mutex.unLock()

#define mThreadMutexedGet(retvar,var) \
    var##mutex.lock(); \
    retvar = var; \
    var##mutex.unLock()

#define mThreadMutexedGetVar(T,retvar,var) \
    var##mutex.lock(); \
    T retvar = var; \
    var##mutex.unLock()

};

#endif
