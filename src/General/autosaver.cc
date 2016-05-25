/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2016
-*/


#include "autosaver.h"
#include "saveable.h"
#include "iodir.h"
#include "iostrm.h"
#include "ioman.h"
#include "settings.h"
#include "separstr.h"
#include "timefun.h"
#include "msgh.h"

static const int defnrsecondsbetweensaves_ = 300;
static const char* sKeyIsActiveByDefault = "AutoSave.Active";
static const char* sKeyNrSecondsBetweenSaves = "AutoSave.Cycle Time";

static void setDefaultNrSecondsBetweenSaves( int nrsecs )
{
    Settings::common().set( sKeyNrSecondsBetweenSaves, nrsecs );
}

static int defaultNrSecondsBetweenSaves()
{
    int nrsecs = defnrsecondsbetweensaves_;
    Settings::common().get( sKeyNrSecondsBetweenSaves, nrsecs );
    return nrsecs;
}

static void setIsActiveByDefault( bool yn )
{
    Settings::common().setYN( sKeyIsActiveByDefault, yn );
}

static bool isActiveByDefault()
{
    bool yn = true;
    Settings::common().getYN( sKeyNrSecondsBetweenSaves, yn );
    return yn;
}


OD::Saveable::Saveable( const Monitorable& obj )
    : monitored_(&obj)
    , monitoredalive_(true)
{
    attachCBToObj();
    mTriggerInstanceCreatedNotifier();
}


OD::Saveable::Saveable( const Saveable& oth )
    : monitored_(oth.monitored_)
{
    *this = oth;
    mTriggerInstanceCreatedNotifier();
}


OD::Saveable::~Saveable()
{
    detachCBFromObj();
    sendDelNotif();
}


mImplMonitorableAssignment(OD::Saveable,Monitorable)

void OD::Saveable::copyClassData( const Saveable& oth )
{
    detachCBFromObj();
    monitored_ = oth.monitored_;
    monitoredalive_ = oth.monitoredalive_;
    storekey_ = oth.storekey_;
    ioobjpars_ = oth.ioobjpars_;
    errmsg_ = oth.errmsg_;
    lastsavedirtycount_ = oth.lastsavedirtycount_;
    attachCBToObj();
}


void OD::Saveable::setMonitored( const Monitorable& obj )
{
    if ( this == &obj )
	{ pErrMsg("Funny, but no go"); return; }
    mLock4Read();
    if ( monitored_ == &obj )
	return;

    AccessLockHandler alh( obj );
    mLock2Write();
    detachCBFromObj();
    monitored_ = &obj;
    monitoredalive_ = true;
    attachCBToObj();
    mSendEntireObjChgNotif();
}


void OD::Saveable::attachCBToObj()
{
    if ( monitoredalive_ )
	mAttachCB( const_cast<Monitorable&>(*monitored_).objectToBeDeleted(),
		   Saveable::objDelCB );
}


void OD::Saveable::detachCBFromObj()
{
    if ( monitoredalive_ )
	mDetachCB( const_cast<Monitorable&>(*monitored_).objectToBeDeleted(),
		   Saveable::objDelCB );
}


const Monitorable* OD::Saveable::monitored() const
{
    mLock4Read();
    return monitored_;
}


Monitorable::DirtyCountType OD::Saveable::curDirtyCount() const
{
    mLock4Read();
    return monitoredalive_ ? monitored_->dirtyCount()
			   : lastsavedirtycount_.get();
}


void OD::Saveable::objDelCB( CallBacker* )
{
    monitoredalive_ = false;
}


bool OD::Saveable::save() const
{
    mLock4Read();
    if ( !monitoredalive_ )
	{ pErrMsg("Attempt to save already deleted object"); return true; }

    PtrMan<IOObj> ioobj = IOM().get( storekey_ );
    if ( ioobj )
    {
	if ( !ioobj->pars().includes(ioobjpars_) )
	{
	    ioobj->pars().merge( ioobjpars_ );
	    IOM().commitChanges( *ioobj );
	    ioobj = IOM().get( storekey_ );
	}
	if ( store(*ioobj) )
	{
	    setNoSaveNeeded();
	    return true;
	}
    }

    if ( storekey_.isEmpty() )
	errmsg_ = tr("Cannot save object without database key");
    else
	errmsg_ = tr("Cannot find database entry for: %1").arg(storekey_);
    return false;
}


bool OD::Saveable::needsSave() const
{
    return !monitoredalive_ ? false
	 : lastsavedirtycount_ != monitored_->dirtyCount();
}


void OD::Saveable::setNoSaveNeeded() const
{
    if ( monitoredalive_ )
	lastsavedirtycount_ = monitored_->dirtyCount();
}


