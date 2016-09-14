/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2016
-*/


#include "saveable.h"
#include "saveablemanager.h"
#include "monitorchangerecorder.h"
#include "autosaver.h"
#include "ioman.h"
#include "ioobj.h"
#include "iodir.h"
#include "ctxtioobj.h"


Saveable::Saveable( const SharedObject& obj )
    : object_(&obj)
    , objectalive_(true)
{
    attachCBToObj();
    mTriggerInstanceCreatedNotifier();
}


Saveable::Saveable( const Saveable& oth )
    : object_(oth.object_)
{
    *this = oth;
    mTriggerInstanceCreatedNotifier();
}


Saveable::~Saveable()
{
    detachAllNotifiers();
    sendDelNotif();
}


mImplMonitorableAssignment(Saveable,Monitorable)

void Saveable::copyClassData( const Saveable& oth )
{
    detachCBFromObj();
    object_ = oth.object_;
    objectalive_ = oth.objectalive_;
    storekey_ = oth.storekey_;
    ioobjpars_ = oth.ioobjpars_;
    errmsg_ = oth.errmsg_;
    lastsavedirtycount_ = oth.lastsavedirtycount_;
    attachCBToObj();
}


void Saveable::setObject( const SharedObject& obj )
{
    mLock4Read();
    if ( object_ == &obj )
	return;

    AccessLockHandler alh( obj );
    mLock2Write();
    detachCBFromObj();
    object_ = &obj;
    objectalive_ = true;
    attachCBToObj();
    mSendEntireObjChgNotif();
}


void Saveable::attachCBToObj()
{
    if ( objectalive_ )
	mAttachCB( const_cast<SharedObject&>(*object_).objectToBeDeleted(),
		   Saveable::objDelCB );
}


void Saveable::detachCBFromObj()
{
    if ( objectalive_ )
	mDetachCB( const_cast<SharedObject&>(*object_).objectToBeDeleted(),
		   Saveable::objDelCB );
}


const SharedObject* Saveable::object() const
{
    mLock4Read();
    return object_;
}


Monitorable::DirtyCountType Saveable::curDirtyCount() const
{
    mLock4Read();
    return objectalive_ ? object_->dirtyCount() : lastsavedirtycount_.get();
}


void Saveable::objDelCB( CallBacker* )
{
    objectalive_ = false;
}


bool Saveable::save() const
{
    mLock4Read();
    if ( !objectalive_ )
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
	if ( !store(*ioobj) )
	    mSendChgNotif( cSaveFailedChangeType(), storekey_.toInt64() );
	else
	{
	    setNoSaveNeeded();
	    mSendChgNotif( cSaveSucceededChangeType(), storekey_.toInt64() );
	    return true;
	}
    }

    if ( storekey_.isValid() )
	errmsg_ = tr("Cannot find database entry for: %1").arg(storekey_);
    else
	errmsg_ = tr("Cannot save object without database key");
    return false;
}


bool Saveable::needsSave() const
{
    return !objectalive_ ? false : lastsavedirtycount_ != object_->dirtyCount();
}


void Saveable::setNoSaveNeeded() const
{
    if ( objectalive_ )
	lastsavedirtycount_ = object_->dirtyCount();
}


bool Saveable::store( const IOObj& ioobj ) const
{
    if ( !objectalive_ )
	{ pErrMsg("Attempt to store already deleted object"); return true; }
    return doStore( ioobj );
}



SaveableManager::SaveableManager( const IOObjContext& ctxt, bool withautosave )
    : ctxt_(*new IOObjContext(ctxt))
    , autosaveable_(withautosave)
    , ObjAdded(this)
    , ObjOrphaned(this)
    , UnsavedObjLastCall(this)
    , ShowRequested(this)
    , HideRequested(this)
    , VanishRequested(this)
{
    chgrecs_.allowNull( true );
    mAttachCB( IOM().surveyToBeChanged, SaveableManager::survChgCB );
    mAttachCB( IOM().applicationClosing, SaveableManager::appExitCB );
    mAttachCB( IOM().entryRemoved, SaveableManager::iomEntryRemovedCB );
}


SaveableManager::~SaveableManager()
{
    detachAllNotifiers();
    sendDelNotif();
    setEmpty();
    delete const_cast<IOObjContext*>( &ctxt_ );
}


void SaveableManager::setEmpty()
{
    deepErase( savers_ );
    deepErase( chgrecs_ );
}


void SaveableManager::setNoSaveNeeded( const ObjID& id ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
	savers_[idx]->setNoSaveNeeded();
}


uiRetVal SaveableManager::doSave( const ObjID& id ) const
{
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
    {
	if ( !savers_[idx]->save() )
	    return savers_[idx]->errMsg();
    }

    return uiRetVal::OK();
}


uiRetVal SaveableManager::save( const SharedObject& obj ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( obj );
    return idx<0 ? uiRetVal::OK() : doSave( savers_[idx]->key() );
}


uiRetVal SaveableManager::save( const ObjID& id ) const
{
    mLock4Read();
    return doSave( id );
}


