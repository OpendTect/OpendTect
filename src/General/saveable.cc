/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2016
-*/


#include "saveable.h"
#include "ioman.h"
#include "ioobj.h"


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
    detachAllNotifiers();
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
