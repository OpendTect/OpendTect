/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "picksetmanager.h"
#include "picksetio.h"
#include "picksettr.h"
#include "picksetchangerecorder.h"
#include "autosaver.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
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
    : SetAdded(this)
    , SetShowRequested(this)
    , SetHideRequested(this)
    , SetVanishRequested(this)
    , UnsavedSetLastCall(this)
    , ctxt_(*new IOObjContext(mIOObjContext(PickSet)))
{
    mAttachCB( IOM().surveyToBeChanged, SetManager::survChgCB );
    mAttachCB( IOM().applicationClosing, SetManager::appExitCB );
    mTriggerInstanceCreatedNotifier();
}


Pick::SetManager::~SetManager()
{
    sendDelNotif();
    setEmpty();
    detachAllNotifiers();
    delete &ctxt_;
}


void Pick::SetManager::setEmpty()
{
    deepErase( savers_ );
    deepErase( chgrecs_ );
}


template <class RefManType,class SetType>
RefManType Pick::SetManager::doFetch( const SetID& id, uiRetVal& uirv,
				      const char* cat ) const
{
    mLock4Read();
    SetType* ps = gtSet( id );
    if ( ps )
	return RefManType( ps );		// already loaded

    SetLoader loader( id );
    loader.setCategory( cat );
    mUnlockAllAccess();
    if ( loader.load() )
    {
	mReLock();
	return RefManType( gtSet(id) );		// now loaded
    }

    uirv = loader.errMsgs();
    return RefManType( 0 );
}


void Pick::SetManager::setNoSaveNeeded( const SetID& id ) const
{
    mLock4Read();
    const int idx = gtIdx( id );
    if ( idx >= 0 )
	savers_[idx]->setNoSaveNeeded();
}


ConstRefMan<Pick::Set> Pick::SetManager::fetch( const SetID& id ) const
{
    uiRetVal msg = uiRetVal::OK();
    return doFetch<ConstRefMan<Set>,const Set>( id, msg );
}


ConstRefMan<Pick::Set> Pick::SetManager::fetch( const SetID& id,
				    uiRetVal& uirv, const char* cat ) const
{
    return doFetch<ConstRefMan<Set>,const Set>( id, uirv, cat );
}


RefMan<Pick::Set> Pick::SetManager::fetchForEdit( const SetID& id )
{
    uiRetVal msg = uiRetVal::OK();
    return doFetch<RefMan<Set>,Set>( id, msg );
}


RefMan<Pick::Set> Pick::SetManager::fetchForEdit( const SetID& id,
				    uiRetVal& uirv, const char* cat )
{
    return doFetch<RefMan<Set>,Set>( id, uirv, cat );
}


uiRetVal Pick::SetManager::doSave( const SetID& id ) const
{
    const int idx = gtIdx( id );
    if ( idx >= 0 )
    {
	if ( !savers_[idx]->save() )
	    return savers_[idx]->errMsg();
    }

    return uiRetVal::OK();
}


uiRetVal Pick::SetManager::save( const Set& ps ) const
{
    mLock4Read();
    const int idx = gtIdx( ps );
    return idx<0 ? uiRetVal::OK() : doSave( savers_[idx]->key() );
}


uiRetVal Pick::SetManager::save( const SetID& id ) const
{
    mLock4Read();
    return doSave( id );
}


uiRetVal Pick::SetManager::saveAs( const SetID& id, const SetID& newid ) const
{
    mLock4Read();

    const int idx = gtIdx( id );
    if ( idx < 0 )
	{ pErrMsg("Save-As not loaded ID"); return uiRetVal::OK(); }

    SetSaver& svr = *const_cast<SetSaver*>( savers_[idx] );
    svr.setKey( newid );
    uiRetVal uirv = doSave( newid );
    if ( uirv.isError() )
	{ svr.setKey( id ); return uirv; } // rollback

    return uiRetVal::OK();
}


