#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "notify.h"
#include "ptrman.h"
#include "refcount.h"


/*!\brief Object that can be MT-safely monitored from cradle to grave.

  Traditionally, the MVC concept was all about automatic updates. This can
  still be done, but nowadays more important seems the integrity for MT.

  The instanceCreated() will tell you when *any* MonitoredObject is created. To
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
  MonitoredObject, and could be available with symbolic constants in that class.
  The default is 0 - 'General change'.

  For simple class variables, there are simple macros for get and set that
  will lock and unlock right: mImplSimpleMonitoredGet, mImplSimpleMonitoredSet,
  and the most common mImplSimpleMonitoredGetSet.

  Locking for more complex things should be done using macros: mLock4Read,
  mLock4Write, mLock2Write, and mUnlockAllAccess. After write (i.e. object
  change) operations, you need to unlock and send the notification. Use
  mSendChgNotif or mSendEntireObjChgNotif.

  To handle change notifications, you'll want to unpack the capsule with one
  of the macros mGetMonitoredChgData or mGetMonitoredChgDataDoAll. Example:

  void MyObj::chgCB( CallBacker* cb )
  {
      mGetMonitoredChgDataDoAll( cb, chgdata, caller, return redrawAll() );
      if ( chgdata.changeType() == MonObj::cSomeChange() )
	 doSomething( chgdata.ID() );
  }

  The choice of cNoChange being zero is deliberate; the guarantee is that you
  can use a ChangeType as a boolean to see whether there is any change.

  Lastly, copying of MonitoredObject needs to be done right. For this, you want to
  use the mDeclMonitorableAssignment and mImplMonitorableAssignment macros:
  these provide correct handling and even make your task easier than otherwise.
  To make it work, in the .cc file you have to implement a void
  copyClassData(const clss&) function. It is called already locked, and should
  only copy the class' own data.

  For typical subclass implementations see NamedMonitoredObject, or Pick::Set.
  For usage, try visSurvey::LocationDisplay.

*/