uiRetVal SaveableManager::saveAs( const ObjID& id, const ObjID& newid ) const
{
    mLock4Read();

    const IdxType idx = gtIdx( id );
    if ( idx < 0 )
	{ pErrMsg("Save-As not loaded ID"); return uiRetVal::OK(); }

    Saveable& svr = *const_cast<Saveable*>( savers_[idx] );
    svr.setKey( newid );
    uiRetVal uirv = doSave( newid );
    if ( uirv.isError() )
	{ svr.setKey( id ); return uirv; } // rollback

    return uiRetVal::OK();
}


bool SaveableManager::needsSave( const ObjID& id ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( id );
    return idx < 0 ? false : savers_[idx]->needsSave();
}


bool SaveableManager::needsSave( const SharedObject& obj ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( obj );
    return idx < 0 ? false : savers_[idx]->needsSave();
}


SaveableManager::ObjID SaveableManager::getIDByName( const char* nm ) const
{
    if ( !nm || !*nm )
	return ObjID::getInvalid();

    mLock4Read();

    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	const Saveable& saver = *savers_[idx];
	const SharedObject* obj = saver.object();
	if ( obj && obj->name() == nm )
	    return saver.key();
    }

    PtrMan<IOObj> ioobj = getIOObj( nm );
    if ( ioobj )
	return ioobj->key();

    return ObjID::getInvalid();
}


SaveableManager::ObjID SaveableManager::getID( const SharedObject& obj ) const
{
    mLock4Read();

    const IdxType idxof = gtIdx( obj );
    return idxof < 0 ? ObjID::getInvalid() : savers_[idxof]->key();
}


IOPar SaveableManager::getIOObjPars( const ObjID& id ) const
{
    if ( id.isInvalid() )
	return IOPar();

    mLock4Read();
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
	return savers_[idx]->ioObjPars();
    mUnlockAllAccess();

    PtrMan<IOObj> ioobj = IOM().get( id );
    return ioobj ? ioobj->pars() : IOPar();
}


IOObj* SaveableManager::getIOObj( const char* nm ) const
{
    if ( !nm || !*nm )
	return 0;

    IODir iodir( ctxt_.getSelDirID() );
    const IOObj* ioobj = iodir.getByName( nm, ctxt_.translatorGroupName() );
    return ioobj ? ioobj->clone() : 0;
}


bool SaveableManager::nameExists( const char* nm ) const
{
    IOObj* ioobj = getIOObj( nm );
    delete ioobj;
    return ioobj;
}


bool SaveableManager::canSave( const ObjID& id ) const
{
    return IOM().isPresent( id );
}


uiRetVal SaveableManager::store( const SharedObject& newobj,
				 const IOPar* ioobjpars ) const
{
    const BufferString nm = newobj.name();
    if ( nm.isEmpty() )
	return tr("Please provide a name");

    PtrMan<IOObj> ioobj = getIOObj( nm );
    if ( !ioobj )
    {
	CtxtIOObj ctio( ctxt_ );
	ctio.setName( newobj.name() );
	ctio.ctxt_.forread_ = false;
	IOM().getEntry( ctio );
	ioobj = ctio.ioobj_;
	ctio.ioobj_ = 0;
    }

    return store( newobj, ioobj->key(), ioobjpars );
}


uiRetVal SaveableManager::store( const SharedObject& newobj, const ObjID& id,
				 const IOPar* ioobjpars ) const
{
    if ( id.isInvalid() )
	return store( newobj, ioobjpars );

    if ( !isLoaded(id) )
	add( newobj, id, ioobjpars, false );
    else
    {
	mLock4Write();
	const IdxType idxof = gtIdx( id );
	if ( idxof >= 0 )
	{
	    SaveableManager& self = *const_cast<SaveableManager*>(this);
	    Saveable& svr = *self.savers_[idxof];
	    if ( svr.object() != &newobj )
	    {
		svr.setObject( newobj );
		delete self.chgrecs_.replace( idxof, getChangeRecorder(newobj));
	    }
	}
    }

    return save( id );
}


void SaveableManager::add( const SharedObject& newobj, const ObjID& id,
				const IOPar* ioobjpars, bool justloaded ) const
{
    Saveable* saver = getSaver( newobj );
    saver->setKey( id );
    if ( ioobjpars )
	saver->setIOObjPars( *ioobjpars );
    if ( justloaded )
	saver->setNoSaveNeeded();

    SaveableManager& self = *const_cast<SaveableManager*>(this);
    mLock4Write();
    self.savers_ += saver;
    self.chgrecs_ += getChangeRecorder( newobj );
    self.addCBsToObj( newobj );
    mUnlockAllAccess();

    self.ObjAdded.trigger( id );
    if ( autosaveable_ )
	OD::AUTOSAVE().add( *saver );
}


void SaveableManager::addCBsToObj( const SharedObject& obj )
{
    mAttachCB( obj.objectToBeDeleted(), SaveableManager::objDelCB );
    mAttachCB( obj.objectChanged(), SaveableManager::objChgCB );
}


bool SaveableManager::isLoaded( const char* nm ) const
{
    if ( !nm || !*nm )
	return false;
    mLock4Read();
    return gtIdx( nm ) >= 0;
}