bool Pick::SetManager::needsSave( const SetID& id ) const
{
    mLock4Read();
    const int idx = gtIdx( id );
    return idx < 0 ? false : savers_[idx]->needsSave();
}


bool Pick::SetManager::needsSave( const Set& ps ) const
{
    mLock4Read();
    const int idx = gtIdx( ps );
    return idx < 0 ? false : savers_[idx]->needsSave();
}


Pick::SetManager::SetID Pick::SetManager::getID( const char* nm ) const
{
    if ( !nm || !*nm )
	return SetID::udf();

    mLock4Read();

    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	const SetSaver& saver = *savers_[idx];
	const Set* ps = saver.pickSet();
	if ( ps && ps->name() == nm )
	    return saver.key();
    }

    PtrMan<IOObj> ioobj = getIOObj( nm );
    if ( ioobj )
	return ioobj->key();

    return SetID::udf();
}


Pick::SetManager::SetID Pick::SetManager::getID( const Set& ps ) const
{
    mLock4Read();

    const int idxof = gtIdx( ps );
    return idxof < 0 ? SetID::udf() : savers_[idxof]->key();
}


IOPar Pick::SetManager::getIOObjPars( const SetID& id ) const
{
    if ( id.isUdf() )
	return IOPar();

    mLock4Read();
    const int idx = gtIdx( id );
    if ( idx >= 0 )
	return savers_[idx]->ioObjPars();
    mUnlockAllAccess();

    PtrMan<IOObj> ioobj = IOM().get( id );
    return ioobj ? ioobj->pars() : IOPar();
}


bool Pick::SetManager::isPolygon( const SetID& id ) const
{
    if ( id.isUdf() )
	return false;

    mLock4Read();
    const int idx = gtIdx( id );
    if ( idx >= 0 )
	return savers_[idx]->pickSet()->isPolygon();
    mUnlockAllAccess();

    PtrMan<IOObj> ioobj = IOM().get( id );
    return ioobj ? PickSetTranslator::isPolygon( *ioobj ) : false;
}


bool Pick::SetManager::hasCategory( const SetID& id, const char* cat ) const
{
    if ( id.isUdf() )
	return false;

    mLock4Read();
    const int idx = gtIdx( id );
    if ( idx >= 0 )
	return FixedString(savers_[idx]->pickSet()->category()) == cat;;
    mUnlockAllAccess();

    const bool defhascat = !cat || !*cat;
    PtrMan<IOObj> ioobj = IOM().get( id );
    return !ioobj ? defhascat : PickSetTranslator::getCategory(*ioobj) == cat;
}


IOObj* Pick::SetManager::getIOObj( const char* nm ) const
{
    if ( !nm || !*nm )
	return 0;

    IODir iodir( ctxt_.getSelKey() );
    const IOObj* ioobj = iodir.get( nm, ctxt_.translatorGroupName() );
    return ioobj ? ioobj->clone() : 0;
}


bool Pick::SetManager::nameExists( const char* nm ) const
{
    IOObj* ioobj = getIOObj( nm );
    delete ioobj;
    return ioobj;
}


bool Pick::SetManager::canSave( const SetID& setid ) const
{
    return IOM().isPresent( setid );
}


uiRetVal Pick::SetManager::store( const Set& newset,
				  const IOPar* ioobjpars ) const
{
    const BufferString nm = newset.name();
    if ( nm.isEmpty() )
	return tr("Please provide a name");

    PtrMan<IOObj> ioobj = getIOObj( nm );
    if ( !ioobj )
    {
	CtxtIOObj ctio( ctxt_ );
	ctio.setName( newset.name() );
	IOM().getEntry( ctio );
	ioobj = ctio.ioobj_;
	ctio.ioobj_ = 0;
    }

    return store( newset, ioobj->key(), ioobjpars );
}


