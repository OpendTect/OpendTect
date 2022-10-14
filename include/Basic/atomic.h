#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "odversion.h"

#ifdef __win__
# include "windows.h"
#endif

# include <atomic>

namespace Threads
{

template <class T>
mClass(Basic) Atomic : public std::atomic<T>
{
public:
				Atomic(T val=0);
				Atomic(const Atomic<T>&);
				~Atomic();

    Atomic<T>&			operator=(T nv);
    Atomic<T>&			operator=(const Atomic<T>&);

    mDeprecated("Use load") inline T get() const
				{ return std::atomic<T>::load(); }

    inline void			setIfLarger(T newval);
				/*!<Sets to newval if newval is larger than
				    current value.*/
    inline void			setIfSmaller(T newval);
				/*!<Sets to newval if newval is smaller than
				    current value. */

    inline bool			setIfValueIs(T curval,T newval,
					     T* actualvalptr = 0);
};


/*!>
 Atomically sets the 'val' only if its value is currently the value of 'curval'.
 If the value in 'val' is identical to the value of 'curval', function will
 change 'val' and return true. Otherwise, it will not change 'val', it
 will return false, and update the value at 'actualvalptr' to the current
 value of 'val'.
 */

mGlobal(Basic) bool atomicSetIfValueIs(volatile int& val,int curval,int newval,
					int* actualvalptr);


/*!
 \brief Atomic instantiated with a pointer. The class really only handles the
 casting from a void* to a T*.
 */

template <class T>
mClass(Basic) AtomicPointer : public std::atomic<T*>
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

    bool		operator!() const { return !std::atomic<T*>::load();  }
			operator bool() const { return std::atomic<T*>::load();}

    T*			operator->() { return std::atomic<T*>::load(); }
    const T*		operator->() const { return std::atomic<T*>::load(); }

    AtomicPointer<T>&	operator=(T* ptr);
    AtomicPointer<T>&	operator=(const AtomicPointer<T>&);

    inline T*	setToNull();
    /*!<Returns the last value of the ptr. */

    inline	AtomicPointer(const AtomicPointer<T>&) = delete;
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
		~SpinLock();
		mOD_DisableCopy(SpinLock)

    void	lock();
    void	unLock();
    bool	tryLock();

protected:
    Atomic<void*>		lockingthread_;
				/*!<0 if unlocked, otherwise set to locking
				      thread */
    Atomic<int>			count_;
    bool			recursive_;

public:
    int				count() const	{ return count_; }
				/*!<Only for debugging.  */
};


/*!
 \brief Is an alternative to ReadWriteLock. It is a lock which causes a thread
 trying to acquire it to simply wait in a loop ("spin") while repeatedly
 checking if the lock is available. Because they avoid overhead from operating
 system process re-scheduling or context switching, spinlocks are efficient if
 threads are only likely to be blocked for a short period.
 */

mExpClass(Basic) SpinRWLock
{
public:
			SpinRWLock();
			/*\If recursive, mutex can be locked
			 multiple times from the same thread without deadlock.
			 It will be unlock when unLock has been called the same
			 number of times as lock(). */
			~SpinRWLock();
			mOD_DisableCopy(SpinRWLock)


    void		readLock();
    void		readUnlock();
    void		writeLock();
    void		writeUnlock();

protected:
    Atomic<int>		count_;

public:
    int			count() const	{ return count_; }
			/*!<Only for debugging.  */
};


//Implementations


template <class T>
void Atomic<T>::setIfLarger( T newval )
{
    T oldval = *this;
    do
    {
	if ( oldval>=newval )
	    return;
    }
    while ( !setIfValueIs( oldval, newval, &oldval ) );
}

template <class T>
void Atomic<T>::setIfSmaller( T newval )
{
    T oldval = *this;
    do
    {
	if ( oldval<=newval )
	    return;
    }
    while ( !setIfValueIs( oldval, newval, &oldval ) );
}


template <class T> inline
Atomic<T>::Atomic( T val )
    : std::atomic<T>( val )
{}


template <class T> inline
Atomic<T>::Atomic( const Atomic<T>& val )
    : std::atomic<T>( sCast(T,val) )
{}


template <class T> inline
Threads::Atomic<T>::~Atomic()
{}


template <class T> inline
Threads::Atomic<T>& Threads::Atomic<T>::operator=(T nv)
{ std::atomic<T>::store(nv); return *this; }


template <class T> inline
Threads::Atomic<T>& Threads::Atomic<T>::operator=(const Threads::Atomic<T>& nv)
{ std::atomic<T>::store(nv); return *this; }


template <class T> inline
bool Atomic<T>::setIfValueIs( T curval, T newval, T* actualvalptr )
{
    T presumedval = curval;
    if ( !this->compare_exchange_strong(presumedval,newval) )
    {
	if ( actualvalptr )
	    *actualvalptr = presumedval;
	return false;
    }

    return true;
}



/* AtomicPointer implementations. */
template <class T> inline
AtomicPointer<T>::AtomicPointer(T* newptr )
    : std::atomic<T*>( newptr )
{}


template <class T> inline
bool AtomicPointer<T>::setIfEqual( const T* oldptr, T* newptr )
{
    T* curval = const_cast<T*>( oldptr );
    return this->compare_exchange_strong( curval, newptr );
}


template <class T> inline
T* AtomicPointer<T>::setToNull()
{
    return std::atomic<T*>::exchange( 0 );
}


template <class T> inline
AtomicPointer<T>& AtomicPointer<T>::operator=(T* ptr)
{
    std::atomic<T*>::store( ptr );
    return *this;
}


template <class T> inline
AtomicPointer<T>& AtomicPointer<T>::operator=(const AtomicPointer<T>& ptr)
{
    return operator=( sCast(T*,ptr) );
}

} // namespace Threads


typedef Threads::Atomic<DirtyCountType>     DirtyCounter;
