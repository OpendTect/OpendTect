#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "monitorable.h"


// Tools for implementation of subclasses of Monitorable


/*!\brief Defines simple MT-safe copyable member get. */

#define mImplSimpleMonitoredGet(fnnm,typ,memb) \
    typ fnnm() const { return getMemberSimple( memb ); }

/*!\brief Defines simple MT-safe copyable member change. */

#define mImplSimpleMonitoredSet(fnnm,typ,memb,chgtyp) \
    void fnnm( typ _set_to_ ) { setMemberSimple( memb, _set_to_, chgtyp, \
				    cUnspecChgID() ); }

/*!\brief Defines simple MT-safe copyable member access.

  Example:

  mImplSimpleMonitoredGetSet( inline, color, setColor, Color, color_,
			      cColorChange() );

*/

#define mImplSimpleMonitoredGetSet(pfx,fnnmget,fnnmset,typ,memb,chgtyp) \
    pfx mImplSimpleMonitoredGet(fnnmget,typ,memb) \
    pfx mImplSimpleMonitoredSet(fnnmset,const typ&,memb,chgtyp)


#define mLock4Read() AccessLocker accesslocker_( *this )
#define mLock4Write() AccessLocker accesslocker_( *this, false )
#define mLock2Write() accesslocker_.convertToWrite()
#define mReLock() accesslocker_.reLock()
#define mUnlockAllAccess() accesslocker_.unlockNow()
#define mAccessLocker() accesslocker_
#define mSendChgNotif(typ,id) sendChgNotif(accesslocker_,typ,id)
#define mSendEntireObjChgNotif() \
    mSendChgNotif( cEntireObjectChange(), cEntireObjectChgID() )



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
        ChangeType	compareWith(const Monitorable&) const


/*!\brief Monitorable subclasses: assignment and comparison.

  Declares assignment method that will emit notifications, by default
  'Entire Object'. Because of the locking, assignment operators are essential.
  These will need to copy both the 'own' members aswell as base class members,
  and emit the notification afterwards. For this, you have to implement a
  function that copies only the class' own data (unlocked):
  void copyClassData(const clss&).

  To be able to provide more fine-grained notifications than always
  'Entire Object Changed', you also have to provide a method compareClassData().
  in this way we also define the equality operators (== and !=).

  */

#define mDeclAbstractMonitorableAssignment(clss) \
    mDeclGenMonitorableAssignment(clss); \
    virtual clss* clone() const		= 0

/*!\brief like mDeclAbstractMonitorableAssignment but for
  non-abstract subclasses. Implements the clone() method. */

#define mDeclMonitorableAssignment(clss) \
    mDeclGenMonitorableAssignment(clss); \
    virtual clss* clone() const		{ return new clss( *this ); }


#define mImplEmptyMonitorableCopyClassData( clssnm ) \
void clssnm::copyClassData( const clssnm& oth ) \
{ \
}

#define mImplEmptyMonitorableCompare( clssnm ) \
Monitorable::ChangeType clssnm::compareClassData( const clssnm& oth ) const \
{ \
    return cNoChange(); \
}


/*!\brief Implementation of assignment and comparison methods for Monitorable's.

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
pfx Monitorable::ChangeType clss::compareWith( const Monitorable& mon ) const \
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

Monitorable::ChangeType Well::Info::compareClassData( const Info& oth ) const
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
inline T Monitorable::getMemberSimple( const T& memb ) const
{
    mLock4Read();
    return memb;
}


/*!\brief the set function used by mImplSimpleMonitoredSet */

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
