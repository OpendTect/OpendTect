/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998 / Mar 2019
-*/


#include "seistableseldata.h"
#include "seispolyseldata.h"
#include "seisrangeseldata.h"
#include "binidvalset.h"
#include "iopar.h"
#include "keystrs.h"
#include "polygon.h"
#include "polyposprovider.h"
#include "survinfo.h"
#include "tableposprovider.h"

#define mGetPolyKey(k) IOPar::compKey(sKey::Polygon(),k)
#define mGetTableKey(k) IOPar::compKey(sKey::Table(),k)
mUseType( Seis::SelData, z_rg_type );
mUseType( Seis::SelData, pos_type );
mUseType( Seis::SelData, pos_rg_type );
mUseType( Seis::SelData, idx_type );
mUseType( Seis::SelData, size_type );
typedef StepInterval<pos_type> pos_steprg_type;


Seis::TableSelData::TableSelData()
    : bvs_(*new BinIDValueSet(1,true))
    , extraz_(0,0)
    , fixedzrange_(z_rg_type(mUdf(z_type),mUdf(z_type)))
{
}


Seis::TableSelData::TableSelData( const BinIDValueSet& bvs,
				  const z_rg_type* extraz )
    : bvs_(*new BinIDValueSet(bvs))
    , extraz_(0,0)
    , fixedzrange_(z_rg_type(mUdf(z_type),mUdf(z_type)))
{
    bvs_.setNrVals( 1 );
    if ( extraz )
	extraz_ = *extraz;
}


Seis::TableSelData::TableSelData( const DBKey& dbky )
    : bvs_(*new BinIDValueSet(1,true))
    , extraz_(0,0)
    , fixedzrange_(z_rg_type(mUdf(z_type),mUdf(z_type)))
{
    IOPar iop;
    iop.set( mGetTableKey(sKey::ID()), dbky );
    Pos::TableProvider3D::getBVSFromPar( iop, bvs_ );
}


Seis::TableSelData::TableSelData( const TableSelData& sd )
    : bvs_(*new BinIDValueSet(1,true))
    , extraz_(0,0)
    , fixedzrange_(z_rg_type(mUdf(z_type),mUdf(z_type)))
{
    copyFrom(sd);
}


Seis::TableSelData::~TableSelData()
{
    delete &bvs_;
}


Seis::SelDataPosIter* Seis::TableSelData::posIter() const
{
    return new TableSelDataPosIter( *this );
}


void Seis::TableSelData::doCopyFrom( const SelData& sd )
{
    if ( this == &sd )
	return;

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const TableSelData&,tsd,sd)
	bvs_ = tsd.bvs_;
	extraz_ = tsd.extraz_;
	fixedzrange_ = tsd.fixedzrange_;
    }
    else
    {
	// This usually delivers an enormous table - most often, not needed
	pErrMsg( "Not impl" );
    }
}


void Seis::TableSelData::merge( const TableSelData& oth )
{
    bvs_.append( oth.bvs_ );
}


pos_rg_type Seis::TableSelData::inlRange() const
{
    return bvs_.inlRange();
}


pos_rg_type Seis::TableSelData::crlRange() const
{
    return bvs_.crlRange();
}


z_rg_type Seis::TableSelData::zRange( idx_type ) const
{
    if ( !mIsUdf(fixedzrange_.start) )
	return fixedzrange_;

    z_rg_type zrg = bvs_.valRange( 0 );
    if ( zrg.isUdf() )
	zrg = SI().zRange();
    else
    {
	zrg.start += extraz_.start;
	zrg.stop += extraz_.stop;
    }

    return zrg;
}


void Seis::TableSelData::doFillPar( IOPar& iop ) const
{
    iop.set( mGetTableKey("FixedZ"), fixedzrange_ );
    iop.set( mGetTableKey("ExtraZ"), extraz_ );
    bvs_.fillPar( iop, mGetTableKey("Data") );
}


void Seis::TableSelData::doUsePar( const IOPar& iop )
{
    bvs_.setEmpty();
    iop.get( mGetTableKey("FixedZ"), fixedzrange_ );
    iop.get( mGetTableKey("ExtraZ"), extraz_ );
    Pos::TableProvider3D::getBVSFromPar( iop, bvs_ );
}


void Seis::TableSelData::doExtendZ( const z_rg_type& zrg )
{
    extraz_.start += zrg.start;
    extraz_.stop += zrg.stop;
}


