/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2016
-*/


#include "autosaver.h"
#include "saveable.h"
#include "dbdir.h"
#include "iostrm.h"
#include "dbman.h"
#include "settings.h"
#include "separstr.h"
#include "timefun.h"
#include "oddirs.h"
#include "transl.h"
#include "keystrs.h"
#include "taskrunner.h"
#include "uistrings.h"
#include "genc.h"

static const int defnrsecondsbetweensaves_ = 300;
static const char* sKeyIsActiveByDefault = "AutoSave.Active";
static const char* sKeyUseHiddenMode = "AutoSave.Hidden";
static const char* sKeyNrSecondsBetweenSaves = "AutoSave.Cycle Time";
const char* OD::AutoSaver::sKeyAutosaved()	{ return "Auto-saved"; }


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
    Settings::common().getYN( sKeyIsActiveByDefault, yn );
    return yn;
}

static void setUseHiddenModeByDefault( bool yn )
{
    Settings::common().setYN( sKeyUseHiddenMode, yn );
}

static bool useHiddenModeByDefault()
{
    bool yn = true;
    Settings::common().getYN( sKeyUseHiddenMode, yn );
    return yn;
}


OD::AutoSaveObj::AutoSaveObj( const Saveable& obj, AutoSaver& mgr )
    : saver_(&obj)
    , mgr_(mgr)
    , lastautosavedirtycount_(-1)
    , lastsaveclockseconds_(mgr.curclockseconds_)
    , lastautosaveioobj_(0)
    , autosavenr_(1)
    , prevsavefailed_(false)
{
    mAttachCB( saver_->objectToBeDeleted(), OD::AutoSaveObj::saverDelCB );
}


OD::AutoSaveObj::~AutoSaveObj()
{
    detachAllNotifiers();
    delete lastautosaveioobj_;
}


bool OD::AutoSaveObj::isFinished() const
{
    Threads::Locker locker( lock_ );
    return !saver_ || !saver_->objectAlive();
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
    ioobj->removeFromDB();
    delete ioobj;
    ioobj = 0;
}


bool OD::AutoSaveObj::time4AutoSave() const
{
    Threads::Locker locker( lock_ );
    return mgr_.curclockseconds_ - lastsaveclockseconds_
	>= mgr_.nrclocksecondsbetweensaves_;
}


int OD::AutoSaveObj::autoSave( bool hidden ) const
{
    Threads::Locker locker( lock_ );
    msgs_.setEmpty();
    if ( !saver_ || !saver_->objectAlive() )
	return 0;

    const DirtyCountType curdirtycount = saver_->curDirtyCount();
    const bool objusersaved = curdirtycount == saver_->lastSavedDirtyCount();
    SilentTaskRunnerProvider trprov;
    if ( !hidden )
    {
	if ( objusersaved )
	    return 0;
	msgs_ = saver_->save( trprov );
	return msgs_.isError() ? -1 : 1;
    }

    const bool objautosaved = curdirtycount == lastautosavedirtycount_;
    const DirtyCountType prevautosavedirtycount = lastautosavedirtycount_;
    lastautosavedirtycount_ = curdirtycount;
    lastsaveclockseconds_ = mgr_.curclockseconds_;
    if ( objusersaved || objautosaved )
	return 0;

    const DBKey saverkey( saver_->key() );
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( saverkey.dirID() );
    PtrMan<IOObj> orgioobj;
    if ( dbdir )
	orgioobj = dbdir->getEntry( saverkey.objID() );
    if ( !orgioobj )
	return 0;

    BufferString storenm( ".autosave_", saverkey, "_" );
    storenm.add( autosavenr_++ );
    IOStream* newstoreioobj = new IOStream( storenm, dbdir->newTmpKey(), true );
    newstoreioobj->setGroup( orgioobj->group() );
    newstoreioobj->setDirName( orgioobj->dirName() );
    newstoreioobj->setTranslator( orgioobj->translator() );
    newstoreioobj->pars() = orgioobj->pars();
    newstoreioobj->pars().set( sKey::CrFrom(), saverkey.toString(),
				orgioobj->name() );
    newstoreioobj->pars().set( AutoSaver::sKeyAutosaved(), sKey::Yes() );
    FileMultiString fms;
    fms.add( GetLocalHostName() ).add( GetPID() );
    newstoreioobj->pars().set( sKey::CrInfo(), fms );
    newstoreioobj->updateCreationPars();

    msgs_ = newstoreioobj->commitChanges();
    if ( msgs_.isOK() )
	msgs_ = saver_->store( *newstoreioobj, trprov );
    if ( !msgs_.isOK() )
    {
	lastautosavedirtycount_ = prevautosavedirtycount;
	removeIOObjAndData( newstoreioobj );
	return -1;
    }

    removeIOObjAndData( lastautosaveioobj_ );
    lastautosaveioobj_ = newstoreioobj;
    return 1;
}


