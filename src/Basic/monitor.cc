/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/


#include "monitorchangerecorder.h"
#include "sharedobject.h"
#include "thread.h"
#include "uistrings.h"

static const int maxnrchangerecs_ = 100;


mDefineInstanceCreatedNotifierAccess(Monitorable)
mDefineInstanceCreatedNotifierAccess(SharedObject)


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


Monitorable& Monitorable::operator =( const Monitorable& oth )
{
    // copyAll does nothing, so nothing here
    return *this;
}


void Monitorable::copyAll( const Monitorable& oth )
{
    // Copying nothing. No locking, monitors, notification - nothing.
    // The function is here so subclasses can call it from their
    // mImplMonitorableAssignment macro expansion.
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
					    cEntireObjectChangeID()) );
}


void Monitorable::sendChgNotif( AccessLockHandler& hndlr, ChangeType ct,
				IDType id ) const
{
    touch();
    hndlr.unlockNow();
    if ( chgnotifblocklevel_ )
	return;

    objectChanged().trigger( ChangeData(ct,id) );
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
    {
	obj_.stopChangeNotifications();
	unblocked_ = false;
    }
}


uiString ChangeRecorder::Record::actionText( Action act ) const
{
    return tr( "%1 %2" )
	.arg( act == Undo ? uiStrings::sUndo() : uiStrings::sRedo() )
	.arg( name() );
}


ChangeRecorder::ChangeRecorder( Monitorable& obj, const char* nm )
    : NamedMonitorable(nm)
    , obj_(&obj)
{
    init();
}


ChangeRecorder::ChangeRecorder( const Monitorable& obj, const char* nm )
    : NamedMonitorable(nm)
    , obj_(const_cast<Monitorable*>(&obj))
{
    init();
}


void ChangeRecorder::init()
{
    idx4redo_ = 0;
    applying_ = false;
    mAttachCB( obj_->objectToBeDeleted(), ChangeRecorder::objDel );
    mAttachCB( obj_->objectChanged(), ChangeRecorder::objChg );
}


ChangeRecorder::~ChangeRecorder()
{
    detachAllNotifiers();
    deepErase( recs_ );
}


ChangeRecorder::ChangeRecorder( const ChangeRecorder& oth )
{
    copyAll( oth );
}

mImplMonitorableAssignment(ChangeRecorder,NamedMonitorable)

void ChangeRecorder::copyClassData( const ChangeRecorder& oth )
{
    if ( obj_ )
	mDetachCB( obj_->objectToBeDeleted(), ChangeRecorder::objDel );
    obj_ = oth.obj_;
    deepErase( recs_ );
    deepCopyClone( recs_, oth.recs_ );
    idx4redo_ = oth.idx4redo_;
    if ( obj_ )
	mAttachCB( obj_->objectToBeDeleted(), ChangeRecorder::objDel );
}


bool ChangeRecorder::isEmpty() const
{
    mLock4Read();
    return recs_.isEmpty();
}


void ChangeRecorder::setEmpty()
{
    if ( isEmpty() )
	return;
    mLock4Write();
    clear();
    mSendEntireObjChgNotif();
}


void ChangeRecorder::clear()
{
    deepErase( recs_ );
    idx4redo_ = 0;
}


bool ChangeRecorder::canApply( Action act ) const
{
    mLock4Read();
    return act == Undo ? idx4redo_ > 0 : idx4redo_ < recs_.size();
}


uiString ChangeRecorder::usrText( Action act ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( act );
    return recs_.validIdx( idx ) ? recs_[idx]->actionText(act)
				 : uiStrings::sNone();
}


bool ChangeRecorder::apply( Action act )
{
    mLock4Read();
    if ( !obj_ )
	return false;

    IdxType idx = gtIdx( act );
    if ( !recs_.validIdx( idx ) )
	return false;

    if ( !mLock2Write() )
    {
	idx = gtIdx( act );
	if ( !recs_.validIdx( idx ) )
	    return false;
    }

    applying_ = true;
    const bool rv = recs_[idx]->apply( *obj_, act );
    applying_ = false;

    idx4redo_ += act == Undo ? -1 : 1;
    if ( idx4redo_ < 0 )
	idx4redo_ = 0;
    else if ( idx4redo_ > recs_.size() )
	idx4redo_ = recs_.size();

    return rv;
}


void ChangeRecorder::addRec( Record* rec )
{
    const int newidx = idx4redo_;
    while ( recs_.size() > newidx )
	delete recs_.removeSingle( recs_.size()-1 );

    recs_.add( rec );
    idx4redo_ = recs_.size();

    while ( recs_.size() > maxnrchangerecs_ )
    {
	delete recs_.removeSingle( 0 );
	idx4redo_--;
    }
}


ChangeRecorder::IdxType ChangeRecorder::gtIdx( Action act ) const
{
    return act == Undo ? idx4redo_-1 : idx4redo_;
}


void ChangeRecorder::objDel( CallBacker* )
{
    mLock4Write();
    obj_ = 0;
}


void ChangeRecorder::objChg( CallBacker* cb )
{
    if ( !applying_ )
    {
	mGetMonitoredChgData( cb, chgdata );
	handleObjChange( chgdata );
    }
}


SharedObject::SharedObject( const char* nm )
    : NamedMonitorable(nm)
{
    mTriggerInstanceCreatedNotifier();
}


SharedObject::SharedObject( const SharedObject& oth )
    : NamedMonitorable(oth)
{
    copyAll( oth );
    mTriggerInstanceCreatedNotifier();
}


SharedObject::~SharedObject()
{
    sendDelNotif();
}


mImplMonitorableAssignment( SharedObject, NamedMonitorable )

void SharedObject::copyClassData( const SharedObject& oth )
{
}
