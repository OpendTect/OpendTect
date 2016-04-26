/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2016
-*/


#include "autosaver.h"
#include "iodir.h"
#include "iostrm.h"
#include "ioman.h"
#include "iopar.h"
#include "separstr.h"
#include "timer.h"
#include "msgh.h"


OD::AutoSaver::AutoSaver( const Monitorable& obj )
    : obj_(obj)
    , nrseconds_(300)
    , objdeleted_(false)
    , savenr_(1)
    , prevstoreioobj_(0)
{
    mAttachCB( const_cast<Monitorable&>(obj).objectToBeDeleted(),
	       AutoSaver::objDel );
    mTriggerInstanceCreatedNotifier();
}


OD::AutoSaver::~AutoSaver()
{
    detachAllNotifiers();
    sendDelNotif();
    delete prevstoreioobj_;
}


void OD::AutoSaver::objDel( CallBacker* )
{
    mLock4Write();
    objdeleted_ = true;
}


void OD::AutoSaver::remove( const IOObj& ioobj ) const
{
    ioobj.implRemove();
}


void OD::AutoSaver::removePrevStored()
{
    if ( !prevstoreioobj_ )
	return;

    remove( *prevstoreioobj_ );
    IOM().permRemove( prevstoreioobj_->key() );
    delete prevstoreioobj_;
    prevstoreioobj_ = 0;
}


bool OD::AutoSaver::needsAct( int clockseconds ) const
{
    mLock4Read();
    if ( objdeleted_ || (!mLock2Write() && objdeleted_) )
	return false;

    curclockseconds_ = clockseconds;
    return curclockseconds_ - lastsaveclockseconds_ >= nrseconds_;
}


bool OD::AutoSaver::act()
{
    return isFinished() || doWork(false);
}



void OD::AutoSaver::userSaveOccurred()
{
    removePrevStored();
    lastsaveclockseconds_ = curclockseconds_ + 1;
}


bool OD::AutoSaver::doWork( bool forcesave )
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
    storenm.add( savenr_++ );
    IOStream* newstoreioobj = new IOStream( storenm, iodir.newTmpKey(), true );
    newstoreioobj->pars().update( sKey::CrFrom(), key_ );
    newstoreioobj->pars().update( sKey::CrInfo(), "Auto-saved" );
    newstoreioobj->updateCreationPars();
    if ( !store(*newstoreioobj) )
    {
	delete newstoreioobj; newstoreioobj = 0;
	prevfingerprint_.setEmpty();
	return false;
    }
    else
    {
	prevfingerprint_ = fingerprint;
	removePrevStored();
	prevstoreioobj_ = newstoreioobj;
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
    : nrcycles_(0)
    , thread_(mCB(this,AutoSaveMgr,go),"AutoSave Manager")
{
    mAttachCB( IOM().applicationClosing, AutoSaveMgr::appExits );
}


void OD::AutoSaveMgr::add( AutoSaver* saver )
{
    Threads::Locker locker( lock_ );
    savers_ += saver;
}


void OD::AutoSaveMgr::appExits( CallBacker* )
{
    detachAllNotifiers();
    thread_.waitForFinish();
}


void OD::AutoSaveMgr::go( CallBacker* )
{
    Threads::Locker locker( lock_ );

    const bool isverbose = DBG::isOn( DBG_IO );
    while ( true )
    {
	nrcycles_++;
	for ( int idx=0; idx<savers_.size(); idx++ )
	{
	    AutoSaver* saver = savers_[idx];
	    if ( saver->isFinished() )
		{ delete savers_.removeSingle(idx); idx--; }
	    else if ( saver->needsAct(nrcycles_) )
	    {
		if ( !saver->act() )
		    ErrMsg( BufferString("Auto-save failed:\n",
					 saver->errMsg().getFullString()) );
		else if ( isverbose )
		    UsrMsg( BufferString("Auto-save succeeded for: ",
					 saver->key()) );
	    }
	}
    }
}
