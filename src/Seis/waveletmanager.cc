/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "waveletmanager.h"
#include "waveletio.h"
#include "ioman.h"
#include "ioobj.h"
#include "separstr.h"

static const char* sKeyScaled = "Scaled";
#define mToWvlt(cnsttyp,reftyp,var) static_cast<cnsttyp Wavelet reftyp>(var)

mDefineSaveableManagerInstance(WaveletManager);


WaveletManager::WaveletManager()
    : SaveableManager(mIOObjContext(Wavelet),true)
{
    mTriggerInstanceCreatedNotifier();
}


WaveletManager::~WaveletManager()
{
    sendDelNotif();
}


template <class RefManType,class WaveletType>
RefManType WaveletManager::doFetch( const ObjID& id, uiRetVal& uirv ) const
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


ConstRefMan<Wavelet> WaveletManager::fetch( const ObjID& id ) const
{
    uiRetVal uirv = uiRetVal::OK();
    return doFetch<ConstRefMan<Wavelet>,const Wavelet>( id, uirv );
}


ConstRefMan<Wavelet> WaveletManager::fetch( const ObjID& id,
					    uiRetVal& uirv ) const
{
    return doFetch<ConstRefMan<Wavelet>,const Wavelet>( id, uirv );
}


RefMan<Wavelet> WaveletManager::fetchForEdit( const ObjID& id )
{
    uiRetVal uirv = uiRetVal::OK();
    return doFetch<RefMan<Wavelet>,Wavelet>( id, uirv );
}


RefMan<Wavelet> WaveletManager::fetchForEdit( const ObjID& id,
				    uiRetVal& uirv )
{
    return doFetch<RefMan<Wavelet>,Wavelet>( id, uirv );
}



WaveletManager::ObjID WaveletManager::getID( const Wavelet& wvlt ) const
{
    return SaveableManager::getID( wvlt );
}


uiRetVal WaveletManager::store( const Wavelet& newwvlt,
				  const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newwvlt, ioobjpars );
}


uiRetVal WaveletManager::store( const Wavelet& newwvlt, const ObjID& id,
			      const IOPar* ioobjpars ) const
{
    return SaveableManager::store( newwvlt, id, ioobjpars );
}


uiRetVal WaveletManager::save( const ObjID& id ) const
{
    return SaveableManager::save( id );
}


uiRetVal WaveletManager::save( const Wavelet& set ) const
{
    return SaveableManager::save( set );
}


bool WaveletManager::needsSave( const ObjID& id ) const
{
    return SaveableManager::needsSave( id );
}


bool WaveletManager::needsSave( const Wavelet& wvlt ) const
{
    return SaveableManager::needsSave( wvlt );
}


ConstRefMan<Wavelet> WaveletManager::get( IdxType idx ) const
{
    const SharedObject* shobj = gtObj( idx );
    return ConstRefMan<Wavelet>( mToWvlt(const,*,shobj) );
}


RefMan<Wavelet> WaveletManager::getForEdit( IdxType idx )
{
    SharedObject* shobj = gtObj( idx );
    return RefMan<Wavelet>( mToWvlt(,*,shobj) );
}


Wavelet* WaveletManager::gtWavelet( const ObjID& id ) const
{
    const IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return 0;
    SharedObject* obj = const_cast<SharedObject*>( savers_[idxof]->object() );
    return mToWvlt(,*,obj);
}


Saveable* WaveletManager::getSaver( const SharedObject& obj ) const
{
    return new WaveletSaver( mToWvlt(const,&,obj) );
}


bool WaveletManager::isScaled( const ObjID& id ) const
{
    if ( id.isUdf() )
	return false;

    IOPar iop( getIOObjPars(id) );
    const BufferString res = iop.find( sKeyScaled );
    return !res.isEmpty();
}


bool WaveletManager::getScalingInfo( const ObjID& id, ObjID& orgid,
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


void WaveletManager::setScalingInfo( const ObjID& id, const ObjID* orgid,
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
