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
#include "seisselection.h"
#include "survinfo.h"
#include "uistrings.h"


// MultiProvider
Seis::MultiProvider::MultiProvider( Policy policy, ZPolicy zpolicy )
    : policy_(policy)
    , zpolicy_(zpolicy)
    , seldata_(0)
{}


Seis::MultiProvider::~MultiProvider()
{
    setEmpty();
    delete seldata_;
}


void Seis::MultiProvider::setEmpty()
{
    ::deepErase( provs_ );
}


uiRetVal Seis::MultiProvider::addInput( const DBKey& dbky )
{
    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( dbky, &uirv );
    if ( !prov ) return uirv;

    provs_ += prov;
    return uirv;
}


void Seis::MultiProvider::addInput( Seis::GeomType gt )
{
    Seis::Provider* prov = Seis::Provider::create( gt );
    if ( prov )
	provs_ += prov;
}


void Seis::MultiProvider::setSelData( SelData* sd )
{
    delete seldata_;
    seldata_ = sd;
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->setSelData( sd->clone() );
}


void Seis::MultiProvider::selectComponent( int iprov, int icomp )
{
    if ( !provs_.validIdx(iprov) )
	{ pErrMsg("Internal: component does not exist"); return; }

    provs_[iprov]->selectComponent( icomp );
}


void Seis::MultiProvider::selectComponents( const TypeSet<int>& comps )
{
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->selectComponents( comps );
}


void Seis::MultiProvider::forceFPData( bool yn )
{
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->forceFPData( yn );
}


void Seis::MultiProvider::setReadMode( ReadMode rm )
{
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->setReadMode( rm );
}


uiRetVal Seis::MultiProvider::getComponentInfo( int iprov,
	BufferStringSet& nms, TypeSet<Seis::DataType>* dts ) const
{
    if ( !provs_.validIdx(iprov) )
	return uiRetVal( tr("Internal: component does not exist") );

    return provs_[iprov]->getComponentInfo( nms, dts );
}


ZSampling Seis::MultiProvider::getZRange() const
{
    if ( provs_.isEmpty() )
	return ZSampling::udf();

    ZSampling zrg = provs_[0]->getZRange();
    for ( int idx=1; idx<provs_.size(); idx++ )
    {
	if ( zpolicy_ == Maximum )
	    zrg.include( provs_[idx]->getZRange(), false );
	else
	    zrg.limitTo( provs_[idx]->getZRange() );
    }

    return zrg;
}


uiRetVal Seis::MultiProvider::fillPar( ObjectSet<IOPar>& iop ) const
{
    uiRetVal uirv;
    doFillPar( iop, uirv );
    return uirv;
}


uiRetVal Seis::MultiProvider::usePar( const ObjectSet<IOPar>& iop )
{
    uiRetVal uirv;
    doUsePar( iop, uirv );
    return uirv;
}


void Seis::MultiProvider::doFillPar(
		ObjectSet<IOPar>& iop, uiRetVal& uirv ) const
{
    for ( int idx=0; idx<provs_.size(); idx++ )
	uirv.add( provs_[idx]->fillPar(*iop[idx]) );
}


void Seis::MultiProvider::doGetNext( SeisTrc& trc, bool dostack,
				     uiRetVal& uirv )
{
    const int nrtrcs = policy_==RequireOnlyOne ? 1 : provs_.size();
    ManagedObjectSet<SeisTrc> trcs; trcs.allowNull();
    for ( int idx=0; idx<nrtrcs; idx++ )
	trcs += 0;

    doGetNextTrcs( trcs, uirv );
    if ( isFinished(uirv) )
	return;

    SeisTrcBuf trcbuf( true );
    trcbuf.addTrcsFrom( trcs );
    doGetStacked( trcbuf, trc );
}


uiRetVal Seis::MultiProvider::getNext( SeisTrc& trc, bool dostack )
{
    uiRetVal uirv;
    doGetNext( trc, dostack, uirv );
    return uirv;
}


uiRetVal Seis::MultiProvider::get(
	const TrcKey& trcky, ObjectSet<SeisTrc>& trcs ) const
{
    uiRetVal uirv;
    doGet( trcky, trcs, uirv );
    return uirv;
}


