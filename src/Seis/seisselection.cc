/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data keys
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "seisselectionimpl.h"
#include "cubesampling.h"
#include "pickset.h"
#include "picksettr.h"
#include "binidvalset.h"
#include "polygon.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "keystrs.h"
#include "linekey.h"
#include "tableposprovider.h"
#include "polyposprovider.h"
#include "strmprov.h"

#define mGetSpecKey(s,k) IOPar::compKey(sKey::s(),k)


Seis::SelData::SelData()
    : linekey_(*new LineKey)
    , isall_(false)
{
}


Seis::SelData::~SelData()
{
    delete &linekey_;
}


void Seis::SelData::copyFrom( const Seis::SelData& sd )
{
    isall_ = sd.isall_;
    linekey_ = sd.linekey_;
}


void Seis::SelData::removeFromPar( IOPar& iop )
{
    iop.removeWithKey( sKey::BinIDSel() );
    iop.set( sKey::Type(), sKey::None() );
}


void Seis::SelData::extendH( const BinID& so, const BinID* stepoutstep )
{
    BinID sos( SI().inlStep(), SI().crlStep() );
    if ( stepoutstep ) sos = *stepoutstep;
    doExtendH( so, sos );
}


Seis::SelData* Seis::SelData::get( Type t )
{
    switch ( t )
    {
    case Seis::Table:	return new Seis::TableSelData;
    case Seis::Polygon:	return new Seis::PolySelData;
    default:		return new Seis::RangeSelData( true );
    }
}



Seis::SelData* Seis::SelData::get( const IOPar& iop )
{
    const Type t = Seis::selTypeOf( iop.find(sKey::Type()) );
    Seis::SelData* sd = get( t );
    sd->usePar( iop );
    return sd;
}


Seis::SelData* Seis::SelData::get( const Pos::Provider& prov )
{
    if ( prov.is2D() )
	return 0;

    return 0;
}


Interval<int> Seis::SelData::inlRange() const
{
    return HorSampling(true).inlRange();
}


Interval<int> Seis::SelData::crlRange() const
{
    return HorSampling(true).crlRange();
}


Interval<float> Seis::SelData::zRange() const
{
    Interval<float> ret; assign( ret, SI().zRange(false) );
    return ret;
}



void Seis::SelData::fillPar( IOPar& iop ) const
{
    const char* typstr = Seis::nameOf(type());
    iop.set( sKey::Type(), isall_ ? (const char*) sKey::None() : typstr );
    if ( linekey_.isEmpty() )
	iop.removeWithKey( sKey::LineKey() );
    else
	iop.set( sKey::LineKey(), linekey_ );
}


void Seis::SelData::usePar( const IOPar& iop )
{
    const char* res = iop.find( sKey::Type() );
    if ( !res )
	res = iop.find( sKey::BinIDSel() );
    isall_ = !res || !*res || *res == *sKey::None();

    iop.get( sKey::LineKey(), linekey_ );
    res = iop.find( sKey::Attribute() );
    if ( res && *res )
	linekey_.setAttrName( res );
}


int Seis::SelData::tracesInSI() const
{
    return HorSampling(true).totalNr();
}


//--- Range ---


Seis::RangeSelData::RangeSelData( bool initsi )
    : cs_(*new CubeSampling(initsi))
{
    if ( initsi )
	setIsAll();
}


Seis::RangeSelData::RangeSelData( const HorSampling& hs )
    : cs_(*new CubeSampling(false))
{
    cs_.hrg = hs;
    cs_.zrg = SI().zRange(false);
}


Seis::RangeSelData::RangeSelData( const CubeSampling& cs )
    : cs_(*new CubeSampling(cs))
{
}


Seis::RangeSelData::RangeSelData( const Seis::RangeSelData& sd )
    : cs_(*new CubeSampling(false))
{
    copyFrom(sd);
}


Seis::RangeSelData::~RangeSelData()
{
    delete &cs_;
}


void Seis::RangeSelData::copyFrom( const Seis::SelData& sd )
{
    if ( this == &sd ) return;

    SelData::copyFrom( sd );
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::RangeSelData&,rsd,sd)
	cs_ = rsd.cs_;
    }
    else
    {
	Interval<int> rg( sd.inlRange() );
	cs_.hrg.start.inl = rg.start; cs_.hrg.stop.inl = rg.stop;
	rg = sd.crlRange();
	cs_.hrg.start.crl = rg.start; cs_.hrg.stop.crl = rg.stop;
	assign( cs_.zrg, sd.zRange() );
    }
}


Interval<int> Seis::RangeSelData::inlRange() const
{
    return isall_ ? Seis::SelData::inlRange() : cs_.hrg.inlRange();
}


