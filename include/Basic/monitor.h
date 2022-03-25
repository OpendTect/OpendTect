#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "monitoredobject.h"


// Tools for implementation of subclasses of MonitoredObject



#define mImplSimpleMonitoredGet_impl(fnnm,typ,memb,ovrspec) \
    typ fnnm() const ovrspec { return getMemberSimple( memb ); }

#define mImplSimpleMonitoredSet_impl(fnnm,typ,memb,chgtyp,ovrspec) \
    void fnnm( typ _set_to_ ) ovrspec \
	{ setMemberSimple( memb, _set_to_, chgtyp, cUnspecChgID() ); }

/*!\brief Defines simple MT-safe copyable member get. */
#define mImplSimpleMonitoredGet( fnnm, typ, memb ) \
	    mImplSimpleMonitoredGet_impl(fnnm,typ,memb,)
#define mImplSimpleMonitoredGetOverride( fnnm, typ, memb ) \
	    mImplSimpleMonitoredGet_impl(fnnm,typ,memb,override)

/*!\brief Defines simple MT-safe copyable member change. */
#define mImplSimpleMonitoredSet(fnnm,typ,memb,chgtyp) \
	    mImplSimpleMonitoredSet_impl(fnnm,typ,memb,chgtyp,)
#define mImplSimpleMonitoredSetOverride(fnnm,typ,memb,chgtyp) \
	    mImplSimpleMonitoredSet_impl(fnnm,typ,memb,chgtyp,override)

/*!\brief Defines simple MT-safe copyable member access.

  Example:

  mImplSimpleMonitoredGetSet( inline, color, setColor, Color, color_,
			      cColorChange() );

*/

#define mImplSimpleMonitoredGetSet(pfx,fnnmget,fnnmset,typ,memb,chgtyp) \
    pfx mImplSimpleMonitoredGet(fnnmget,typ,memb) \
    pfx mImplSimpleMonitoredSet(fnnmset,const typ&,memb,chgtyp)


// Generalized; also available for friend classes
#define mLockMonitorable4Read(obj) \
	MonitoredObject::AccessLocker accesslocker_( obj )
#define mLockMonitorable4Write(obj) \
	MonitoredObject::AccessLocker accesslocker_( obj, false )
#define mMonitorableLock2Write(obj) accesslocker_.convertToWrite()
#define mMonitorableReLock(obj) accesslocker_.reLock()
#define mMonitorableUnlockAllAccess(obj) accesslocker_.unlockNow()
#define mMonitorableAccessLocker(obj) accesslocker_
#define mSendMonitorableChgNotif(obj,typ,id) \
	(obj).sendChgNotif(accesslocker_,typ,id)
#define mSendMonitorableEntireObjChgNotif(obj) \
	mSendMonitorableChgNotif(obj,MonitoredObject::cEntireObjectChange(), \
				 MonitoredObject::cEntireObjectChgID())

// Shorthands for class implementation
#define mLock4Read() mLockMonitorable4Read(*this)
#define mLock4Write() mLockMonitorable4Write(*this)
#define mLock2Write() mMonitorableLock2Write(*this)
#define mReLock() mMonitorableReLock(*this)
#define mUnlockAllAccess() mMonitorableUnlockAllAccess(*this)
#define mAccessLocker() mMonitorableAccessLocker(*this)
#define mSendChgNotif(typ,id) mSendMonitorableChgNotif(*this,typ,id)
#define mSendEntireObjChgNotif() mSendMonitorableEntireObjChgNotif(*this)



#define mDeclGenMonitorableAssignment(clss) \
    private: \
        void		copyClassData(const clss&); \
        ChangeType	compareClassData(const clss&) const; \
    protected: \
        void		copyAll(const clss&); \
    public: \
			clss(const clss&); \
	clss&		operator =(const clss&); \
	bool		operator ==(const clss&) const; \
	inline bool	operator !=( const clss& oth ) const \
			{ return !(*this == oth); } \
	clss*		clone() const override	{ return (clss*)getClone(); } \
        ChangeType	compareWith(const MonitoredObject&) const override


/*!\brief MonitoredObject subclasses: assignment and comparison.

  Declares assignment method that will emit notifications, by default
  'Entire Object'. Because of the locking, assignment operators are essential.
  These will need to copy both the 'own' members aswell as base class members,
  and emit the notification afterwards. For this, you have to implement a
  function that copies only the class' own data (unlocked):
  void copyClassData(const clss&).

  To be able to provide more fine-grained notifications than always
  'Entire Object Changed', you also have to provide a method compareClassData().
  in this way we also define the equality operators (== and !=).

  The stuff with clone() vs getClone() is because of Windows, and my obsession
  to make clone() return a pointer to the actual class.

  */

#define mDeclAbstractMonitorableAssignment(clss) \
    mDeclGenMonitorableAssignment(clss)

/*!\brief like mDeclAbstractMonitorableAssignment but for
  non-abstract subclasses. Implements the clone() method. */

#define mDeclMonitorableAssignment(clss) \
    mDeclGenMonitorableAssignment(clss); \
    clss* getClone() const override		{ return new clss( *this ); }


