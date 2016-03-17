#ifndef refcount_h
#define refcount_h

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

template <class T> class ObsPtr;
template <class T> class RefMan;


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
  class A : public RefCount::Referenced
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

     ObsPtr<A> ptr = variable; //ptr is set
     variable->unRef();        //ptr becomes null
  \endcode

*/

//!\cond
namespace RefCount
{
class ObsPtrBase;

/*! Actual implementation of the reference counting. Normally not used by
    application developers.  */

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

    od_int32		count() const { return count_.get(); }
    bool		refIfReffed();
			//!<Don't use in production, for debugging

    void		clearAllObservers();
    void		addObserver(ObsPtrBase* obj);
    void		removeObserver(ObsPtrBase* obj);

			Counter() : count_( 0 ) {}
                        Counter(const Counter& a) : count_( 0 ) {}

    static od_int32		cInvalidRefCount();

private:
    ObjectSet<ObsPtrBase>	observers_;
    Threads::SpinLock		observerslock_;

    Threads::Atomic<od_int32>	count_;
};

/*!Base class for reference counted object. Inhereit and refcounting will be
   enabled. Ensure to make your destructor protected to enforce correct
   usage. */

mExpClass(Basic) Referenced
{
public:
    void			ref() const;
    void			unRef() const;
    void			unRefNoDelete() const;

protected:
    virtual			~Referenced()			{}
private:
    friend			class ObsPtrBase;
    virtual void		refNotify() const		{}
    virtual void		unRefNotify() const		{}
    virtual void		unRefNoDeleteNotify() const	{}
    virtual void		prepareForDelete()		{}
				/*!<Override to be called just before delete */

    mutable Counter		refcount_;

public:
    int				nrRefs() const;
				//!<Only for expert use
    bool			refIfReffed() const;
				//!<Don't use in production, for debugging
    bool			tryRef() const;
				//!<Not for normal use. May become private

    void			addObserver(ObsPtrBase* obs);
				//!<Not for normal use. May become private
    void			removeObserver(ObsPtrBase* obs);
				//!<Not for normal use. May become private

};


mExpClass(Basic) ObsPtrBase
{
public:
				operator bool()  const;
    bool			operator!() const;
protected:
				ObsPtrBase();
    void			set(Referenced*);

    friend class		Counter;

    void			clearPtr();
    mutable Threads::SpinLock	lock_;
    Referenced*			ptr_;
};


}; //RefCount namespace end


/*!Observes a refereence counted object. If you wish to use the pointer,
   you have to obtain it using get().
*/
template <class T>
mClass(Basic) ObsPtr : public RefCount::ObsPtrBase
{
public:
			ObsPtr(RefCount::Referenced* p = 0) { set(p); }
			ObsPtr(const ObsPtr<T>& p) { set( p.get().ptr() ); }
			ObsPtr(const RefMan<T>& p) { set( p.ptr() ); }
			~ObsPtr() { set( 0 ); }

    ObsPtr<T>&		operator=(const ObsPtr<T>& p)
			{ set(p.get().ptr()); return *this; }
    RefMan<T>&		operator=(RefMan<T>& p)
			{ set(p.ptr()); return p; }
    T*			operator=(T* p)
			{ set(p); return p; }

    RefMan<T>		get() const;
};


/*! Un-reference class pointer. Works for null pointers. */
mGlobal(Basic) void unRefPtr( const RefCount::Referenced* ptr );

/*! Un-reference class pointer without delete. Works for null pointers. */
mGlobal(Basic) void unRefNoDeletePtr( const RefCount::Referenced* ptr );

//! Reference class pointer. Works for null pointers.
mGlobal(Basic) void refPtr( const RefCount::Referenced* ptr );

//!Un-reference class pointer, and set it to zero. Works for null-pointers.
template <class T> inline
void unRefAndZeroPtr( T*& ptr )
{ unRefPtr( static_cast<RefCount::Referenced*>( ptr ) ); ptr = 0; }

template <class T> inline
void unRefAndZeroPtr( const T*& ptr )
{ unRefPtr( static_cast<const RefCount::Referenced*>( ptr ) ); ptr = 0; }


mObjectSetApplyToAllFunc( deepUnRef, unRefPtr( os[idx] ), os.plainErase() )
mObjectSetApplyToAllFunc( deepUnRefNoDelete, unRefNoDeletePtr( os[idx] ),
			 os.plainErase() )
mObjectSetApplyToAllFunc( deepRef, refPtr( os[idx] ), )



//Implementations and legacy stuff below

template <class T>
RefMan<T> ObsPtr<T>::get() const
{
    RefMan<T> res = 0;
    if ( ptr_ && ptr_->tryRef() )
    {
	//reffed once through tryRef
	res = (T*) ptr_;
	
	//unref the ref from tryRef
	ptr_->unRef();
    }
    
    return res;
}


#endif