Interval<int> Seis::RangeSelData::crlRange() const
{
    return isall_ ? Seis::SelData::crlRange() : cs_.hrg.crlRange();
}


Interval<float> Seis::RangeSelData::zRange() const
{
    return isall_ ? Seis::SelData::zRange() : cs_.zrg;
}


bool Seis::RangeSelData::setInlRange( Interval<int> rg )
{
    cs_.hrg.start.inl = rg.start; cs_.hrg.stop.inl = rg.stop;
    return true;
}


bool Seis::RangeSelData::setCrlRange( Interval<int> rg )
{
    cs_.hrg.start.crl = rg.start; cs_.hrg.stop.crl = rg.stop;
    return true;
}


bool Seis::RangeSelData::setZRange( Interval<float> rg )
{
    cs_.zrg.start = rg.start;
    cs_.zrg.stop = rg.stop;
    return true;
}


void Seis::RangeSelData::fillPar( IOPar& iop ) const
{
    Seis::SelData::fillPar( iop );
    if ( !isall_ )
	cs_.fillPar( iop );
}


void Seis::RangeSelData::usePar( const IOPar& iop )
{
    Seis::SelData::usePar( iop );
    if ( !isall_ )
	cs_.usePar( iop );
}


void Seis::RangeSelData::extendZ( const Interval<float>& zrg )
{
    cs_.zrg.start += zrg.start;
    cs_.zrg.stop += zrg.stop;
}


void Seis::RangeSelData::doExtendH( BinID so, BinID sos )
{
    cs_.hrg.start.inl -= so.inl * sos.inl;
    cs_.hrg.start.crl -= so.crl * sos.crl;
    cs_.hrg.stop.inl += so.inl * sos.inl;
    cs_.hrg.stop.crl += so.crl * sos.crl;
}


void Seis::RangeSelData::include( const Seis::SelData& sd )
{
    if ( isall_ )
	return;
    if ( sd.isAll() )
	{ isall_ = true; return; }

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::RangeSelData&,rsd,sd)
	cs_.include( rsd.cs_ );
	return;
    }

    Interval<int> rg( sd.inlRange() );
    if ( cs_.hrg.start.inl > rg.start ) cs_.hrg.start.inl = rg.start;
    if ( cs_.hrg.stop.inl < rg.stop ) cs_.hrg.stop.inl = rg.stop;
    rg = sd.crlRange();
    if ( cs_.hrg.start.crl > rg.start ) cs_.hrg.start.crl = rg.start;
    if ( cs_.hrg.stop.crl < rg.stop ) cs_.hrg.stop.crl = rg.stop;
    const Interval<float> zrg( sd.zRange() );
    if ( cs_.zrg.start > rg.start ) cs_.zrg.start = rg.start;
    if ( cs_.zrg.stop < rg.stop ) cs_.zrg.stop = rg.stop;
}


int Seis::RangeSelData::selRes( const BinID& bid ) const
{
    if ( isall_ ) return 0;

    int inlres = cs_.hrg.start.inl > bid.inl || cs_.hrg.stop.inl < bid.inl
		? 2 : 0;
    int crlres = cs_.hrg.start.crl > bid.crl || cs_.hrg.stop.crl < bid.crl
		? 2 : 0;
    int rv = inlres + 256 * crlres;
    if ( rv != 0 ) return rv;

    BinID step( cs_.hrg.step.inl, cs_.hrg.step.crl );
    if ( step.inl < 1 ) step.inl = 1;
    if ( step.crl < 1 ) step.crl = 1;
    inlres = (bid.inl - cs_.hrg.start.inl) % step.inl ? 1 : 0;
    crlres = (bid.crl - cs_.hrg.start.crl) % step.crl ? 1 : 0;
    return inlres + 256 * crlres;
}


int Seis::RangeSelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    if ( isall_ ) return tracesInSI();

    HorSampling hs( cs_.hrg );
    if ( step ) hs.step = *step;
    const int nrinl = for2d ? 1 : hs.nrInl();
    const int nrcrl = hs.nrCrl();
    return nrinl * nrcrl;
}


//--- Table ---


Seis::TableSelData::TableSelData()
    : bvs_(*new BinIDValueSet(1,false))
    , extraz_(0,0)
    , fixedzrange_(Interval<float>(mUdf(float),mUdf(float)))
{
}


Seis::TableSelData::TableSelData( const BinIDValueSet& bvs,
				  const Interval<float>* extraz )
    : bvs_(*new BinIDValueSet(bvs))
    , extraz_(0,0)
    , fixedzrange_(Interval<float>(mUdf(float),mUdf(float)))
{
    bvs_.setNrVals( 1, true );
    if ( extraz ) extraz_ = *extraz;
}


