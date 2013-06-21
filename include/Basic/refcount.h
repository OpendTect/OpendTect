#ifndef refcount_h
#define refcount_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		13-11-2003
 Contents:	Basic functionality for reference counting
 RCS:		$Id$
________________________________________________________________________

-*/

#include "atomic.h"
#include "objectset.h"
#include "errh.h"


template <class T> class ObjectSet;

#define mInvalidRefCount (-1)


/*!
\ingroup Basic
\page refcount Reference Counting
   Reference counter is an integer that tracks how many references have been
   made to a class. When a reference-counted object is created, the reference
   count is 0. When the ref() function is called, it is incremented, and when
   unRef() is called it is decremented. If it then reaches 0, the object is
   deleted.

\section example Example usage       
  A refcount class is set up by:
  \code
  class A
  {
     mRefCountImpl(A);
     public:
        //Your class stuff
  };
  \endcode

  This expands to a number of class variables and functions:
  \code
  public:
      void			A::ref() const;
      void			A::unRef() const;

      void			A::unRefNoDelete() const;

				//For debugging only Don't use
      bool			A::refIfReffed() const;

				//For debugging only Don't use
      int			A::nrRefs() const;
  private:
    virtual void		A::refNotify() const {}
    virtual void		A::unRefNotify() const {}
    virtual void		A::unRefNoDeleteNotify() const {}
    mutable ReferenceCounter	A::refcount_;
  protected:
    				A::~A();
  \endcode

  The macro will define a protected destructor, so you have to implement one
  (even if it's a dummy {}).

  If you don't want a destructor on your class use the mRefCountImplNoDestructor
  instead:

  \code
  class A
  {
      mRefCountImplNoDestructor(A);
  public:
          //Your class stuff
  };
  \endcode

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
*/

//!\cond
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
private: \
    virtual void		refNotify() const {} \
    virtual void		unRefNotify() const {} \
    virtual void		unRefNoDeleteNotify() const {} \
    mutable ReferenceCounter	refcount_;	\
protected: \
    		DestructorImpl; \
private:

//!\endcond

//!Macro to setup a class with destructor for reference counting
#define mRefCountImpl(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName(), delete this; );

//!Macro to setup a class without destructor for reference counting
#define mRefCountImplNoDestructor(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName() {}, delete this; );

//!Un-reference class pointer, and set it to zero. Works for null-pointers. 
template <class T> inline
void unRefAndZeroPtr( T*& ptr )
{
    if ( !ptr ) return;
    ptr->unRef();
    ptr = 0;
}


/*! Un-reference class pointer. Works for null pointers. */
template <class T> inline
void unRefPtr( const T* ptr )
{
    if ( !ptr ) return;
    ptr->unRef();
}

//! Reference class pointer. Works for null pointers.
template <class T> inline
void refPtr( const T* ptr )
{
    if ( !ptr ) return;
    ptr->ref();
}

mObjectSetApplyToAllFunc( deepUnRef, unRefPtr( os[idx] ), os.plainErase() )
mObjectSetApplyToAllFunc( deepRef, refPtr( os[idx] ), )


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
    
    od_int32		count() const { return count_.get(); }
    			//!<Don't use in production, for debugging
    inline bool		refIfReffed();
    			//!<Don't use in production, for debugging
    
private:
    
    Threads::Atomic<od_int32>	count_;
};


#ifdef __win__
# define mDeclareCounters	od_int32 oldcount = count_.get(), newcount = 0
#else
# define mDeclareCounters    	od_int32 oldcount = count_.get(), newcount;
#endif

inline void ReferenceCounter::ref()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    pErrMsg("Invalid ref");
#ifdef __debug__
	    DBG::forceCrash(false);
#else
	    newcount = 1; //Hoping for the best
#endif
	}
	else
	{
	    newcount = oldcount+1;
	}
	
    } while ( !count_.weakSetIfEqual( newcount, oldcount ) );
}


inline bool ReferenceCounter::unRef()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
#else
	    return false;
#endif
	}
	else if ( oldcount==1 )
	    newcount = mInvalidRefCount;
	else
	    newcount = oldcount-1;
	
    } while ( !count_.weakSetIfEqual(newcount,oldcount ) );
    
    return newcount==mInvalidRefCount;
}


inline bool ReferenceCounter::refIfReffed()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
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
	
    } while ( !count_.weakSetIfEqual( newcount, oldcount ) );
    
    return true;
}


inline void ReferenceCounter::unRefDontInvalidate()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
#else
	    newcount = 0; //Hope for the best
#endif
	}
	else
	    newcount = oldcount-1;
	
    } while ( !count_.weakSetIfEqual(newcount,oldcount ) );
}

#undef mDeclareCounters


#endif
