#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		13-11-2003
 Contents:	Basic functionality for reference counting
________________________________________________________________________

-*/

#include "atomic.h"
#include "objectset.h"
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
    bool		operator==( const WeakPtrBase& r ) const
				{ return ptr_==r.ptr_; }

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


/*!Observes a refereence counted object. If you wish to use the pointer,
   you have to obtain it using get().
*/
template <class T>
mClass(Basic) WeakPtr : public RefCount::WeakPtrBase
{
public:

			WeakPtr(RefCount::Referenced* p = 0) { set(p); }
			WeakPtr(const WeakPtr<T>& p) : WeakPtr<T>( p.ptr_ ) {}
			WeakPtr(RefMan<T>& p) : WeakPtr<T>(p.ptr()) {}
			~WeakPtr() { set( 0 ); }

    inline WeakPtr<T>&	operator=(const WeakPtr<T>& p);
    RefMan<T>&		operator=(RefMan<T>& p)
			{ set(p.ptr()); return p; }
    T*			operator=(T* p)
			{ set(p); return p; }

    RefMan<T>		get() const;

};


//A collection of weak pointers

template <class T>
mClass(Basic) WeakPtrSet : public RefCount::WeakPtrSetBase
{
public:

    typedef Threads::SpinLock		LockType;
    typedef TypeSet<WeakPtr<T> >	SetType;
    typedef int				idx_type;
    typedef int				size_type;

    bool		operator+=(const WeakPtr<T>&);
			//Returns if added (i.e. not duplicate)
    bool		operator+=(RefMan<T>&);
			//Returns if added (i.e. not duplicate)
    size_type		size() const;
    bool		validIdx(idx_type) const;
    void		erase();
    void		removeRange(idx_type from,idx_type to);
    void		removeSingle(idx_type,bool keep_order=true);
    RefMan<T>		operator[](idx_type);
    ConstRefMan<T>	operator[](idx_type) const;

    idx_type		indexOf(T*) const;


private:

    mutable LockType	lock_;
    SetType		ptrs_;

};



#define mRefCountImplWithDestructor(ClassName, DestructorImpl, delfunc ) \
public: \
    void	ref() const \
		{ \
		    refcount_.ref(); \
		    refNotify(); \
		} \
    bool	refIfReffed() const \
		{ \
		    if ( !refcount_.refIfReffed() ) \
			return false; \
		    \
		    refNotify(); \
		    return true; \
		} \
    void	unRef() const \
		{ \
		    unRefNotify(); \
		    if ( refcount_.unRef() ) \
			delfunc; \
		    return; \
		} \
 \
    void	unRefNoDelete() const \
		{ \
		    unRefNoDeleteNotify(); \
		    refcount_.unRefDontInvalidate(); \
		} \
    int		nrRefs() const { return refcount_.count(); } \
 \
    static void refPtr( ClassName* ptr ) \
		{ \
		    if ( ptr ) \
			ptr->ref(); \
		} \
    static void unRefPtr( ClassName* ptr ) \
		{ \
		    if ( ptr ) \
			ptr->unRef(); \
		} \
    static void unRefAndZeroPtr( ClassName*& ptr ) \
		{ \
		    if ( ptr ) \
			ptr->unRef(); \
		    else \
			return; \
		    ptr = nullptr; \
		} \
private: \
    virtual void		refNotify() const {} \
    virtual void		unRefNotify() const {} \
    virtual void		unRefNoDeleteNotify() const {} \
    mutable ReferenceCounter	refcount_;	\
protected: \
		DestructorImpl; \
private:


//!Macro to setup a class with destructor for reference counting
#define mRefCountImpl(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName(), delete this; )

//!Macro to setup a class without destructor for reference counting
#define mRefCountImplNoDestructor(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName() {}, delete this; )


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
void unRefAndZeroPtr( T*& ptr )
{
    unRefPtr( static_cast<RefCount::Referenced*>( ptr ) );
    ptr = nullptr;
}

template <class T> inline
void unRefAndZeroPtr( const T*& ptr )
{
    unRefPtr( static_cast<const RefCount::Referenced*>( ptr ) );
    ptr = nullptr;
}


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
bool WeakPtrSet<T>::operator+=( RefMan<T>& toadd )
{
    T* ptr = toadd.ptr();
    return WeakPtrSet<T>::operator+=( WeakPtr<T>(ptr) );
}


template <class T> inline
bool WeakPtrSet<T>::operator+=( const WeakPtr<T>& toadd )
{
    lock_.lock();

    const bool docleanup = blockcleanup_.setIfValueIs( 0, -1, nullptr );
    for ( int idx=ptrs_.size()-1; idx>=0; idx-- )
    {
	if ( docleanup && !ptrs_[idx] )
	    { ptrs_.removeSingle( idx ); continue; }

	if ( ptrs_[idx]==toadd )
	{
	    if ( docleanup )
		blockcleanup_ = 0;
	    lock_.unLock();
	    return false;
	}
    }

    if ( docleanup )
	blockcleanup_ = 0;

    ptrs_ += toadd;
    lock_.unLock();

    return true;
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
    RefMan<T> res = ptrs_.validIdx(idx) ? ptrs_[idx].get() :
							RefMan<T>( nullptr );
    lock_.unLock();
    return res;
}


template <class T> inline
ConstRefMan<T> WeakPtrSet<T>::operator[]( int idx ) const
{
    lock_.lock();
    ConstRefMan<T> res = ptrs_.validIdx(idx) ? ptrs_[idx].get() :
					       ConstRefMan<T>( nullptr );
    lock_.unLock();
    return res;
}


template <class T> inline
int WeakPtrSet<T>::indexOf( T* p ) const
{
    lock_.lock();
    int res = -1;
    for ( int idx=0; idx<ptrs_.size(); idx++ )
    {
	if ( ptrs_[idx].get().ptr()==p )
	{
	    res = idx;
	    break;
	}
    }
    lock_.unLock();
    return res;
}
