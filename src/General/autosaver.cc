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


namespace OD
{

mExpClass(General) AutoSaveObj : public CallBacker
{
public:

    typedef Monitorable::DirtyCountType DirtyCountType;

			AutoSaveObj(const Saveable&,AutoSaver&);
			~AutoSaveObj();

    bool		isActive() const;

    const Saveable*	saver_;
    mutable Threads::Lock lock_;
    AutoSaver&		mgr_;

    mutable DirtyCountType lastsavedirtycount_;
    mutable int		lastsaveclockseconds_;
    mutable IOStream*	lastautosaveioobj_;
    mutable int		autosavenr_;

    bool		needsAutoSave() const;
    int			autoSave() const;

    void		removeIOObjAndData(IOStream*&) const;
    void		saverDelCB(CallBacker*);

};

} // namespace OD



OD::AutoSaveObj::AutoSaveObj( const Saveable& obj, AutoSaver& mgr )
    : saver_(&obj)
    , mgr_(mgr)
    , lastsavedirtycount_(-1)
    , lastsaveclockseconds_(mgr.curclockseconds_)
    , lastautosaveioobj_(0)
    , autosavenr_(1)
{
    mAttachCB( saver_->objectToBeDeleted(), OD::AutoSaveObj::saverDelCB );
}


OD::AutoSaveObj::~AutoSaveObj()
{
    detachAllNotifiers();
    delete lastautosaveioobj_;
}


bool OD::AutoSaveObj::isActive() const
{
    Threads::Locker locker( lock_ );
    return saver_ && saver_->monitoredAlive();
}


void OD::AutoSaveObj::saverDelCB( CallBacker* )
{
    Threads::Locker locker( lock_ );
    saver_ = 0;
}


void OD::AutoSaveObj::removeIOObjAndData( IOStream*& ioobj ) const
{
    if ( !ioobj )
	return;

    ioobj->implRemove();
    IOM().permRemove( ioobj->key() );
    delete ioobj;
    ioobj = 0;
}


bool OD::AutoSaveObj::needsAutoSave() const
{
    Threads::Locker locker( lock_ );
    return mgr_.curclockseconds_ - lastsaveclockseconds_
	>= mgr_.nrclocksecondsbetweenautosaves_;
}


int OD::AutoSaveObj::autoSave() const
{
    Threads::Locker locker( lock_ );
    if ( !saver_ || !saver_->monitoredAlive() )
	return true;

    const DirtyCountType curdirtycount = saver_->curDirtyCount();
    const bool objusersaved = curdirtycount == saver_->lastSavedDirtyCount();
    const bool objautosaved = curdirtycount == lastsavedirtycount_;
    lastsavedirtycount_ = curdirtycount;
    lastsaveclockseconds_ = mgr_.curclockseconds_;
    if ( objusersaved || objautosaved )
	return 0;

    const MultiID saverkey( saver_->key() );
    const IODir iodir( saverkey );
    BufferString storenm( ".autosave_", saverkey, "_" );
    storenm.add( autosavenr_++ );
    IOStream* newstoreioobj = new IOStream( storenm, iodir.newTmpKey(), true );
    newstoreioobj->pars().update( sKey::CrFrom(), saverkey );
    newstoreioobj->pars().update( sKey::CrInfo(), "Auto-saved" );
    newstoreioobj->updateCreationPars();
    if ( !saver_->store( *newstoreioobj ) )
    {
	lastsavedirtycount_ = saver_->lastSavedDirtyCount();
	removeIOObjAndData( newstoreioobj );
	return false;
    }

    removeIOObjAndData( lastautosaveioobj_ );
    lastautosaveioobj_ = newstoreioobj;
    return true;
}


static OD::AutoSaver* themgr_ = 0;
static Threads::Lock themgrlock_;
OD::AutoSaver& OD::AutoSaver::getInst()
{
    Threads::Locker locker( themgrlock_ );
    if ( !themgr_ )
	themgr_ = new OD::AutoSaver;
    return *themgr_;
}


OD::AutoSaver::AutoSaver()
    : appexits_(false)
    , active_(isActiveByDefault())
    , curclockseconds_(-1)
    , nrclocksecondsbetweenautosaves_(defaultNrSecondsBetweenSaves())
    , saveDone(this)
    , saveFailed(this)
{
    mAttachCB( IOM().surveyToBeChanged, AutoSaver::setEmpty );
    mAttachCB( IOM().applicationClosing, AutoSaver::appExits );
    thread_ = new Threads::Thread( mSCB(AutoSaver::goCB), "AutoSave Manager");
}