Seis::TableSelData::TableSelData( const Seis::TableSelData& sd )
    : bvs_(*new BinIDValueSet(1,false))
    , extraz_(0,0)
    , fixedzrange_(Interval<float>(mUdf(float),mUdf(float)))
{
    copyFrom(sd);
}


Seis::TableSelData::~TableSelData()
{
    delete &bvs_;
}


void Seis::TableSelData::copyFrom( const Seis::SelData& sd )
{
    if ( this == &sd ) return;

    SelData::copyFrom( sd );
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::TableSelData&,tsd,sd)
	bvs_ = tsd.bvs_;
	extraz_ = tsd.extraz_;
	fixedzrange_ = tsd.fixedzrange_;
    }
    else
    {
	//This delivers an enormous table - usually for nothing
	pErrMsg( "Not impl" );
    }
}


Interval<int> Seis::TableSelData::inlRange() const
{
    if ( isall_  ) return Seis::SelData::inlRange();
    return bvs_.inlRange();
}


Interval<int> Seis::TableSelData::crlRange() const
{
    if ( isall_  ) return Seis::SelData::crlRange();
    return bvs_.crlRange();
}


Interval<float> Seis::TableSelData::zRange() const
{
    if ( isall_ ) return Seis::SelData::zRange();

    Interval<float> zrg = bvs_.valRange( 0 ) + extraz_;
    if ( zrg.isUdf() )
	zrg = SI().zRange(true);

    return !bvs_.nrVals() ? fixedzrange_ : zrg;
}


bool Seis::TableSelData::setZRange( Interval<float> rg )
{
    fixedzrange_ = rg;
    return true;
}


#define mGetTableKey(k) mGetSpecKey(Table,k)

void Seis::TableSelData::fillPar( IOPar& iop ) const
{
    Seis::SelData::fillPar( iop );
    if ( isall_ ) return;
    iop.set( mGetTableKey("ExtraZ"), extraz_ );
    bvs_.fillPar( iop, mGetTableKey("Data") );
}


void Seis::TableSelData::usePar( const IOPar& iop )
{
    bvs_.setEmpty();
    Seis::SelData::usePar( iop );
    if ( isall_ ) return;

    iop.get( mGetTableKey("ExtraZ"), extraz_ );
    Pos::TableProvider3D::getBVSFromPar( iop, bvs_ );
}


void Seis::TableSelData::extendZ( const Interval<float>& zrg )
{
    extraz_.start += zrg.start;
    extraz_.stop += zrg.stop;
}


void Seis::TableSelData::doExtendH( BinID so, BinID sos )
{
    bvs_.extend( so, sos );
}


void Seis::TableSelData::include( const Seis::SelData& sd )
{
    if ( isall_ )
	return;
    if ( sd.isAll() )
	{ isall_ = true; return; }

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::TableSelData&,tsd,sd)
	bvs_.append( tsd.bvs_ );
	if ( extraz_.start < tsd.extraz_.start )
	    extraz_.start = tsd.extraz_.start;
	if ( extraz_.stop > tsd.extraz_.stop )
	    extraz_.stop = tsd.extraz_.stop;
    }
    else
    {
	pErrMsg( "Not impl" );
    }
}


int Seis::TableSelData::selRes( const BinID& bid ) const
{
    if ( isall_ ) return 0;

    const BinIDValueSet::Pos pos( bvs_.findFirst(bid) );
    if ( pos.j >= 0 ) return 0;

    const int inlres = pos.i < 0 ? 2 : 0;
    const int crlres = 1; // Maybe not true, but safe
    return inlres + 256 * crlres;
}


int Seis::TableSelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    return isall_ ? tracesInSI() : bvs_.totalSize();
}


//--- Poly ---


Seis::PolySelData::PolySelData()
    : stepoutreach_(0,0)
{
    initZrg( 0 );
}


Seis::PolySelData::PolySelData( const ODPolygon<float>& poly,
			        const Interval<float>* zrg )
    : stepoutreach_(0,0)
{
    polys_ += new ODPolygon<float>( poly );
    initZrg( zrg );
}


Seis::PolySelData::PolySelData( const ODPolygon<int>& poly,
			        const Interval<float>* zrg )
    : stepoutreach_(0,0)
{
    polys_ += new ODPolygon<float>;

    for ( int idx=0; idx<poly.size(); idx++ )
    {
	const Geom::Point2D<int>& pt = poly.getVertex( idx );
	polys_[0]->add( Geom::Point2D<float>( pt.x, pt.y ) );
    }
    initZrg( zrg );
}


