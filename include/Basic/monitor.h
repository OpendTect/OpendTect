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

  Lastly, copying of Monitorables needs to be done right. For this, you want to
  use the mDeclMonitorableAssignment and mImplMonitorableAssignment macros:
  these provide correct handling and even make your task easier than otherwise.
  To make it work, in the .cc file you have to implement a void
  copyClassData(const clss&) function. It is called already locked, and should
  only copy the class' own data.

  For typical subclass implementations see NamedMonitorable, or Pick::Set.
  For usage, try visSurvey::LocationDisplay.

*/

mExpClass(Basic) Monitorable : public CallBacker
{
public:

    typedef int		ChangeType;
    typedef od_int64	IDType;
    typedef od_int64	DirtyCountType;

    mExpClass(Basic) ChangeData : public std::pair<ChangeType,IDType>
    {
    public:

	mExpClass(Basic) AuxData : public RefCount::Referenced
	{
	    protected:
		virtual	~AuxData()			{}
	};

			ChangeData( ChangeType typ, IDType id, AuxData* ad=0 )
			    : std::pair<ChangeType,IDType>(typ,id)
			    , auxdata_(ad)		{}
			ChangeData(const ChangeData&);
	virtual		~ChangeData()		{}
	ChangeData&	operator =(const ChangeData&);

	ChangeType	changeType() const	{ return first; }
	IDType		ID() const		{ return second; }
	bool		isEntireObject() const	{ return first == -1; }
	bool		isNoChange() const	{ return mIsUdf(first); }
	bool		includes( ChangeType ct ) const
			{ return ct == first || isEntireObject(); }

	static inline ChangeType cEntireObjectChgType()	{ return -1; }
	static inline ChangeType cNoChgType()	{ return mUdf(ChangeType); }
	static inline IDType cUnspecChgID()	{ return -1; }
	static inline ChangeData AllChanged()	{ return ChangeData(-1,-1); }
	static inline ChangeData NoChange()	{ return ChangeData(
							 mUdf(ChangeType),-1); }
	RefMan<AuxData>	auxdata_;
	template<class T> inline const T* auxDataAs() const
	{ return static_cast<const T*>( auxdata_.ptr() ); }
	template<class T> inline T* auxDataAs()
	{ return static_cast<T*>( auxdata_.ptr() ); }
    };

			Monitorable(const Monitorable&);
    virtual		~Monitorable();
    Monitorable&	operator =(const Monitorable&);
    virtual Monitorable* clone() const		= 0;

    virtual CNotifier<Monitorable,ChangeData>& objectChanged() const
	{ return const_cast<Monitorable*>(this)->chgnotif_; }
    virtual Notifier<Monitorable>&	objectToBeDeleted() const
	{ return const_cast<Monitorable*>(this)->delnotif_; }
    mDeclInstanceCreatedNotifierAccess(	Monitorable );
					//!< defines static instanceCreated()

    virtual void	touch() const		{ dirtycount_++; }
    virtual DirtyCountType dirtyCount() const	{ return dirtycount_; }

    void		sendEntireObjectChangeNotification() const;

    static IDType	cEntireObjectChangeID()
			{ return ChangeData::cUnspecChgID(); }
    static inline ChangeType cEntireObjectChangeType()
			{ return ChangeData::cEntireObjectChgType(); }
    static ChangeType	changeNotificationTypeOf(CallBacker*);

    virtual ChangeData	compareWith( const Monitorable& oth ) const
			{ return this == &oth ? ChangeData::NoChange()
					      : ChangeData::AllChanged(); }

    mExpClass(Basic) AccessLocker : public Threads::Locker
    {
    public:
			AccessLocker(const Monitorable&,bool forread=true);
	inline bool	convertToWrite()	{ return convertToWriteLock(); }
    };

protected:

			Monitorable();

    mutable Threads::Lock accesslock_;

    void		copyAll(const Monitorable&);
    void		sendChgNotif(AccessLocker&,const ChangeData&) const;
				//!< objectChanged called with released lock
    void		sendChgNotif(AccessLocker&,ChangeType,IDType) const;
    void		sendDelNotif() const;
    void		stopChangeNotifications() const
			{ chgnotifblocklevel_++; }
    void		resumeChangeNotifications() const;

    template <class T>
    inline T		getMemberSimple(const T&) const;
    template <class TMember,class TSetTo>
    inline void		setMemberSimple(TMember&,TSetTo,ChangeType,IDType);

    typedef Threads::Atomic<DirtyCountType>	DirtyCounter;

private:

    mutable DirtyCounter		dirtycount_;
    mutable Threads::Atomic<int>	chgnotifblocklevel_;

    mutable CNotifier<Monitorable,ChangeData> chgnotif_;
    mutable Notifier<Monitorable>	delnotif_;
    mutable bool			delalreadytriggered_;

    friend class			MonitorLock;
    friend class			ChangeNotifyBlocker;

};


#define mImplSimpleMonitoredGet(fnnm,typ,memb) \
    typ fnnm() const { return getMemberSimple( memb ); }
#define mImplSimpleMonitoredSet(fnnm,typ,memb,chgtyp) \
    void fnnm( typ _set_to_ ) { setMemberSimple( memb, _set_to_, chgtyp, 0 ); }
