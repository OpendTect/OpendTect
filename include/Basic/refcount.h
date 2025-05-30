#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "atomic.h"
#include "manobjectset.h"
#include "thread.h"

template <class T> class WeakPtr;
template <class T> class RefMan;
template <class T> class ConstRefMan;

/*!\ingroup Basic
\page refcount Reference Counting
   Reference counter is an integer that tracks how many references have been
   made to a class. When a reference-counted object is created, the reference
   count is 0. When the ref() function is called, it is incremented, and when
   unRef() is called it is decremented. If it then reaches 0, the object is
   deleted.

\section example Example usage
  A refcount class is set up by:
  \code
  class A : public ReferencedObject
  {
     public:
        //Your class stuff
  };
  \endcode

  This gives access to a number of class variables and functions:
  \code
  public:
      void			ref() const;
      void			unRef() const;

      void			unRefNoDelete() const;
   \endcode

  You should ensure that the destructor is not public in your class.

\section unrefnodel unRefNoDelete
  The unRefNoDelete() is only used when you want to bring the class back to the
  original state of refcount==0. Such an example may be a static create
  function:

  \code
  A* createA()
  {
      A* res = new A;
      res->ref();		//refcount goes to 1
      fillAWithData( res );	//May do several refs, unrefs
      res->unRefNoDelete();	//refcount goes back to 0
      return res;
  }
  \endcode

\section sets ObjectSet with reference counted objects
  ObjectSets with ref-counted objects can be modified by either:
  \code
  ObjectSet<RefCountClass> set:

  deepRef( set );
  deepUnRefNoDelete(set);

  deepRef( set );
  deepUnRef(set);

  \endcode

\section smartptr Smart pointers ot reference counted objects.
  A pointer management is handled by the class RefMan, which has the same usage
  as PtrMan.

\section localvar Reference counted objects on the stack
  Reference counted object cannot be used directly on the stack, as
  they have no public destructor. Instead, use the RefMan<A>:

  \code
      RefMan<A> variable = new A;
  \endcode

  Further, there are Observation pointers that can observe your ref-counted
  objects.

  \code
     A* variable = new A;
     variable->ref();

     WeakPtr<A> ptr = variable; //ptr is set
     variable->unRef();        //ptr becomes null
  \endcode

*/