void Seis::TableSelData::doExtendH( BinID so, BinID sos )
{
    if ( bvs_.isEmpty() )
	return;

    const BinIDValueSet orgbvs( bvs_ );
    bvs_.extendHor( so, sos );

    const auto zrg( orgbvs.valRange(0) );
    const auto avgz = zrg.center();

    BinIDValueSet::SPos spos;
    while ( bvs_.next(spos) )
    {
	const BinID bid( bvs_.getBinID(spos) );
	if ( !orgbvs.includes(bid) )
	    bvs_.set( spos, avgz );
    }
}


int Seis::TableSelData::selRes2D( GeomID gid, pos_type trcnr ) const
{
    if ( bvs_.is2D() )
	return selRes3D( BinID(gid.lineNr(),trcnr) );
	// BinIDValueSets can actually be Bin2DValueSets

    return SelData::selRes2D( gid, trcnr );
}


int Seis::TableSelData::selRes3D( const BinID& bid ) const
{
    const auto pos = bvs_.find( bid );
    if ( pos.isValid() >= 0 )
	return 0;

    if ( !mIsZero(searchradius_,0.01) )
    {
	const auto nearestpos = bvs_.findNearest( bid );
	if ( nearestpos.isValid() )
	{
	    const auto foundbid = bvs_.getBinID( nearestpos );
	    const Coord reqcoord = SI().transform( bid );
	    const Coord foundcoord = SI().transform( foundbid );
	    if ( foundcoord.distTo<dist_type>(reqcoord) <= searchradius_ )
		return 0;
	}
    }

    const int inlres = pos.i < 0 ? 2 : 0;
    const int crlres = 1; // Maybe not true, but safe
    return inlres + 256 * crlres;
}


uiString Seis::TableSelData::gtUsrSummary() const
{
    const auto nrpos = expectedNrTraces();
    return toUiString("%1 (%2)")
		.arg( uiStrings::sPosition(nrpos) ).arg( nrpos );
}


size_type Seis::TableSelData::expectedNrTraces() const
{
    return (size_type)bvs_.totalSize();
}


Seis::TableSelDataPosIter::TableSelDataPosIter( const TableSelData& tsd )
    : SelDataPosIter(tsd)
{
}


Seis::TableSelDataPosIter::TableSelDataPosIter( const TableSelDataPosIter& oth )
    : SelDataPosIter(oth)
    , spos_(oth.spos_)
{
}


BinID Seis::TableSelDataPosIter::binID() const
{
    if ( !spos_.isValid() )
	return BinID::udf();

    return tableSelData().bvs_.getBinID( spos_ );
}


Seis::PolySelData::PolySelData()
    : stepoutreach_(0,0)
{
    initZrg( 0 );
}


Seis::PolySelData::PolySelData( const ODPolygon<float>& poly,
			        const z_rg_type* zrg )
    : stepoutreach_(0,0)
{
    polys_ += new ODPolygon<float>( poly );
    initZrg( zrg );
}


Seis::PolySelData::PolySelData( const ODPolygon<int>& poly,
			        const z_rg_type* zrg )
    : stepoutreach_(0,0)
{
    polys_ += new ODPolygon<float>;

    for ( int idx=0; idx<poly.size(); idx++ )
    {
	const Geom::Point2D<int>& pt = poly.getVertex( idx );
	polys_[0]->add( Geom::Point2D<float>((float)pt.x_,(float)pt.y_) );
    }
    initZrg( zrg );
}


Seis::PolySelData::PolySelData( const DBKey& dbky )
    : stepoutreach_(0,0)
{
    IOPar iop;
    iop.set( mGetPolyKey("NrPolygons"), 1 );
    iop.set( mGetPolyKey(sKey::ID()), dbky );
    doUsePar( iop );
}


Seis::PolySelData::PolySelData( const PolySelData& sd )
    : stepoutreach_(0,0)
    , zrg_(sd.zrg_)
{
    for ( int idx=0; idx<sd.polys_.size(); idx++ )
	polys_ += new ODPolygon<float>( *sd.polys_[idx] );
}


void Seis::PolySelData::initZrg( const z_rg_type* zrg )
{
    if ( zrg )
	zrg_ = *zrg;
    else
	assign( zrg_, SI().zRange() );
}


