/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisselectionimpl.h"
#include "trckeyzsampling.h"
#include "pickset.h"
#include "picksettr.h"
#include "binidvalset.h"
#include "polygon.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "keystrs.h"
#include "linekey.h"
#include "tableposprovider.h"
#include "polyposprovider.h"
#include "strmprov.h"

#define mGetSpecKey(s,k) IOPar::compKey(sKey::s(),k)
static const char* sKeyBinIDSel = "BinID selection";


Seis::SelData::SelData()
    : isall_(false)
    , geomid_(Survey::default3DGeomID())
{
}


Seis::SelData::~SelData()
{}


void Seis::SelData::copyFrom( const Seis::SelData& sd )
{
    isall_ = sd.isall_;
    geomid_ = sd.geomID();
}


bool Seis::SelData::operator ==( const Seis::SelData& oth ) const
{
    if ( &oth == this )
	return true;

    if ( type() != oth.type() || geomid_ != oth.geomid_ ||
	 isall_ != oth.isall_ )
	return false;

    IOPar thisiop, othiop;
    fillPar( thisiop );
    oth.fillPar( othiop );

    return thisiop == othiop;
}


bool Seis::SelData::operator !=( const Seis::SelData& oth ) const
{
    return !(*this == oth);
}


void Seis::SelData::removeFromPar( IOPar& iop )
{
    iop.removeWithKey( sKeyBinIDSel );
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
	return nullptr;

    return nullptr;
}


Interval<int> Seis::SelData::inlRange() const
{
    return TrcKeySampling(true).inlRange();
}


Interval<int> Seis::SelData::crlRange() const
{
    return TrcKeySampling(true).crlRange();
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
    if ( geomid_ == Survey::GM().cUndefGeomID() )
	iop.removeWithKey( sKey::GeomID() );
    else
	iop.set( sKey::GeomID(), geomid_ );
}


void Seis::SelData::usePar( const IOPar& iop )
{
    const char* res = iop.find( sKey::Type() );
    if ( !res )
	res = iop.find( sKeyBinIDSel );
    isall_ = !res || !*res || *res == *sKey::None();

    iop.get( sKey::GeomID(), geomid_ );
    if ( geomid_.asInt() < 0 )
    {
	res = iop.find( sKey::LineKey() );
	if ( res && *res )
	    geomid_ = Survey::GM().getGeomID( res );
    }
}


int Seis::SelData::tracesInSI() const
{
    return sCast( int, TrcKeySampling(true).totalNr() );
}


bool Seis::SelData::isOK( const TrcKey& tk ) const
{
    return tk.is2D() ? isOK( tk.geomID(), tk.trcNr() )
		     : isOK( tk.position() );
}


bool Seis::SelData::isOK( const Pos::IdxPair& pos ) const
{
    return Survey::is2DGeom( geomID() )
		    ? isOK( Pos::GeomID(pos.row()), pos.trcNr() )
		    : isOK( BinID(pos) );
}


int Seis::SelData::selRes2D( Pos::GeomID gid, int trcnr ) const
{
    if ( isAll() )
	return 0;

    if ( geomID() != gid || !Survey::isValidGeomID(gid) )
	return 256+2;

    const Survey::Geometry* geom = Survey::GM().getGeometry( gid );
    if ( !geom || !geom->is2D() )
	return 256+2;

    return geom->as2D()->data().isPresent(trcnr) ? 0 : 256;
}


//--- Range ---


Seis::RangeSelData::RangeSelData( bool initsi )
    : tkzs_(*new TrcKeyZSampling(initsi))
{
    if ( initsi )
	setIsAll();
}


Seis::RangeSelData::RangeSelData( const TrcKeySampling& hs )
    : tkzs_(*new TrcKeyZSampling(false))
{
    tkzs_.hsamp_ = hs;
    if ( hs.is2D() )
	setGeomID( hs.getGeomID() );
    else
	tkzs_.zsamp_ = SI().zRange(false);
}


Seis::RangeSelData::RangeSelData( const TrcKeyZSampling& cs )
    : tkzs_(*new TrcKeyZSampling(cs))
{
    if ( tkzs_.is2D() )
	setGeomID( tkzs_.hsamp_.getGeomID() );
}


