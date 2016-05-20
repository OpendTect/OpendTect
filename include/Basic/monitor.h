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

  Similarly, you have to trigger the objectToBeDeleted() at the start of your
  destructor - if you do not have one, make one. For that, use sendDelNotif(),
  it avoids double notifications; this base class will eventually do such a
  thing but then it's too late for many purposes: the subclass part of the
  object is then already dead. Thus, at the beginning of the your destructor,
  call sendDelNotif().

  You can also monitor the object's changes. First of all, a dirtyCount() is
  maintained. To make more precise event handling possible, the change notifier
  will deliver an int with a change type. This type can be specific for the
  Monitorable, and could be available with symbolic constants in that class.
  The default is 0 - 'General change'.

  For typical usage see NamedMonitorable.
  Example unpacking change notifications:

  void MyObj::chgCB( CallBacker* inpcb )
  {
      mCBCapsuleUnpackWithCaller( Monitorable::ChangeData, chgdata, cb, inpcb );
      if ( chgdata.changeType() == MonObj::cSomeChange() )
	 doSomething( chgdata.subIdx() );
  }

*/

mExpClass(Basic) Monitorable : public CallBacker
{
public:

    typedef int				ChangeType;
    typedef od_int64			SubIdxType;
    typedef od_int64			DirtyCountType;

    mExpClass(Basic) ChangeData : public std::pair<ChangeType,SubIdxType>
    {
    public:
		    ChangeData( ChangeType typ, SubIdxType idx )
			: std::pair<ChangeType,SubIdxType>(typ,idx) {}

	ChangeType  changeType() const	{ return first; }
	SubIdxType  subIdx() const	{ return second; }
    };

					Monitorable(const Monitorable&);
    virtual				~Monitorable();
    Monitorable&			operator =(const Monitorable&);

    virtual CNotifier<Monitorable,ChangeData>& objectChanged() const
	{ return const_cast<Monitorable*>(this)->chgnotif_; }
    virtual Notifier<Monitorable>&	objectToBeDeleted() const
	{ return const_cast<Monitorable*>(this)->delnotif_; }
    mDeclInstanceCreatedNotifierAccess(	Monitorable );
					//!< defines static instanceCreated()

    void			touch() const		{ dirtycount_++; }
    DirtyCountType		dirtyCount() const	{ return dirtycount_; }
    void			setDirtyCount( DirtyCountType nr ) const
							{ dirtycount_ = nr; }

    void			sendEntireObjectChangeNotification() const;

    static ChangeType		cEntireObjectChangeType()	{ return -1; }
    static SubIdxType		cEntireObjectChangeSubIdx()	{ return -1; }

protected:

				Monitorable();

    mutable Threads::Lock	accesslock_;

    mExpClass(Basic) AccessLockHandler
    {
    public:
				AccessLockHandler(const Monitorable&,
						  bool forread=true);
				~AccessLockHandler();
	bool			convertToWrite();
	void			unlockNow()	{ locker_->unlockNow(); }
	void			reLock()	{ locker_->reLock(); }
    private:
	const Monitorable&	obj_;
	Threads::Locker*	locker_;
	void			waitForMonitors();
    };

    void			sendChgNotif(AccessLockHandler&,ChangeType,
					     SubIdxType) const;
				//!< objectChanged called with released lock
    void			sendDelNotif() const;
    void			stopChangeNotifications() const
				{ changemonitorstoplevel_++; }
    void			resumeChangeNotifications() const;

    template <class T>
    inline T			getMemberSimple(const T&) const;
    template <class TMember,class TSetTo>
    inline void			setMemberSimple(TMember&,TSetTo,ChangeType,
					        SubIdxType);

private:

    mutable Threads::Atomic<int>		nrmonitors_;
    mutable Threads::Atomic<DirtyCountType>	dirtycount_;
    mutable Threads::Atomic<int>		changemonitorstoplevel_;

    mutable CNotifier<Monitorable,ChangeData>	chgnotif_;
    mutable Notifier<Monitorable>		delnotif_;
    mutable bool				delalreadytriggered_;

    friend class				MonitorLock;
    friend class				ChangeNotifyBlocker;

};


/*!\brief protects a Monitorable against change.

  Compare the locking with thread-locking tools:

  1) The Monitorable has (should have) methods that have the effect of Atomic's.
     You call a method, and are guaranteed the whole operation succeeds safely.

  2) Sometimes operations on Monitorable's are dependent on each other. For
     example, when you are iterating through a list. If changes in the list
     (size of list, or elements changing) are unacceptable, you need a tool to
     prevent any change. This is the MonitorLock.

     Note that not releasing the lock will almost certainly stop the entire app,
     therefore the MonitorLock will always release when it goes out of scope.
     You always want to release asap, therefore you will often call unlockNow()
     immediately when done.

  Beware: you cannot use the MonitorLock and still change the object, a
  DEADLOCK will be your reward. To write while reading, make a copy of the
  object, change it, and assign the object to that. The assignment operator
  of the object should be 'atomic' again.

 */

mExpClass(Basic) MonitorLock
{
public:
			MonitorLock(const Monitorable&);
			~MonitorLock();

    void		unlockNow();
    void		reLock();

protected:

    const Monitorable&	obj_;
    bool		unlocked_;

};


/*!\brief prevents change notifications coming out of a Monitorable.

  Use to stop tons of change notifications going out. Will send an
  'Entire object changed' event when it goes out of scope. To prevent that,
  call unBlockNow(false) explicitly beforehand.

*/

mExpClass(Basic) ChangeNotifyBlocker
{
public:
			ChangeNotifyBlocker(const Monitorable&);
			~ChangeNotifyBlocker();

    void		unBlockNow(bool send_entobj_notif=true);
    void		reBlock();

protected:

    const Monitorable&	obj_;
    bool		unblocked_;

};



//! For use in subclasses of Monitorable
#define mLock4Read() AccessLockHandler accesslockhandler_( *this )
#define mLock4Write() AccessLockHandler accesslockhandler_( *this, false )
#define mLock2Write() accesslockhandler_.convertToWrite()
#define mUnlockAllAccess() accesslockhandler_.unlockNow()
#define mSendChgNotif(typ,subidx) sendChgNotif(accesslockhandler_,typ,subidx)


template <class T>
inline T Monitorable::getMemberSimple( const T& memb ) const
{
    mLock4Read();
    return memb;
}

template <class TMember,class TSetTo>
inline void Monitorable::setMemberSimple( TMember& memb, TSetTo setto, int typ,
					  SubIdxType subidx )
{
    mLock4Read();
    if ( memb == setto )
	return; // common

    mLock2Write();
    if ( !(memb == setto) ) // someone may have beat me to it!
    {
	memb = setto;
	mSendChgNotif( typ, subidx );
    }
}


#define mImplSimpleMonitoredGet(fnnm,typ,memb) \
    typ fnnm() const { return getMemberSimple( memb ); }
#define mImplSimpleMonitoredSet(fnnm,typ,memb,chgtyp) \
    void fnnm( typ _set_to_ ) { setMemberSimple( memb, _set_to_, chgtyp, 0 ); }
#define mImplSimpleMonitoredGetSet(pfx,fnnmget,fnnmset,typ,memb,chgtyp) \
    pfx mImplSimpleMonitoredGet(fnnmget,typ,memb) \
    pfx mImplSimpleMonitoredSet(fnnmset,const typ&,memb,chgtyp)


#endif