void OD::AutoSaveObj::removeHiddenSaves()
{
    if ( lastautosaveioobj_ )
	removeIOObjAndData( lastautosaveioobj_ );
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
    , surveychanges_(false)
    , isactive_(isActiveByDefault())
    , usehiddenmode_(useHiddenModeByDefault())
    , curclockseconds_(-1)
    , nrclocksecondsbetweensaves_(defaultNrSecondsBetweenSaves())
    , saveDone(this)
    , saveFailed(this)
{
    mAttachCB( DBM().surveyToBeChanged, AutoSaver::survChgCB );
    mAttachCB( DBM().applicationClosing, AutoSaver::appExitCB );
    thread_ = new Threads::Thread( mSCB(AutoSaver::goCB), "AutoSaver" );
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

    for ( int iasobj=0; iasobj<asobjs_.size(); iasobj++ )
	asobjs_[iasobj]->removeHiddenSaves();
    deepErase( asobjs_ );
}


bool OD::AutoSaver::isActive() const
{
    Threads::Locker locker( lock_ );
    return isactive_;
}


void OD::AutoSaver::setActive( bool yn )
{
    setIsActiveByDefault( yn );
    Threads::Locker locker( lock_ );
    isactive_ = yn;
}


bool OD::AutoSaver::useHiddenMode() const
{
    Threads::Locker locker( lock_ );
    return usehiddenmode_;
}


void OD::AutoSaver::setUseHiddenMode( bool yn )
{
    setUseHiddenModeByDefault( yn );
    Threads::Locker locker( lock_ );
    usehiddenmode_ = yn;
}


int OD::AutoSaver::nrSecondsBetweenSaves() const
{
    Threads::Locker locker( lock_ );
    return nrclocksecondsbetweensaves_;
}


void OD::AutoSaver::setNrSecondsBetweenSaves( int nrsecs )
{
    if ( nrsecs < 1 )
	{ setActive( false ); return; }

    setDefaultNrSecondsBetweenSaves( nrsecs );
    Threads::Locker locker( lock_ );
    nrclocksecondsbetweensaves_ = nrsecs;
}


bool OD::AutoSaver::restore( IOStream& iostrm, const char* newnm )
{
    const DBKey tmpky( iostrm.key() );

    IOStream tmpiostrm( iostrm );
    tmpiostrm.setName( newnm );
    PtrMan<Translator> transl = iostrm.createTranslator();
    if ( transl )
	tmpiostrm.setExt( transl->defExtension() );
    tmpiostrm.genFileName();
    const BufferString newfnm( tmpiostrm.mainFileName() );
    if ( iostrm.implRename(newfnm) )
	iostrm.fileSpec().setFileName( newfnm );
    iostrm.setName( newnm );
    iostrm.pars().removeWithKey( sKey::CrFrom() );
    iostrm.pars().removeWithKey( sKey::CrInfo() );
    iostrm.pars().removeWithKey( sKeyAutosaved() );
    iostrm.updateCreationPars();

    iostrm.setKeyForNewEntry( tmpky.dirID() );
    if ( iostrm.commitChanges().isOK() )
	{ DBM().removeEntry( tmpky ); return true; }

    return false;
}


void OD::AutoSaver::handleSurvChg()
{
    setEmpty();
    surveychanges_ = false;
}


void OD::AutoSaver::survChgCB( CallBacker* )
{
    surveychanges_ = true;
    setEmpty();
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
	AutoSaveObj& asobj = *asobjs_[iasobj];
	if ( svr == asobj.saver_ )
	{
	    asobj.removeHiddenSaves();
	    delete asobjs_.removeSingle( iasobj );
	    return;
	}
    }
}


void OD::AutoSaver::appExitCB( CallBacker* )
{
    detachAllNotifiers();
    setEmpty();
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

    while ( !appexits_ )
    {
	const int mselapsed = Time::getMilliSeconds() - time0ms;
	curclockseconds_ = mselapsed / 1000;
	if ( curclockseconds_ == prevclockseconds )
	    { Threads::sleep( (1000 - mselapsed % 1000) * 0.001 ); continue; }
	prevclockseconds = curclockseconds_;

	if ( !isactive_ || appexits_ || surveychanges_ )
	{
	    if ( surveychanges_ )
		 handleSurvChg();
	    continue;
	}

	ObjectSet<AutoSaveObj> asobjs;
	Threads::Locker locker( lock_ );
	if ( !asobjs_.isEmpty() )
	    asobjs = asobjs_;
	locker.unlockNow();
	if ( asobjs.isEmpty() )
	    continue;

	ObjectSet<AutoSaveObj> finishedasobjs;
	for ( int iasobj=0; iasobj<asobjs.size(); iasobj++ )
	{
	    AutoSaveObj* asobj = asobjs[iasobj];
	    if ( asobj->isFinished() )
		finishedasobjs += asobj;
	    else if ( asobj->time4AutoSave() )
	    {
		if ( appexits_ || surveychanges_ )
		    break;

		const int res = asobj->autoSave( usehiddenmode_ );

		if ( appexits_ || surveychanges_ )
		    break;

		if ( res < 0 )
		{
		    saveFailed.trigger( asobj );
		    asobj->prevsavefailed_ = true;
		}
		else
		{
		    if ( res > 0 )
			saveDone.trigger( asobj );
		    asobj->prevsavefailed_ = false;
		}
	    }
	}

	if ( surveychanges_ )
	    { handleSurvChg(); continue; }

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