Seis::RangeSelData::RangeSelData( const Seis::RangeSelData& sd )
    : tkzs_(*new TrcKeyZSampling(false))
{
    copyFrom(sd);
}


Seis::RangeSelData::~RangeSelData()
{
    delete &tkzs_;
}


void Seis::RangeSelData::copyFrom( const Seis::SelData& sd )
{
    if ( this == &sd ) return;

    SelData::copyFrom( sd );
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::RangeSelData&,rsd,sd)
	tkzs_ = rsd.tkzs_;
    }
    else
    {
	Interval<int> rg( sd.inlRange() );
	tkzs_.hsamp_.start_.inl() = rg.start;
	tkzs_.hsamp_.stop_.inl() = rg.stop;
	rg = sd.crlRange();
	tkzs_.hsamp_.start_.crl() = rg.start;
	tkzs_.hsamp_.stop_.crl() = rg.stop;
	assign( tkzs_.zsamp_, sd.zRange() );
    }
}


Interval<int> Seis::RangeSelData::inlRange() const
{
    return isall_ && !tkzs_.is2D() ? Seis::SelData::inlRange()
				   : tkzs_.hsamp_.inlRange();
}


Interval<int> Seis::RangeSelData::crlRange() const
{
    return isall_ && !tkzs_.is2D() ? Seis::SelData::crlRange()
				   : tkzs_.hsamp_.crlRange();
}


Interval<float> Seis::RangeSelData::zRange() const
{
    return isall_ && !tkzs_.is2D() ? Seis::SelData::zRange()
				   : tkzs_.zsamp_;
}


bool Seis::RangeSelData::setInlRange( const Interval<int>& rg )
{
    if ( tkzs_.is2D() )
	{ pErrMsg("Should not be called for 2D. Use setGeomID"); }

    tkzs_.hsamp_.start_.inl() = rg.start;
    tkzs_.hsamp_.stop_.inl() = rg.stop;
    if ( rg.hasStep() )
    {
	mDynamicCastGet(const StepInterval<int>*,inlrg,&rg);
	if ( inlrg )
	    tkzs_.hsamp_.step_.inl() = inlrg->step;
    }

    return true;
}


bool Seis::RangeSelData::setCrlRange( const Interval<int>& rg )
{
    tkzs_.hsamp_.start_.crl() = rg.start;
    tkzs_.hsamp_.stop_.crl() = rg.stop;
    if ( rg.hasStep() )
    {
	mDynamicCastGet(const StepInterval<int>*,crlrg,&rg);
	if ( crlrg )
	    tkzs_.hsamp_.step_.crl() = crlrg->step;
    }

    testIsAll2D();
    return true;
}


bool Seis::RangeSelData::setZRange( const Interval<float>& rg )
{
    tkzs_.zsamp_.start = rg.start;
    tkzs_.zsamp_.stop = rg.stop;
    if ( rg.hasStep() )
    {
	mDynamicCastGet(const StepInterval<int>*,zrg,&rg);
	if ( zrg )
	    tkzs_.zsamp_.step = zrg->step;
    }

    testIsAll2D();
    return true;
}


void Seis::RangeSelData::fillPar( IOPar& iop ) const
{
    Seis::SelData::fillPar( iop );
    if ( !isall_ )
	tkzs_.fillPar( iop );
}


void Seis::RangeSelData::usePar( const IOPar& iop )
{
    Seis::SelData::usePar( iop );
    if ( !isall_ )
	tkzs_.usePar( iop );

    StepInterval<int> trcrg;
    if ( iop.get(IOPar::compKey(sKey::TrcRange(),0),trcrg) )
    {
	BufferString linenm;
	Pos::GeomID gid = mUdfGeomID;
	if ( iop.get(sKey::LineKey(),linenm) && !linenm.isEmpty() )
	    gid = Survey::GM().getGeomID( linenm );

	if ( Survey::is2DGeom(gid) )
	    tkzs_.hsamp_.init( gid );
	else
	    tkzs_.hsamp_.set2DDef();

	tkzs_.hsamp_.setTrcRange( trcrg );
	iop.get( IOPar::compKey(sKey::ZRange(),0), tkzs_.zsamp_ );
    }

    testIsAll2D();
}


