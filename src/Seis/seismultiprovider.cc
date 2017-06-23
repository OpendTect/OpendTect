/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2017
________________________________________________________________________

-*/

#include "seismultiprovider.h"

#include "keystrs.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "seisprovider.h"
#include "seisbuf.h"
#include "seistrcprop.h"
#include "seisselection.h"
#include "survinfo.h"
#include "uistrings.h"

mDefineEnumUtils(Seis::MultiProvider,Policy,"Policy") {
    "Get everywhere",
    "Require only one",
    "Require atleast one",
    "Require all",
    0
};

mDefineEnumUtils(Seis::MultiProvider,ZPolicy,"Z policy") {
    "Maximum",
    "Minimum",
    0
};

static const char* sKeySpecialValue()	{ return "Special Value"; }
static const char* sKeyGeomIDs()	{ return "Geom IDs"; }
static const char* sKeyCurrentLineIdx() { return "Current line index"; }


// MultiProvider
Seis::MultiProvider::MultiProvider( Policy policy, ZPolicy zpolicy,
				    float specialvalue )
    : policy_(policy)
    , zpolicy_(zpolicy)
    , specialvalue_(specialvalue)
    , zsampling_(ZSampling::udf())
    , seldata_(0)
    , totalnr_(-1)
    , setupchgd_(false)
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
    setupchgd_ = true;
    return uirv;
}


void Seis::MultiProvider::addInput( Seis::GeomType gt )
{
    Seis::Provider* prov = Seis::Provider::create( gt );
    if ( !prov ) return;

    provs_ += prov;
    setupchgd_ = true;
}


uiRetVal Seis::MultiProvider::reset() const
{
    uiRetVal uirv;
    doReset( uirv );
    if ( uirv.isOK() )
	setupchgd_ = false;

    return uirv;
}


od_int64 Seis::MultiProvider::totalNr() const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return -1;

    return totalnr_;
}


void Seis::MultiProvider::setSelData( SelData* sd )
{
    Threads::Locker locker( lock_ );
    delete seldata_;
    seldata_ = sd;
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->setSelData( sd->clone() );
    setupchgd_ = true;
}


void Seis::MultiProvider::selectComponent( int iprov, int icomp )
{
    Threads::Locker locker( lock_ );
    if ( !provs_.validIdx(iprov) )
	{ pErrMsg("Internal: component does not exist"); return; }

    provs_[iprov]->selectComponent( icomp );
    setupchgd_ = true;
}


void Seis::MultiProvider::selectComponents( const TypeSet<int>& comps )
{
    Threads::Locker locker( lock_ );
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->selectComponents( comps );
    setupchgd_ = true;
}


void Seis::MultiProvider::forceFPData( bool yn )
{
    Threads::Locker locker( lock_ );
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->forceFPData( yn );
}


void Seis::MultiProvider::setReadMode( ReadMode rm )
{
    Threads::Locker locker( lock_ );
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->setReadMode( rm );
    setupchgd_ = true;
}


uiRetVal Seis::MultiProvider::getComponentInfo( int iprov,
			BufferStringSet& nms, DataType* dt ) const
{
    Threads::Locker locker( lock_ );
    if ( !provs_.validIdx(iprov) )
	return uiRetVal(tr("Internal: Info requested for bad component index"));

    return provs_[iprov]->getComponentInfo( nms, dt );
}


ZSampling Seis::MultiProvider::getZRange() const
{
    Threads::Locker locker( lock_ );
    return zsampling_;
}


uiRetVal Seis::MultiProvider::fillPar( IOPar& iop ) const
{
    iop.set( sKey::TotalNr(), totalnr_ );
    iop.set( sKey::ZRange(), zsampling_ );
    iop.set( sKeySpecialValue(), specialvalue_ );
    iop.set( PolicyDef().name(), MultiProvider::toUiString(policy_) );
    iop.set( ZPolicyDef().name(), MultiProvider::toUiString(zpolicy_) );

    uiRetVal uirv;
    doFillPar( iop, uirv );
    return uirv;
}