namespace RefCount
{
class WeakPtrBase;

/*! Actual implementation of the reference counting. Normally not used by
    application developers. */

mExpClass(Basic) Counter
{
public:

    void		ref();
    bool		tryRef();
			/*!<Refs if not invalid. Note that you have to have
			    guarantees that object is not dead. *. */

    bool		unRef();
			/*!<Unref to zero will set it to an deleted state,
			 and return true. */

    void		unRefDontInvalidate();
			//!<Will allow it to go to zero

    od_int32		count() const { return count_.load(); }
    bool		refIfReffed();
			//!<Don't use in production, for debugging

    void		clearAllObservers();
    void		addObserver(WeakPtrBase* obj);
    void		removeObserver(WeakPtrBase* obj);

			Counter();
			Counter(const Counter& a);

    static od_int32	cInvalidRefCount();
    static od_int32	cStartRefCount();

private:

    ObjectSet<WeakPtrBase>	observers_;
    Threads::SpinLock		observerslock_;

    Threads::Atomic<od_int32>	count_;
};

/*!Base class for reference counted object. Inhereit and refcounting will be
   enabled. Ensure to make your destructor protected to enforce correct
   usage. */

mExpClass(Basic) Referenced
{
public:

    void		ref() const;
    void		unRef() const;
    void		unRefNoDelete() const;

protected:

			Referenced()			{}
			Referenced(const Referenced&);
    Referenced&		operator =(const Referenced&);
    virtual		~Referenced();

private:

    friend class	WeakPtrBase;
    virtual void	refNotify() const		{}
    virtual void	unRefNotify() const		{}
    virtual void	unRefNoDeleteNotify() const	{}
    virtual void	prepareForDelete()		{}

    mutable Counter	refcount_;

public:

    int			nrRefs() const;
				//!<Only for expert use
    bool		refIfReffed() const;
				//!<Don't use in production, for debugging
    bool		tryRef() const;
				//!<Not for normal use. May become private

    void		addObserver(WeakPtrBase* obs);
				//!<Not for normal use. May become private
    void		removeObserver(WeakPtrBase* obs);
				//!<Not for normal use. May become private
    static bool		isSane(const Referenced*);
				/*Returns true if this really is a referenced
				  (i.e. has magicnumber set ) */

private:

    const od_uint64	magicnumber_ = 0x123456789abcdef;

};


template <class T>
inline void refIfObjIsReffed( const T* obj )
{
    if ( obj )
    {
	mDynamicCastGet( const RefCount::Referenced*, reffed, obj );
	if ( reffed )
	    reffed->ref();
    }
}

template <class T>
inline void unRefIfObjIsReffed( const T* obj )
{
    if ( obj )
    {
	mDynamicCastGet( const RefCount::Referenced*, reffed, obj );
	if ( reffed )
	    reffed->unRef();
    }
}

template <class T> inline void refIfObjIsReffed( const T& obj )
{ refIfObjIsReffed( &obj ); }
template <class T> inline void unRefIfObjIsReffed( const T& obj )
{ unRefIfObjIsReffed( &obj ); }



mExpClass(Basic) WeakPtrBase
{
public:

    typedef Threads::SpinLock	LockType;

			operator bool() const;
    bool		operator!() const;
    bool		operator==( const WeakPtrBase& oth ) const
			{ return ptr_==oth.ptr_; }
    bool		operator!=( const WeakPtrBase& oth ) const
			{ return ptr_!=oth.ptr_; }

protected:

			WeakPtrBase();
    void		set(Referenced*);

    friend class	Counter;

    void		clearPtr();
    mutable LockType	lock_;
    Referenced*		ptr_;

};


mExpClass(Basic) WeakPtrSetBase
{
public:

    mExpClass(Basic) CleanupBlocker
    {
    public:
			CleanupBlocker( WeakPtrSetBase& base )
			    : base_( base )	{ base_.blockCleanup(); }

			~CleanupBlocker()	{ base_.unblockCleanup(); }
    private:

        WeakPtrSetBase&	base_;

    };

protected:

    Threads::Atomic<int> blockcleanup_	= 0;

private:

    friend class	CleanupBlocker;
    void		blockCleanup();
    void		unblockCleanup();

};

} // namespace RefCount

using ReferencedObject = RefCount::Referenced;


/*!Observes a reference counted object. If you wish to use the pointer,
   you have to obtain it using get().
*/
template <class T>
mClass(Basic) WeakPtr : public RefCount::WeakPtrBase
{
public:

			WeakPtr( RefCount::Referenced* p =nullptr ) { set(p); }
			WeakPtr( const WeakPtr<T>& p ) : WeakPtr<T>(p.ptr_) {}
			WeakPtr( RefMan<T>& p ) : WeakPtr<T>(p.ptr())	{}
			~WeakPtr() { set( nullptr ); }

    inline WeakPtr<T>&	operator=(const WeakPtr<T>& p);
    RefMan<T>&		operator=( RefMan<T>& p )
			{ set( p.ptr() ); return p; }
    T*			operator=( T* p )
			{ set(p); return p; }

    RefMan<T>		get() const;

};


//A collection of weak pointers

