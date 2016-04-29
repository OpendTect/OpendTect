/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2016
-*/


#include "autosaver.h"
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
    : obj_(obj)
    , objdeleted_(false)
{
    mAttachCB( const_cast<Monitorable&>(obj).objectToBeDeleted(),
	       Saveable::objDel );
    mTriggerInstanceCreatedNotifier();
}


OD::Saveable::~Saveable()
{
    detachAllNotifiers();
    sendDelNotif();
}


void OD::Saveable::objDel( CallBacker* )
{
    mLock4Write();
    objdeleted_ = true;
}


bool OD::Saveable::store( const IOObj& ioobj ) const
{
    return doStore( ioobj );
}


OD::AutoSaveable::AutoSaveable( const Monitorable& obj )
    : Saveable(obj)
    , nrclocksecondsbetweensaves_(defaultNrSecondsBetweenSaves())
    , autosavenr_(1)
    , prevautosaveioobj_(0)
{
    mTriggerInstanceCreatedNotifier();
}


OD::AutoSaveable::~AutoSaveable()
{
    detachAllNotifiers();
    sendDelNotif();
    delete prevautosaveioobj_;
}


bool OD::AutoSaveable::store( const IOObj& ioobj ) const
{
    if ( !doStore(ioobj) )
	return false;

    userSaveOccurred();
    return true;
}


void OD::AutoSaveable::remove( const IOObj& ioobj ) const
{
    ioobj.implRemove();
}


void OD::AutoSaveable::removePrevAutoSaved() const
{
    if ( !prevautosaveioobj_ )
	return;

    remove( *prevautosaveioobj_ );
    IOM().permRemove( prevautosaveioobj_->key() );
    delete prevautosaveioobj_;
    prevautosaveioobj_ = 0;
}


void OD::AutoSaveable::initAutoSave() const
{
    prevfingerprint_ = getFingerPrint();
}


bool OD::AutoSaveable::needsAutoSaveAct( int clockseconds ) const
{
    mLock4Read();
    if ( objdeleted_ || (!mLock2Write() && objdeleted_) )
	return false;

    curclockseconds_ = clockseconds;
    return curclockseconds_ - lastsaveclockseconds_
	>= nrclocksecondsbetweensaves_;
}


bool OD::AutoSaveable::autoSave() const
{
    return isFinished() || doAutoSaveWork(false);
}



void OD::AutoSaveable::userSaveOccurred() const
{
    removePrevAutoSaved();
    lastsaveclockseconds_ = curclockseconds_ + 1;
}


bool OD::AutoSaveable::doAutoSaveWork( bool forcesave ) const
{
    AccessLockHandler lockhandler( obj_ );	// locks the object
    mLock4Read();				// locks me
    if ( objdeleted_ )
	return true;

    lastsaveclockseconds_ = curclockseconds_;
    const BufferString fingerprint( getFingerPrint() );
    if ( !forcesave && fingerprint == prevfingerprint_ )
	return true;

    const IODir iodir( key_ );
    BufferString storenm( ".autosave_", key_, "_" );
    storenm.add( autosavenr_++ );
    IOStream* newstoreioobj = new IOStream( storenm, iodir.newTmpKey(), true );
    newstoreioobj->pars().update( sKey::CrFrom(), key_ );
    newstoreioobj->pars().update( sKey::CrInfo(), "Auto-saved" );
    newstoreioobj->updateCreationPars();
    if ( !doStore(*newstoreioobj) )
    {
	delete newstoreioobj; newstoreioobj = 0;
	prevfingerprint_.setEmpty();
	return false;
    }
    else
    {
	prevfingerprint_ = fingerprint;
	removePrevAutoSaved();
	prevautosaveioobj_ = newstoreioobj;
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


void OD::AutoSaveMgr::add( AutoSaveable* saver )
{
    if ( !saver )
	return;

    mAttachCB( saver->objectToBeDeleted(), AutoSaveMgr::saverDel );
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


void OD::AutoSaveMgr::saverDel( CallBacker* cb )
{
    mDynamicCastGet(AutoSaveable*,autosaver,cb)
    if ( !autosaver )
    {
	if ( !cb )
	    { pErrMsg( "Huh" ); return; }
	pErrMsg( "Add a sendDelNotif() to your destructor" );
	autosaver = (AutoSaveable*)cb;
    }
}


void OD::AutoSaveMgr::remove( AutoSaveable* autosaver )
{
    Threads::Locker locker( lock_ );
    const int idxof = savers_.indexOf( autosaver );
    if ( idxof < 0 )
	return; // no pErrMsg so you can always remove them

    savers_.removeSingle( idxof );
}


void OD::AutoSaveMgr::appExits( CallBacker* )
{
    setEmpty();
    Threads::Locker locker( lock_ );
    detachAllNotifiers();
    appexits_ = true;
    locker.unlockNow();
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
	bool isactive; ObjectSet<AutoSaveable> svrs;
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

	ObjectSet<AutoSaveable> finishedsvrs;
	for ( int isvr=0; isvr<svrs.size(); isvr++ )
	{
	    AutoSaveable* saver = svrs[isvr];
	    if ( saver->isFinished() )
		finishedsvrs += saver;
	    else if ( saver->needsAutoSaveAct(nrcycles) )
		(saver->autoSave() ? saveDone : saveFailed).trigger( saver );
	}

	if ( !finishedsvrs.isEmpty() )
	{
	    locker.reLock();
	    for ( int isvr=0; isvr<finishedsvrs.size(); isvr++ )
	    {
		AutoSaveable* saver = finishedsvrs[isvr];
		const int idxof = savers_.indexOf( saver );
		if ( idxof >= 0 )
		    delete savers_.removeSingle( idxof );
	    }
	}
    }
}