Seis::PolySelData::~PolySelData()
{
    deepErase( polys_ );
}


Seis::SelDataPosIter* Seis::PolySelData::posIter() const
{
    return new PolySelDataPosIter( *this );
}


void Seis::PolySelData::doCopyFrom( const SelData& sd )
{
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const PolySelData&,psd,sd)
	stepoutreach_ = psd.stepoutreach_;
	zrg_ = psd.zrg_;

	deepErase( polys_ );
	for ( int idx=0; idx<psd.polys_.size(); idx++ )
	    polys_ += new ODPolygon<float>( *psd.polys_[idx] );
    }
    else if ( sd.type() == Range )
    {
	mDynamicCastGet(const RangeSelData*,rsd,&sd)
	if ( !rsd )
	    { pErrMsg( "Huh" ); return; }
	ODPolygon<float>* poly = new ODPolygon<float>;
	const auto inlrg = sd.inlRange();
	const auto crlrg = sd.crlRange();
#define mAddPt( b1, b2 ) \
	poly->add( Geom::Point2D<float>( (float)inlrg.b1, (float)crlrg.b2 ) )
	mAddPt( start, start );
	mAddPt( start, stop );
	mAddPt( stop, stop );
	mAddPt( stop, start );
	polys_ += poly;
    }
}


void Seis::PolySelData::merge( const PolySelData& oth )
{
    for ( int idx=0; idx<oth.polys_.size(); idx++ )
	polys_ += new ODPolygon<float>( *oth.polys_[idx] );
}


pos_rg_type Seis::PolySelData::inlRange() const
{
    if ( polys_.isEmpty() )
	return pos_rg_type( mUdf(pos_type), mUdf(pos_type) );

    z_rg_type floatrg( polys_[0]->getRange(true) );
    for ( int idx=1; idx<polys_.size(); idx++ )
	floatrg.include( polys_[idx]->getRange(true) );

    pos_rg_type intrg( mNINT32(floatrg.start), mNINT32(floatrg.stop) );
    intrg.widen( stepoutreach_.inl() );
    intrg.limitTo( SelData::inlRange() );

    return intrg;
}


pos_rg_type Seis::PolySelData::crlRange() const
{
    if ( polys_.isEmpty() )
	return pos_rg_type( mUdf(pos_type), mUdf(pos_type) );

    z_rg_type floatrg( polys_[0]->getRange(false) );
    for ( int idx=1; idx<polys_.size(); idx++ )
	floatrg.include( polys_[idx]->getRange(false) );

    pos_rg_type intrg( mNINT32(floatrg.start), mNINT32(floatrg.stop) );
    intrg.widen( stepoutreach_.crl() );
    intrg.limitTo( SelData::crlRange() );

    return intrg;
}


z_rg_type Seis::PolySelData::zRange( idx_type ) const
{
    return zrg_;
}


void Seis::PolySelData::doFillPar( IOPar& iop ) const
{
    iop.set( mGetPolyKey(sKey::ZRange()), zrg_ );
    iop.set( mGetPolyKey("Stepoutreach"), stepoutreach_ );

    iop.set( mGetPolyKey("NrPolygons"), polys_.size() );
    for ( int idx=0; idx<polys_.size(); idx++ )
	::fillPar( iop, *polys_[idx], mGetPolyKey(idx) );
}


void Seis::PolySelData::doUsePar( const IOPar& iop )
{
    const bool wasfilled = !polys_.isEmpty();

    iop.get( mGetPolyKey(sKey::ZRange()), zrg_ );
    iop.get( mGetPolyKey("Stepoutreach"), stepoutreach_ );

    int nrpolys = 0;
    iop.get( mGetPolyKey("NrPolygons"), nrpolys );
    if ( nrpolys < 2 )
    {
	BufferString polynm;
	ODPolygon<float>* poly = Pos::PolyProvider3D::polyFromPar(
					iop, 0, &polynm );
	if ( poly )
	{
	    deepErase( polys_ );
	    polys_ += poly;
	    polynm_ = polynm;
	    return;
	}
    }

    if ( nrpolys == 0 && wasfilled )
	return;

    deepErase( polys_ );
    polynm_.setEmpty();
    for ( int idx=0; idx<nrpolys; idx++ )
    {
	ODPolygon<float>* poly = new ODPolygon<float>;
	::usePar( iop, *poly, mGetPolyKey(idx) );
	polys_ += poly;
    }
}


