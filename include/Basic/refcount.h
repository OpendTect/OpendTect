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

#include "ptrman.h"
#include "thread.h"
#include "objectset.h"

template <class T> class ObjectSet;

/*!
Macros to set up reference counting in a class. A refcount class is set up
by:
\code
class A
{
    mRefCountImpl(A);
public:
    //Your class stuff
};
\endcode

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



The macro will define a protected destructor, so you have to implement one
(even if it's a dummy {}).

ObjectSets with ref-counted objects can be modified by either:
\code
ObjectSet<RefCountClass> set:

deepRef( set );
deepUnRefNoDelete(set);
deepRef( set );
deepUnRef(set);

\code

A pointer management is handled by the class RefMan, which has the same usage as
PtrMan.

*/

#define mRefCountImplWithDestructor(ClassName, DestructorImpl, delfunc ) \
public: \
    void	ref() const \
		{ \
		    refcount_++; \
		    refNotify(); \
		} \
    bool	refIfReffed() const \
		{ \
		    if ( !refcount_ )  \
		    { \
			return false; \
		    } \
\
		    refcount_++; \
		    refNotify(); \
		    return true; \
		} \
    void	unRef() const \
		{ \
		    unRefNotify(); \
		    if ( !--refcount_ ) \
		    { \
			delfunc \
			return; \
		    } \
		} \
 \
    void	unRefNoDelete() const \
		{ \
    		    unRefNoDeleteNotify(); \
		    refcount_--; \
		} \
    int		nrRefs() const { return this ? (int) refcount_ : 0 ; } \
private: \
    virtual void		    refNotify() const {} \
    virtual void		    unRefNotify() const {} \
    virtual void		    unRefNoDeleteNotify() const {} \
    mutable Threads::Atomic<long>   refcount_;	\
protected: \
    		DestructorImpl; \
private:


#define mRefCountImpl(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName(), delete this; );

#define mRefCountImplNoDestructor(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName() {}, delete this; );

mObjectSetApplyToAllFunc( deepUnRef, if ( os[idx] ) os[idx]->unRef(),
		      os.plainErase() )
mObjectSetApplyToAllFunc( deepRef, if ( os[idx] ) os[idx]->ref(), )
mObjectSetApplyToAllFunc( deepUnRefNoDelete,
		      if ( os[idx] ) os[idx]->unRefNoDelete(),
		      os.plainErase() )


//Macro to implement a refman class
#define mDefRefMan( clss, reffunc, unreffunc ) \
mDefPtrMan1(clss, if ( ptr_ ) ptr_->reffunc(), if ( ptr_ ) ptr_->unreffunc() ) \
inline clss(const clss<T>& p) : ptr_( 0 ) {  set(p.ptr_); } \
inline clss<T>& operator=(const clss<T>& p) { set(p.ptr_); return *this; }\
mDefPtrMan2(clss, if (ptr_) ptr_->reffunc(), if ( ptr_ ) ptr_->unreffunc() )\
mDefPtrMan3(clss, if (ptr_) ptr_->reffunc(), if ( ptr_ ) ptr_->unreffunc() )

//Implement RefMan
mDefRefMan( RefMan, ref, unRef )



#endif