uiRetVal Seis::MultiProvider::usePar( const IOPar& iop )
{
    iop.get( sKey::TotalNr(), totalnr_ );
    iop.get( sKey::ZRange(), zsampling_ );
    iop.get( sKeySpecialValue(), specialvalue_ );
    PolicyDef().parse( iop, PolicyDef().name(), policy_ );
    ZPolicyDef().parse( iop, ZPolicyDef().name(), zpolicy_ );

    uiRetVal uirv;
    doUsePar( iop, uirv );
    if ( !provs_.isEmpty() && provs_[0]->selData() )
	setSelData( provs_[0]->selData()->clone() );

    return uirv;
}


bool Seis::MultiProvider::handleSetupChanges( uiRetVal& uirv ) const
{
    if ( setupchgd_ )
	uirv = reset();
    return uirv.isOK();
}


void Seis::MultiProvider::handleTraces( ObjectSet<SeisTrc>& trcs ) const
{
    ensureRightZSampling( trcs );
}


void Seis::MultiProvider::ensureRightZSampling(
			ObjectSet<SeisTrc>& trcs ) const
{
    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	if ( !trcs[idx] || trcs[idx]->zRange()==zsampling_ )
	    continue;

	const ZSampling trczrg = trcs[idx]->zRange();
	const int nrpts = zsampling_.nrSteps() + 1;
	SeisTrc trc( nrpts );
	trc.info() = trcs[idx]->info();
	for ( int icomp=0; icomp<trc.nrComponents(); icomp++ )
	{
	    if ( icomp > 0 ) trc.addComponent( nrpts );
	    for ( int zidx=0; zidx<nrpts; zidx++ )
	    {
		const float zpos = trc.samplePos( zidx );
		const float sampval = trczrg.includes(zpos,false)
		    ? trcs[idx]->getValue(zpos,icomp) : specialvalue_;

		trc.set( zidx, sampval, icomp );
	    }
	}
	*trcs[idx] = trc;
    }
}


void Seis::MultiProvider::doFillPar( IOPar& iop, uiRetVal& uirv ) const
{
    IOPar par;
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	uirv.add( provs_[idx]->fillPar(par) );
	const FixedString key( IOPar::compKey(sKey::Provider(),idx) );
	iop.mergeComp( par, key );
    }

    iter_.fillPar( par );
    iop.merge( par );
}


void Seis::MultiProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    setEmpty();
    int idx = -1;
    while ( true )
    {
	const FixedString key( IOPar::compKey(sKey::Provider(),++idx) );
	PtrMan<IOPar> subpar = iop.subselect( key );
	if ( !subpar || subpar->isEmpty() )
	    break;

	addInput( is2D() ? Line : Vol );
	uirv.add( provs_[idx]->usePar(*subpar) );
    }

    iter_.usePar( iop );
}


void Seis::MultiProvider::doGetNext(
	SeisTrc& trc, bool dostack, uiRetVal& uirv ) const
{
    const int nrtrcs = policy_==RequireOnlyOne ? 1 : provs_.size();
    ManagedObjectSet<SeisTrc> trcs; trcs.allowNull();
    for ( int idx=0; idx<nrtrcs; idx++ )
	trcs += 0;

    doGetNextTrcs( trcs, uirv );
    if ( !uirv.isOK() || isFinished(uirv) )
	return;

    handleTraces( trcs );

    SeisTrcBuf trcbuf( true );
    trcbuf.addTrcsFrom( trcs );
    doGetStacked( trcbuf, trc );
}


uiRetVal Seis::MultiProvider::getNext( SeisTrc& trc, bool dostack )
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGetNext( trc, dostack, uirv );
    return uirv;
}


uiRetVal Seis::MultiProvider::getNext( ObjectSet<SeisTrc>& trcs )
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    deepErase( trcs );
    trcs.allowNull();
    const int nrtrcs = policy_==RequireOnlyOne ? 1 : provs_.size();
    for ( int idx=0; idx<nrtrcs; idx++ )
	trcs += 0;

    doGetNextTrcs( trcs, uirv );
    if ( !uirv.isOK() || isFinished(uirv) )
	return uirv;

    handleTraces( trcs );
    return uirv;
}


