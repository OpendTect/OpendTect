/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisvolprovider.h"
#include "seislineprovider.h"
#include "seisps2dprovider.h"
#include "seisps3dprovider.h"
#include "seisioobjinfo.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "keystrs.h"
#include "uistrings.h"
#include "dbman.h"


Seis::Provider::Provider()
    : forcefpdata_(false)
    , readmode_(Prod)
    , zstep_(mUdf(float))
    , seldata_(0)
    , setupchgd_(false)
{
}


Seis::Provider::~Provider()
{
    delete seldata_;
}


BufferString Seis::Provider::name() const
{
    return DBM().nameOf( dbky_ );
}


Seis::Provider* Seis::Provider::create( Seis::GeomType gt )
{
    switch ( gt )
    {
    case Vol:
	return new VolProvider;
    case VolPS:
	return new PS3DProvider;
    case Line:
	return new LineProvider;
    case LinePS:
	return new PS2DProvider;
    }

    // can't reach
    return 0;
}


Seis::Provider* Seis::Provider::create( const DBKey& dbky, uiRetVal* uirv )
{
    SeisIOObjInfo objinf( dbky );
    Provider* ret = 0;
    if ( !objinf.isOK() )
    {
	if ( uirv )
	    uirv->set( uiStrings::phrCannotFindDBEntry(dbky.toUiString()) );
    }
    else
    {
	ret = create( objinf.geomType() );
	uiRetVal dum; if ( !uirv ) uirv = &dum;
	*uirv = ret->setInput( dbky );
	if ( !uirv->isOK() )
	    { delete ret; ret = 0; }
    }

    return ret;
}


Seis::Provider* Seis::Provider::create( const IOPar& iop, uiRetVal* uirv )
{
    const DBKey ky = DBKey::getFromString( iop.find(sKey::ID()) );
    if ( ky.isInvalid() )
	return 0;

    Provider* ret = create( ky, uirv );
    if ( ret )
	ret->usePar( iop );

    return ret;
}


uiRetVal Seis::Provider::reset() const
{
    uiRetVal uirv;
    doReset( uirv );
    if ( uirv.isOK() )
    {
	if ( seldata_ && !seldata_->isAll() )
	    totalnr_ = seldata_->expectedNrTraces( is2D() );
	else
	    totalnr_ = getTotalNrInInput();
	setupchgd_ = false;
    }
    return uirv;
}


od_int64 Seis::Provider::totalNr() const
{
    Threads::Locker locker( lock_ );
    return totalnr_;
}


int Seis::Provider::nrOffsets() const
{
    return gtNrOffsets();
}


uiRetVal Seis::Provider::getComponentInfo( BufferStringSet& nms,
				       TypeSet<Seis::DataType>* pdts ) const
{
    TypeSet<Seis::DataType> dts;
    nms.setEmpty();
    if ( pdts )
	pdts->setEmpty();

    uiRetVal uirv = doGetComponentInfo( nms, dts );
    if ( uirv.isOK() && pdts )
	*pdts = dts;
    return uirv;
}


uiRetVal Seis::Provider::setInput( const DBKey& dbky )
{
    Threads::Locker locker( lock_ );
    dbky_ = dbky;
    setupchgd_ = true;
    return reset();
}


void Seis::Provider::setSampleInterval( float zs )
{
    Threads::Locker locker( lock_ );
    zstep_ = zs;
    setupchgd_ = true;
}


void Seis::Provider::selectComponent( int icomp )
{
    Threads::Locker locker( lock_ );
    selcomps_.setEmpty();
    selcomps_ += icomp;
    setupchgd_ = true;
}


void Seis::Provider::selectComponents( const TypeSet<int>& comps )
{
    Threads::Locker locker( lock_ );
    selcomps_ = comps;
    setupchgd_ = true;
}


void Seis::Provider::forceFPData( bool yn )
{
    Threads::Locker locker( lock_ );
    forcefpdata_ = yn;
}


void Seis::Provider::setReadMode( ReadMode rm )
{
    Threads::Locker locker( lock_ );
    readmode_ = rm;
    setupchgd_ = true;
}


uiRetVal Seis::Provider::usePar( const IOPar& iop )
{
    forcefpdata_ = iop.isTrue( sKeyForceFPData() );
    uiRetVal ret;
    doUsePar( iop, ret );
    return ret;
}


void Seis::Provider::setSelData( SelData* sd )
{
    Threads::Locker locker( lock_ );
    delete seldata_;
    seldata_ = sd;
    setupchgd_ = true;
}


void Seis::Provider::putTraceInGather( const SeisTrc& trc, SeisTrcBuf& tbuf )
{
    const int nrcomps = trc.data().nrComponents();
    const int trcsz = trc.size();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	SeisTrc* newtrc = new SeisTrc( trcsz,
			    trc.data().getInterpreter(icomp)->dataChar() );
	newtrc->info() = trc.info();
	newtrc->info().offset_ = icomp * 100.f;
	newtrc->data().copyFrom( trc.data(), icomp, 0 );
	tbuf.add( newtrc );
    }
}