bool OD::Saveable::store( const IOObj& ioobj ) const
{
    if ( !monitoredalive_ )
	{ pErrMsg("Attempt to store already deleted object"); return true; }
    return doStore( ioobj );
}


OD::AutoSaver::AutoSaver( const Saveable& obj )
    : saver_(&obj)
    , nrclocksecondsbetweenautosaves_(defaultNrSecondsBetweenSaves())
    , autosavenr_(1)
    , lastautosaveioobj_(0)
{
    mAttachCB( saver_->objectToBeDeleted(), OD::AutoSaver::saverDelCB );
    mTriggerInstanceCreatedNotifier();
}


OD::AutoSaver::AutoSaver( const AutoSaver& oth )
    : Monitorable(oth)
    , lastautosaveioobj_(0)
{
    *this = oth;
    mTriggerInstanceCreatedNotifier();
}


OD::AutoSaver::~AutoSaver()
{
    sendDelNotif();
    detachAllNotifiers();
    delete lastautosaveioobj_;
}


bool OD::AutoSaver::isActive() const
{
    mLock4Read();
    return saver_ && saver_->monitoredAlive();
}


mImplMonitorableAssignment(OD::AutoSaver,Monitorable)

void OD::AutoSaver::copyClassData( const AutoSaver& oth )
{
    if ( saver_ )
	mDetachCB( saver_->objectToBeDeleted(), OD::AutoSaver::saverDelCB );

    saver_ = oth.saver_;
    nrclocksecondsbetweenautosaves_ = oth.nrclocksecondsbetweenautosaves_;
    lastautosaveclockseconds_ = oth.lastautosaveclockseconds_;
    curclockseconds_ = oth.curclockseconds_;
    delete lastautosaveioobj_;
    if ( oth.lastautosaveioobj_ )
	lastautosaveioobj_ = new IOStream( *oth.lastautosaveioobj_ );
    else
	lastautosaveioobj_ = 0;
    mAttachCB( saver_->objectToBeDeleted(), OD::AutoSaver::saverDelCB );
}


void OD::AutoSaver::saverDelCB( CallBacker* )
{
    saver_ = 0;
}


void OD::AutoSaver::removePrevAutoSaved() const
{
    if ( !lastautosaveioobj_ )
	return;

    lastautosaveioobj_->implRemove();
    IOM().permRemove( lastautosaveioobj_->key() );
    delete lastautosaveioobj_;
    lastautosaveioobj_ = 0;
}


void OD::AutoSaver::initAutoSave() const
{
    lastautosavedirtycount_ = saver_ ? saver_->curDirtyCount() : -1;
}


bool OD::AutoSaver::needsAutoSaveAct( int clockseconds ) const
{
    mLock4Write();
    curclockseconds_ = clockseconds;
    return curclockseconds_ - lastautosaveclockseconds_
	>= nrclocksecondsbetweenautosaves_;
}


bool OD::AutoSaver::autoSave() const
{
    return !saver_ || doAutoSaveWork(false);
}



void OD::AutoSaver::userSaveOccurred() const
{
    removePrevAutoSaved();
    lastautosaveclockseconds_ = curclockseconds_ + 1;
}


bool OD::AutoSaver::doAutoSaveWork( bool forcesave ) const
{
    if ( !saver_ )
	return true;

    mLock4Write();

    const DirtyCountType dirtycount = saver_->curDirtyCount();
    lastautosaveclockseconds_ = curclockseconds_;
    if ( !forcesave && (dirtycount == saver_->lastSavedDirtyCount()
		     || dirtycount == lastautosavedirtycount_ ) )
	return true;

    const MultiID saverkey( saver_->key() );
    const IODir iodir( saverkey );
    BufferString storenm( ".autosave_", saverkey, "_" );
    storenm.add( autosavenr_++ );
    IOStream* newstoreioobj = new IOStream( storenm, iodir.newTmpKey(), true );
    newstoreioobj->pars().update( sKey::CrFrom(), saverkey );
    newstoreioobj->pars().update( sKey::CrInfo(), "Auto-saved" );
    newstoreioobj->updateCreationPars();
    if ( !saver_->store(*newstoreioobj) )
    {
	delete newstoreioobj; newstoreioobj = 0;
	lastautosavedirtycount_ = 0;
	return false;
    }
    else
    {
	lastautosavedirtycount_ = saver_ ? saver_->curDirtyCount() : dirtycount;
	removePrevAutoSaved();
	lastautosaveioobj_ = newstoreioobj;
	if ( !saver_ )
	    removePrevAutoSaved();
    }

    return true;
}


static OD::AutoSaveMgr* themgr_ = 0;
static Threads::Lock themgrlock_;
OD::AutoSaveMgr& OD::AutoSaveMgr::getInst()
{
    Threads::Locker locker( themgrlock_ );
    if ( !themgr_ )
	themgr_ = new OD::AutoSaveMgr;
    return *themgr_;
}


