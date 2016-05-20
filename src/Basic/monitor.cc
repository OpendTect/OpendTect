/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/


#include "monitor.h"
#include "thread.h"


mDefineInstanceCreatedNotifierAccess(Monitorable)

Monitorable::AccessLockHandler::AccessLockHandler( const Monitorable& obj,
						   bool forread )
    : obj_(obj)
{
    if ( forread )
	locker_ = new Threads::Locker( obj.accesslock_,
				       Threads::Locker::ReadLock );
    else
    {
	waitForMonitors();
	locker_ = new Threads::Locker( obj.accesslock_,
				       Threads::Locker::WriteLock );
    }
}


Monitorable::AccessLockHandler::~AccessLockHandler()
{
    delete locker_;
}


void Monitorable::AccessLockHandler::waitForMonitors()
{
    while ( obj_.nrmonitors_ > 0 )
	Threads::sleep( 1 );
}


bool Monitorable::AccessLockHandler::convertToWrite()
{
    waitForMonitors();
    bool rv = locker_->convertToWriteLock();
    waitForMonitors();
    return rv;
}


Monitorable::Monitorable()
    : nrmonitors_(0)
    , dirtycount_(0)
    , chgnotifblocklevel_(0)
    , chgnotif_(this)
    , delnotif_(this)
    , delalreadytriggered_(false)
    , accesslock_(Threads::Lock::MultiRead)
{
    mTriggerInstanceCreatedNotifier();
}


Monitorable::Monitorable( const Monitorable& oth )
    : CallBacker(oth)
    , nrmonitors_(0)
    , chgnotifblocklevel_(0)
    , dirtycount_(0)
    , chgnotif_(this)
    , delnotif_(this)
    , delalreadytriggered_(false)
{
    // no operator==, we want to copy nothing
    mTriggerInstanceCreatedNotifier();
}


Monitorable::~Monitorable()
{
    if ( nrmonitors_ > 0 )
	{ pErrMsg(BufferString("Nr monitors should be 0, is ",nrmonitors_)); }
    sendDelNotif();
}



Monitorable& Monitorable::operator =( const Monitorable& )
{
    // copying nothing. no locking, monitors, notification - nothing.
    touch();
    return *this;
}


void Monitorable::resumeChangeNotifications() const
{
    if ( chgnotifblocklevel_ < 1 )
	{ pErrMsg( "chgnotifblocklevel_ < 1 " ); }
    else
	chgnotifblocklevel_--;
}


void Monitorable::sendEntireObjectChangeNotification() const
{
    if ( !chgnotifblocklevel_ )
	objectChanged().trigger( ChangeData(cEntireObjectChangeType(),
					    cEntireObjectChangeSubIdx()) );
}


void Monitorable::sendChgNotif( AccessLockHandler& hndlr, ChangeType ct,
				SubIdxType subidx ) const
{
    touch();
    hndlr.unlockNow();
    if ( chgnotifblocklevel_ )
	return;

    objectChanged().trigger( ChangeData(ct,subidx) );
}


void Monitorable::sendDelNotif() const
{
    if ( !delalreadytriggered_ )
    {
	delalreadytriggered_ = true;
	objectToBeDeleted().trigger();
	// this should even work from ~Monitorable(), using delnotif_
    }
}


Monitorable::ChangeType Monitorable::changeNotificationTypeOf( CallBacker* cb )
{
    mCBCapsuleUnpack( ChangeData, chgdata, cb );
    return chgdata.changeType();
}


MonitorLock::MonitorLock( const Monitorable& obj )
    : obj_(obj)
    , unlocked_(false)
{
    obj_.nrmonitors_++;
}


MonitorLock::~MonitorLock()
{
    if ( !unlocked_ )
    {
	if ( obj_.nrmonitors_ < 1 )
	    { pErrMsg(BufferString("Nr monitors == ",obj_.nrmonitors_)); }
	else
	    obj_.nrmonitors_--;
    }
}


void MonitorLock::unlockNow()
{
    if ( !unlocked_ )
    {
	unlocked_ = true;
	if ( obj_.nrmonitors_ > 0 )
	    obj_.nrmonitors_--;
    }
}


void MonitorLock::reLock()
{
    if ( unlocked_ )
    {
	unlocked_ = false;
	obj_.nrmonitors_++;
    }
}


ChangeNotifyBlocker::ChangeNotifyBlocker( const Monitorable& obj )
    : obj_(obj)
    , unblocked_(true)
{
    reBlock();
}


ChangeNotifyBlocker::~ChangeNotifyBlocker()
{
    unBlockNow();
}


void ChangeNotifyBlocker::unBlockNow( bool sendnotif )
{
    if ( !unblocked_ )
    {
	obj_.resumeChangeNotifications();
	unblocked_ = true;
	if ( sendnotif )
	    obj_.sendEntireObjectChangeNotification();
    }
}


void ChangeNotifyBlocker::reBlock()
{
    if ( unblocked_ )
	obj_.stopChangeNotifications();
}
