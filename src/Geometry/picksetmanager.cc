/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "picksetmanager.h"
#include "picksetio.h"
#include "ioman.h"
#include "uistrings.h"

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
    , newChangeEvent(this)
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
    setEmpty();
    detachAllNotifiers();
}


void Pick::SetManager::setEmpty()
{
    deepErase( savers_ );
    deepErase( records_ );
}


Pick::Set& Pick::SetManager::fetch( const Pick::SetManager::SetID& id,
				    uiString& errmsg )
{
    errmsg = mTODONotImplPhrase();
    return Set::dummySet();
}


uiString Pick::SetManager::save( const SetID& id ) const
{
    mLock4Read();

    const int idx = gtIdx( id );
    if ( idx >= 0 )
    {
	if ( !savers_[idx]->save() )
	    return savers_[idx]->errMsg();

	deepErase( records_[idx]->events_ );
    }

    return uiString::emptyString();
}


Pick::SetManager::SetID Pick::SetManager::getID( const Set& ps ) const
{
    mLock4Read();

    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	const SetSaver& saver = *savers_[idx];
	if ( &saver.pickSet() == &ps )
	    return saver.key();
    }

    return SetID::udf();
}


bool Pick::SetManager::add( const SetID& id, Set* newset )
{
    if ( !newset )
	{ pErrMsg("Null set passed"); return false; }
    else if ( isLoaded(id) )
	{ delete newset; return false; }

    SetSaver* saver = new SetSaver( *newset );
    saver->setKey( id );
    OD::AutoSaveMGR().add( saver );

    mAttachCB( newset->objectToBeDeleted(), Pick::SetManager::setDelCB );
    mAttachCB( newset->objectChanged(), Pick::SetManager::setChgCB );

    mLock4Write();
    savers_ += saver;
    records_ += new ChangeRecord(id);
    mUnlockAllAccess();

    setAdded.trigger( id );
    return true;
}


bool Pick::SetManager::isLoaded( const SetID& id ) const
{
    mLock4Read();
    return gtIdx( id ) >= 0;
}


int Pick::SetManager::size() const
{
    mLock4Read();
    return savers_.size();
}


const Pick::Set& Pick::SetManager::get( int idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->pickSet() : Set::emptySet();
}


Pick::Set& Pick::SetManager::get( int idx )
{
    mLock4Read();
    return savers_.validIdx(idx) ? const_cast<Set&>( savers_[idx]->pickSet() )
				 : Set::dummySet();
}


MultiID Pick::SetManager::getID( int idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->key() : SetID::udf();
}


int Pick::SetManager::gtIdx( const SetID& id ) const
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->key() == id )
	    return idx;
    }

    return -1;
}


void Pick::SetManager::broadcastChanges( const SetID& id )
{
}


void Pick::SetManager::broadcastChanges( const Set& ps )
{
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
