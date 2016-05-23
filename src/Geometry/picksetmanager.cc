/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "picksetmanager.h"
#include "picksetio.h"
#include "picksettr.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "uistrings.h"

static const int maxnrlocevrecs_ = 100;

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
    , SetDisplayRequested(this)
    , SetSaveNeeded(this)
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
    deepErase( locevrecs_ );
}


template <class RefManType,class SetType>
RefManType Pick::SetManager::doFetch( const SetID& id, uiString& errmsg,
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
	accesslockhandler_.reLock();
	return RefManType( gtSet(id) );		// now loaded
    }

    errmsg = loader.errMsgs().first();
    return RefManType( 0 );
}


void Pick::SetManager::setNoSaveNeeded( const SetID& id ) const
{
    uiString msg;
    mLock4Read();
    const int idx = gtIdx( id );
    if ( idx >= 0 )
	savers_[idx]->setNoSaveNeeded();
}


ConstRefMan<Pick::Set> Pick::SetManager::fetch( const SetID& id ) const
{
    uiString msg;
    return doFetch<ConstRefMan<Set>,const Set>( id, msg );
}


ConstRefMan<Pick::Set> Pick::SetManager::fetch( const SetID& id,
				    uiString& errmsg, const char* cat ) const
{
    return doFetch<ConstRefMan<Set>,const Set>( id, errmsg, cat );
}


RefMan<Pick::Set> Pick::SetManager::fetchForEdit( const SetID& id )
{
    uiString msg;
    return doFetch<RefMan<Set>,Set>( id, msg );
}


RefMan<Pick::Set> Pick::SetManager::fetchForEdit( const SetID& id,
				    uiString& errmsg, const char* cat )
{
    return doFetch<RefMan<Set>,Set>( id, errmsg, cat );
}


uiString Pick::SetManager::doSave( const SetID& id ) const
{
    const int idx = gtIdx( id );
    if ( idx >= 0 )
    {
	if ( !savers_[idx]->save() )
	    return savers_[idx]->errMsg();
    }

    return uiString::emptyString();
}


uiString Pick::SetManager::save( const Set& ps ) const
{
    mLock4Read();
    const int idx = gtIdx( ps );
    return idx<0 ? uiString::emptyString() : doSave( savers_[idx]->key() );
}


uiString Pick::SetManager::save( const SetID& id ) const
{
    mLock4Read();
    return doSave( id );
}


uiString Pick::SetManager::saveAs( const SetID& id, const SetID& newid ) const
{
    mLock4Read();

    const int idx = gtIdx( id );
    if ( idx < 0 )
	{ pErrMsg("Save-As not loaded ID"); return uiString::emptyString(); }

    SetSaver& svr = *const_cast<SetSaver*>( savers_[idx] );
    svr.setKey( newid );
    uiString errmsg = doSave( newid );
    if ( !errmsg.isEmpty() )
	{ svr.setKey( id ); return errmsg; } // rollback

    return uiString::emptyString();
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


uiString Pick::SetManager::store( const Set& newset,
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


uiString Pick::SetManager::store( const Set& newset, const SetID& id,
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
		self.locevrecs_[idxof]->setEmpty();
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
    self.locevrecs_ += new LocEvRecord;
    mUnlockAllAccess();

    self.addCBsToSet( newset );
    self.SetAdded.trigger( id );

    //TODO OD::AutoSaveMGR().add( saver );
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


void Pick::SetManager::pushLocEvent( const SetID& id, const LocEvent& ev )
{
    mLock4Write();
    const int idxof = gtIdx( id );
    if ( idxof < 0 )
	{ pErrMsg("Huh"); return; }

    LocEvRecord& rec = *locevrecs_[ idxof ];
    if ( rec.size() >= maxnrlocevrecs_ )
	rec.removeSingle( 0 );
    rec += ev;
}


Pick::SetManager::LocEvent Pick::SetManager::popLocEvent( const SetID& id )
{
    mLock4Read();
    const int idxof = gtIdx( id );
    if ( idxof < 0 )
	{ pErrMsg("Huh"); return LocEvent::udf(); }
    LocEvRecord& rec = *locevrecs_[ idxof ];
    if ( rec.isEmpty() )
	return LocEvent::udf();

    mLock2Write();
    return rec.isEmpty() ? LocEvent::udf() : rec.pop();
}


void Pick::SetManager::requestDisplayFor( const MultiID& setid )
{
    SetDisplayRequested.trigger( setid );
}


void Pick::SetManager::handleUnsaved()
{
    mLock4Read();
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	ConstRefMan<Set> ps = savers_[idx]->pickSet();
	if ( !ps )
	    continue;

	if ( savers_[idx]->lastSavedDirtyCount() != ps->dirtyCount() )
	    SetSaveNeeded.trigger( savers_[idx]->key() );
    }
}


void Pick::SetManager::survChgCB( CallBacker* )
{
    handleUnsaved();
    setEmpty();
}


void Pick::SetManager::appExitCB( CallBacker* )
{
    handleUnsaved();
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

    mLock2Write();
    idxof = gtIdx( *ps );
    if ( idxof >= 0 )
    {
	delete savers_.removeSingle( idxof );
	delete locevrecs_.removeSingle( idxof );
    }
}


void Pick::SetManager::setChgCB( CallBacker* inpcb )
{
    mGetMonitoredChgDataWithCaller( inpcb, chgdata, cb );
    const bool isentire = chgdata.changeType() == cEntireObjectChangeType();
    if ( isentire )
    {
	AccessLockHandler alh( *this );
	locevrecs_.setEmpty();
	return;
    }

    const bool isinsert = chgdata.changeType() == Set::cLocationInsert();
    const bool isremove = chgdata.changeType() == Set::cLocationRemove();
    if ( !isinsert && !isremove )
	return;

    mHandleSetChgCBStart();

    const LocEvent::IdxType locidx = (LocEvent::IdxType)chgdata.subIdx();
    const SetID setid = savers_[idxof]->key();
    LocEvent ev( locidx, Location::udf(), Location::udf() );
    ev.type_ = isinsert ? LocationChangeEvent::Create
			: LocationChangeEvent::Delete;
    if ( isinsert )
	ev.loc_ = ps->get( locidx );
    else
	ev.prevloc_ = ps->get( locidx );

    mUnlockAllAccess();
    pushLocEvent( setid, ev );
}