#define mImplSimpleMonitoredGetSet(pfx,fnnmget,fnnmset,typ,memb,chgtyp) \
    pfx mImplSimpleMonitoredGet(fnnmget,typ,memb) \
    pfx mImplSimpleMonitoredSet(fnnmset,const typ&,memb,chgtyp)

#define mGetIDFromChgData( typ, var, chgdata ) \
    const typ var = typ::get( (typ::IDType)chgdata.ID() )


/*!\brief protects a Monitorable against change.

  Compare the locking with thread-locking tools:

  1) The Monitorable has (should have) methods that make a method call sort-of
     atomic. You call it, and are guaranteed the whole operation succeeds safely
     without interruption.

  2) Sometimes operations on Monitorable's are dependent on each other. For
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
			MonitorLock(const Monitorable&);
			~MonitorLock();

    void		unlockNow();
    void		reLock();

protected:

    Monitorable::AccessLocker locker_;
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
#define mLock4Read() AccessLocker accesslocker_( *this )
#define mLock4Write() AccessLocker accesslocker_( *this, false )
#define mLock2Write() accesslocker_.convertToWrite()
#define mReLock() accesslocker_.reLock()
#define mUnlockAllAccess() accesslocker_.unlockNow()
#define mAccessLocker() accesslocker_
#define mSendChgNotif(typ,id) sendChgNotif(accesslocker_,typ,id)
#define mSendEntireObjChgNotif() \
    mSendChgNotif( cEntireObjectChangeType(), cEntireObjectChangeID() )


#define mGetMonitoredChgData(cb,chgdata) \
    mCBCapsuleUnpack( Monitorable::ChangeData, chgdata, cb )

#define mGetMonitoredChgDataWithAux(cb,chgdata,T,auxvar) \
    mCBCapsuleUnpack( Monitorable::ChangeData, chgdata, cb ); \
    T* auxvar = chgdata.auxDataAs<T>()

#define mGetMonitoredChgDataWithCaller(cb,chgdata,caller) \
    mCBCapsuleUnpackWithCaller( Monitorable::ChangeData, chgdata, caller, cb )

#define mGetMonitoredChgDataWithAuxAndCaller(cb,chgdata,T,auxvar,caller) \
    mGetMonitoredChgDataWithCaller(cb,chgdata,caller); \
    T* auxvar = chgdata.auxDataAs<T>()

#define mGetMonitoredChgDataDoAll(cb,chgdata,doallact) \
    mGetMonitoredChgData(chgdata,cb); \
    if ( chgdata.changeType() == Monitored::cEntireObjectChangeType() ) \
	{ doallact; }


/*!\brief For subclasses: declaration of assignment method that will emit
  notifications, by default 'Entire Object'.

  Because of the locking, assignment operators are essential. These will
  need to copy both the 'own' members aswell as base class members, and emit
  the notification afterwards. For this, you have to implement a function that
  copies only the class' own data (unlocked): void copyClassData(const clss&).

  Note that if you want to give more fine-grained notifications than always
  'Entire Object Changed', then you can define (and implement) your own
  compareWith().

  */

#define mDeclAbstractMonitorableAssignment(clss) \
    private: \
        void	    copyClassData(const clss&); \
    protected: \
        void	    copyAll(const clss&); \
    public: \
		    clss(const clss&); \
	clss&	    operator =(const clss&)

/*!\brief For subclasses: like mDeclAbstractMonitorableAssignment but for
  non-abstract subclasses. Adds the clone() method. */

#define mDeclMonitorableAssignment(clss) \
    mDeclAbstractMonitorableAssignment(clss); \
    virtual clss* clone() const		{ return new clss( *this ); }

/*!\brief For subclasses: implementation of assignment method. You have to
  implement the copyClassData function yourself, no locking at all. As in:

  void MyClass::copyClassData( const MyClass& oth )
  {
      x_ = oth.x_;
      y_ = oth.y_;
  }

  */

#define mImplMonitorableAssignment(clss,baseclss) \
clss& clss::operator =( const clss& oth ) \
{ \
    const ChangeData changedata = compareWith( oth ); \
    if ( !changedata.isNoChange() ) \
    { \
	mLock4Write(); \
	AccessLocker lckr( oth ); \
	copyAll( oth ); \
	touch(); \
	mUnlockAllAccess(); \
	mSendChgNotif( changedata.changeType(), changedata.ID() ); \
    } \
    return *this; \
} \
\
void clss::copyAll( const clss& oth ) \
{ \
    baseclss::copyAll( oth ); \
    copyClassData( oth ); \
}


template <class T>
inline T Monitorable::getMemberSimple( const T& memb ) const
{
    mLock4Read();
    return memb;
}

template <class TMember,class TSetTo>
inline void Monitorable::setMemberSimple( TMember& memb, TSetTo setto,
					  ChangeType typ, IDType id )
{
    mLock4Read();
    if ( memb == setto )
	return; // common

    if ( mLock2Write() || !(memb == setto) )
    {
	memb = setto;
	mSendChgNotif( typ, id );
    }
}