void Seis::Provider::putGatherInTrace( const SeisTrcBuf& tbuf, SeisTrc& trc )
{
    const int nrcomps = tbuf.size();
    if ( nrcomps < 1 )
	return;

    trc.info() = tbuf.get(0)->info();
    trc.info().offset_ = 0.f;
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	const SeisTrc& buftrc = *tbuf.get( icomp );
	trc.data().addComponent( buftrc.size(),
			      buftrc.data().getInterpreter(0)->dataChar() );
	trc.data().copyFrom( buftrc.data(), 0, icomp );
    }
}


void Seis::Provider::handleTrace( SeisTrc& trc ) const
{
    ensureRightZSampling( trc );
    ensureRightDataRep( trc );
    nrdone_++;
}


void Seis::Provider::handleTraces( SeisTrcBuf& tbuf ) const
{
    for ( int idx=0; idx<tbuf.size(); idx++ )
	handleTrace( *tbuf.get(idx) );
}


bool Seis::Provider::handleSetupChanges( uiRetVal& uirv ) const
{
    if ( setupchgd_ )
	uirv = reset();
    return uirv.isOK();
}


uiRetVal Seis::Provider::getNext( SeisTrc& trc ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGetNext( trc, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTrace( trc );
    return uirv;
}


uiRetVal Seis::Provider::get( const TrcKey& trcky, SeisTrc& trc ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;
    doGet( trcky, trc, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTrace( trc );
    return uirv;
}


uiRetVal Seis::Provider::getNextGather( SeisTrcBuf& tbuf ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGetNextGather( tbuf, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTraces( tbuf );
    return uirv;
}


uiRetVal Seis::Provider::getGather( const TrcKey& trcky,
				    SeisTrcBuf& tbuf ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;
    doGetGather( trcky, tbuf, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTraces( tbuf );
    return uirv;
}


void Seis::Provider::ensureRightDataRep( SeisTrc& trc ) const
{
    if ( !forcefpdata_ )
	return;

    const int nrcomps = trc.nrComponents();
    for ( int idx=0; idx<nrcomps; idx++ )
	trc.data().convertToFPs();
}


void Seis::Provider::ensureRightZSampling( SeisTrc& trc ) const
{
    if ( mIsUdf(zstep_) )
	return;

    const ZSampling trczrg( trc.zRange() );
    ZSampling targetzs( trczrg );
    targetzs.step = zstep_;
    int nrsamps = (int)( (targetzs.stop-targetzs.start)/targetzs.step + 1.5 );
    targetzs.stop = targetzs.atIndex( nrsamps-1 );
    if ( targetzs.stop - targetzs.step*0.001f > trczrg.stop )
    {
	nrsamps--;
	if ( nrsamps < 1 )
	    nrsamps = 1;
    }
    targetzs.stop = targetzs.atIndex( nrsamps-1 );

    TraceData newtd;
    const TraceData& orgtd = trc.data();
    const int newsz = targetzs.nrSteps() + 1;
    const int nrcomps = trc.nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	const DataInterpreter<float>& di = *orgtd.getInterpreter(icomp);
	const DataCharacteristics targetdc( forcefpdata_
		? (di.nrBytes()>4 ? OD::F64 : OD::F32) : di.dataChar() );
	newtd.addComponent( newsz, targetdc );
	for ( int isamp=0; isamp<newsz; isamp++ )
	    newtd.setValue( isamp, trc.getValue(targetzs.atIndex(isamp),icomp));
    }

    trc.data() = newtd;
}


void Seis::Provider::doGetNext( SeisTrc& trc, uiRetVal& uirv ) const
{
    SeisTrcBuf tbuf( true );
    doGetNextGather( tbuf, uirv );
    putGatherInTrace( tbuf, trc );
}


void Seis::Provider::doGet( const TrcKey& tkey, SeisTrc& trc,
			    uiRetVal& uirv ) const
{
    SeisTrcBuf tbuf( true );
    doGetGather( tkey, tbuf, uirv );
    putGatherInTrace( tbuf, trc );
}


void Seis::Provider::doGetNextGather( SeisTrcBuf& tbuf, uiRetVal& uirv ) const
{
    SeisTrc trc;
    tbuf.erase();
    doGetNext( trc, uirv );
    if ( uirv.isOK() )
	putTraceInGather( trc, tbuf );
}


void Seis::Provider::doGetGather( const TrcKey& tkey, SeisTrcBuf& tbuf,
				  uiRetVal& uirv ) const
{
    SeisTrc trc;
    tbuf.erase();
    doGet( tkey, trc, uirv );
    if ( uirv.isOK() )
	putTraceInGather( trc, tbuf );
}
