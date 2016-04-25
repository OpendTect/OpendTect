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


bool OD::AutoSaver::act( int clockseconds )
{
    mLock4Read();
    if ( objdeleted_ || (!mLock2Write() && objdeleted_) )
	return true;

    curclockseconds_ = clockseconds;
    if ( curclockseconds_ - lastsaveclockseconds_ < nrseconds_ )
	return true;

    lastsaveclockseconds_ = curclockseconds_;
    return doWork( false );
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