void Seis::RangeSelData::extendZ( const Interval<float>& zrg )
{
    tkzs_.zsamp_.start += zrg.start;
    tkzs_.zsamp_.stop += zrg.stop;
}


void Seis::RangeSelData::doExtendH( BinID so, BinID sos )
{
    tkzs_.hsamp_.start_.inl() -= so.inl() * sos.inl();
    tkzs_.hsamp_.start_.crl() -= so.crl() * sos.crl();
    tkzs_.hsamp_.stop_.inl() += so.inl() * sos.inl();
    tkzs_.hsamp_.stop_.crl() += so.crl() * sos.crl();
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
	tkzs_.include( rsd.tkzs_ );
	return;
    }

    Interval<int> rg( sd.inlRange() );
    if ( tkzs_.hsamp_.start_.inl() > rg.start )
	tkzs_.hsamp_.start_.inl() = rg.start;
    if ( tkzs_.hsamp_.stop_.inl() < rg.stop )
	tkzs_.hsamp_.stop_.inl() = rg.stop;
    rg = sd.crlRange();
    if ( tkzs_.hsamp_.start_.crl() > rg.start )
	tkzs_.hsamp_.start_.crl() = rg.start;
    if ( tkzs_.hsamp_.stop_.crl() < rg.stop )
	tkzs_.hsamp_.stop_.crl() = rg.stop;
    const Interval<float> zrg( sd.zRange() );
    if ( tkzs_.zsamp_.start > rg.start )
	tkzs_.zsamp_.start = mCast(float,rg.start);
    if ( tkzs_.zsamp_.stop < rg.stop )
	tkzs_.zsamp_.stop = mCast(float,rg.stop);
}


int Seis::RangeSelData::selRes3D( const BinID& bid ) const
{
    if ( isAll() )
	return 0;

    int inlres = tkzs_.hsamp_.start_.inl() > bid.inl() ||
		 tkzs_.hsamp_.stop_.inl() < bid.inl()
		? 2 : 0;
    int crlres = tkzs_.hsamp_.start_.crl() > bid.crl() ||
		 tkzs_.hsamp_.stop_.crl() < bid.crl()
		? 2 : 0;
    int rv = inlres + 256 * crlres;
    if ( rv != 0 ) return rv;

    BinID step( tkzs_.hsamp_.step_.inl(), tkzs_.hsamp_.step_.crl() );
    if ( step.inl() < 1 ) step.inl() = 1;
    if ( step.crl() < 1 ) step.crl() = 1;
    inlres = (bid.inl() - tkzs_.hsamp_.start_.inl()) % step.inl() ? 1 : 0;
    crlres = (bid.crl() - tkzs_.hsamp_.start_.crl()) % step.crl() ? 1 : 0;
    return inlres + 256 * crlres;
}


int Seis::RangeSelData::selRes2D( Pos::GeomID gid, int trcnr ) const
{
    if ( isAll() )
	return 0;

    const int res = SelData::selRes2D( gid, trcnr );
    if ( res != 0 )
	return res;

    return crlRange().includes(trcnr,false) ? 0 : 256;
}


int Seis::RangeSelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    if ( isall_ && !for2d )
	return tracesInSI();

    TrcKeySampling hs( tkzs_.hsamp_ );
    if ( step ) hs.step_ = *step;
    const int nrinl = for2d ? 1 : hs.nrInl();
    const int nrcrl = hs.nrCrl();
    return nrinl * nrcrl;
}


void Seis::RangeSelData::setGeomID( Pos::GeomID gid )
{
    SelData::setGeomID( gid );
    ConstRefMan<Survey::Geometry> geom = Survey::GM().getGeometry( gid );
    if ( tkzs_.hsamp_.getGeomID() == gid )
    {
	testIsAll2D();
	return;
    }

    tkzs_.hsamp_.init( gid );
    tkzs_.zsamp_ = geom ? geom->sampling().zsamp_ : SI().zRange(false);
    if ( geom )
	setIsAll( true );
}


void Seis::RangeSelData::testIsAll2D()
{
    if ( !tkzs_.is2D() )
	return;

    ConstRefMan<Survey::Geometry> geom =
			Survey::GM().getGeometry( tkzs_.hsamp_.getGeomID() );
    if ( !geom )
	return;

    setIsAll( tkzs_ == geom->sampling() );
}