OD::AutoSaveMgr::AutoSaveMgr()
    : appexits_(false)
    , active_(isActiveByDefault())
    , saveDone(this)
    , saveFailed(this)
{
    mAttachCB( IOM().surveyToBeChanged, AutoSaveMgr::setEmpty );
    mAttachCB( IOM().applicationClosing, AutoSaveMgr::appExits );
    thread_ = new Threads::Thread( mSCB(AutoSaveMgr::goCB), "AutoSave Manager");
}


OD::AutoSaveMgr::~AutoSaveMgr()
{
    thread_->waitForFinish();
    delete thread_;
}


void OD::AutoSaveMgr::add( AutoSaver* saver )
{
    if ( !saver )
	return;

    mAttachCB( saver->objectToBeDeleted(), AutoSaveMgr::saverDelCB );
    Threads::Locker locker( lock_ );
    saver->initAutoSave();
    savers_ += saver;
}


void OD::AutoSaveMgr::setEmpty()
{
    Threads::Locker locker( lock_ );
    deepErase( savers_ );
}


bool OD::AutoSaveMgr::isActive( bool fordef ) const
{
    if ( fordef )
	return isActiveByDefault();

    Threads::Locker locker( lock_ );
    return active_;
}


void OD::AutoSaveMgr::setActive( bool yn, bool mkdef )
{
    if ( mkdef )
	setIsActiveByDefault( yn );

    Threads::Locker locker( lock_ );
    active_ = yn;
}


int OD::AutoSaveMgr::nrSecondsBetweenSaves() const
{
    return defaultNrSecondsBetweenSaves();
}


void OD::AutoSaveMgr::setNrSecondsBetweenSaves( int nrsecs )
{
    if ( nrsecs < 1 )
	{ setActive( false ); return; }

    setDefaultNrSecondsBetweenSaves( nrsecs );
    Threads::Locker locker( lock_ );
    for ( int isvr=0; isvr<savers_.size(); isvr++ )
	savers_[isvr]->setNrSecondsBetweenSaves( nrsecs );
}


void OD::AutoSaveMgr::saverDelCB( CallBacker* cb )
{
    mDynamicCastGet(AutoSaver*,autosaver,cb)
    if ( !autosaver )
    {
	if ( !cb )
	    { pErrMsg( "Add a sendDelNotif() to your destructor" ); return; }
	autosaver = (AutoSaver*)cb;
	remove( autosaver );
    }
}


void OD::AutoSaveMgr::remove( AutoSaver* autosaver )
{
    Threads::Locker locker( lock_ );
    const int idxof = savers_.indexOf( autosaver );
    if ( idxof < 0 )
	return; // no pErrMsg so you can always remove them

    savers_.removeSingle( idxof );
}


void OD::AutoSaveMgr::appExits( CallBacker* )
{
    detachAllNotifiers();
    setEmpty();
    Threads::Locker locker( lock_ );
    appexits_ = true;
}


void OD::AutoSaveMgr::goCB( CallBacker* )
{
    getInst().go();
}


void OD::AutoSaveMgr::go()
{
    const int time0ms = Time::getMilliSeconds();
    int prevnrcycles = -1;

    while ( true )
    {
	bool isactive; ObjectSet<AutoSaver> svrs;
	Threads::Locker locker( lock_ );
	    if ( appexits_ )
		break;
	    isactive = active_;
	    svrs = savers_;
	locker.unlockNow();

	const int mselapsed = Time::getMilliSeconds() - time0ms;
	const int nrcycles = mselapsed / 1000;
	if ( nrcycles == prevnrcycles )
	    { Threads::sleep( 1000 - mselapsed % 1000 ); continue; }
	prevnrcycles = nrcycles;
	if ( !isactive )
	    continue;

	ObjectSet<AutoSaver> finishedsvrs;
	for ( int isvr=0; isvr<svrs.size(); isvr++ )
	{
	    AutoSaver* saver = svrs[isvr];
	    if ( !saver->isActive() )
		finishedsvrs += saver;
	    else if ( saver->needsAutoSaveAct(nrcycles) )
		(saver->autoSave() ? saveDone : saveFailed).trigger( saver );
	}

	if ( !finishedsvrs.isEmpty() )
	{
	    locker.reLock();
	    for ( int isvr=0; isvr<finishedsvrs.size(); isvr++ )
	    {
		AutoSaver* saver = finishedsvrs[isvr];
		const int idxof = savers_.indexOf( saver );
		if ( idxof >= 0 )
		    delete savers_.removeSingle( idxof );
	    }
	}
    }
}
