/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "waveletmanager.h"
#include "waveletio.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "separstr.h"
#include "uistrings.h"


mDefineInstanceCreatedNotifierAccess(WaveletManager);

static const char* sKeyScaled = "Scaled";
static WaveletManager* theinst_ = 0;
static Threads::Lock theinstcreatelock_(true);


WaveletManager& WaveletManager::getInstance()
{
    if ( !theinst_ )
    {
	Threads::Locker locker( theinstcreatelock_ );
	if ( !theinst_ )
	    theinst_ = new WaveletManager;
    }
    return *theinst_;
}



WaveletManager::WaveletManager()
    : WaveletAdded(this)
    , ctxt_(*new IOObjContext(mIOObjContext(Wavelet)))
{
    mAttachCB( IOM().surveyToBeChanged, WaveletManager::survChgCB );
    mTriggerInstanceCreatedNotifier();
}


WaveletManager::~WaveletManager()
{
    sendDelNotif();
    setEmpty();
    detachAllNotifiers();
    delete &ctxt_;
}


void WaveletManager::setEmpty()
{
    deepErase( savers_ );
}


template <class RefManType,class WaveletType>
RefManType WaveletManager::doFetch( const WvltID& id, uiRetVal& uirv ) const
{
    mLock4Read();
    WaveletType* wvlt = gtWavelet( id );
    if ( wvlt )
	return RefManType( wvlt );			// already loaded

    WaveletLoader loader( id );
    mUnlockAllAccess();
    uirv = loader.load();
    if ( uirv.isOK() )
    {
	mReLock();
	return RefManType( gtWavelet(id) );		// now loaded
    }

    return RefManType( 0 );
}


ConstRefMan<Wavelet> WaveletManager::fetch( const WvltID& id ) const
{
    uiRetVal msg = uiRetVal::OK();
    return doFetch<ConstRefMan<Wavelet>,const Wavelet>( id, msg );
}


ConstRefMan<Wavelet> WaveletManager::fetch( const WvltID& id,
					    uiRetVal& uirv ) const
{
    return doFetch<ConstRefMan<Wavelet>,const Wavelet>( id, uirv );
}


RefMan<Wavelet> WaveletManager::fetchForEdit( const WvltID& id )
{
    uiRetVal msg = uiRetVal::OK();
    return doFetch<RefMan<Wavelet>,Wavelet>( id, msg );
}


RefMan<Wavelet> WaveletManager::fetchForEdit( const WvltID& id,
				    uiRetVal& uirv )
{
    return doFetch<RefMan<Wavelet>,Wavelet>( id, uirv );
}


uiRetVal WaveletManager::doSave( const WvltID& id ) const
{
    const int idx = gtIdx( id );
    if ( idx >= 0 )
    {
	if ( !savers_[idx]->save() )
	    return savers_[idx]->errMsg();
    }

    return uiRetVal::OK();
}


uiRetVal WaveletManager::save( const Wavelet& wvlt ) const
{
    mLock4Read();
    const int idx = gtIdx( wvlt );
    return idx<0 ? uiRetVal::OK() : doSave( savers_[idx]->key() );
}


uiRetVal WaveletManager::save( const WvltID& id ) const
{
    mLock4Read();
    return doSave( id );
}


uiRetVal WaveletManager::saveAs( const WvltID& id, const WvltID& newid ) const
{
    mLock4Read();

    const int idx = gtIdx( id );
    if ( idx < 0 )
	{ pErrMsg("Save-As not loaded ID"); return uiRetVal::OK(); }

    WaveletSaver& svr = *const_cast<WaveletSaver*>( savers_[idx] );
    svr.setKey( newid );
    uiRetVal uirv = doSave( newid );
    if ( uirv.isError() )
	{ svr.setKey( id ); return uirv; } // rollback

    return uiRetVal::OK();
}


