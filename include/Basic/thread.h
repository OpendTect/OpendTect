#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
________________________________________________________________________

*/

#include "basicmod.h"
#include "commondefs.h"
#include "plftypes.h"

mFDQtclass(QThread)
mFDQtclass(QMutex)
mFDQtclass(QWaitCondition)

class CallBack;

/*!\brief interface to threads that should be portable.

As usual, other thread systems are available but they are as far as we know
simply too big and dependent.

*/

namespace Threads
{

typedef void* ThreadID;

/*!
\brief Is a lock that allows a thread to have exlusive rights to something.

  It is guaranteed that once locked, no one else will be able to lock it before
  it is unlocked. If a thread tries to lock it, it will be postponed until
  the thread that has locked it will unlock it.
*/

mExpClass(Basic) Mutex
{
public:
		Mutex( bool recursive=false );
		/*\If recursive, mutex can be locked
		   multiple times from the same thread without deadlock.
		   It will be unlock when unLock has been called the same
		   number of times as lock(). */
		Mutex(const Mutex&);
    virtual	~Mutex();

    void	lock();
    void	unLock();

    bool	tryLock();
		/*!< Returns true if mutex is locked.
		     If it is locked, it you must unLock it when
		     you are finished. If it returns false,
		     carry on with your life.
		*/

protected:

    mQtclass(QMutex*)		qmutex_;

    ThreadID			lockingthread_;
				//!<Only set in debug-mode
    int				count_;
				//!<Only set in debug-mode
public:
    int				getCount() const { return count_; }
};



/*!
\brief Is an object that faciliates many threads to wait for something to
happen.

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


mExpClass(Basic) ConditionVar : public Mutex
{
public:
				ConditionVar();
				ConditionVar(const ConditionVar&);
				~ConditionVar();

    void			wait();
    bool			wait(unsigned long timeout);
				/*!<\param timeout is in milliseconds.
				    \retval false if timeout was hit
				*/
    void			signal(bool all);
				/*!< If all is true, all threads that have
				     called wait() will be Notified about the
				     signal. If all is false, only one thread
				     will respond.
				*/

protected:

#ifndef OD_NO_QT
    mQtclass(QWaitCondition*)		cond_;
#endif
};


/*!
\brief Lock that permits multiple readers to lock the object at the same time,
but it will not allow any readers when writelocked, and no writelock is allowed
when readlocked.
*/

mExpClass(Basic) ReadWriteLock
{
public:
			ReadWriteLock();
			ReadWriteLock(const ReadWriteLock&);
    virtual		~ReadWriteLock();

    bool		isLocked() const		{ return status_ != 0;}
    bool		isLockedForWrite() const	{ return status_ < 0; }
    bool		isLockedForRead() const		{ return status_ > 0; }

    void		readLock();
			//!<No writers will be active.
    bool		tryReadLock();
			//!<No writers will be active.
    void		writeLock();
			//!<No readers will be active.
    bool		tryWriteLock();
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


/*!
\ingroup Basic
\brief Is an object that is convenient to use when a mutex should be
locked and unlocked automatically when returning.

  Example:

  int function()
  {
     MutexLocker lock( myMutex );
     //Do whatever you want to do
  }
*/

#define mLockerClassImpl( mod, clssnm, clss, lockfn, unlockfn, trylockfn ) \
mExpClass(mod) clssnm \
{ \
public: \
		clssnm( clss& thelock, bool wait=true ) \
		    : lock_( thelock ) \
		    , islocked_( true ) \
		{ \
		    if ( wait ) thelock.lockfn; \
		    else islocked_ = thelock.trylockfn; \
		} \
 \
		~clssnm() { if ( islocked_ ) lock_.unlockfn; } \
    bool	isLocked() const { return islocked_; } \
 \
    void	unLock() { islocked_ = false; lock_.unlockfn; } \
		/*!<Use at own risk! To be safe, it should only be called \
		    by the process that created the lock. */ \
    void	lock() { islocked_ = true; lock_.lockfn; } \
		/*!<Use at own risk! To be safe, it should only be called \
		    by the process that created the lock, and have \
		    called the unLock(). */ \
 \
protected: \
 \
    clss&	lock_; \
    bool	islocked_; \
};

mLockerClassImpl( Basic, MutexLocker, Mutex, lock(), unLock(), tryLock() )
mLockerClassImpl( Basic, ReadLockLocker, ReadWriteLock,
		  readLock(), readUnLock(), tryReadLock() )
mLockerClassImpl( Basic, WriteLockLocker, ReadWriteLock,
		  writeLock(), writeUnLock(), tryWriteLock() )


/*!
\brief Waits for a number of threads to reach a certain point (i.e. the call to
Barrier::waitForAll). Once everyone has arrived, everyone is released.
*/

mExpClass(Basic) Barrier
{
public:
			Barrier(int nrthreads=-1,bool immediatrelease=true);
    void		setNrThreads(int);
    int			nrThreads() const		{ return nrthreads_; }

    bool		waitForAll(bool unlock=true);
			/*!<\returns true if current thread is the first
				     one to return. If immediaterelease_ is
				     false, this thread has to release all
				     other threads with releaseAll() or
				     releaseAllNoLock().
			    \param   unlock If false, the mutex will still be
				     locked when returning, and mutex().unLock()
				     must be called to allow other threads to
				     be released(). */
    void		releaseAll();
			/*!<Locks, and releases all. */
    void		releaseAllNoLock();
			/*!<Releases all. */

    Mutex&		mutex()				{ return condvar_; }

protected:
    void		releaseAllInternal();

    ConditionVar	condvar_;
    int			nrthreads_;
    int			threadcount_;
    bool		dorelease_;

    bool		immediaterelease_;
};


/*!
\brief Is the base class for all threads. Start it by creating it and give it
the function or CallBack to execute.

  The process that has created the thread must call destroy() or detach().

  Try to give each thread a meaningful name, because the name will show up in
  crash reports. If you really can't think of any, you can still pass null.
*/

mExpClass(Basic) Thread
{
public:

				Thread(void (*)(void*),const char* nm=nullptr);
				Thread(const CallBack&,const char* nm=nullptr);
    virtual			~Thread();

    ThreadID			threadID() const;

    void			waitForFinish();
				/*!< Stop the thread with this function.
				    Will wait for the thread to return.  */

    const char*			getName() const;


protected:

    mQtclass(QThread*)			thread_;
};

/*! Fetches number of processors from operating system, unless:
  * DTECT_USE_MULTIPROC is set to 'n' or 'N'
  * The user settings contain a 'Nr Processors' entry.
*/

mGlobal(Basic) int getSystemNrProcessors();
mGlobal(Basic) int getNrProcessors();
mGlobal(Basic) ThreadID currentThread();
mGlobal(Basic) void setCurrentThreadProcessorAffinity( int cpu );
/*!<Tells the current thread to always run on a certain cpu. Set to
-1 to remove binding.  Not guaranteed to work on all platforms. */

/*! Causes the current thread to sleep */
mGlobal(Basic) void sleep(double seconds);


} //namespace