OD::AutoSaver::~AutoSaver()
{
    thread_->waitForFinish();
    delete thread_;
}


void OD::AutoSaver::add( const Saveable& saver )
{
    AutoSaveObj* asobj = new AutoSaveObj( saver, *this );
    mAttachCB( saver.objectToBeDeleted(), AutoSaver::svrDelCB );
    Threads::Locker locker( lock_ );
    asobjs_ += asobj;
}


void OD::AutoSaver::setEmpty()
{
    Threads::Locker locker( lock_ );
    deepErase( asobjs_ );
}


bool OD::AutoSaver::isActive( bool fordef ) const
{
    if ( fordef )
	return isActiveByDefault();

    Threads::Locker locker( lock_ );
    return active_;
}


void OD::AutoSaver::setActive( bool yn, bool mkdef )
{
    if ( mkdef )
	setIsActiveByDefault( yn );

    Threads::Locker locker( lock_ );
    active_ = yn;
}


int OD::AutoSaver::nrSecondsBetweenSaves() const
{
    Threads::Locker locker( lock_ );
    return nrclocksecondsbetweenautosaves_;
}


void OD::AutoSaver::setNrSecondsBetweenSaves( int nrsecs )
{
    if ( nrsecs < 1 )
	{ setActive( false ); return; }

    setDefaultNrSecondsBetweenSaves( nrsecs );
    Threads::Locker locker( lock_ );
    nrclocksecondsbetweenautosaves_ = nrsecs;
}


void OD::AutoSaver::svrDelCB( CallBacker* cb )
{
    if ( !cb )
	{ pErrMsg("Huh"); return; }

    mDynamicCastGet(Saveable*,svr,cb)
    if ( !svr )
    {
	pErrMsg( "Add a sendDelNotif() to your destructor" );
	svr = (Saveable*)cb;
    }

    Threads::Locker locker( lock_ );
    for ( int iasobj=0; iasobj<asobjs_.size(); iasobj++ )
    {
	if ( svr == asobjs_[iasobj]->saver_ )
	{
	    delete asobjs_.removeSingle( iasobj );
	    return;
	}
    }
}


void OD::AutoSaver::appExits( CallBacker* )
{
    detachAllNotifiers();
    setEmpty();
    Threads::Locker locker( lock_ );
    appexits_ = true;
}


void OD::AutoSaver::goCB( CallBacker* )
{
    getInst().go();
}


void OD::AutoSaver::go()
{
    const int time0ms = Time::getMilliSeconds();
    int prevclockseconds = curclockseconds_;

    while ( true )
    {
	bool isactive; ObjectSet<AutoSaveObj> asobjs;
	Threads::Locker locker( lock_ );
	    if ( appexits_ )
		break;
	    isactive = active_;
	    asobjs = asobjs_;
	locker.unlockNow();

	const int mselapsed = Time::getMilliSeconds() - time0ms;
	curclockseconds_ = mselapsed / 1000;
	if ( curclockseconds_ == prevclockseconds )
	    { Threads::sleep( (1000 - mselapsed % 1000) * 0.001 ); continue; }
	prevclockseconds = curclockseconds_;
	if ( !isactive )
	    continue;

	ObjectSet<AutoSaveObj> finishedasobjs;
	for ( int iasobj=0; iasobj<asobjs.size(); iasobj++ )
	{
	    AutoSaveObj* asobj = asobjs[iasobj];
	    if ( !asobj->isActive() )
		finishedasobjs += asobj;
	    else if ( asobj->needsAutoSave() )
	    {
		const int res = asobj->autoSave();
		if ( res < 0 )
		    saveFailed.trigger( asobj );
		else if ( res > 0 )
		    saveDone.trigger( asobj );
	    }
	}

	if ( !finishedasobjs.isEmpty() )
	{
	    locker.reLock();
	    for ( int iasobj=0; iasobj<finishedasobjs.size(); iasobj++ )
	    {
		AutoSaveObj* asobj = finishedasobjs[iasobj];
		const int idxof = asobjs_.indexOf( asobj );
		if ( idxof >= 0 )
		    delete asobjs_.removeSingle( idxof );
	    }
	}
    }
}
