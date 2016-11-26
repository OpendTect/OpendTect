/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		September 2016
________________________________________________________________________

-*/

#include "seisvolprovider.h"


Seis::Provider::Provider()
    : datarep_(OD::AutoFPRep)
    , zstep_(mUdf(float))
{
}


Seis::Provider* Seis::Provider::create( Seis::GeomType gt )
{
    switch ( gt )
    {
    case Vol:
	return new VolumeProvider;
    case VolPS:
	{ pFreeFnErrMsg("Implement VolPS"); return 0; }
    case Line:
	{ pFreeFnErrMsg("Implement Line"); return 0; }
    case LinePS:
	{ pFreeFnErrMsg("Implement LinePS"); return 0; }
    }

    pFreeFnErrMsg("Add switch case");
    return 0;
}


uiRetVal Seis::Provider::usePar( const IOPar& iop )
{
    const BufferString datareptxt = iop.find( sKeyForcedDataChar() );
    if ( datareptxt.isEmpty() )
	datarep_ = OD::AutoFPRep;
    else
	datarep_ = DataCharacteristics::UserTypeDef().parse( datareptxt );

    uiRetVal ret;
    doUsePar( iop, ret );
    return ret;
}


uiRetVal Seis::Provider::getNext( SeisTrc& trc ) const
{
    uiRetVal uirv;
    doGetNext( trc, uirv );
    if ( uirv.isOK() )
    {
	ensureRightZSampling( trc );
	ensureRightDataRep( trc );
    }
    return uirv;
}


uiRetVal Seis::Provider::get( const TrcKey& trcky, SeisTrc& trc ) const
{
    uiRetVal uirv;
    doGet( trcky, trc, uirv );
    if ( uirv.isOK() )
    {
	ensureRightZSampling( trc );
	ensureRightDataRep( trc );
    }
    return uirv;
}


void Seis::Provider::ensureRightDataRep( SeisTrc& trc ) const
{
    if ( datarep_ == OD::AutoFPRep )
	return;

    const DataCharacteristics targetdc( datarep_ );
    const int nrcomps = trc.nrComponents();
    for ( int idx=0; idx<nrcomps; idx++ )
	trc.data().convertTo( targetdc );
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
	const DataCharacteristics targetdc( datarep_ == OD::AutoFPRep
		? orgtd.getInterpreter(icomp)->dataChar() : datarep_ );
	newtd.addComponent( newsz, targetdc );
	for ( int isamp=0; isamp<newsz; isamp++ )
	    newtd.setValue( isamp, trc.getValue(targetzs.atIndex(isamp),icomp));
    }

    trc.data() = newtd;
}


Seis::VolumeProvider::VolumeProvider()
{
}


Seis::VolumeProvider::~VolumeProvider()
{
}


uiRetVal Seis::VolumeProvider::setInput( const DBKey& dbky )
{
    return uiRetVal();
}


BufferStringSet Seis::VolumeProvider::getComponentInfo() const
{
    return BufferStringSet();
}


ZSampling Seis::VolumeProvider::getZSampling() const
{
    return ZSampling( 0.f, 0.f, 1.f );
}


TrcKeySampling Seis::VolumeProvider::getHSampling() const
{
    return TrcKeySampling();
}


void Seis::VolumeProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
}


void Seis::VolumeProvider::doGetNext( SeisTrc& trc, uiRetVal& uirv ) const
{
}


void Seis::VolumeProvider::doGet( const TrcKey& trcky, SeisTrc& trc,
				  uiRetVal& uirv ) const
{
}
