#ifndef perthreadrepos_h
#define perthreadrepos_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
 RCS:		$Id$
________________________________________________________________________
-*/


#include "basicmod.h"

#include "manobjectset.h"
#include "threadlock.h"
#include "thread.h"

/*!
 \brief Class that keeps one object per thread. This enables temporary
 passing of objects (such as strings) where needed.
 */


template <class T>
mClass(Basic) PerThreadObjectRepository
{
public:
    T&				getObject();

private:
    ManagedObjectSet<T>		objects_;
    ObjectSet<const void>	threadids_;
    Threads::Lock		lock_;

};

typedef PerThreadObjectRepository<BufferString> StaticStringManager;

#define _mDeclStaticString(nm) \
    mDefineStaticLocalObject( StaticStringManager, nm##_ssm, \
			      = StaticStringManager() ) \
    BufferString& nm = nm##_ssm.getObject()

#ifdef __debug__
#define mDeclStaticString(nm) \
    _mDeclStaticString(nm); \
    addToStaticStringRepos( &nm )
#else
#define mDeclStaticString(nm) \
    _mDeclStaticString(nm)
#endif

/*!For Debugging only. Keeps a list of static strings and can tell
   if a particular string is a static string or not. */

mGlobal(Basic) void addToStaticStringRepos(const OD::String*);
mGlobal(Basic) bool isStaticString(const OD::String*);

//Implementation

template <class T>
T& PerThreadObjectRepository<T>::getObject()
{
    const void* threadid = Threads::currentThread();
    Threads::Locker lock( lock_ );
    int idx = threadids_.indexOf( threadid );
    if ( idx<0 )
    {
	idx = threadids_.size();
	threadids_ += threadid;
	objects_ += new T;
    }

    return *objects_[idx];
}

#endif