bool SaveableManager::isLoaded( const ObjID& id ) const
{
    if ( id.isInvalid() )
	return false;
    mLock4Read();
    return gtIdx( id ) >= 0;
}


SaveableManager::IdxType SaveableManager::size() const
{
    mLock4Read();
    return savers_.size();
}


DBKey SaveableManager::getIDByIndex( IdxType idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->key() : ObjID::getInvalid();
}


IOPar SaveableManager::getIOObjParsByIndex( IdxType idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->ioObjPars() : IOPar();
}


SaveableManager::IdxType SaveableManager::gtIdx( const char* nm ) const
{
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	const SharedObject* obj = savers_[idx]->object();
	if ( obj && obj->name() == nm )
	    return idx;
    }
    return -1;
}


SaveableManager::IdxType SaveableManager::gtIdx( const ObjID& id ) const
{
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->key() == id )
	    return idx;
    }
    return -1;
}


SaveableManager::IdxType SaveableManager::gtIdx( const SharedObject& obj ) const
{
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->object() == &obj )
	    return idx;
    }
    return -1;
}


SharedObject* SaveableManager::gtObj( IdxType idx ) const
{
    return !savers_.validIdx(idx) ? 0
	 : const_cast<SharedObject*>( savers_[idx]->object() );
}


void SaveableManager::clearChangeRecords( const ObjID& id )
{
    mLock4Read();
    IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return;
    ChangeRecorder* rec = chgrecs_[ idxof ];
    if ( !rec || rec->isEmpty() )
	return;

    if ( !mLock2Write() )
    {
	idxof = gtIdx( id );
	if ( idxof < 0 )
	    return;
	rec = chgrecs_[ idxof ];
    }

    if ( rec )
	rec->setEmpty();
}


void SaveableManager::getChangeInfo( const ObjID& id, uiString& undotxt,
				      uiString& redotxt ) const
{
    undotxt.setEmpty(); redotxt.setEmpty();
    mLock4Read();
    IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return;
    const ChangeRecorder* rec = chgrecs_[ idxof ];
    if ( !rec || rec->isEmpty() )
	return;

    if ( rec->canApply(ChangeRecorder::Undo) )
	undotxt = rec->usrText( ChangeRecorder::Undo );
    if ( rec->canApply(ChangeRecorder::Redo) )
	redotxt = rec->usrText( ChangeRecorder::Redo );
}


bool SaveableManager::useChangeRecord( const ObjID& id, bool forundo )
{
    mLock4Read();
    IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return false;

    ChangeRecorder* rec = chgrecs_[ idxof ];
    mUnlockAllAccess();
    return !rec ? true
	 : rec->apply( forundo ? ChangeRecorder::Undo : ChangeRecorder::Redo );
}


void SaveableManager::displayRequest( const ObjID& objid, DispOpt opt )
{
    switch ( opt )
    {
	case Show:
	    ShowRequested.trigger( objid );
	break;
	case Hide:
	    HideRequested.trigger( objid );
	break;
	case Vanish:
	{
	    if ( needsSave(objid) )
		UnsavedObjLastCall.trigger( objid );
	    VanishRequested.trigger( objid );
	}
	break;
    }
}


void SaveableManager::handleUnsavedLastCall()
{
    mLock4Read();
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	ConstRefMan<SharedObject> obj = savers_[idx]->object();
	if ( !obj )
	    continue;

	if ( savers_[idx]->lastSavedDirtyCount() != obj->dirtyCount() )
	    UnsavedObjLastCall.trigger( savers_[idx]->key() );
    }
}


void SaveableManager::iomEntryRemovedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( ObjID, id, cb );
    if ( isLoaded(id) )
	ObjOrphaned.trigger( id );
}


void SaveableManager::survChgCB( CallBacker* )
{
    handleUnsavedLastCall();
    setEmpty();
}


void SaveableManager::appExitCB( CallBacker* )
{
    handleUnsavedLastCall();
}


#define mHandleObjChgCBStart() \
    mDynamicCastGet(SharedObject*,obj,cb) \
    if ( !obj ) \
	{ pErrMsg("CB is not a SharedObject"); return; } \
 \
    mLock4Read(); \
    IdxType idxof = gtIdx( *obj ); \
    if ( idxof < 0 ) \
	{ pErrMsg("idxof < 0"); return; }


void SaveableManager::objDelCB( CallBacker* cb )
{
    mHandleObjChgCBStart();

    if ( !mLock2Write() )
	idxof = gtIdx( *obj );

    if ( idxof >= 0 )
    {
	delete savers_.removeSingle( idxof );
	delete chgrecs_.removeSingle( idxof );
    }
}


void SaveableManager::objChgCB( CallBacker* inpcb )
{
    mGetMonitoredChgDataWithCaller( inpcb, chgdata, cb );
    if ( !chgdata.isEntireObject() )
	return;

    mHandleObjChgCBStart();

    if ( !mLock2Write() )
	idxof = gtIdx( *obj );

    if ( idxof >= 0 )
    {
	ChangeRecorder* rec = chgrecs_[ idxof ];
	if ( rec )
	    rec->setEmpty();
    }
}