template <class T>
mClass(Basic) WeakPtrSet : public RefCount::WeakPtrSetBase
{
public:

    typedef Threads::SpinLock		LockType;
    typedef int				idx_type;
    typedef int				size_type;

    void		operator+=(const WeakPtr<T>&);
    void		operator+=(RefMan<T>&);
			//Returns if added (i.e. not duplicate)
    size_type		size() const;
    bool		validIdx(idx_type) const;
    void		erase();
    void		cleanupNullPtrs();
    void		removeRange(idx_type from,idx_type to);
    void		removeSingle(idx_type,bool keep_order=true);
    RefMan<T>		operator[](idx_type);
    ConstRefMan<T>	operator[](idx_type) const;

    void		add(const WeakPtr<T>&);
    void		add(RefMan<T>&);
    bool		addIfNew(const WeakPtr<T>&);
			//Returns if added (i.e. not duplicate)
    bool		addIfNew(RefMan<T>&);
			//Returns if added (i.e. not duplicate)



    idx_type		indexOf(const T*) const;

private:

    mutable LockType	lock_;
    ManagedObjectSet<WeakPtr<T> >	ptrs_;

};


//! Reference class pointer. Works for null pointers.
inline void refPtr( const RefCount::Referenced* ptr )
{
    if ( ptr )
	ptr->ref();
}

/*! Un-reference class pointer. Works for null pointers. */
inline void unRefPtr( const RefCount::Referenced* ptr )
{
    if ( ptr )
	ptr->unRef();
}

/*! Un-reference class pointer without delete. Works for null pointers. */
inline void unRefNoDeletePtr( const RefCount::Referenced* ptr )
{
    if ( ptr )
	ptr->unRefNoDelete();
}

//!Un-reference class pointer, and set it to zero. Works for null-pointers.
template <class T> inline
void unRefAndNullPtr( T*& ptr )
{
    unRefPtr( static_cast<RefCount::Referenced*>( ptr ) );
    ptr = nullptr;
}

template <class T> inline
void unRefAndNullPtr( const T*& ptr )
{
    unRefPtr( static_cast<const RefCount::Referenced*>( ptr ) );
    ptr = nullptr;
}

template <class T> inline
void unRefAndZeroPtr( T*& ptr )
{ unRefAndNullPtr( ptr ); }

template <class T> inline
void unRefAndZeroPtr( const T*& ptr )
{ unRefAndNullPtr( ptr ); }

#define mDefineRefUnrefObjectSetFn( fn, op, extra ) \
template <class T> \
inline void fn( ObjectSet<T>& os ) \
{ \
    for ( auto* obj : os ) \
	op( obj ); \
    extra; \
}

mDefineRefUnrefObjectSetFn( deepUnRef, unRefPtr, os.plainErase() )
mDefineRefUnrefObjectSetFn( deepUnRefNoDelete, unRefNoDeletePtr,
			    os.plainErase() )
mDefineRefUnrefObjectSetFn( deepRef, refPtr, )


/*! Actual implementation of the reference counting. Normally not used by
    application developers. Use mRefCountImpl marcro instead. */

mClass(Basic) ReferenceCounter
{
public:
    inline void		ref();
    inline bool		unRef();
			/*!<Unref to zero will set it to an deleted state,
			 and return true. */

    inline void		unRefDontInvalidate();
			//!<Will allow it to go to zero

    od_int32		count() const { return count_.load(); }
    inline bool		refIfReffed();
			//!<Don't use in production, for debugging

    static od_int32	cInvalidRefCount()
			{ return RefCount::Counter::cInvalidRefCount(); }

private:

    Threads::Atomic<od_int32>	count_;
};


#ifdef __win__
# define mDeclareCounters	od_int32 oldcount = count_.load(), newcount = 0
#else
# define mDeclareCounters	od_int32 oldcount = count_.load(), newcount;
#endif

inline void ReferenceCounter::ref()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==cInvalidRefCount() )
	{
	    pErrMsg("Invalid ref");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //To fool unitialized code warning
#else
	    newcount = 1; //Hoping for the best
#endif
	}
	else
	{
	    newcount = oldcount+1;
	}

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );
}


inline bool ReferenceCounter::unRef()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==cInvalidRefCount() )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //To fool unitialized code warning
#else
	    return false;
