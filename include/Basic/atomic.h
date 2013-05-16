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

    inline bool	strongSetIfEqual(T newval,T expected);
    /*!<Sets the val_ only if value is previously set
     to expected. */
    inline bool	weakSetIfEqual(T newval, T& expected);
    /*!<Sets the val_ only if value is previously set
     to expected. If it fails, current val is set in expected argument.
     This function is more effective than strongSetIfEqual, but may
     return false spuriously, hence it should be used in a loop. */
      
private:
    			Atomic( const Atomic<T>& )	{}
    
#ifdef __STDATOMICS__
    std::atomic<T>	val_;
#elif (defined __WINATOMICS__)
    volatile T		values_[64];
    volatile T*		valptr_;

    Mutex*			lock_;

#else
    volatile T		val_;
#endif
};


#ifdef __win__
#define mAtomicPointerType od_uint64
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
    
    inline bool	setIfEqual(T* newptr,const T* oldptr);
    
    inline void	unRef();
    /*!<Don't be confused, class works for non-ref-counted objects
     as well. Just don't call ref/unRef(); */
    inline void	ref();
    
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
bool Atomic<T>::strongSetIfEqual( T newval, T expected )
{
    return val_.compare_exchange_strong( expected, newval );
}


template <class T> inline
bool Atomic<T>::weakSetIfEqual( T newval, T& expected )
{
    return val_.compare_exchange_weak( expected, newval );
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
bool Atomic<T>::strongSetIfEqual( T newval, T expected )
{
    return __sync_val_compare_and_swap( &val_, expected, newval )==expected;
}


template <class T> inline
bool Atomic<T>::weakSetIfEqual( T newval, T& expected )
{
    const T prevval = __sync_val_compare_and_swap( &val_, expected, newval );
    if ( prevval==expected )
	return true;
    
    expected = prevval;
    return false;
}


template <class T> inline
T Atomic<T>::exchange( T newval )
{
    T expected = val_;
    while ( !weakSetIfEqual( newval, expected ) )
    {}
    
    return expected;
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
    T expected = *valptr_;
    while ( !weakSetIfEqual( newval, expected ) )
    {}

    return expected;
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
bool Atomic<T>::strongSetIfEqual(T newval, T expected )
{
    MutexLocker lock( *lock_ );
    const bool res = (*valptr_)==expected;
    if ( res )
		(*valptr_) = newval;
    
    return res;
}


template <class T> inline
bool Atomic<T>::weakSetIfEqual(T newval, T& expected )
{
    MutexLocker lock( *lock_ );
    const bool res = (*valptr_)==expected;
    if ( res )
		(*valptr_) = newval;
    else
		expected = (*valptr_);
    
    return res;
}


#ifdef __win64__

template <> inline
Atomic<od_int64>::Atomic( od_int64 val )
	: lock_( 0 )
{
    valptr_ = &values_[0];
    while ( ((long) valptr_) % 64  )
		valptr_++;
    
    *valptr_ = val;
}
	 
	 
template <> inline
bool Atomic<od_int64>::strongSetIfEqual(od_int64 newval, od_int64 expected )
{
    return InterlockedCompareExchange64( valptr_,newval,expected)==expected;
}
	 
	 
template <> inline
bool Atomic<od_int64>::weakSetIfEqual(od_int64 newval, od_int64& expected )
{
    const od_int64 prevval =
    	InterlockedCompareExchange64(valptr_,newval,expected);
    if ( prevval==expected )
		return true;
    expected = prevval;
    return false; 
}
	 
	 
template <> inline
od_int64 Atomic<od_int64>::operator += (od_int64 b)
{
    return InterlockedAdd64( valptr_, b );
}
	 
	 
template <> inline
od_int64 Atomic<od_int64>::operator -= (od_int64 b)
{
    return InterlockedAdd64( valptr_, -b );
}
	 
	 
template <> inline
od_int64 Atomic<od_int64>::operator ++()
{
    return InterlockedIncrement64( valptr_ );
}
	 
	 
template <> inline
od_int64 Atomic<od_int64>::operator -- ()
{
    return InterlockedDecrement64( valptr_ );
}
	 
	 
template <> inline
od_int64 Atomic<od_int64>::operator ++(int)
{
    return InterlockedIncrement64( valptr_ )-1;
}
	 
	 
template <> inline
od_int64 Atomic<od_int64>::exchange(od_int64 newval)
{
    return InterlockedExchange64( valptr_, newval );
}
	 
	 
template <> inline
od_int64 Atomic<od_int64>::operator -- (int)
{
    return InterlockedDecrement64( valptr_ )+1;
}

# endif //not win32

template <> inline
Atomic<od_int32>::Atomic( od_int32 val )
	: lock_( 0 )
{
    valptr_ = &values_[0];
    while ( ((long) valptr_) % 32  )
		valptr_++;
    
    *valptr_ = val;
}


template <> inline
bool Atomic<od_int32>::strongSetIfEqual(od_int32 newval, od_int32 expected )
{
    return InterlockedCompareExchange( (volatile long*) valptr_, newval, expected)==expected;
}


template <> inline
bool Atomic<od_int32>::weakSetIfEqual(od_int32 newval, od_int32& expected )
{
    const od_int32 prevval =
		InterlockedCompareExchange( (volatile long*) valptr_,  newval,  expected);
    if ( prevval==expected )
		return true;
    expected = prevval;
    return false;
}


template <> inline
od_int32 Atomic<od_int32>::operator += (od_int32 b)
{
    return InterlockedAdd( (volatile long*) valptr_, (long) b );
}


template <> inline
od_int32 Atomic<od_int32>::operator -= (od_int32 b)
{
    return InterlockedAdd( (volatile long*) valptr_, -b );
}


template <> inline
od_int32 Atomic<od_int32>::operator ++()
{
    return InterlockedIncrement( (volatile long*) valptr_ );
}


template <> inline
od_int32 Atomic<od_int32>::operator -- ()
{
    return InterlockedDecrement( (volatile long*) valptr_ );
}


template <> inline
od_int32 Atomic<od_int32>::operator ++(int)
{
    return InterlockedIncrement( (volatile long*) valptr_ )-1;
}


template <> inline
od_int32 Atomic<od_int32>::exchange(od_int32 newval)
{
    return InterlockedExchange( (volatile long*) valptr_, newval );
}


template <> inline
od_int32 Atomic<od_int32>::operator -- (int)
{
    return InterlockedDecrement( (volatile long*) valptr_ )+1;
}


#endif //__WINATOMICS__

/* AtomicPointer implementations. */
template <class T> inline
AtomicPointer<T>::AtomicPointer(T* newptr )
: ptr_( (mAtomicPointerType) newptr )
{}


template <class T> inline
bool AtomicPointer<T>::setIfEqual( T* newptr, const T* oldptr )
{
    return ptr_.strongSetIfEqual( (mAtomicPointerType) newptr,
				 (mAtomicPointerType) oldptr );
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
    while ( oldptr && !ptr_.weakSetIfEqual( 0, oldptr ) )
    {}
    
    return (T*) oldptr;
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
    T* old = (T*) ptr_.get(); \
 \
    while ( !ptr_.strongSetIfEqual( (mAtomicPointerType) (op), \
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

