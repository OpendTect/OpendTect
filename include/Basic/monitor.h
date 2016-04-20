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

  Traditionally, the MVC concept was all about automatic updates. This can
  still be done, but nowadays more important seems the integrity for MT.

  The instanceCreated() will tell you when *any* Monitorable is created. To
  make your class really monitorable, use mDeclInstanceCreatedNotifierAccess
  for your subclass, too. Note that you'll need to add
  mTriggerInstanceCreatedNotifier() to the constructor, and add
  mDefineInstanceCreatedNotifierAccess(YourClassName) to a .cc.

  Similarly, you have to trigger the objectToBeDeleted() from your destructor.
  This base class will do such a thing but then it's too late for many
  purposes: the subclass part of the object is then already dead. Thus, at
  the beginning of the your destructor, call sendDelNotif().

  For typical usage see NamedObject.

*/

mExpClass(Basic) Monitorable : public CallBacker
{
public:

					Monitorable(const Monitorable&);
    virtual				~Monitorable();
    Monitorable&			operator =(const Monitorable&);

    virtual Notifier<Monitorable>&	objectChanged()
					{ return chgnotif_; }
    virtual Notifier<Monitorable>&	objectToBeDeleted()
					{ return delnotif_; }
    mDeclInstanceCreatedNotifierAccess(	Monitorable );
					//!< defines static instanceCreated()

protected:

				Monitorable();

    mutable Threads::Lock	accesslock_;

    //!\brief makes locking easier and safer (unlocks when it goes out of scope)
    mExpClass(Basic) AccessLockHandler
    {
    public:
				AccessLockHandler( Monitorable& m )
				    : locker_(m.accesslock_)	    {}
				AccessLockHandler( const Monitorable& m )
				    : locker_(m.accesslock_)	    {}
	Threads::Locker		locker_;
    };

    void			sendChgNotif(AccessLockHandler&);
				//!< objectChanged called with released lock
    void			sendDelNotif();

private:

    Notifier<Monitorable>	chgnotif_;
    Notifier<Monitorable>	delnotif_;
    bool			delalreadytriggered_;

};

//! For use in subclasses of Monitorable
#define mLock4Access() AccessLockHandler accesslockhandler_( *this )
#define mSendChgNotif() sendChgNotif( accesslockhandler_ )


#endif