#endif
	}
	else if ( oldcount==1 )
	    newcount = cInvalidRefCount();
	else
	    newcount = oldcount-1;

    } while ( !count_.setIfValueIs(oldcount,newcount, &oldcount ) );

    return newcount==cInvalidRefCount();
}


inline bool ReferenceCounter::refIfReffed()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==cInvalidRefCount() )
	{
	    pErrMsg("Invalid ref");
#ifdef __debug__
	    DBG::forceCrash(false);
#else
	    return false; //Hoping for the best
#endif
	}
	else if ( !oldcount )
	    return false;

	newcount = oldcount+1;

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );

    return true;
}


inline void ReferenceCounter::unRefDontInvalidate()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==cInvalidRefCount() )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //Fool the unitialized warning
#else
	    newcount = 0; //Hope for the best
#endif
	}
	else
	    newcount = oldcount-1;

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );
}

#undef mDeclareCounters


//Implementations and legacy stuff below

template <class T>
WeakPtr<T>& WeakPtr<T>::operator=( const WeakPtr<T>& wp )
{
    RefMan<T> ptr = wp.get();
    ptr.setNoDelete( true );
    set( ptr.ptr() );
    return *this;
}


template <class T>
RefMan<T> WeakPtr<T>::get() const
{
    RefMan<T> res = 0;
    if ( ptr_ && ptr_->tryRef() )
    {
	//reffed once through tryRef
	res = (T*)ptr_;

	//unref the ref from tryRef
	ptr_->unRef();
    }

    return res;
}


template <class T> inline
void WeakPtrSet<T>::operator+=( RefMan<T>& toadd )
{
    add( toadd );
}


template <class T> inline
bool WeakPtrSet<T>::addIfNew( RefMan<T>& toadd )
{
    T* ptr = toadd.ptr();
    return WeakPtrSet<T>::addIfNew( WeakPtr<T>(ptr) );
}


template <class T> inline
void WeakPtrSet<T>::add( RefMan<T>& toadd )
{
    T* ptr = toadd.ptr();
    WeakPtrSet<T>::add( WeakPtr<T>(ptr) );
}


template <class T> inline
void WeakPtrSet<T>::cleanupNullPtrs()
{
    lock_.lock();

    const bool docleanup = blockcleanup_.setIfValueIs( 0, -1, nullptr );
    for ( int idx=ptrs_.size()-1; idx>=0; idx-- )
    {
	const WeakPtr<T>& ptr = *ptrs_[idx];
	if ( docleanup && !ptr )
	    ptrs_.removeSingle( idx );
    }

    if ( docleanup )
	blockcleanup_ = 0;

    lock_.unLock();
}


template <class T> inline
bool WeakPtrSet<T>::addIfNew( const WeakPtr<T>& toadd )
{
    lock_.lock();

    const bool docleanup = blockcleanup_.setIfValueIs( 0, -1, nullptr );
    for ( int idx=ptrs_.size()-1; idx>=0; idx-- )
    {
	const WeakPtr<T>& ptr = *ptrs_[idx];
	if ( docleanup && !ptr )
	    { ptrs_.removeSingle( idx ); continue; }

	if ( ptr==toadd )
	{
	    if ( docleanup )
		blockcleanup_ = 0;
	    lock_.unLock();
	    return false;
	}
    }

    if ( docleanup )
	blockcleanup_ = 0;

    ptrs_ += new WeakPtr<T>( toadd );
    lock_.unLock();

    return true;
}


template <class T> inline
void WeakPtrSet<T>::operator+=( const WeakPtr<T>& toadd )
{
    add( toadd );
}


template <class T> inline
void WeakPtrSet<T>::add( const WeakPtr<T>& toadd )
{
    lock_.lock();
    ptrs_ += new WeakPtr<T>( toadd );
    lock_.unLock();
}


template <class T> inline
int WeakPtrSet<T>::size() const
{
    lock_.lock();
    const int res = ptrs_.size();
    lock_.unLock();
    return res;
}