#define mImplEmptyMonitorableCopyClassData( clssnm ) \
void clssnm::copyClassData( const clssnm& oth ) \
{ \
}

#define mImplAlwaysDifferentMonitorableCompareClassData( clssnm ) \
MonitoredObject::ChangeType clssnm::compareClassData( const clssnm& oth ) const\
{ \
    return cEntireObjectChange(); \
}

#define mImplEmptyMonitorableCompare( clssnm ) \
MonitoredObject::ChangeType clssnm::compareClassData( const clssnm& oth ) const\
{ \
    return cNoChange(); \
}


/*!\brief Implementation of assignment and comparison methods for
	  MonitoredObject's.

  you have to implement the copyClassData and compareClassData functions
  yourself. These functions are calles in a locked state, so do not use
  locking member functions.

  void MyClass::copyClassData( const MyClass& oth )
  {
      x_ = oth.x_;
      y_ = oth.y_;
  }
  bool MyClass::compareClassData( const MyClass& oth ) const
  {
      if ( x_ != oth.x_ )
	   return cXChanged();
      if ( y_ != oth.y_ )
	   return cYChanged();
      return cNoChange();
  }

  For standard situations, there are macros to help you implement the
  compareClassData function.

  Usually, you can use the mImplMonitorableAssignment macro.

  */

#define mGenImplMonitorableAssignment(pfx,clss,baseclss) \
pfx clss& clss::operator =( const clss& oth ) \
{ \
    const ChangeType chgtyp = compareWith( oth ); \
    if ( chgtyp != cNoChange() ) \
    { \
	mLock4Write(); \
	AccessLocker lckr( oth ); \
	copyAll( oth ); \
	mSendChgNotif( chgtyp, cUnspecChgID() ); \
    } \
    return *this; \
} \
\
pfx bool clss::operator ==( const clss& oth ) const \
\
{ \
    return compareWith( oth ) == cNoChange(); \
} \
\
pfx void clss::copyAll( const clss& oth ) \
{ \
    baseclss::copyAll( oth ); \
    AccessLocker lckr( oth ); \
    copyClassData( oth ); \
} \
\
pfx MonitoredObject::ChangeType clss::compareWith(\
					const MonitoredObject& mon ) const \
{ \
    if ( this == &mon ) \
	return cNoChange(); \
    mDynamicCastGet( const clss*, oth, &mon ); \
    if ( !oth ) \
	return cEntireObjectChange(); \
\
    mLock4Read(); \
    AccessLocker lckr( *oth ); \
    const ChangeType ct = compareClassData( *oth ); \
    mUnlockAllAccess(); \
\
    return ct != cNoChange() ? ct : baseclss::compareWith( *oth ); \
}


#define mImplMonitorableAssignment(clss,baseclss) \
    mGenImplMonitorableAssignment(,clss,baseclss)


#define mImplMonitorableAssignmentWithNoMembers( clss, baseclss ) \
    mImplMonitorableAssignment(clss,baseclss) \
    mImplEmptyMonitorableCopyClassData( clss ) \
    mImplEmptyMonitorableCompare( clss )



/*!\brief Helper macro to easily implement your compareClassData()
  in standard situations.

  The idea is that a single change type can be returned, if more than one
  change has happened you need to return cEntireObjectChange().

  Example:

MonitoredObject::ChangeType Well::Info::compareClassData(const Info& oth) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( uwid_, cUWIDChange() );
    mHandleMonitorableCompare( oper_, cInfoChange() );
    mHandleMonitorableComparePtrContents( a_ptr_, cDataChange() );
    mDeliverMonitorableCompare();
}

  */

#define mStartMonitorableCompare() ChangeType chgtype = cNoChange()

#define mHandleMonitorableCompare( memb, val ) \
    if ( !(memb == oth.memb) ) \
    { \
	if ( chgtype == cNoChange() || chgtype == val ) \
	    chgtype = val; \
	else \
	    return cEntireObjectChange(); \
    }

#define mHandleMonitorableComparePtrContents( memb, val ) \
    if ( (memb && !oth.memb) || (!memb && oth.memb) || !(*memb == *oth.memb) ) \
    { \
	if ( chgtype == cNoChange() || chgtype == val ) \
	    chgtype = val; \
	else \
	    return cEntireObjectChange(); \
    }

#define mDeliverMonitorableCompare() return chgtype;


/*!\brief Helper macro to implement a simple yes/no change compareClassData() */

#define mDeliverSingCondMonitorableCompare(nochgcond,chgtype) \
    return (nochgcond) ? cNoChange() : chgtype


/*!\brief Helper macro to implement a simple yes/no change compareClassData() */

#define mDeliverYesNoMonitorableCompare(nochgcond) \
    mDeliverSingCondMonitorableCompare( nochgcond, cEntireObjectChange() )


/*!\brief the get function used by mImplSimpleMonitoredGet */

template <class T>
inline T MonitoredObject::getMemberSimple( const T& memb ) const
{
    mLock4Read();
    return memb;
}


/*!\brief the set function used by mImplSimpleMonitoredSet */

template <class TMember,class TSetTo>
inline void MonitoredObject::setMemberSimple( TMember& memb, TSetTo setto,
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