uiRetVal Seis::MultiProvider::get(
	const TrcKey& trcky, ObjectSet<SeisTrc>& trcs ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGet( trcky, trcs, uirv );
    return uirv;
}


void Seis::MultiProvider::doGet(
	const TrcKey& tk, ObjectSet<SeisTrc>& trcs, uiRetVal& uirv ) const
{
    if ( provs_.size() != trcs.size() )
	{ pErrMsg("Size of providers and traces do not match."); return; }

    for ( int idx=0; idx<provs_.size(); idx++ )
	uirv.add( provs_[idx]->get(tk,*trcs[idx]) );

    if ( uirv.isOK() )
	handleTraces( trcs );
}


void Seis::MultiProvider::doGetNextTrcs(
	ObjectSet<SeisTrc>& trcs, uiRetVal& uirv ) const
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

	isfinished = !doMoveToNext();
	if ( isfinished )
	    break;

    } while ( !nrtrcs && policy_!=GetEveryWhere );

    if ( isfinished && !nrtrcs )
	uirv.set( uiStrings::sFinished() );
}


void Seis::MultiProvider::doGetStacked(
	SeisTrcBuf& trcbuf, SeisTrc& trc ) const
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


void Seis::MultiProvider3D::doReset( uiRetVal& uirv ) const
{
    TrcKeyZSampling tkzs( false );
    tkzs.hsamp_.survid_ = Survey::GM().default3DSurvID();
    if ( !getRanges(tkzs) )
	return;

    iter_.setSampling( tkzs.hsamp_ );
    totalnr_ = iter_.totalNr();
    zsampling_ = tkzs.zsamp_;
}


void Seis::MultiProvider3D::doGetNext(
	SeisTrc& trc, bool dostack, uiRetVal& uirv ) const
{
    Seis::MultiProvider::doGetNext( trc, dostack, uirv );
}


bool Seis::MultiProvider3D::doMoveToNext() const
{
    return iter_.next();
}


void Seis::MultiProvider3D::doGet(
	const TrcKey& tk, ObjectSet<SeisTrc>& trcs, uiRetVal& uirv ) const
{
    Seis::MultiProvider::doGet( tk, trcs, uirv );
}


#define mUpdateRange( rg, newrg, doincl ) \
if ( idx == 0 ) \
    rg = newrg; \
else if ( doincl ) \
    rg.include( newrg, false ); \
else \
    rg.limitTo( newrg );

bool Seis::MultiProvider3D::getRanges( TrcKeyZSampling& sampling ) const
{
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	TrcKeyZSampling tkzs;
	mDynamicCastGet(const Provider3D&,prov3d,*provs_[idx]);
	if ( !prov3d.getRanges(tkzs) )
	    return false;

	if ( idx == 0 )
	    sampling.hsamp_ = tkzs.hsamp_;
	else if ( policy_ != RequireAll )
	    sampling.hsamp_.include( tkzs.hsamp_ );
	else
	    sampling.hsamp_.getInterSection( tkzs.hsamp_, sampling.hsamp_ );

	mUpdateRange( sampling.zsamp_, tkzs.zsamp_, zpolicy_==Maximum );
    }

    return true;
}


void Seis::MultiProvider3D::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	PosInfo::CubeData cubedata;
	mDynamicCastGet(const Provider3D&,prov3d,*provs_[idx]);
	prov3d.getGeometryInfo( cubedata );
	cd.merge( cubedata, policy_!=RequireAll );
    }
}


// MultiProvider2D
Seis::MultiProvider2D::MultiProvider2D(Policy pl,ZPolicy zpl)
    : MultiProvider(pl,zpl)
    , curlidx_(-1)
{}


#define mUpdateTypeSet( vals, newvals ) \
if ( idx == 0 ) \
    vals = newvals; \
else if ( policy_ == RequireAll ) \
    vals.createIntersection( newvals ); \