Seis::PolySelData::PolySelData( const Seis::PolySelData& sd )
    : stepoutreach_(0,0)
    , zrg_(sd.zrg_)
{
    for ( int idx=0; idx<sd.polys_.size(); idx++ )
	polys_ += new ODPolygon<float>( *sd.polys_[idx] );
}


void Seis::PolySelData::initZrg( const Interval<float>* zrg )
{
    if ( zrg )
	zrg_ = *zrg;
    else
	assign( zrg_, SI().zRange(false) );
}


Seis::PolySelData::~PolySelData()
{
    deepErase( polys_ );
}


void Seis::PolySelData::copyFrom( const Seis::SelData& sd )
{
    if ( this == &sd ) return;

    SelData::copyFrom( sd );
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::PolySelData&,psd,sd)
	stepoutreach_ = psd.stepoutreach_;
	zrg_ = psd.zrg_;

	deepErase( polys_ );
	for ( int idx=0; idx<psd.polys_.size(); idx++ )
	    polys_ += new ODPolygon<float>( *psd.polys_[idx] );
    }
    else if ( sd.type() == Seis::Range )
    {
	mDynamicCastGet(const Seis::RangeSelData*,rsd,&sd)
	if ( !rsd ) pErrMsg( "Huh" );
	ODPolygon<float>* poly = new ODPolygon<float>;
	const CubeSampling& cs = rsd->cubeSampling();
	poly->add( Geom::Point2D<float>(cs.hrg.start.inl,cs.hrg.start.crl) );
	poly->add( Geom::Point2D<float>(cs.hrg.stop.inl,cs.hrg.start.crl) );
	poly->add( Geom::Point2D<float>(cs.hrg.stop.inl,cs.hrg.stop.crl) );
	poly->add( Geom::Point2D<float>(cs.hrg.start.inl,cs.hrg.stop.crl) );
	deepErase( polys_ );
	polys_ += poly;
    }
}


Interval<int> Seis::PolySelData::inlRange() const
{
    if ( isall_ ) return Seis::SelData::inlRange();
    
    if ( polys_.isEmpty() ) 
	return Interval<int>( mUdf(int), mUdf(int) );

    Interval<float> floatrg( polys_[0]->getRange(true) );
    for ( int idx=1; idx<polys_.size(); idx++ )
	floatrg.include( polys_[idx]->getRange(true) );

    Interval<int> intrg( mNINT32(floatrg.start), mNINT32(floatrg.stop) );
    intrg.widen( stepoutreach_.inl );
    intrg.limitTo( Seis::SelData::inlRange() );

    return intrg;
}


Interval<int> Seis::PolySelData::crlRange() const
{
    if ( isall_  ) return Seis::SelData::crlRange();
    
    if ( polys_.isEmpty() ) 
	return Interval<int>( mUdf(int), mUdf(int) );
    
    Interval<float> floatrg( polys_[0]->getRange(false) );
    for ( int idx=1; idx<polys_.size(); idx++ )
	floatrg.include( polys_[idx]->getRange(false) );
    
    Interval<int> intrg( mNINT32(floatrg.start), mNINT32(floatrg.stop) );
    intrg.widen( stepoutreach_.crl );
    intrg.limitTo( Seis::SelData::crlRange() );
    
    return intrg;
}


Interval<float> Seis::PolySelData::zRange() const
{
    if ( isall_ ) return Seis::SelData::zRange();
    return zrg_;
}


#define mGetPolyKey(k) mGetSpecKey(Polygon,k)

void Seis::PolySelData::fillPar( IOPar& iop ) const
{
    Seis::SelData::fillPar( iop );
    if ( isall_ ) return;

    iop.set( mGetPolyKey(sKey::ZRange()), zrg_ );
    iop.set( mGetPolyKey("Stepoutreach"), stepoutreach_ );

    iop.set( mGetPolyKey("NrPolygons"), polys_.size() );
    for ( int idx=0; idx<polys_.size(); idx++ )
	::fillPar( iop, *polys_[idx], mGetPolyKey(idx) );
}


