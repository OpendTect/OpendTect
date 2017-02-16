/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2017
________________________________________________________________________

-*/

#include "seisproviderset.h"

#include "posinfo.h"
#include "seisprovider.h"
#include "seisbuf.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "uistrings.h"


Seis::ProviderSet::ProviderSet( Policy policy )
    : policy_(policy)
{}


Seis::ProviderSet::~ProviderSet()
{
    ::deepErase( provs_ );
}


void Seis::ProviderSet::deepErase()
{
    ::deepErase( provs_ );
}


void Seis::ProviderSet::addInput( Seis::GeomType gt )
{
    Seis::Provider* prov = Seis::Provider::create( gt );
    if ( prov )
	provs_ += prov;
}


uiRetVal Seis::ProviderSet::addInput( const DBKey& dbky )
{
    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( dbky, &uirv );
    if ( prov )
	provs_ += prov;

    return uirv;
}


ZSampling Seis::ProviderSet::getZRange() const
{
    ZSampling zrg( mUdf(float), -mUdf(float), SI().zStep() );
    for ( int idx=0; idx<provs_.size(); idx++ )
	zrg.include( provs_[idx]->getZRange(), false );

    return zrg;
}


uiRetVal Seis::ProviderSet::fillPar( ObjectSet<IOPar>& iop ) const
{
    uiRetVal uirv;
    doFillPar( iop, uirv );
    return uirv;
}


uiRetVal Seis::ProviderSet::usePar( const ObjectSet<IOPar>& iop )
{
    uiRetVal uirv;
    doUsePar( iop, uirv );
    return uirv;
}


void Seis::ProviderSet::doFillPar(
		ObjectSet<IOPar>& iop, uiRetVal& uirv ) const
{
    for ( int idx=0; idx<provs_.size(); idx++ )
	uirv.add( provs_[idx]->fillPar(*iop[idx]) );
}


uiRetVal Seis::ProviderSet::getNext( SeisTrc& trc )
{
    uiRetVal uirv;
    doGetNext( trc, uirv );
    return uirv;
}


uiRetVal Seis::ProviderSet::get(
	const TrcKey& trcky, ObjectSet<SeisTrc>& trcs ) const
{
    uiRetVal uirv;
    doGet( trcky, trcs, uirv );
    return uirv;
}


Seis::ProviderSet3D::ProviderSet3D( Policy pl )
    : ProviderSet(pl)
{}


void Seis::ProviderSet3D::doGetNext( SeisTrc& trc, uiRetVal& uirv )
{
    const int nrtrcs = policy_==RequireOnlyOne ? 1 : provs_.size();
    ManagedObjectSet<SeisTrc> trcs; trcs.allowNull();
    for ( int idx=0; idx<nrtrcs; idx++ )
	trcs += 0;

    doGetNextTrcs( trcs, uirv );
    SeisTrcBuf trcbuf( true );
    trcbuf.addTrcsFrom( trcs );
    getStacked( trcbuf, trc );
}


void Seis::ProviderSet3D::doGetNextTrcs(
	ObjectSet<SeisTrc>& trcs, uiRetVal& uirv )
{
    if ( provs_.isEmpty() || trcs.size()<provs_.size() )
	{ uirv.set( uiStrings::sFinished() ); return; }

    int nrtrcs = 0;
    do
    {
	for ( int idx=0; idx<provs_.size(); idx++ )
	{
	    SeisTrc* trc = new SeisTrc;
	    const uiRetVal ret = provs_[idx]->get( iter_.curTrcKey(), *trc );
	    if ( !ret.isOK() )
		{ delete trc; continue; }

	    trcs.replace( idx, trc );
	    if ( ++nrtrcs==1 && policy_==RequireOnlyOne )
		break;
	}
    } while ( iter_.next() && !nrtrcs );

    if ( !nrtrcs )
	uirv.set( uiStrings::sFinished() );
}


void Seis::ProviderSet3D::doGet(
	const TrcKey& tk, ObjectSet<SeisTrc>& trcs, uiRetVal& uirv ) const
{
    if ( provs_.isEmpty() || trcs.size()<provs_.size() )
	return;

    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	uirv.add( provs_[idx]->get(tk,*trcs[idx]) );
	if ( !uirv.isOK() )
	    { errmsg_ = uirv; return; }
    }
}


void Seis::ProviderSet3D::getStacked( SeisTrcBuf& trcbuf, SeisTrc& trc )
{
    int nrtrcs = trcbuf.size();
    if ( nrtrcs < 1 )
	return;
    else if ( nrtrcs == 1 )
	{ trc = *trcbuf.first(); return; }

    SeisTrcBuf nulltrcs( true );
    for ( int idx=nrtrcs-1; idx>-1; idx-- )
	if ( !trcbuf.get(idx) || trcbuf.get(idx)->isNull() )
	    nulltrcs.add( trcbuf.remove(idx) );

    nrtrcs = trcbuf.size();
    if ( nrtrcs < 1 )
	return;
    if ( nrtrcs == 1 )
	{ trc = *trcbuf.first(); return; }

    SeisTrcPropChg stckr( *trcbuf.first() );
    for ( int idx=1; idx<nrtrcs; idx++ )
	stckr.stack( *trcbuf.get(idx), false, mCast(float,idx) );

    trc = *trcbuf.first();
}


bool Seis::ProviderSet3D::getRanges( TrcKeyZSampling& sampling ) const
{
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	TrcKeyZSampling tkzs;
	mDynamicCastGet(const Provider3D&,prov3d,*provs_[idx]);
	if ( !prov3d.getRanges(tkzs) )
	    return false;

	sampling.include( tkzs );
    }

    return true;
}


void Seis::ProviderSet3D::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	PosInfo::CubeData cubedata;
	mDynamicCastGet(const Provider3D&,prov3d,*provs_[idx]);
	prov3d.getGeometryInfo( cubedata );
	cd.merge( cubedata, true );
    }
}


void Seis::ProviderSet3D::doUsePar(
	const ObjectSet<IOPar>& iop, uiRetVal& uirv )
{
    for ( int idx=0; idx<provs_.size(); idx++ )
	uirv.add( provs_[idx]->usePar(*iop[idx]) );

    TrcKeySampling tks( false );
    tks.survid_ = Survey::GM().default3DSurvID();
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	TrcKeyZSampling tkzs;
	mDynamicCastGet(const Provider3D&,prov3d,*provs_[idx]);
	if ( !prov3d.getRanges(tkzs) )
	    continue;

	tks.include( tkzs.hsamp_ );
    }

    iter_.setSampling( tks );
}
