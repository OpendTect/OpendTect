#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________
-*/


#include "basicmod.h"

#include "manobjectset.h"
#include "threadlock.h"
#include "thread.h"

/*!\brief Class that keeps one object per thread. This enables temporary
 passing of objects (such as strings) where needed. */


template <class T>
mClass(Basic) PerThreadObjectRepository
{
public:

    inline T&			getObject();

private:

    ManagedObjectSet<T>		objects_;
    ObjectSet<const void>	threadids_;
    Threads::Lock		lock_;

};


template <class T>
inline T& PerThreadObjectRepository<T>::getObject()
{
    const Threads::ThreadID threadid = Threads::currentThread();
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