WaveletManager::WvltID WaveletManager::getID( const char* nm ) const
{
    if ( !nm || !*nm )
	return WvltID::udf();

    mLock4Read();

    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	const WaveletSaver& saver = *savers_[idx];
	const Wavelet* wvlt = saver.wavelet();
	if ( wvlt && wvlt->name() == nm )
	    return saver.key();
    }

    PtrMan<IOObj> ioobj = getIOObj( nm );
    if ( ioobj )
	return ioobj->key();

    return WvltID::udf();
}


WaveletManager::WvltID WaveletManager::getID( const Wavelet& wvlt ) const
{
    mLock4Read();

    const int idxof = gtIdx( wvlt );
    return idxof < 0 ? WvltID::udf() : savers_[idxof]->key();
}


IOPar WaveletManager::getIOObjPars( const WvltID& id ) const
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


bool WaveletManager::isScaled( const WvltID& id ) const
{
    if ( id.isUdf() )
	return false;

    IOPar iop( getIOObjPars(id) );
    const BufferString res = iop.find( sKeyScaled );
    return !res.isEmpty();
}


bool WaveletManager::getScalingInfo( const WvltID& id, WvltID& orgid,
		MultiID& horid, MultiID& seisid, BufferString& lvlnm ) const
{
    if ( id.isUdf() )
	return false;

    IOPar iop( getIOObjPars(id) );
    const BufferString res = iop.find( sKeyScaled );
    if ( res.isEmpty() )
	return false;

    FileMultiString fms( res );
    if ( fms.size() < 3 )
	{ orgid.setUdf(); return true; }

    orgid = fms[0]; horid = fms[1]; seisid = fms[2]; lvlnm = fms[3];
    return true;
}


void WaveletManager::setScalingInfo( const WvltID& id, const WvltID* orgid,
		const MultiID* horid, const MultiID* seisid, const char* lvlnm )
{
    IOPar iop( getIOObjPars(id) );
    if ( !orgid )
	iop.removeWithKey( sKeyScaled );
    else if ( orgid->isUdf() )
	iop.set( sKeyScaled, "External" );
    else
    {
	FileMultiString fms( orgid->buf() );
	if ( horid )
	    fms.add( *horid );
	if ( seisid )
	    fms.add( *seisid );
	if ( lvlnm )
	    fms.add( lvlnm );
	iop.set( sKeyScaled, fms );
    }
}


IOObj* WaveletManager::getIOObj( const char* nm ) const
{
    if ( !nm || !*nm )
	return 0;

    IODir iodir( ctxt_.getSelKey() );
    const IOObj* ioobj = iodir.get( nm, ctxt_.translatorGroupName() );
    return ioobj ? ioobj->clone() : 0;
}


bool WaveletManager::nameExists( const char* nm ) const
{
    IOObj* ioobj = getIOObj( nm );
    delete ioobj;
    return ioobj;
}


bool WaveletManager::canSave( const WvltID& wvltid ) const
{
    return IOM().isPresent( wvltid );
}


uiRetVal WaveletManager::store( const Wavelet& newwvlt,
				  const IOPar* ioobjpars ) const
{
    const BufferString nm = newwvlt.name();
    if ( nm.isEmpty() )
	return tr("Please provide a name");

    PtrMan<IOObj> ioobj = getIOObj( nm );
    if ( !ioobj )
    {
	CtxtIOObj ctio( ctxt_ );
	ctio.setName( newwvlt.name() );
	IOM().getEntry( ctio );
	ioobj = ctio.ioobj_;
	ctio.ioobj_ = 0;
    }

    return store( newwvlt, ioobj->key(), ioobjpars );
}


uiRetVal WaveletManager::store( const Wavelet& newwvlt, const WvltID& id,
				const IOPar* ioobjpars ) const
{
    if ( id.isUdf() )
	return store( newwvlt, ioobjpars );

    if ( !isLoaded(id) )
	add( newwvlt, id, ioobjpars, false );
    else
    {
	mLock4Write();
	const int idxof = gtIdx( id );
	if ( idxof >= 0 )
	{
	    WaveletManager& self = *const_cast<WaveletManager*>(this);
	    WaveletSaver& svr = *self.savers_[idxof];
	    if ( svr.object() != &newwvlt )
		svr.setWavelet( newwvlt );
	}
    }

    return save( id );
}