uiRetVal Pick::SetManager::store( const Set& newset, const SetID& id,
			      const IOPar* ioobjpars ) const
{
    if ( id.isUdf() )
	return store( newset, ioobjpars );

    if ( !isLoaded(id) )
	add( newset, id, ioobjpars, false );
    else
    {
	mLock4Write();
	const int idxof = gtIdx( id );
	if ( idxof >= 0 )
	{
	    SetManager& self = *const_cast<SetManager*>(this);
	    SetSaver& svr = *self.savers_[idxof];
	    if ( svr.monitored() != &newset )
	    {
		svr.setPickSet( newset );
		delete self.chgrecs_.replace( idxof,
				new SetChangeRecorder(newset) );
	    }
	}
    }

    return save( id );
}


void Pick::SetManager::add( const Set& newset, const SetID& id,
				const IOPar* ioobjpars, bool justloaded ) const
{
    SetSaver* saver = new SetSaver( newset );
    saver->setKey( id );
    if ( ioobjpars )
	saver->setIOObjPars( *ioobjpars );
    if ( justloaded )
	saver->setNoSaveNeeded();

    SetManager& self = *const_cast<SetManager*>(this);
    mLock4Write();
    self.savers_ += saver;
    self.chgrecs_ += new SetChangeRecorder( newset );
    mUnlockAllAccess();

    self.addCBsToSet( newset );
    self.SetAdded.trigger( id );

    OD::AUTOSAVE().add( *saver );
}


void Pick::SetManager::addCBsToSet( const Set& ps )
{
    mAttachCB( ps.objectToBeDeleted(), Pick::SetManager::setDelCB );
    mAttachCB( ps.objectChanged(), Pick::SetManager::setChgCB );
}


bool Pick::SetManager::isLoaded( const char* nm ) const
{
    if ( !nm || !*nm )
	return false;
    mLock4Read();
    return gtIdx( nm ) >= 0;
}


bool Pick::SetManager::isLoaded( const SetID& id ) const
{
    if ( id.isUdf() )
	return false;
    mLock4Read();
    return gtIdx( id ) >= 0;
}


int Pick::SetManager::size() const
{
    mLock4Read();
    return savers_.size();
}


ConstRefMan<Pick::Set> Pick::SetManager::get( int idx ) const
{
    mLock4Read();
    if ( savers_.validIdx(idx) )
	return ConstRefMan<Set>( savers_[idx]->pickSet() );

    pErrMsg("Invalid index");
    return ConstRefMan<Set>( 0 );
}


RefMan<Pick::Set> Pick::SetManager::getForEdit( int idx )
{
    mLock4Read();
    if ( savers_.validIdx(idx) )
    {
	ConstRefMan<Set> ps = savers_[idx]->pickSet();
	return RefMan<Set>( const_cast<Set*>(ps.ptr()) );
    }

    pErrMsg("Invalid index");
    return RefMan<Set>( 0 );
}


MultiID Pick::SetManager::getID( int idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->key() : SetID::udf();
}


IOPar Pick::SetManager::getIOObjPars( int idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->ioObjPars() : IOPar();
}


int Pick::SetManager::gtIdx( const char* nm ) const
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	const Pick::Set* ps = savers_[idx]->pickSet();
	if ( ps && ps->name() == nm )
	    return idx;
    }
    return -1;
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


int Pick::SetManager::gtIdx( const Set& ps ) const
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->monitored() == &ps ) // you cannot use pickSet()
	    return idx;
    }
    return -1;
}


Pick::Set* Pick::SetManager::gtSet( const SetID& id ) const
{
    const int idxof = gtIdx( id );
    if ( idxof < 0 )
	return 0;

    ConstRefMan<Set> ps = savers_[idxof]->pickSet();
    ps.setNoDelete( true );
    return const_cast<Set*>( ps.ptr() );
}


