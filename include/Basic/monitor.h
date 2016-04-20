#ifndef monitor_h
#define monitor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "notify.h"


/*!\brief Object that can be MT-safely monitored from cradle to grave.

  The instanceCreated() will tell you when *any* Monitorable is created. To
  make your class really monitorable, use mDeclInstanceCreatedNotifierAccess
  for your subclass, too. Note that you'll need to add
  mTriggerInstanceCreatedNotifier() to the constructor, and add
  mDefineInstanceCreatedNotifierAccess(YourClassName) to a .cc.

  Similarly, you have to trigger the objectToBeDeleted() from your destructor.
  This base class could do such a thing but then it's too late for many
  purposes: the subclass part of the object is then already dead. Thus, this
  base class destructor does *not* trigger the notifier as in:
  objectToBeDeleted().trigger();

*/

mExpClass(Basic) Monitorable : public CallBacker
{
public:

    virtual				~Monitorable()
					{ /* nothing, see class comments! */ }

    virtual Notifier<Monitorable>&	objectChanged()
					{ return objchgd_; }
    virtual Notifier<Monitorable>&	objectToBeDeleted()
					{ return objtobedel_; }
    mDeclInstanceCreatedNotifierAccess(	Monitorable );
					//!< defines static instanceCreated()

protected:

				Monitorable();

    Threads::Lock		editlock_;
    Notifier<Monitorable>	objchgd_;
    Notifier<Monitorable>	objtobedel_;

    //!\brief makes locking easier and safer (unlocks when it goes out of scope)
    mExpClass(Basic) EditLocker
    {
    public:
				EditLocker( Monitorable& m )
				    : locker_(m.editlock_)	    {}
	Threads::Locker		locker_;
    };
};

//! For use in subclasses of Monitorable
#define mLock4Edit() EditLocker editlocker_( *this )


#endif
