#ifndef atomic_h
#define atomic_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "commondefs.h"
#include "plftypes.h"

#ifdef __cpp11__
# define __STDATOMICS__
# include <atomic>
#else
# ifdef __win__
#  define __WINATOMICS__
#  include <Windows.h>
#  include "thread.h"
# else
#  define __GCCATOMICS__
# endif
#endif

namespace Threads {

class Mutex;

template <class T>
mClass(Basic) Atomic
{
public:
    		Atomic(T val=0);
			~Atomic();

		operator T() const { return get(); }
    T		get() const;
    
    T		operator=(T v);

    inline T	operator+=(T);
    inline T	operator-=(T);
    inline T	operator++();
    inline T	operator--();
    inline T	operator++(int);
    inline T	operator--(int);

    inline T	exchange(T newval);
    		/*!<Returns old value. */

    inline bool	setIfValueIs(T& curval,T newval);
    /*!<Sets the 'val_' only if its value is currently the value of 'curval'.
     If the value in 'val_' is identical to the value of 'curval', function will
     change 'val_' and return true. Otherwise, it will not change 'val_', it
     will return false, and update 'curval' to the current value of 'val_'.
    */
      
private:
    			Atomic( const Atomic<T>& )	{}
    
#ifdef __STDATOMICS__
    std::atomic<T>	val_;
#elif (defined __WINATOMICS__)
    volatile T		values_[8];
    volatile T*		valptr_;

    Mutex*		lock_;

#else
    volatile T		val_;
#endif
};



/*!>
 Atomically sets the 'val' only if its value is currently the value of 'curval'.
 If the value in 'val' is identical to the value of 'curval', function will
 change 'val' and return true. Otherwise, it will not change 'val', it
 will return false, and update 'curval' to the current value of 'val'.
 */


inline bool atomicSetIfValueIs( volatile int& val, int& curval, int newval )
{
# ifdef __win__
    const int oldval =InterlockedCompareExchange( (volatile long*) &val, newval,
                                                 curval );
    if ( oldval!=curval )
    {
        curval = oldval;
        return false;
    }

    return true;

# else
    const int old = __sync_val_compare_and_swap( &val, curval, newval );
    if ( old!=curval )
    {
        curval = old;
        return false;
    }

    return true;
#endif
}


#ifdef __win__
#define mAtomicPointerType long long
#else
#define mAtomicPointerType T*
#endif


/*!
 \brief Atomic instantiated with a pointer. The class really only handles the
 casting from a void* to a T*.
 */

template <class T>
mClass(Basic) AtomicPointer
{
public:
    inline	AtomicPointer(T* newptr = 0);
    
    inline bool	setIfEqual(const T* curptr, T* newptr);
    /*!<Sets the 'ptr_' only if its pointer is identical to the pointer in
       'curptr'.
        If the pointer in 'ptr_' is identical to the pointer of 'curptr',
        function will change 'ptr_' and return true. Otherwise, it will not
        change 'ptr_', it will return false, and update 'curptr' to the current
        value of 'ptr'.
 */

    inline T*	setToNull();
    /*!<Returns the last value of the ptr. */
    
    T*		exchange(T* newptr);
    //*!<\returns old value
    
    inline	operator T*() const;
    
    inline T*	operator+=(int);
    inline T*	operator-=(int);
    inline T*	operator++();
    inline T*	operator--();
    inline T*	operator++(int);
    inline T*	operator--(int);
    
protected:
    
    
    Atomic<mAtomicPointerType>	ptr_;
};


/*!
\brief Is an alternative to Mutex. It is a lock which causes a thread trying to
 acquire it to simply wait in a loop ("spin") while repeatedly checking if the
 lock is available. Because they avoid overhead from operating system process
 re-scheduling or context switching, spinlocks are efficient if threads are only
 likely to be blocked for a short period.
*/

mExpClass(Basic) SpinLock
{
public:
		SpinLock(bool recursive = false);
		/*\If recursive, mutex can be locked
		   multiple times from the same thread without deadlock.
		   It will be unlock when unLock has been called the same
		   number of times as lock(). */
		SpinLock(const SpinLock&);
		~SpinLock();

    SpinLock&	operator=(const SpinLock& b)
		{ recursive_ = b.recursive_; return *this; }

    void	lock();
    void	unLock();
    bool	tryLock();

protected:
    AtomicPointer<const void>	lockingthread_;
    				/*!<0 if unlocked, otherwise set to locking
				      thread */
    int				count_;
    bool			recursive_;

public:
    int				count() const 	{ return count_; }
				/*!<Only for debugging.  */
};


//Implementations

#ifdef __STDATOMICS__
template <class T> inline
Atomic<T>::Atomic( T val )
    : val_( val )
{}


template <class T> inline
Atomic<T>::Atomic( const Atomic<T>& val )
    : val_( val.get() )
{}


template <class T> inline
T Atomic<T>::operator += (T b)
{
    return val_ += b;
}


template <class T> inline
T Atomic<T>::operator -= (T b)
{
    return val_ -= b;
}


template <class T> inline
T Atomic<T>::operator ++()
{
    return ++val_;
}


template <class T> inline
T Atomic<T>::operator -- ()
{
    return --val_;
}


template <class T> inline
T Atomic<T>::operator ++(int)
{
    return val_++;
}


template <class T> inline
T Atomic<T>::operator -- (int)
{
    return val_--;
}


template <class T> inline
bool Atomic<T>::setIfValueIs( T& curval, T newval )
{
    return val_.compare_exchange_strong( curval, newval );
}


template <class T> inline
T Atomic<T>::exchange( T newval )
{
    return val_.exchange( newval );
}


#endif  //STDATOMICS

#ifdef __GCCATOMICS__
template <class T> inline
Atomic<T>::Atomic( T val )
    : val_( val )
{}
    
    
template <class T> inline
Atomic<T>::~Atomic<T>()
{}
    

template <class T> inline
T Atomic<T>::get() const
{
    return val_;
}

  
template <class T> inline
T Atomic<T>::operator=( T val )
{
    val_ = val;
    return val_;
}


template <class T> inline
T Atomic<T>::operator += (T b)
{
    return __sync_add_and_fetch(&val_, b);
}


template <class T> inline
T Atomic<T>::operator -= (T b)
{
    return __sync_sub_and_fetch(&val_, b);
}


template <class T> inline
T Atomic<T>::operator ++()
{
    return __sync_add_and_fetch(&val_, 1);
}


template <class T> inline
T Atomic<T>::operator -- ()
{
    return __sync_sub_and_fetch(&val_, 1);
}


template <class T> inline
T Atomic<T>::operator ++(int)
{
    return __sync_fetch_and_add(&val_, 1);
}


template <class T> inline
T Atomic<T>::operator -- (int)
{
    return __sync_fetch_and_sub(&val_, 1);
}


template <class T> inline
bool Atomic<T>::setIfValueIs(T& curval, T newval )
{
    const T old = __sync_val_compare_and_swap( &val_, curval, newval );
    if ( old!=curval )
    {
        curval = old;
        return false;
    }

    return true;
}


template <> inline
bool Atomic<int>::setIfValueIs( int& curval, int newval )
{
    return atomicSetIfValueIs( val_, curval, newval );
}


template <class T> inline
T Atomic<T>::exchange( T newval )
{
    return __sync_lock_test_and_set( &val_, newval );
}
#endif //__GCCATOMICS__



#ifdef __WINATOMICS__


template <class T> inline
Atomic<T>::Atomic( T val )
    : lock_( new Mutex )
    , valptr_( values_ )
{
    *valptr_ = val;
}


template <class T> inline
Atomic<T>::~Atomic()
{
    delete lock_;
}


template <class T> inline
T Atomic<T>::get() const
{
    return *valptr_;
}


template <class T> inline
T Atomic<T>::exchange( T newval )
{
    T curval = *valptr_;
    while ( !setIfValueIs( curval, newval ) )
    {}

    return curval;
}


template <class T> inline
T Atomic<T>::operator=(T val)
{
    *valptr_ = val;
    return *valptr_;
}


template <class T> inline
T Atomic<T>::operator += (T b)
{
    MutexLocker lock( *lock_ );
    return (*valptr_) += b;
}


template <class T> inline
T Atomic<T>::operator -= (T b)
{
    MutexLocker lock( *lock_ );
    return (*valptr_) -= b;
}


template <class T> inline
T Atomic<T>::operator ++()
{
    MutexLocker lock( *lock_ );
    return ++(*valptr_);
}


template <class T> inline
T Atomic<T>::operator -- ()
{
    MutexLocker lock( *lock_ );
    return --(*valptr_);
}


template <class T> inline
T Atomic<T>::operator ++(int)
{
    MutexLocker lock( *lock_ );
    return (*valptr_)++;
}


template <class T> inline
T Atomic<T>::operator -- (int)
{
    MutexLocker lock( *lock_ );
    return (*valptr_)--;
}



template <class T> inline
bool Atomic<T>::setIfValueIs(T& curval, T newval )
{
    MutexLocker lock( *lock_ );
    const bool res = (*valptr_)==curval;
    if ( res )
        (*valptr_) = newval;
    else
        curval = (*valptr_);
    
    return res;
}


#ifdef __win64__

template <> inline
Atomic<long long>::Atomic( long long val )
	: lock_( 0 )
{
    valptr_ = &values_[0];
    while ( ((long long) valptr_) % 64  )
		valptr_++;
    
    *valptr_ = val;
}

	 
template <> inline
bool Atomic<long long>::setIfValueIs(long long& curval, long long newval )
{
    const long long prevval =
    	InterlockedCompareExchange64(valptr_,newval,curval);
    if ( prevval==curval )
	return true;
    curval = prevval;
    return false; 
}
	 
	 
template <> inline
long long Atomic<long long>::operator += (long long b)
{
    return InterlockedAdd64( valptr_, b );
}
	 
	 
template <> inline
long long Atomic<long long>::operator -= (long long b)
{
    return InterlockedAdd64( valptr_, -b );
}
	 
	 
template <> inline
long long Atomic<long long>::operator ++()
{
    return InterlockedIncrement64( valptr_ );
}
	 
	 
template <> inline
long long Atomic<long long>::operator -- ()
{
    return InterlockedDecrement64( valptr_ );
}
	 
	 
template <> inline
long long Atomic<long long>::operator ++(int)
{
    return InterlockedIncrement64( valptr_ )-1;
}
	 
	 
template <> inline
long long Atomic<long long>::exchange(long long newval)
{
    return InterlockedExchange64( valptr_, newval );
}
	 
	 
template <> inline
long long Atomic<long long>::operator -- (int)
{
    return InterlockedDecrement64( valptr_ )+1;
}


# endif //not win32

template <> inline
Atomic<int>::Atomic( int val )
	: lock_( 0 )
{
    valptr_ = &values_[0];
    while ( ((int) valptr_) % 32  )
		valptr_++;
    
    *valptr_ = val;
}
	 

template <> inline
bool Atomic<int>::setIfValueIs( int& curval, int newval )
{
    const int prevval =
    	InterlockedCompareExchange((volatile long*) valptr_,newval,curval);
    if ( prevval==curval )
        return true;
    curval = prevval;
    return false;
}
	 
	 
template <> inline
int Atomic<int>::operator += (int b)
{
    return InterlockedExchangeAdd( (volatile long*) valptr_, b ) + b;
}
	 
	 
template <> inline
int Atomic<int>::operator -= (int b)
{
    return InterlockedExchangeAdd( (volatile long*) valptr_, -b ) -b;
}
	 
	 
template <> inline
int Atomic<int>::operator ++()
{
    return InterlockedIncrement( (volatile long*) valptr_ );
}
	 
	 
template <> inline
int Atomic<int>::operator -- ()
{
    return InterlockedDecrement( (volatile long*) valptr_ );
}
	 
	 
template <> inline
int Atomic<int>::operator ++(int)
{
    return InterlockedIncrement( (volatile long*) valptr_ )-1;
}
	 
	 
template <> inline
int Atomic<int>::exchange(int newval)
{
    return InterlockedExchange( (volatile long*) valptr_, newval );
}
	 
	 
template <> inline
int Atomic<int>::operator -- (int)
{
    return InterlockedDecrement( (volatile long*) valptr_ )+1;
}


template <> inline
Atomic<long>::Atomic( long val )
	: lock_( 0 )
{
    valptr_ = &values_[0];
    while ( ((long long) valptr_) % 32  )
		valptr_++;
    
    *valptr_ = val;
}


template <> inline
bool Atomic<long>::setIfValueIs( long& curval, long newval )
{
    const long prevval =
		InterlockedCompareExchange( valptr_,  newval,  curval );
    if ( prevval==curval  )
        return true;
    curval = prevval;
    return false;
}


template <> inline
long Atomic<long>::operator += (long b)
{
    return  InterlockedExchangeAdd( valptr_, (long) b ) + b;
}


template <> inline
long Atomic<long>::operator -= (long b)
{
    return  InterlockedExchangeAdd( valptr_, -b ) - b;
}


template <> inline
long Atomic<long>::operator ++()
{
    return InterlockedIncrement( valptr_ );
}


template <> inline
long Atomic<long>::operator -- ()
{
    return InterlockedDecrement( valptr_ );
}


template <> inline
long Atomic<long>::operator ++(int)
{
    return InterlockedIncrement( valptr_ )-1;
}


template <> inline
long Atomic<long>::exchange(long newval)
{
    return InterlockedExchange( valptr_, newval );
}


template <> inline
long Atomic<long>::operator -- (int)
{
    return InterlockedDecrement( valptr_ )+1;
}


#endif //__WINATOMICS__

/* AtomicPointer implementations. */
template <class T> inline
AtomicPointer<T>::AtomicPointer(T* newptr )
: ptr_( (mAtomicPointerType) newptr )
{}


template <class T> inline
bool AtomicPointer<T>::setIfEqual( const T* oldptr, T* newptr )
{
    mAtomicPointerType curval = (mAtomicPointerType) oldptr;
    return ptr_.setIfValueIs( curval, (mAtomicPointerType) newptr );
}


template <class T> inline
T* AtomicPointer<T>::exchange( T* newptr )
{
    return (T*) ptr_.exchange( (mAtomicPointerType) newptr );
}


template <class T> inline
T* AtomicPointer<T>::setToNull()
{
    mAtomicPointerType oldptr = (mAtomicPointerType) ptr_.get();
    while ( oldptr && !ptr_.setIfValueIs( oldptr, 0 ) )
    {}
    
    return (T*) oldptr;
}


template <class T> inline
AtomicPointer<T>::operator T*() const { return (T*) ptr_.get(); }


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
    mAtomicPointerType old = (mAtomicPointerType) ptr_.get(); \
 \
    while ( !ptr_.setIfValueIs( old, (mAtomicPointerType) (op) ) ) \
	{} \
 \
    return ret; \
}


mImplAtomicPointerOperator( operator++(), old+1, old+1 );
mImplAtomicPointerOperator( operator--(), old-1, old-1 );
mImplAtomicPointerOperator( operator++(int), old+1, old );
mImplAtomicPointerOperator( operator--(int), old-1, old );


} //namespace

#endif