mExpClass(Basic) MonitoredObject : public CallBacker
{
public:

    class ChangeData;


    mUseType( Threads,				Locker );
    typedef int					ChangeType;
    typedef od_int64				IDType;
    typedef CNotifier<MonitoredObject,ChangeData>	ChangeDataNotifier;

			MonitoredObject(const MonitoredObject&);
    virtual		~MonitoredObject();
    MonitoredObject&	operator =(const MonitoredObject&);
    bool		operator ==(const MonitoredObject&) const;
    virtual MonitoredObject* clone() const	{ return getClone(); }

    virtual ChangeDataNotifier&		objectChanged() const
					{ return mSelf().chgnotif_; }
    virtual Notifier<MonitoredObject>&	objectToBeDeleted() const
					{ return mSelf().delnotif_; }
    mDeclInstanceCreatedNotifierAccess(	MonitoredObject );
					//!< defines static instanceCreated()

    virtual void	touch() const		{ dirtycount_++; }
    virtual DirtyCountType dirtyCount() const	{ return dirtycount_; }
    virtual ChangeType	compareWith(const MonitoredObject&) const;

    static inline ChangeType cEntireObjectChange()
			{ return ChangeData::cEntireObjectChgType(); }
    static inline ChangeType cNoChange()
			{ return ChangeData::cNoChgType(); }
    static IDType	cUnspecChgID()
			{ return ChangeData::cUnspecChgID(); }
    static IDType	cEntireObjectChgID()
			{ return ChangeData::cUnspecChgID(); }

    mExpClass(Basic)	ChangeData : public std::pair<ChangeType,IDType>
    {
    public:

	class AuxData;

			ChangeData( ChangeType typ, IDType id, AuxData* data=0 )
			    : std::pair<ChangeType,IDType>(typ,id)
			    , auxdata_(data)		{}
			ChangeData(const ChangeData&);
	virtual		~ChangeData()		{}
	ChangeData&	operator =(const ChangeData&);

	ChangeType	changeType() const	{ return first; }
	IDType		ID() const		{ return second; }
	bool		isEntireObject() const	{ return first == -1; }
	bool		isNoChange() const	{ return mIsUdf(first); }
	bool		hasUnspecID() const	{ return second == -1; }
	bool		includes( ChangeType ct ) const
			{ return ct == first || isEntireObject(); }

	static inline ChangeType cEntireObjectChgType()	{ return -1; }
	static inline ChangeType cNoChgType()	{ return 0; }
	static inline IDType cUnspecChgID()	{ return -1; }
	static inline ChangeData AllChanged()	{ return ChangeData(-1,-1); }
	static inline ChangeData NoChange()	{ return ChangeData(0,-1); }

	mExpClass(Basic) AuxData : public ReferencedObject
	{
	    protected:
		virtual	~AuxData()			{}
	};

	RefMan<AuxData>	auxdata_;
	template<class T> inline
	const T* auxDataAs() const
			{ return static_cast<const T*>( auxdata_.ptr() ); }
	template<class T> inline
	T*		auxDataAs()
			{ return static_cast<T*>( auxdata_.ptr() ); }
    };
    static ChangeType	changeNotificationTypeOf(CallBacker*);
    void		sendChangeNotification(const ChangeData&) const;
    void		sendEntireObjectChangeNotification() const;
    void		transferNotifsTo(const MonitoredObject&,
					const CallBacker* onlyfor=0) const;

    mExpClass(Basic)	AccessLocker
    {
    public:

			AccessLocker(const MonitoredObject&,bool forread=true);
			AccessLocker(const AccessLocker&);
	AccessLocker&	operator =(const AccessLocker&);
			~AccessLocker();

	bool		isLocked() const;
	void		unlockNow();
	void		reLock(Locker::WaitType wt=Locker::WaitIfLocked);
	bool		convertToWrite();

	static void	enableLocking(bool yn);
			//!< don't use unless you understand the implications

	Locker*		theLock()	{ return thelock_; }

    protected:

	Locker*		thelock_	= nullptr;

    };

protected:

			MonitoredObject();
    virtual MonitoredObject* getClone() const	= 0;

    mutable Threads::Lock accesslock_;

    void		copyAll(const MonitoredObject&);
    void		sendChgNotif(AccessLocker&,const ChangeData&) const;
				//!< calls objectChanged with released lock
    void		sendChgNotif(AccessLocker&,ChangeType,IDType) const;
				//!< calls objectChanged with released lock
    void		sendDelNotif() const;
    void		stopChangeNotifications() const
			{ chgnotifblocklevel_++; }
    void		resumeChangeNotifications() const;

    template <class T>
    inline T		getMemberSimple(const T&) const;
    template <class TMember,class TSetTo>
    inline void		setMemberSimple(TMember&,TSetTo,ChangeType,IDType);

private:

    mutable DirtyCounter		dirtycount_;
    mutable Threads::Atomic<int>	chgnotifblocklevel_;

    mutable ChangeDataNotifier		chgnotif_;
    mutable Notifier<MonitoredObject>	delnotif_;
    mutable bool			delalreadytriggered_;

    friend class			MonitorLock;
    friend class			ChangeNotifyBlocker;

    ChangeType		compareClassData(const MonitoredObject&) const;

};


#define mGetMonitoredChgData(cb,chgdata) \
    mCBCapsuleUnpack( MonitoredObject::ChangeData, chgdata, cb )

#define mGetMonitoredChgDataWithAux(cb,chgdata,T,auxvar) \
    mCBCapsuleUnpack( MonitoredObject::ChangeData, chgdata, cb ); \
    T* auxvar = chgdata.auxDataAs<T>()

#define mGetMonitoredChgDataWithCaller(cb,chgdata,caller) \
    mCBCapsuleUnpackWithCaller( MonitoredObject::ChangeData, chgdata, caller, cb )

#define mGetMonitoredChgDataWithAuxAndCaller(cb,chgdata,T,auxvar,caller) \
    mGetMonitoredChgDataWithCaller(cb,chgdata,caller); \
    T* auxvar = chgdata.auxDataAs<T>()

#define mGetMonitoredChgDataDoAll(cb,chgdata,doallact) \
    mGetMonitoredChgData(chgdata,cb); \
    if ( chgdata.changeType() == Monitored::cEntireObjectChange() ) \
	{ doallact; }

#define mGetIDFromChgData( typ, var, chgdata ) \
    const typ var = typ::get( (typ::IDType)chgdata.ID() )