void Seis::MultiProvider::doGet(
	const TrcKey& tk, ObjectSet<SeisTrc>& trcs, uiRetVal& uirv ) const
{
    if ( provs_.isEmpty() || trcs.size()!=provs_.size() )
	{ pErrMsg("Size of providers and traces do not match."); return; }

    for ( int idx=0; idx<provs_.size(); idx++ )
	uirv.add( provs_[idx]->get(tk,*trcs[idx]) );
}


void Seis::MultiProvider::doGetStacked( SeisTrcBuf& trcbuf, SeisTrc& trc )
{
    SeisTrcBuf nulltrcs( true );
    for ( int idx=trcbuf.size()-1; idx>=0; idx-- )
    {
	if ( !trcbuf.get(idx) )
	    trcbuf.remove( idx );
	if ( trcbuf.get(idx)->isNull() )
	    nulltrcs.add( trcbuf.remove(idx) );
    }

    if ( trcbuf.size() < 1 )
    {
	if ( !nulltrcs.isEmpty() )
	    trc = *nulltrcs.first();
	return;
    }

    SeisTrcPropChg stckr( *trcbuf.first() );
    for ( int idx=1; idx<trcbuf.size(); idx++ )
	stckr.stack( *trcbuf.get(idx), false, mCast(float,idx) );

    trc = *trcbuf.first();
}


// MultiProvider3D
Seis::MultiProvider3D::MultiProvider3D( Policy pl, ZPolicy zpl )
    : MultiProvider(pl,zpl)
{}


void Seis::MultiProvider3D::doGetNext(
	SeisTrc& trc, bool dostack, uiRetVal& uirv )
{
    Seis::MultiProvider::doGetNext( trc, dostack, uirv );
}


void Seis::MultiProvider3D::doGetNextTrcs(
	ObjectSet<SeisTrc>& trcs, uiRetVal& uirv )
{
    bool isfinished = false; int nrtrcs = 0;
    do
    {
	for ( int idx=0; idx<provs_.size(); idx++ )
	{
	    SeisTrc* trc = new SeisTrc;
	    const uiRetVal ret = provs_[idx]->get( iter_.curTrcKey(), *trc);
	    if ( !ret.isOK() )
	    {
		delete trc;
		if ( policy_ == RequireAll )
		    { nrtrcs = 0; break; }
		continue;
	    }

	    if ( ++nrtrcs==1 && policy_==RequireOnlyOne )
		{ trcs.replace( 0, trc ); break; }

	    trcs.replace( idx, trc );
	}

	if ( (isfinished = !iter_.next()) )
	    break;

    } while ( !nrtrcs && policy_!=GetEveryWhere );

    if ( isfinished )
	uirv.set( uiStrings::sFinished() );
}


void Seis::MultiProvider3D::doGet(
	const TrcKey& tk, ObjectSet<SeisTrc>& trcs, uiRetVal& uirv ) const
{
    Seis::MultiProvider::doGet( tk, trcs, uirv );
}


bool Seis::MultiProvider3D::getRanges(
	TrcKeyZSampling& sampling, bool incl ) const
{
    if ( provs_.isEmpty() )
	return false;

    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	TrcKeyZSampling tkzs;
	mDynamicCastGet(const Provider3D&,prov3d,*provs_[idx]);
	if ( !prov3d.getRanges(tkzs) )
	    return false;

	if ( idx == 0 )
	    sampling = tkzs;
	else if ( incl )
	    sampling.include( tkzs );
	else
	    sampling.getIntersection( tkzs, sampling );
    }

    return true;
}


void Seis::MultiProvider3D::getGeometryInfo(
		PosInfo::CubeData& cd, bool incl ) const
{
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	PosInfo::CubeData cubedata;
	mDynamicCastGet(const Provider3D&,prov3d,*provs_[idx]);
	prov3d.getGeometryInfo( cubedata );
	cd.merge( cubedata, incl );
    }
}


void Seis::MultiProvider3D::doUsePar(
	const ObjectSet<IOPar>& iop, uiRetVal& uirv )
{
    setEmpty();
    for ( int idx=0; idx<iop.size(); idx++ )
	addInput( Vol );

    for ( int idx=0; idx<provs_.size(); idx++ )
	uirv.add( provs_[idx]->usePar(*iop[idx]) );

    TrcKeyZSampling tkzs( false );
    tkzs.hsamp_.survid_ = Survey::GM().default3DSurvID();
    if ( !getRanges(tkzs,policy_!=RequireAll) )
	return;

    iter_.setSampling( tkzs.hsamp_ );
}