void Seis::PolySelData::usePar( const IOPar& iop )
{
    Seis::SelData::usePar( iop );
    if ( isall_ ) { deepErase( polys_ ); return; }

    const bool wasfilled = !polys_.isEmpty();

    iop.get( mGetPolyKey(sKey::ZRange()), zrg_ );
    iop.get( mGetPolyKey("Stepoutreach"), stepoutreach_ );

    int nrpolys = 0;
    iop.get( mGetPolyKey("NrPolygons"), nrpolys );
    if ( nrpolys < 2 )
    {
	ODPolygon<float>* poly = Pos::PolyProvider3D::polyFromPar(iop,0);
	if ( poly )
	{
	    deepErase( polys_ );
	    polys_ += poly;
	    return;
	}
    }

    if ( nrpolys == 0 && wasfilled )
	return;

    deepErase( polys_ );
    for ( int idx=0; idx<nrpolys; idx++ )
    {
	ODPolygon<float>* poly = new ODPolygon<float>;
	::usePar( iop, *poly, mGetPolyKey(idx) );
	polys_ += poly;
    }
}


void Seis::PolySelData::extendZ( const Interval<float>& zrg )
{
    zrg_.start += zrg.start;
    zrg_.stop += zrg.stop;
}


void Seis::PolySelData::doExtendH( BinID so, BinID sos )
{ stepoutreach_ += so * sos; }


void Seis::PolySelData::include( const Seis::SelData& sd )
{
    if ( isall_ )
	return;
    if ( sd.isAll() )
	{ isall_ = true; return; }

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::PolySelData&,psd,sd)
	if ( zrg_.start < psd.zrg_.start )
	    zrg_.start = psd.zrg_.start;
	if ( zrg_.stop > psd.zrg_.stop )
	    zrg_.stop = psd.zrg_.stop;
	
	for ( int idx=0; idx<psd.polys_.size(); idx++ )
	    polys_ += new ODPolygon<float>( *psd.polys_[idx] );
    }
    else
    {
	mDynamicCastGet(const Seis::RangeSelData*,rsd,&sd)
	if ( rsd )
	{
	    const Interval<int> inlrg = rsd->inlRange();
	    const Interval<int> crlrg = rsd->crlRange();
	    const Interval<float> zrg = rsd->zRange();
	    
	    // Must withdraw stepout already included in RangeSelData, since 
	    // PolySelData will add it again on-the-fly. Equality is assumed.
	    
	    ODPolygon<float>* rect = new ODPolygon<float>();
	    Geom::Point2D<float> point( inlrg.start + stepoutreach_.inl,
		    			crlrg.start + stepoutreach_.crl );
	    rect->add( point );
	    point.y = crlrg.stop - stepoutreach_.crl;
	    rect->add( point );
	    point.x = inlrg.stop - stepoutreach_.inl;
	    rect->add( point );
	    point.y = crlrg.start + stepoutreach_.crl;
	    rect->add( point );

	    polys_ += rect;
		 
	    if ( zrg_.start < zrg.start )
		zrg_.start = zrg.start;
	    if ( zrg_.stop > zrg.stop )
		zrg_.stop = zrg.stop;
	}
	else
	{
	    pErrMsg( "Not impl" );
	}
    }
}


int Seis::PolySelData::selRes( const BinID& bid ) const
{
    if ( isall_ ) return 0;
    
    Interval<float> inlrg( bid.inl, bid.inl);
    inlrg.widen( stepoutreach_.inl );
    Interval<float> crlrg( bid.crl, bid.crl);
    crlrg.widen( stepoutreach_.crl );

    for ( int idx=0; idx<polys_.size(); idx++ )
    {
	if ( polys_[idx]->windowOverlaps(inlrg, crlrg, 0.5) )
	    return 0;
    }

    const int inlres = inlRange().includes(bid.inl,true) ? 0 : 2;
    const int crlres = 1; // Maybe not true, but safe
    return inlres + 256 * crlres;
}


int Seis::PolySelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    if ( isall_ || for2d ) return tracesInSI();

    int estnrtraces = 0;
    // Estimation does not compensate for eventual overlap between polys
    for ( int idx=0; idx<polys_.size(); idx++ )
    {
	const Interval<float> polyinlrg = polys_[idx]->getRange( true );
	const Interval<float> polycrlrg = polys_[idx]->getRange( false );
	const float rectarea = polyinlrg.width() * polycrlrg.width();
    const float coverfrac = rectarea ? polys_[idx]->area()/rectarea : 1.0f;
	
	Interval<int> inlrg( mNINT32(polyinlrg.start), mNINT32(polyinlrg.stop) );
	inlrg.widen( stepoutreach_.inl );
	Interval<int> crlrg( mNINT32(polycrlrg.start), mNINT32(polycrlrg.stop) );
	crlrg.widen( stepoutreach_.crl );
	
	HorSampling hs; 
	hs.set( inlrg, crlrg );
	if ( step ) hs.step = *step;
	hs.snapToSurvey();
	estnrtraces += mNINT32( coverfrac * hs.totalNr() );
    }

    return mMIN( estnrtraces, tracesInSI() );
}