void Pick::SetManager::clearChangeRecords( const SetID& id )
{
    mLock4Read();
    int idxof = gtIdx( id );
    if ( idxof < 0 )
	return;
    SetChangeRecorder* rec = chgrecs_[ idxof ];
    if ( rec->isEmpty() )
	return;

    if ( !mLock2Write() )
    {
	idxof = gtIdx( id );
	if ( idxof < 0 )
	    return;
	rec = chgrecs_[ idxof ];
    }

    rec->setEmpty();
}


void Pick::SetManager::getChangeInfo( const SetID& id, uiString& undotxt,
				      uiString& redotxt ) const
{
    undotxt.setEmpty(); redotxt.setEmpty();
    mLock4Read();
    int idxof = gtIdx( id );
    if ( idxof < 0 )
	return;
    const SetChangeRecorder* rec = chgrecs_[ idxof ];
    if ( rec->isEmpty() )
	return;

    if ( rec->canApply(ChangeRecorder::Undo) )
	undotxt = rec->usrText( ChangeRecorder::Undo );
    if ( rec->canApply(ChangeRecorder::Redo) )
	redotxt = rec->usrText( ChangeRecorder::Redo );
}


bool Pick::SetManager::useChangeRecord( const SetID& id, bool forundo )
{
    mLock4Read();
    int idxof = gtIdx( id );
    if ( idxof < 0 )
	return false;

    SetChangeRecorder* rec = chgrecs_[ idxof ];
    mUnlockAllAccess();
    return rec->apply( forundo ? ChangeRecorder::Undo : ChangeRecorder::Redo );
}


void Pick::SetManager::displayRequest( const MultiID& setid, DispOpt opt )
{
    switch ( opt )
    {
	case Show:
	    SetShowRequested.trigger( setid );
	break;
	case Hide:
	    SetHideRequested.trigger( setid );
	break;
	case Vanish:
	{
	    if ( needsSave(setid) )
		UnsavedSetLastCall.trigger( setid );
	    SetVanishRequested.trigger( setid );
	}
	break;
    }
}


void Pick::SetManager::handleUnsavedLastCall()
{
    mLock4Read();
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	ConstRefMan<Set> ps = savers_[idx]->pickSet();
	if ( !ps )
	    continue;

	if ( savers_[idx]->lastSavedDirtyCount() != ps->dirtyCount() )
	    UnsavedSetLastCall.trigger( savers_[idx]->key() );
    }
}


void Pick::SetManager::survChgCB( CallBacker* )
{
    handleUnsavedLastCall();
    setEmpty();
}


void Pick::SetManager::appExitCB( CallBacker* )
{
    handleUnsavedLastCall();
}


#define mHandleSetChgCBStart() \
    mDynamicCastGet(Set*,ps,cb) \
    if ( !ps ) \
	{ pErrMsg("CB is not Pick::Set"); return; } \
 \
    mLock4Read(); \
    int idxof = gtIdx( *ps ); \
    if ( idxof < 0 ) \
	{ pErrMsg("idxof < 0"); return; }


void Pick::SetManager::setDelCB( CallBacker* cb )
{
    mHandleSetChgCBStart();

    if ( !mLock2Write() )
	idxof = gtIdx( *ps );

    if ( idxof >= 0 )
    {
	delete savers_.removeSingle( idxof );
	delete chgrecs_.removeSingle( idxof );
    }
}


void Pick::SetManager::setChgCB( CallBacker* inpcb )
{
    mGetMonitoredChgDataWithCaller( inpcb, chgdata, cb );
    if ( !chgdata.isEntireObject() )
	return;

    mHandleSetChgCBStart();
    const Set::LocID locid( Set::LocID::get((Set::LocID::IDType)chgdata.ID()) );
    const SetID setid = savers_[idxof]->key();

    if ( !mLock2Write() )
	idxof = gtIdx( *ps );

    if ( idxof >= 0 )
	chgrecs_[ idxof ]->setEmpty();
}
