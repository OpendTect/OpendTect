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
    , changemonitorstoplevel_(0)
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
    , changemonitorstoplevel_(0)
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
    if ( changemonitorstoplevel_ < 1 )
	{ pErrMsg( "changemonitorstoplevel_ < 1 " ); }
    else
	changemonitorstoplevel_--;
}


void Monitorable::sendEntireObjectChangeNotification() const
{
    if ( !changemonitorstoplevel_ )
	objectChanged().trigger( ChangeData(cEntireObjectChangeType(),
					    cEntireObjectChangeSubIdx()) );
}


void Monitorable::sendChgNotif( AccessLockHandler& hndlr, ChangeType ct,
				SubIdxType subidx ) const
{
    touch();
    hndlr.unlockNow();
    if ( changemonitorstoplevel_ )
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


MonitorLock::MonitorLock( const Monitorable& obj )
    : obj_(obj)
    , needunlock_(true)
{
    obj_.nrmonitors_++;
}


MonitorLock::~MonitorLock()
{
    if ( needunlock_ )
    {
	if ( obj_.nrmonitors_ < 1 )
	    { pErrMsg(BufferString("Nr monitors == ",obj_.nrmonitors_)); }
	else
	    obj_.nrmonitors_--;
    }
}


void MonitorLock::unlockNow()
{
    if ( needunlock_ )
    {
	needunlock_ = false;
	if ( obj_.nrmonitors_ > 0 )
	    obj_.nrmonitors_--;
    }
}


void MonitorLock::reLock()
{
    if ( !needunlock_ )
    {
	needunlock_ = true;
	obj_.nrmonitors_++;
    }
}