/*!\brief protects a MonitoredObject against change.

  Compare the locking with thread-locking tools:

  1) The MonitoredObject has (should have) methods that make a method call sort-of
     atomic. You call it, and are guaranteed the whole operation succeeds safely
     without interruption.

  2) Sometimes operations on MonitoredObject's are dependent on each other. For
     example, when you are iterating through a list. If changes in the list
     (size of list, or elements changing) are unacceptable, you need a tool to
     prevent any change. This is the MonitorLock.

     Note that not releasing the lock will almost certainly stop the entire app,
     therefore the MonitorLock will always release when it goes out of scope.
     Most often though you want to release asap, therefore you'll often call
     unlockNow() immediately when done.

  Beware: you cannot use the MonitorLock and still change the object, a
  DEADLOCK will be your reward. To write while reading, make a copy of the
  object, change it, and assign the object to that. The assignment operator
  of the object should be 'atomic' again, thanks to the assignment operator
  macros.

 */

mExpClass(Basic) MonitorLock
{
public:
			MonitorLock(const MonitoredObject&);
			~MonitorLock();

    void		unlockNow();
    void		reLock();

protected:

    MonitoredObject::AccessLocker locker_;
    bool		unlocked_;

};


/*!\brief prevents change notifications coming out of a MonitoredObject.

  Use to stop tons of change notifications going out. Will send an
  'Entire object changed' event when it goes out of scope. To prevent that,
  set send_notif to false.

*/

mExpClass(Basic) ChangeNotifyBlocker
{
public:
			ChangeNotifyBlocker(const MonitoredObject&,
					    bool send_notif=true);
			~ChangeNotifyBlocker();

    void		unBlockNow();
    void		reBlock();

protected:

    const MonitoredObject&	obj_;
    bool		unblocked_;
    bool		sendnotif_;

};



/*!\brief replaces a ref to a MonitoredObject with a new one.

  If you hold a RefMan to a monitorable that you monitor, then you can replace
  the ref with another one and make sure you start monitoring the new one,
  and no longer the old one. Like in:

  MonitoredObject::ChangeType ct = replaceMonitoredRef( sequence_, newseq, this );

  Returns whether any change is made to your ref. It checks whether the new
  object is different from the old one.

  If you have suscribers to an object that you own (and that others monitor
  through your ref) then you'll probably not want to pass 'this'. In that way
  all notifications are transferred to the new object. Note that this is a
  dangerous thing to do - some managing objects may depend on having notifiers
  on all objects they monitor.

  */

template <class Mon> inline
MonitoredObject::ChangeType replaceMonitoredRef( ConstRefMan<Mon>& ref,
				const Mon* newptr, CallBacker* only_for )
{
    const Mon* curptr = ref.ptr();
    if ( curptr == newptr )
	return false;

    MonitoredObject::ChangeType ct = MonitoredObject::cNoChange();
    if ( (newptr && !curptr) || (!newptr && curptr) )
	ct = MonitoredObject::cEntireObjectChange();
    else if ( curptr && newptr )
    {
	ct = curptr->compareWith( *newptr );
	curptr->transferNotifsTo( *newptr, only_for );
    }

    ref = newptr;
    return ct;
}

template <class Mon> inline
MonitoredObject::ChangeType replaceMonitoredRef( ConstRefMan<Mon>& ref,
			 const Mon& newref, CallBacker* only_for )
{
    return replaceMonitoredRef( ref, &newref, only_for );
}

template <class Mon> inline
MonitoredObject::ChangeType replaceMonitoredRef( ConstRefMan<Mon>& ref,
			ConstRefMan<Mon>& newref, CallBacker* only_for )
{
    return replaceMonitoredRef( ref, newref.ptr(), only_for );
}

template <class Mon> inline
MonitoredObject::ChangeType replaceMonitoredRef( RefMan<Mon>& ref, Mon* newptr,
				CallBacker* only_for )
{
    return replaceMonitoredRef( (ConstRefMan<Mon>&)ref, (const Mon*)newptr,
				only_for );
}

template <class Mon> inline
MonitoredObject::ChangeType replaceMonitoredRef( RefMan<Mon>& ref, Mon& newref,
				CallBacker* only_for )
{
    return replaceMonitoredRef( ref, &newref, only_for );
}

template <class Mon> inline
MonitoredObject::ChangeType replaceMonitoredRef( RefMan<Mon>& ref, RefMan<Mon>& newref,
				CallBacker* only_for )
{
    return replaceMonitoredRef( ref, newref.ptr(), only_for );
}