void Seis::PolySelData::doExtendZ( const z_rg_type& zrg )
{
    zrg_.start += zrg.start;
    zrg_.stop += zrg.stop;
}


void Seis::PolySelData::doExtendH( BinID so, BinID sos )
{
    stepoutreach_ += so * sos;
}


int Seis::PolySelData::selRes3D( const BinID& bid ) const
{
    z_rg_type inlrg( (float)bid.inl(), (float)bid.inl() );
    inlrg.widen( (float)stepoutreach_.inl() );
    z_rg_type crlrg( (float)bid.crl(), (float)bid.crl() );
    crlrg.widen( (float)stepoutreach_.crl() );

    for ( int idx=0; idx<polys_.size(); idx++ )
    {
	if ( polys_[idx]->windowOverlaps(inlrg, crlrg, 0.5) )
	    return 0;
    }

    const int inlres = inlRange().includes(bid.inl(),true) ? 0 : 2;
    const int crlres = 1; // Maybe not true, but safe
    return inlres + 256 * crlres;
}


uiString Seis::PolySelData::gtUsrSummary() const
{
    const auto nrpolys = polys_.size();

    if ( nrpolys < 2 )
	return polynm_.isEmpty() ? uiStrings::sPolygon()
				 : toUiString("[%1]").arg( polynm_ );

    return toUiString("%1 (%2)")
		.arg( uiStrings::sPolygon(nrpolys) ).arg( nrpolys );
}


size_type Seis::PolySelData::expectedNrTraces() const
{
    int estnrtraces = 0;
    // Estimation does not compensate for possible overlap between polys
    for ( int idx=0; idx<polys_.size(); idx++ )
    {
	const z_rg_type polyinlrg = polys_[idx]->getRange( true );
	const z_rg_type polycrlrg = polys_[idx]->getRange( false );
	const float rectarea = polyinlrg.width() * polycrlrg.width();
        const float coverfrac = rectarea ? (float)polys_[idx]->area()/rectarea
					 : 1.0f;

	pos_steprg_type inlrg( mNINT32(polyinlrg.start),
				mNINT32(polyinlrg.stop),
				SI().inlStep() );
	pos_steprg_type crlrg( mNINT32(polycrlrg.start),
				mNINT32(polycrlrg.stop),
				SI().crlStep() );
	inlrg.widen( stepoutreach_.inl() );
	crlrg.widen( stepoutreach_.crl() );

	size_type nrpolytrcs = (inlrg.nrSteps()+1) * (crlrg.nrSteps()+1);
	estnrtraces += mNINT32( coverfrac * nrpolytrcs );
    }

    const auto sinrtrcs = nrTrcsInSI();
    return sinrtrcs < estnrtraces ? sinrtrcs : estnrtraces;
}


Seis::PolySelDataPosIter::PolySelDataPosIter( const PolySelData& psd )
    : SelDataPosIter(psd)
    , curbid_(BinID::udf())
{
    const auto inlrg = polySelData().inlRange();
    const auto crlrg = polySelData().crlRange();
    firstbid_.inl() = inlrg.start; firstbid_.crl() = crlrg.start;
    lastbid_.inl() = inlrg.stop; lastbid_.crl() = crlrg.stop;
    bidstep_.inl() = SI().inlStep(); bidstep_.crl() = SI().crlStep();
}


Seis::PolySelDataPosIter::PolySelDataPosIter( const PolySelDataPosIter& oth )
    : SelDataPosIter(oth)
    , firstbid_(oth.firstbid_)
    , lastbid_(oth.lastbid_)
    , bidstep_(oth.bidstep_)
    , curbid_(oth.curbid_)
{
}


bool Seis::PolySelDataPosIter::next()
{
    if ( mIsUdf(curbid_.inl()) )
    {
	curbid_.inl() = firstbid_.inl();
	curbid_.crl() = firstbid_.crl() - bidstep_.crl();
    }

    do
    {
	curbid_.crl() += bidstep_.crl();
	if ( curbid_.crl() > lastbid_.crl() )
	{
	    curbid_.inl() += bidstep_.inl();
	    if ( curbid_.inl() > lastbid_.inl() )
		{ curbid_ = BinID::udf(); return false; }
	    curbid_.crl() = firstbid_.crl();
	}
    } while ( !polySelData().isOK(curbid_) );

    return true;
}
