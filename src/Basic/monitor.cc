/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/


#include "monitorchangerecorder.h"
#include "sharedobject.h"
#include "uistrings.h"

static const int maxnrchangerecs_ = 100;
static bool lockingactive_ = false;


mDefineInstanceCreatedNotifierAccess(Monitorable)
mDefineInstanceCreatedNotifierAccess(SharedObject)


void Monitorable::AccessLocker::enableLocking( bool yn )
{
    lockingactive_ = yn;
}


Monitorable::AccessLocker::AccessLocker( const Monitorable& obj, bool forread )
{
    if ( lockingactive_ )
	thelock_ = new Locker( obj.accesslock_,
			forread ? Locker::ReadLock : Locker::WriteLock );
}


Monitorable::AccessLocker::AccessLocker( const AccessLocker& oth )
    : thelock_(nullptr)
{
}


Monitorable::AccessLocker::~AccessLocker()
{
    delete thelock_;
}


Monitorable::AccessLocker& Monitorable::AccessLocker::operator =(
					const AccessLocker& oth )
{
    if ( &oth != this )
	{ delete thelock_; thelock_ = nullptr; }
    return *this;
}


bool Monitorable::AccessLocker::isLocked() const
{
    return thelock_ ? thelock_->isLocked() : false;
}


void Monitorable::AccessLocker::unlockNow()
{
    if ( thelock_ )
	thelock_->unlockNow();
}


void Monitorable::AccessLocker::reLock( Locker::WaitType wt )
{
    if ( thelock_ )
	thelock_->reLock( wt );
}


bool Monitorable::AccessLocker::convertToWrite()
{
    return thelock_ ? thelock_->convertToWriteLock() : true;
}


Monitorable::ChangeData::ChangeData( const ChangeData& oth )
    : auxdata_(0)
{
    *this = oth;
}


Monitorable::ChangeData& Monitorable::ChangeData::operator =(
						const ChangeData& oth )
{
    if ( this != &oth )
    {
	std::pair<ChangeType,IDType>::operator =( oth );
	auxdata_ = oth.auxdata_;
    }
    return *this;
}


Monitorable::Monitorable()
    : dirtycount_(0)
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
    , chgnotifblocklevel_(0)
    , dirtycount_(oth.dirtycount_)
    , chgnotif_(this)
    , delnotif_(this)
    , delalreadytriggered_(false)
{
    mTriggerInstanceCreatedNotifier();
}


Monitorable::~Monitorable()
{
    sendDelNotif();
}


Monitorable& Monitorable::operator =( const Monitorable& oth )
{
    copyAll( oth );
    return *this;
}


bool Monitorable::operator ==( const Monitorable& oth ) const
{
    return compareWith( oth ) == cNoChange();
}


Monitorable::ChangeType Monitorable::compareWith( const Monitorable& oth ) const
{
    return cNoChange();
}


void Monitorable::copyAll( const Monitorable& oth )
{
    dirtycount_ = oth.dirtycount_;
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
	objectChanged().trigger( ChangeData(cEntireObjectChange(),
					    cEntireObjectChgID()) );
}


void Monitorable::sendChgNotif( AccessLocker& locker, ChangeType ct,
				IDType id ) const
{
    sendChgNotif( locker, ChangeData(ct,id) );
}


void Monitorable::sendChgNotif( AccessLocker& locker,
				const ChangeData& cd ) const
{
    locker.unlockNow();
    sendChangeNotification( cd );
}


void Monitorable::sendChangeNotification( const ChangeData& cd ) const
{
    touch();
    if ( chgnotifblocklevel_ < 1 )
	objectChanged().trigger( cd );
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


void Monitorable::transferNotifsTo( const Monitorable& to,
				    const CallBacker* onlyfor ) const
{
    objectChanged().transferCBSTo( to.objectChanged(), onlyfor, this );
    objectToBeDeleted().transferCBSTo( to.objectToBeDeleted(), onlyfor, this );
}


MonitorLock::MonitorLock( const Monitorable& obj )
    : locker_(obj,true)
    , unlocked_(false)
{
}


MonitorLock::~MonitorLock()
{
}


void MonitorLock::unlockNow()
{
    if ( !unlocked_ )
    {
	unlocked_ = true;
	locker_.unlockNow();
    }
}


void MonitorLock::reLock()
{
    if ( unlocked_ )
    {
	unlocked_ = false;
	locker_.reLock();
    }
}


ChangeNotifyBlocker::ChangeNotifyBlocker( const Monitorable& obj, bool snd )
    : obj_(obj)
    , unblocked_(true)
    , sendnotif_(snd)
{
    reBlock();
}


ChangeNotifyBlocker::~ChangeNotifyBlocker()
{
    unBlockNow();
}


void ChangeNotifyBlocker::unBlockNow()
{
    if ( !unblocked_ )
    {
	obj_.resumeChangeNotifications();
	unblocked_ = true;
	if ( sendnotif_ )
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
    return toUiString("%1 %2")
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
    copyClassData( oth );
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


Monitorable::ChangeType ChangeRecorder::compareClassData(
				    const ChangeRecorder& oth ) const
{
    // not worth the effort
    return cEntireObjectChange();
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
    const idx_type idx = gtIdx( act );
    return recs_.validIdx( idx ) ? recs_[idx]->actionText(act)
				 : uiStrings::sNone();
}


bool ChangeRecorder::apply( Action act )
{
    mLock4Read();
    if ( !obj_ )
	return false;

    idx_type idx = gtIdx( act );
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


ChangeRecorder::idx_type ChangeRecorder::gtIdx( Action act ) const
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
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


SharedObject::~SharedObject()
{
    sendDelNotif();
}


mImplMonitorableAssignmentWithNoMembers( SharedObject, NamedMonitorable )
