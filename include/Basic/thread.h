#ifndef thread_h
#define thread_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: thread.h,v 1.58 2012-07-02 10:43:46 cvskris Exp $
________________________________________________________________________

*/

#include "callback.h"

#ifndef OD_NO_QT
class QThread;
class QMutex;
class QWaitCondition;
#endif

#ifdef __win__
#include "windows.h"
#define mHasAtomic
#define mAtomicWithMutex
#endif

/*!\brief interface to threads that should be portable.

As usual, other thread systems are available but they are as far as we know
simply too big and dependent.

*/

namespace Threads
{
class Mutex;


/*! Atomic variable where an operation (add, subtract) can
    be done without locking in a multithreaded environment. Only
    available for long, unsigned long */

template <class T>
mClass Atomic
{
public:
    		Atomic(T val=0);
#ifdef mAtomicWithMutex
		Atomic( const Atomic<T>& );
		~Atomic();
#endif

		operator T() const	{ return val_; }
    T		get() const		{ return val_; }
    
    T		operator=(T v)		{ val_=v; return val_; }

    inline T	operator+=(T);
    inline T	operator-=(T);
    inline T	operator++();
    inline T	operator--();
    inline T	operator++(int);
    inline T	operator--(int);

    inline bool	setIfEqual(T newval, T oldval );
    /*!<Sets the val_ only if value is previously set
     to oldval */
    
      
protected:
    volatile T	val_;
    
#ifdef mAtomicWithMutex
    Mutex*	lock_;
#endif

};

#ifdef __win__
#define mAtomicPointerType od_uint64
#else
#define mAtomicPointerType void*
#endif

    
/*Atomic instanciated with a pointer. The class really only handles the
  casting from a void* to a T*. */
template <class T>
mClass AtomicPointer
{
public:
    inline	AtomicPointer(T* newptr = 0);

    inline bool	setIfOld(const T* oldptr, T* newptr);
    
    inline void	unRef();
    		/*!<Don't be confused, class works for non-ref-counted objects
		    as well. Just don't call ref/unRef(); */
    inline void	ref();
    
    inline T*	setToNull();
		/*!<Returns the last value of the ptr. */
    
    inline	operator T*();

    inline T*	operator+=(int);
    inline T*	operator-=(int);
    inline T*	operator++();
    inline T*	operator--();
    inline T*	operator++(int);
    inline T*	operator--(int);

protected:


    Atomic<mAtomicPointerType>	ptr_;
};


/*!\brief Is a lock that allows a thread to have exlusive rights to something.

It is guaranteed that once locked, noone else will be able to lock it before
it is unlocked. If a thread tries to lock it, it will be postponed until
the thread that has locked it will unlock it.
*/

mClass Mutex
{
public:
			Mutex( bool recursive=false );
			Mutex(const Mutex&);
    virtual		~Mutex();	

    void		lock();
    void		unLock();

    bool		tryLock();
    			/*!< Returns true if mutex is locked.
			     If it is locked, it you must unLock it when
			     you are finished. If it returns false, 
			     carry on with your life.
			 */

protected:

#ifndef OD_NO_QT
    QMutex*		qmutex_;
#endif
};


mClass SpinLock
{
public:
			SpinLock();
			SpinLock(const SpinLock&);
			~SpinLock();

    void		lock();
    void		unLock();
    bool		tryLock();

protected:
#ifdef mHasAtomic
    Atomic<long>	spinlock_;
#else
    Mutex		spinlock_;
#endif
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

    void			wait();
    void 			signal(bool all);
    				/*!< If all is true, all threads that have
				     called wait() will be Notified about the
				     signal. If all is false, only one thread
				     will respond.
				*/

protected:

#ifndef OD_NO_QT
    QWaitCondition*		cond_;
#endif
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


/*!\brief Is an object that is convenient to use when a mutex should be
  locked and unlocked automaticly when returning.

Example:

int function()
{
    MutexLocker lock( myMutex );
    //Do whatever you want to do
}
*/

#define mLockerClassImpl( clssnm, clss, lockfn, unlockfn, trylockfn ) \
class clssnm \
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

mLockerClassImpl( MutexLocker, Mutex, lock(), unLock(), tryLock() )
mLockerClassImpl( SpinLockLocker, SpinLock, lock(), unLock(), tryLock() )
mLockerClassImpl( ReadLockLocker, ReadWriteLock,
		  readLock(), readUnLock(), tryReadLock() )
mLockerClassImpl( WriteLockLocker, ReadWriteLock,
		  writeLock(), writeUnLock(), tryWriteLock() )


/*!Waits for a number of threads to reach a certain point (i.e. the call to
   Barrier::waitForAll). Once everyone has arrived, everyone is released. */

mClass Barrier
{
public:
    			Barrier(int nrthreads=-1,bool immediatrelease=true);
    void		setNrThreads(int);
    int			nrThreads() const 		{ return nrthreads_; }

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


/*!\brief
is the base class for all threads. Start it by creating it and give it the
function or CallBack to execute. 

The process that has created the thread must call destroy() or detach().


*/

mClass Thread
{
public:

				Thread(void (*)(void*));
				Thread(const CallBack&);
    virtual			~Thread();

    const void*			threadID() const;

    void			waitForFinish();
    				/*!< Stop the thread with this function.
				    Will wait for the thread to return.  */

protected:

#ifndef OD_NO_QT
    QThread*			thread_;
#endif
};

/*! Fetches number of processors from operating system, unless:
  * DTECT_USE_MULTIPROC is set to 'n' or 'N'
  * The user settings contain a 'Nr Processors' entry.
*/

mGlobal int getNrProcessors();
mGlobal const void* currentThread();



/*! Causes the current thread to sleep */
mGlobal void sleep(double time); /*!< Time in seconds */


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

// Atomic implementations
#ifdef __win__

#define mAtomicSpecialization( type, postfix ) \
template <> inline \
Atomic<type>::Atomic( type val ) \
: val_( val ) \
, lock_( 0 ) \
{} \
\
\
template <> inline \
bool Atomic<type>::setIfEqual(type newval, type oldval ) \
{ \
if ( newval==oldval ) \
return true; \
\
return InterlockedCompareExchange##postfix( &val_, newval, oldval )!=newval; \
} \
\
template <> inline \
type Atomic<type>::operator += (type b) \
{  \
    return InterlockedExchangeAdd##postfix( &val_, b );  \
}  \
\
template <> inline \
type Atomic<type>::operator -= (type b) \
{ \
    return InterlockedExchangeAdd##postfix( &val_, -b ); \
} \
\
\
template <> inline \
type Atomic<type>::operator ++() \
{ \
    return InterlockedIncrement##postfix( &val_); \
} \
\
\
template <> inline \
type Atomic<type>::operator -- () \
{ \
    return InterlockedDecrement##postfix( &val_ ); \
} \
\
\
template <> inline \
type Atomic<type>::operator ++(int) \
{ \
    return InterlockedIncrement##postfix( &val_ )-1; \
} \
\
\
template <> inline \
type Atomic<type>::operator -- (int) \
{ \
    return InterlockedDecrement##postfix( &val_ )+1; \
} \

mAtomicSpecialization( long, )
mAtomicSpecialization( unsigned long, )
#ifdef _WIN64
mAtomicSpecialization( long long, 64 )
#endif

#undef mAtomicSpecialization

template <class T> inline
Atomic<T>::Atomic( T val )
    : val_( val )
    , lock_( new Mutex )
{}


template <class T> inline
T Atomic<T>::operator += (T b)
{
    MutexLocker lock( *lock_ );
    return val_ += b;
}


template <class T> inline
T Atomic<T>::operator -= (T b)
{
    MutexLocker lock( *lock_ );
    return val_ -= b;
}


template <class T> inline
T Atomic<T>::operator ++()
{
    MutexLocker lock( *lock_ );
    return ++val_;
}


template <class T> inline
T Atomic<T>::operator -- ()
{
    MutexLocker lock( *lock_ );
    return --val_;
}


template <class T> inline
T Atomic<T>::operator ++(int)
{
    MutexLocker lock( *lock_ );
    return val_++;
}


template <class T> inline
T Atomic<T>::operator -- (int)
{
    MutexLocker lock( *lock_ );
    return val_--;
}


template <class T> inline
bool Atomic<T>::setIfEqual(T newval, T oldval )
{
    MutexLocker lock( *lock_ );
    const bool res = val_==oldval;
    if ( res )
	val_ = newval;
    return res;
}

#else //not win


template <class T> inline
Atomic<T>::Atomic( T val )
    : val_( val )
#ifdef mAtomicWithMutex
    , lock_( new Mutex )
#endif
{}


template <class T> inline
T Atomic<T>::operator += (T b)
{
#ifdef mAtomicWithMutex
    MutexLocker lock( *lock_ );
    return val_ += b;
#else
    return __sync_add_and_fetch(&val_, b);
#endif
}


template <class T> inline
T Atomic<T>::operator -= (T b)
{
#ifdef mAtomicWithMutex
    MutexLocker lock( *lock_ );
    return val_ -= b;
#else
    return __sync_sub_and_fetch(&val_, b);
#endif
}


template <class T> inline
T Atomic<T>::operator ++()
{
#ifdef mAtomicWithMutex
    MutexLocker lock( *lock_ );
    return ++val_;
#else
    return __sync_add_and_fetch(&val_, 1);
#endif
}


template <class T> inline
T Atomic<T>::operator -- ()
{
#ifdef mAtomicWithMutex
    MutexLocker lock( *lock_ );
    return --val_;
#else
    return __sync_sub_and_fetch(&val_, 1);
#endif
}


template <class T> inline
T Atomic<T>::operator ++(int)
{
#ifdef mAtomicWithMutex
    MutexLocker lock( *lock_ );
    return val_++;
#else
    return __sync_fetch_and_add(&val_, 1);
#endif
}


template <class T> inline
T Atomic<T>::operator -- (int)
{
#ifdef mAtomicWithMutex
    MutexLocker lock( *lock_ );
    return val_--;
#else
    return __sync_fetch_and_sub(&val_, 1);
#endif
}


template <class T> inline
bool Atomic<T>::setIfEqual(T newval, T oldval )
{
#ifdef mAtomicWithMutex
    MutexLocker lock( *lock_ );
    const bool res = val_==oldval;
    if ( res )
	val_ = newval;
    return res;
#else
    return __sync_bool_compare_and_swap( &val_, oldval, newval );
#endif
}

#endif // not win

#ifdef mAtomicWithMutex
template <class T> inline
Atomic<T>::Atomic( const Atomic<T>& b )
    : lock_( new Mutex ), val_( b.val_ )
{}

template <class T> inline
Atomic<T>::~Atomic()
{
    delete lock_;
}

#undef mAtomicWithMutex
#endif

    
    


/* AtomicPointer implementations. */
template <class T> inline
AtomicPointer<T>::AtomicPointer(T* newptr )
    : ptr_( (mAtomicPointerType) newptr )
{}


template <class T> inline
bool AtomicPointer<T>::setIfOld(const T* oldptr, T* newptr)
{
    return ptr_.setIfEqual( (mAtomicPointerType) newptr,
			    (mAtomicPointerType)oldptr );
}
    
    
template <class T> inline
T* AtomicPointer<T>::setToNull()
{
    T* oldptr = (T*) ptr_.get();
    while ( oldptr && !setIfOld( oldptr, 0 ) )
	oldptr = (T*) ptr_.get();
    
    return oldptr;    
}


template <class T> inline
void AtomicPointer<T>::unRef()
{
    T* oldptr = setToNull();
    if ( oldptr )
	oldptr->unRef();
}


template <class T> inline
void AtomicPointer<T>::ref() { ((T*) ptr_ )->ref(); }


template <class T> inline
AtomicPointer<T>::operator T*() { return (T*) ptr_.get(); }


template <class T> inline
T* AtomicPointer<T>::operator+=( int b )
{ return ptr_ += b*sizeof(T); }


template <class T> inline
T* AtomicPointer<T>::operator-=(int b)
{ return ptr_ -= b*sizeof(T); }


#define mImplAtomicPointerOperator( func, op, ret ) \
template <class T> inline \
T* AtomicPointer<T>::func \
{ \
    T* old = (T*) ptr_.get(); \
 \
    while ( !ptr_.setIfEqual( (mAtomicPointerType) (op), \
			      (mAtomicPointerType) old ) ) \
	old = (T*) ptr_.get(); \
 \
    return ret; \
}


mImplAtomicPointerOperator( operator++(), old+1, old+1 );
mImplAtomicPointerOperator( operator--(), old-1, old-1 );
mImplAtomicPointerOperator( operator++(int), old+1, old );
mImplAtomicPointerOperator( operator--(int), old-1, old );


} //namespace

#endif