else \
    vals.createUnion( newvals );

void Seis::MultiProvider2D::doReset( uiRetVal& uirv ) const
{
    curlidx_ = -1, totalnr_ = 0, geomids_.setEmpty();

    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	TypeSet<Pos::GeomID> geomids;
	mDynamicCastGet(const Provider2D&,prov2d,*provs_[idx]);
	for ( int lineidx=0; lineidx<prov2d.nrLines(); lineidx++ )
	    geomids += prov2d.geomID( lineidx );

	mUpdateTypeSet( geomids_, geomids );
    }

    StepInterval<int> trcrg; ZSampling zrg;
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	getRanges( idx, trcrg, zrg );
	totalnr_ += (trcrg.nrSteps() + 1);
	mUpdateRange( zsampling_, zrg, zpolicy_==Maximum );
    }

    doMoveToNextLine();
}


void Seis::MultiProvider2D::doGetNext(
	SeisTrc& trc, bool dostack, uiRetVal& uirv ) const
{
    Seis::MultiProvider::doGetNext( trc, dostack, uirv );
}


bool Seis::MultiProvider2D::doMoveToNext() const
{
    return iter_.next() || doMoveToNextLine();
}


bool Seis::MultiProvider2D::doMoveToNextLine() const
{
    if ( !geomids_.validIdx(++curlidx_) )
	return false;

    StepInterval<int> trcrg; ZSampling zrg;
    getRanges( curlidx_, trcrg, zrg );

    TrcKeySampling tks; tks.set2DDef();
    const Pos::GeomID geomid = geomID( curlidx_ );
    tks.setLineRange( Interval<int>(geomid,geomid) );
    tks.setTrcRange( trcrg );
    iter_.setSampling( tks );
    return true;
}


void Seis::MultiProvider2D::doFillPar( IOPar& iop, uiRetVal& uirv ) const
{
    MultiProvider::doFillPar( iop, uirv );

    iop.set( sKeyGeomIDs(), geomids_ );
    iop.set( sKeyCurrentLineIdx(), curlidx_ );
}


void Seis::MultiProvider2D::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    MultiProvider::doUsePar( iop, uirv );

    iop.get( sKeyGeomIDs(), geomids_ );
    iop.get( sKeyCurrentLineIdx(), curlidx_ );
}


void Seis::MultiProvider2D::doGet(
	const TrcKey& tk, ObjectSet<SeisTrc>& trcs, uiRetVal& uirv ) const
{
    Seis::MultiProvider::doGet( tk, trcs, uirv );
}


bool Seis::MultiProvider2D::getRanges( int iln, StepInterval<int>& trcrg,
				       ZSampling& zrg ) const
{
    const Pos::GeomID geomid = geomID( iln );
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	mDynamicCastGet(const Provider2D&,prov2d,*provs_[idx]);
	const int linenr = prov2d.lineNr( geomid );
	if ( linenr == -1 )
	    continue;

	StepInterval<int> tracerg; ZSampling zsamp;
	if ( !prov2d.getRanges(linenr,tracerg,zsamp) )
	    return false;

	mUpdateRange( trcrg, tracerg, policy_!=RequireAll );
	mUpdateRange( zrg, zsamp, zpolicy_==Maximum );
    }

    return true;
}


void Seis::MultiProvider2D::getGeometryInfo(
		int iln, PosInfo::Line2DData& ld ) const
{
    const Pos::GeomID geomid = geomID( iln );
    TypeSet<PosInfo::Line2DPos> positions;
    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	mDynamicCastGet(const Provider2D&,prov2d,*provs_[idx]);
	const int linenr = prov2d.lineNr( geomid );
	if ( linenr == -1 )
	    continue;

	PosInfo::Line2DData l2dd;
	prov2d.getGeometryInfo( linenr, l2dd );
	mUpdateTypeSet( positions, l2dd.positions() );
    }

    ld.setLineName( lineName(iln) );
    ld.setPositions( positions );
    ld.setZRange( zsampling_ );
}