//--- Table ---


Seis::TableSelData::TableSelData()
    : bvs_(*new BinIDValueSet(1,true))
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
    : bvs_(*new BinIDValueSet(1,true))
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


bool Seis::TableSelData::setZRange( const Interval<float>& rg )
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


int Seis::TableSelData::selRes3D( const BinID& bid ) const
{
    if ( isall_ ) return 0;

    const BinIDValueSet::SPos pos( bvs_.find(bid) );
    if ( pos.j >= 0 ) return 0;

    const int inlres = pos.i < 0 ? 2 : 0;
    const int crlres = 1; // Maybe not true, but safe
    return inlres + 256 * crlres;
}


int Seis::TableSelData::selRes2D( Pos::GeomID /* gid */, int /*trcnr */ ) const
{
    //TODO: impl
    return 256+2;
}


int Seis::TableSelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    return mCast( int, isall_ ? tracesInSI() : bvs_.totalSize() );
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
	polys_[0]->add( Geom::Point2D<float>(
				 mCast(float,pt.x), mCast(float,pt.y) ) );
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
	if ( !rsd )
	    { pErrMsg( "Huh" ); }
	ODPolygon<float>* poly = new ODPolygon<float>;
	const TrcKeyZSampling& cs = rsd->cubeSampling();
	poly->add( Geom::Point2D<float>( (float) cs.hsamp_.start_.inl(),
					 (float) cs.hsamp_.start_.crl()) );
	poly->add( Geom::Point2D<float>( (float) cs.hsamp_.stop_.inl(),
					 (float) cs.hsamp_.start_.crl()) );
	poly->add( Geom::Point2D<float>( (float) cs.hsamp_.stop_.inl(),
					 (float) cs.hsamp_.stop_.crl()) );
	poly->add( Geom::Point2D<float>( (float) cs.hsamp_.start_.inl(),
					 (float) cs.hsamp_.stop_.crl()) );
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
    intrg.widen( stepoutreach_.inl() );
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
    intrg.widen( stepoutreach_.crl() );
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
	    Geom::Point2D<float> point(
			mCast(float,inlrg.start + stepoutreach_.inl()),
			mCast(float,crlrg.start + stepoutreach_.crl()) );
	    rect->add( point );
	    point.y = mCast( float, crlrg.stop - stepoutreach_.crl() );
	    rect->add( point );
	    point.x = mCast( float, inlrg.stop - stepoutreach_.inl() );
	    rect->add( point );
	    point.y = mCast( float, crlrg.start + stepoutreach_.crl() );
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


int Seis::PolySelData::selRes3D( const BinID& bid ) const
{
    if ( isall_ ) return 0;

    Interval<float> inlrg( mCast(float,bid.inl()), mCast(float,bid.inl()) );
    inlrg.widen( mCast(float,stepoutreach_.inl()) );
    Interval<float> crlrg( mCast(float,bid.crl()), mCast(float,bid.crl()) );
    crlrg.widen( mCast(float,stepoutreach_.crl()) );

    for ( int idx=0; idx<polys_.size(); idx++ )
    {
	if ( polys_[idx]->windowOverlaps(inlrg, crlrg, 0.5) )
	    return 0;
    }

    const int inlres = inlRange().includes(bid.inl(),true) ? 0 : 2;
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
        const float coverfrac = rectarea
	    ? (float) polys_[idx]->area()/rectarea
	    : 1.0f;

	Interval<int> inlrg( mNINT32(polyinlrg.start), mNINT32(polyinlrg.stop));
	inlrg.widen( stepoutreach_.inl() );
	Interval<int> crlrg( mNINT32(polycrlrg.start), mNINT32(polycrlrg.stop));
	crlrg.widen( stepoutreach_.crl() );

	TrcKeySampling hs;
	hs.set( inlrg, crlrg );
	if ( step ) hs.step_ = *step;
	hs.snapToSurvey();
	estnrtraces += mNINT32( coverfrac * hs.totalNr() );
    }

    return mMIN( estnrtraces, tracesInSI() );
}
