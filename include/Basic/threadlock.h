#ifndef threadlock_h
#define threadlock_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2013
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "commondefs.h"
#include "plftypes.h"


/*!\brief simple interface to thread locking

The two ingredients are:
* The lock itself - commonly a class variable
* A locker (managing the setting and releasing of the lock)

Typical usage:

class X
{
//...
    Threads::Lock	datalock_;
//...
};

void X::doSomething()
{
    Threads::Locker locker( datalock_ );
    for ( int i.....
}

*/

namespace Threads
{
class Mutex;
class SpinLock;
class ReadWriteLock;

/*!\brief A lock of a type that (hopefully) suits your needs.
  To use it, you need the Locker class. */


mExpClass(Basic) Lock
{
public:

    enum Type			{ BigWork, SmallWork, MultiRead };

    				Lock(bool for_just_a_few_operations=false);
    				Lock(Type);
    				Lock(const Lock&);
    Lock&			operator =(const Lock&);
    virtual			~Lock();


    // For thread-specialists:
    inline bool			isMutex() const		{ return (bool)mutex_; }
    inline bool			isSpinLock() const	{ return (bool)splock_;}
    inline bool			isRWLock() const	{ return (bool)rwlock_;}
    inline Mutex&		mutex()			{ return *mutex_; }
    inline SpinLock&		spinLock()		{ return *splock_; }
    inline ReadWriteLock&	readWriteLock()		{ return *rwlock_; }

protected:

    Mutex*		mutex_;
    SpinLock*		splock_;
    ReadWriteLock*	rwlock_;

};


/*!\brief Locks the lock, shutting out access from other threads if needed. */

mExpClass(Basic) Locker
{
public:

    enum WaitType		{ WaitIfLocked, DontWaitForLock };
    				//!< if DontWaitForLock, check isLocked()
    enum RWType			{ ReadLock, WriteLock };
    				//!< only interesting for MultiRead locks

    				// default = WaitIfLocked / ReadLock
    				Locker(Lock&);
    				Locker(Lock&,WaitType);
    				Locker(Lock&,RWType);
    				Locker(Lock&,WaitType,RWType);
    				Locker(Lock&,RWType,WaitType);
    virtual			~Locker()		{ unlockNow(); }

    inline bool			isLocked() const	{ return needunlock_; }
     				//<! only useful if DontWaitForLock
    void			unlockNow();
     				//<! to explicitly release earlier than the
     				//!< Locker goes out of scope
    void			reLock(WaitType wt=WaitIfLocked);

protected:

     Lock&			lock_;
     bool			needunlock_;
     bool			isread_;

private:

     				Locker(const Locker&);
					//!< intentionally not implemented

};

} // namespace Threads


#endif