void WaveletManager::add( const Wavelet& newwvlt, const WvltID& id,
			  const IOPar* ioobjpars, bool justloaded ) const
{
    WaveletSaver* saver = new WaveletSaver( newwvlt );
    saver->setKey( id );
    if ( ioobjpars )
	saver->setIOObjPars( *ioobjpars );
    if ( justloaded )
	saver->setNoSaveNeeded();

    WaveletManager& self = *const_cast<WaveletManager*>(this);
    mLock4Write();
    self.savers_ += saver;
    mUnlockAllAccess();

    self.addCBsToWvlt( newwvlt );
    self.WaveletAdded.trigger( id );
}


void WaveletManager::addCBsToWvlt( const Wavelet& wvlt )
{
    mAttachCB( wvlt.objectToBeDeleted(), WaveletManager::wvltDelCB );
}


bool WaveletManager::isLoaded( const char* nm ) const
{
    if ( !nm || !*nm )
	return false;
    mLock4Read();
    return gtIdx( nm ) >= 0;
}


bool WaveletManager::isLoaded( const WvltID& id ) const
{
    if ( id.isUdf() )
	return false;
    mLock4Read();
    return gtIdx( id ) >= 0;
}


int WaveletManager::size() const
{
    mLock4Read();
    return savers_.size();
}


ConstRefMan<Wavelet> WaveletManager::get( int idx ) const
{
    mLock4Read();
    if ( savers_.validIdx(idx) )
	return ConstRefMan<Wavelet>( savers_[idx]->wavelet() );

    pErrMsg("Invalid index");
    return ConstRefMan<Wavelet>( 0 );
}


RefMan<Wavelet> WaveletManager::getForEdit( int idx )
{
    mLock4Read();
    if ( savers_.validIdx(idx) )
    {
	ConstRefMan<Wavelet> wvlt = savers_[idx]->wavelet();
	return RefMan<Wavelet>( const_cast<Wavelet*>(wvlt.ptr()) );
    }

    pErrMsg("Invalid index");
    return RefMan<Wavelet>( 0 );
}


MultiID WaveletManager::getID( int idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->key() : WvltID::udf();
}


IOPar WaveletManager::getIOObjPars( int idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->ioObjPars() : IOPar();
}


int WaveletManager::gtIdx( const char* nm ) const
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	const Wavelet* wvlt = savers_[idx]->wavelet();
	if ( wvlt && wvlt->name() == nm )
	    return idx;
    }
    return -1;
}


int WaveletManager::gtIdx( const WvltID& id ) const
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->key() == id )
	    return idx;
    }
    return -1;
}


int WaveletManager::gtIdx( const Wavelet& wvlt ) const
{
    for ( int idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->object() == &wvlt ) // you cannot use wavelet()
	    return idx;
    }
    return -1;
}


Wavelet* WaveletManager::gtWavelet( const WvltID& id ) const
{
    const int idxof = gtIdx( id );
    if ( idxof < 0 )
	return 0;

    ConstRefMan<Wavelet> wvlt = savers_[idxof]->wavelet();
    wvlt.setNoDelete( true );
    return const_cast<Wavelet*>( wvlt.ptr() );
}


void WaveletManager::survChgCB( CallBacker* )
{
    setEmpty();
}


void WaveletManager::wvltDelCB( CallBacker* cb )
{
    mDynamicCastGet(Wavelet*,wvlt,cb)
    if ( !wvlt )
	{ pErrMsg("CB is not Wavelet"); return; }

    mLock4Read();
    int idxof = gtIdx( *wvlt );
    if ( idxof < 0 )
	{ pErrMsg("idxof < 0"); return; }

    if ( !mLock2Write() )
	idxof = gtIdx( *wvlt );

    if ( idxof >= 0 )
	delete savers_.removeSingle( idxof );
}