template <class T> inline
bool WeakPtrSet<T>::validIdx( int idx ) const
{
    lock_.lock();
    const bool res = ptrs_.validIdx( idx );
    lock_.unLock();
    return res;
}


template <class T> inline
void WeakPtrSet<T>::erase()
{
    lock_.lock();
    ptrs_.erase();
    lock_.unLock();
}


template <class T> inline
void WeakPtrSet<T>::removeRange( int from, int to )
{
    lock_.lock();
    ptrs_.removeRange( from, to );
    lock_.unLock();
}


template <class T> inline
void WeakPtrSet<T>::removeSingle( int idx, bool keep_order )
{
    lock_.lock();
    if ( ptrs_.validIdx(idx) )
	ptrs_.removeSingle( idx, keep_order );

    lock_.unLock();
}


template <class T> inline
RefMan<T> WeakPtrSet<T>::operator[]( int idx )
{
    lock_.lock();
    RefMan<T> res = ptrs_.validIdx(idx) ? ptrs_[idx]->get()
					: RefMan<T>( nullptr );
    lock_.unLock();
    return res;
}


template <class T> inline
ConstRefMan<T> WeakPtrSet<T>::operator[]( int idx ) const
{
    lock_.lock();
    ConstRefMan<T> res = ptrs_.validIdx(idx) ? ptrs_[idx]->get() :
					       ConstRefMan<T>( nullptr );
    lock_.unLock();
    return res;
}


template <class T> inline
int WeakPtrSet<T>::indexOf( const T* p ) const
{
    lock_.lock();
    int res = -1;
    for ( int idx=0; idx<ptrs_.size(); idx++ )
    {
	if ( ptrs_[idx]->get().ptr()==p )
	{
	    res = idx;
	    break;
	}
    }
    lock_.unLock();
    return res;
}


/*!ObjectSet for reference counted objects. All members are referenced
   once when added to the set, and unreffed when removed from the set.
*/


template <class T>
mClass(Basic) RefObjectSet : public ManagedObjectSetBase<T>
{
public:

    typedef typename ObjectSet<T>::size_type	size_type;
    typedef typename ObjectSet<T>::idx_type	idx_type;

				RefObjectSet();
				RefObjectSet(const RefObjectSet<T>&);
				RefObjectSet(const ObjectSet<T>&);
    RefObjectSet*		clone() const override
				{ return new RefObjectSet(*this); }

    RefObjectSet<T>&		operator=(const ObjectSet<T>&);
    inline T*			replace(idx_type,T*) override;
    inline void			insertAt(T*,idx_type) override;

protected:

    ObjectSet<T>&		doAdd(T*) override;
    static void			unRef( T* ptr ) { unRefPtr(ptr); }

};


template <class T> inline
RefObjectSet<T>::RefObjectSet()
    : ManagedObjectSetBase<T>( unRef )
{}


template <class T> inline
RefObjectSet<T>::RefObjectSet( const ObjectSet<T>& os )
    : ManagedObjectSetBase<T>( unRef )
{
    *this = os;
}


template <class T> inline
RefObjectSet<T>::RefObjectSet( const RefObjectSet<T>& os )
    : ManagedObjectSetBase<T>( unRef )
{
    *this = os;
}


template <class T> inline
RefObjectSet<T>& RefObjectSet<T>::operator =(const ObjectSet<T>& os)
{
    ObjectSet<T>::operator=(os);
    return *this;
}


template <class T> inline
T* RefObjectSet<T>::replace( idx_type vidx, T *ptr )
{
    refPtr( ptr );
    return ManagedObjectSetBase<T>::replace( vidx, ptr );
}


template <class T> inline
void RefObjectSet<T>::insertAt( T *ptr, idx_type vidx )
{
    refPtr( ptr );
    ManagedObjectSetBase<T>::insertAt( ptr, vidx );
}


template <class T> inline
ObjectSet<T>& RefObjectSet<T>::doAdd( T *ptr )
{
    refPtr( ptr );
    return ObjectSet<T>::doAdd(ptr);
}
