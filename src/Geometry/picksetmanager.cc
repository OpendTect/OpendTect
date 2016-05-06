/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "picksetmanager.h"
#include "picksetio.h"
#include "ioman.h"
#include "iopar.h"

mDefineInstanceCreatedNotifierAccess(Pick::SetManager);

static Pick::SetManager* theinst_ = 0;
static Threads::Lock theinstcreatelock_(true);

Pick::SetManager& Pick::SetManager::getInstance()
{
    if ( !theinst_ )
    {
	Threads::Locker locker( theinstcreatelock_ );
	if ( !theinst_ )
	    theinst_ = new Pick::SetManager;
    }
    return *theinst_;
}


Pick::SetManager::SetManager()
    : setToBeRemoved(this)
    , setAdded(this)
    , setDispChanged(this)
    , setContentChanged(this)
    , newChangeRecord(this)
{
    records_.allowNull();
    mAttachCB( IOM().entryRemoved, SetManager::iomEntryRemovedCB );
    mAttachCB( IOM().surveyToBeChanged, SetManager::survChgCB );
    mAttachCB( IOM().applicationClosing, SetManager::appExitCB );
    mTriggerInstanceCreatedNotifier();
}


Pick::SetManager::~SetManager()
{
    sendDelNotif();
    detachAllNotifiers();
    deepErase( savers_ );
    deepErase( records_ );
}


void Pick::SetManager::add( const MultiID& ky, Set* newset, bool autosave,
			    bool keepchgrecords )
{
    if ( !newset )
	{ pErrMsg("Null set passed"); return; }

    SetSaver* saver = new SetSaver( *newset );
    saver->setKey( ky );
    if ( autosave )
	OD::AutoSaveMGR().add( saver );

    mAttachCB( newset->objectToBeDeleted(), Pick::SetManager::setDelCB );
    mAttachCB( newset->objectChanged(), Pick::SetManager::setChgCB );

    savers_ += saver;
    records_ += keepchgrecords ? new ChangeRecord : 0;
    setAdded.trigger( ky );
}


void Pick::SetManager::iomEntryRemovedCB( CallBacker* )
{
}


void Pick::SetManager::survChgCB( CallBacker* )
{
}


void Pick::SetManager::appExitCB( CallBacker* )
{
}


void Pick::SetManager::setDelCB( CallBacker* cb )
{
    mDynamicCastGet(Set*,ps,cb)
    if ( !ps )
	{ pErrMsg("Huh"); return; }
}


void Pick::SetManager::setChgCB( CallBacker* )
{
}
