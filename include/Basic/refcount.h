#ifndef refcount_h
#define refcount_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		13-11-2003
 Contents:	Basic functionality for reference counting
 RCS:		$Id: refcount.h,v 1.9 2006-08-28 08:54:16 cvskris Exp $
________________________________________________________________________

-*/

#include "ptrman.h"

template <class T> class ObjectSet;

/*!The refcount itself. Used internally by refcounted objects. */
class RefCount
{
public:
    		RefCount() : refcount_( 0 ) {}
    int		refcount_;
};

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

#define mRefCountImplWithDestructor(ClassName, DestructorImpl ) \
public: \
    void	ref() const \
		{ \
		    __refcount.refcount_++; \
		    refNotify(); \
		} \
    void	unRef() const \
		{ \
		    unRefNotify(); \
		    if ( !this ) \
			return; \
		    if ( !--__refcount.refcount_ ) \
			delete this; \
		} \
 \
    void	unRefNoDelete() const \
		{ \
		    __refcount.refcount_--; \
    		    unRefNoDeleteNotify(); \
		} \
    int		nrRefs() const { return this ? __refcount.refcount_ : 0 ; } \
private: \
    virtual void	refNotify() const {} \
    virtual void	unRefNotify() const {} \
    virtual void	unRefNoDeleteNotify() const {} \
    mutable RefCount	__refcount;	\
protected: \
    		DestructorImpl; \
private:


#define mDeepRef(funcname,func,extra) \
template <class T> inline void deep##funcname( ObjectSet<T>& set )\
{\
    for ( int idx=0; idx<set.size(); idx++ )\
    {\
	if ( set[idx] ) set[idx]->func();\
    }\
\
   extra;\
}


#define mRefCountImpl(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName() );

#define mRefCountImplNoDestructor(ClassName) \
mRefCountImplWithDestructor(ClassName, virtual ~ClassName() {} );


mDeepRef(UnRef,unRef, set.erase() );
mDeepRef(Ref,ref,);
mDeepRef(UnRefNoDelete,unRefNoDelete,);

mDefPtrMan1(RefMan, if ( ptr_ ) ptr_->ref(), if ( ptr_ ) ptr_->unRef() )
mDefPtrMan2(RefMan, if ( ptr_ ) ptr_->ref(), if ( ptr_ ) ptr_->unRef() )
mDefPtrMan3(RefMan, if ( ptr_ ) ptr_->ref(), if ( ptr_ ) ptr_->unRef() )


#endif
