/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kris / Bert
 Date:          2013 / Nov 2018
________________________________________________________________________

-*/

#include "survgeom2d.h"
#include "survgeom3d.h"
#include "survgeommgr.h"

#include "coordsystem.h"
#include "dbkey.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "trckey.h"
#include "trckeyzsampling.h"
#include "task.h"

mUseType( Pos,		GeomID);
mUseType( SurvGeom,	idx_type);
mUseType( SurvGeom,	pos_type);
mUseType( SurvGeom,	idx_type);
mUseType( SurvGeom,	pos_steprg_type);
mUseType( SurvGeom,	z_type);
mUseType( SurvGeom,	z_steprg_type);
mUseType( SurvGeom,	linenr_type);
mUseType( SurvGeom,	trcnr_type);
mUseType( SurvGeom2D,	spnr_type);
mUseType( SurvGeom2D,	size_type);

GeomID Survey::Geometry::cSynthGeomID()
{ return GeomID( (GeomID::IDType)OD::SynthGeom ); }


BufferString nameOf( Pos::GeomID geomid )
{
    return SurvGeom::get( geomid ).name();
}


Pos::GeomID geomIDOf( const DBKey& dbky )
{
    const auto id = dbky.objID();
    return id.isValid() ? Pos::GeomID( id.getI() ) : Pos::GeomID();
}


Survey::Geometry::Geometry( GeomID geomid )
    : geomid_(geomid)
{
}


Survey::Geometry::~Geometry()
{
}


Survey::Geometry& Survey::Geometry::operator =( const Geometry& oth )
{
    if ( this != &oth )
    {
	const_cast<GeomID&>( geomid_ ) = oth.geomid_;
	trcnrrg_ = oth.trcnrrg_;
	zrg_ = oth.zrg_;
    }
    return *this;
}


const Survey::Geometry& Survey::Geometry::get( GeomID gid )
{
    const auto gs = geomSystemOf( gid );
    if ( gs == OD::VolBasedGeom )
       return get3D();
    return get2D( gid );
}


const Survey::Geometry3D& Survey::Geometry::get3D( OD::SurvLimitType slt )
{
    return SI().gt3DGeom( slt );
}


const Survey::Geometry2D& Survey::Geometry::get2D( const char* linenm )
{
    return Geometry2D::get( linenm );
}


const Survey::Geometry2D& Survey::Geometry::get2D( GeomID gid )
{
    return Geometry2D::get( gid );
}


const Survey::Geometry& Survey::Geometry::get( const TrcKey& tk )
{
    return get( tk.geomID() );
}


Pos::GeomID Survey::Geometry::getGeomID( const char* linenm )
{
    return Geometry2D::get( linenm ).geomID();
}


void Survey::Geometry::list2D( GeomIDSet& gids, BufferStringSet* nms )
{
    return GM().list2D( gids, nms );
}


bool Survey::Geometry::isPresent( GeomID gid )
{
    return !gid.is2D() || Geometry2D::isPresent( gid );
}


bool Survey::Geometry::isUsable( GeomID gid )
{
    return !gid.is2D() || !get2D( gid ).isEmpty();
}


Survey::Geometry::dist_type Survey::Geometry::distanceTo(
						const Coord& crd ) const
{
    dist_type dist = mUdf( dist_type );
    if ( is2D() )
	as2D()->nearestTracePosition( crd, &dist );
    else
	as3D()->nearestTracePosition( crd, &dist );
    return dist;
}


void Survey::Geometry::getNearestTracePosition( const Coord& crd, TrcKey& tk,
				     dist_type* dist ) const
{
    if ( is2D() )
    {
	const auto tnr = as2D()->nearestTracePosition( crd, dist );
	tk = TrcKey( as2D()->geomID(), tnr );
    }
    else
    {
	const auto bid = as3D()->nearestTracePosition( crd, dist );
	tk = TrcKey( bid );
    }
}


void Survey::Geometry::getTracePosition( const Coord& crd, TrcKey& tk,
					 dist_type maxdist ) const
{
    if ( is2D() )
    {
	const auto tnr = as2D()->tracePosition( crd, maxdist );
	tk = TrcKey( as2D()->geomID(), tnr );
    }
    else
    {
	const auto bid = as3D()->tracePosition( crd, maxdist );
	tk = TrcKey( bid );
    }
}


Survey::Geometry::dist_type Survey::Geometry::averageTrcDist() const
{
    return is2D() ? as2D()->averageTrcDist() : as3D()->averageTrcDist();
}


void Survey::Geometry::snapPos( pos_type& pos, const pos_steprg_type& rg,
				SnapDir sd )
{
    if ( rg.step > 1 )
	pos = rg.snapAndLimit( pos, sd );
}


void Survey::Geometry::snapStep( pos_type& pos, const pos_steprg_type& rg )
{
    rg.snapStep( pos );
}


void Survey::Geometry::snapTrcNr( pos_type& pos, SnapDir sd ) const
{
    snapPos( pos, trcnrrg_, sd );
}


void Survey::Geometry::snapTrcNrStep( pos_type& pos ) const
{
    snapStep( pos, trcnrrg_ );
}


void Survey::Geometry::snapZ( z_type& z, SnapDir dir ) const
{
    z = zrg_.snapAndLimit( z, dir );
}


void Survey::Geometry::snapZStep( z_type& step ) const
{
    step = zrg_.snapStep( step );
}


bool Survey::Geometry::includes( const TrcKey& tk )
{
    return tk.is3D() ? get3D().includes( tk.binID() )
		     : get2D( tk.geomID() ).includes( tk.trcNr() );
}


Coord Survey::Geometry::toCoord( GeomSystem gs,
				 linenr_type lnr, trcnr_type tnr )
{
    switch ( gs )
    {
	case OD::VolBasedGeom:
	    return get3D().transform( BinID(lnr,tnr) );
	case OD::LineBasedGeom:
	    return get2D( GeomID(lnr) ).getCoord( tnr );
	default:
	    break;
    }
    return Coord::udf();
}


Coord Survey::Geometry::toCoord( const TrcKey& tk )
{
    return tk.getCoord();
}


Survey::Geometry::RelationType Survey::Geometry::compare(
				const Geometry& oth, bool usezrg ) const
{
    if ( oth.geomSystem() != geomSystem() )
	return UnRelated;
    return is2D() ? as2D()->compare( *oth.as2D(), usezrg )
		  : as3D()->compare( *oth.as3D(), usezrg );
}


bool Survey::Geometry::isCompatibleWith( const Geometry& oth ) const
{
    return compare( oth, false ) != UnRelated;
}


z_type Survey::Geometry::zScale() const
{
    return SI().zScale();
}



//-- 3D

Survey::Geometry3D::Geometry3D( const char* nm )
    : Geometry(GeomID::get3D())
    , name_(nm)
    , inlrg_(0,0,1)
{
    trcnrrg_ = pos_steprg_type( 0, 0, 1 );
    zrg_ = z_steprg_type( 0, 0, 1.f );
}


Survey::Geometry3D::Geometry3D( const Geometry3D& oth )
    : Geometry(oth)
    , name_(oth.name_)
    , inlrg_(oth.inlrg_)
    , b2c_(oth.b2c_)
{
}


bool Survey::Geometry3D::includes( const BinID& bid ) const
{
    return inlrg_.isPresent( bid.inl() ) && trcnrrg_.isPresent( bid.crl() );
}


Coord Survey::Geometry3D::getCoord( const BinID& bid ) const
{
    return transform( bid );
}


BinID Survey::Geometry3D::nearestTracePosition( const Coord& crd,
						dist_type* dist ) const
{
    auto bid( transform(crd) );
    snap( bid );
    if ( dist )
    {
	Coord bidpos( transform(bid) );
	*dist = bidpos.distTo<dist_type>( crd );
    }
    return bid;
}


BinID Survey::Geometry3D::tracePosition( const Coord& crd,
					 dist_type maxdist ) const
{
    BinID ret = transform( crd );
    if ( !mIsUdf(maxdist) )
    {
	auto dist = crd.distTo<dist_type>( transform(ret) );
	if ( dist > maxdist )
	    ret.setUdf();
    }
    return ret;
}


BinID Survey::Geometry3D::transform( const Coord& c ) const
{
    return BinID( b2c_.transformBack( c, BinID(inlrg_.start,trcnrrg_.start),
					 BinID(inlrg_.step,trcnrrg_.step) ) );
}


Coord Survey::Geometry3D::transform( const BinID& b ) const
{
    return b2c_.transform( b );
}


BinID Survey::Geometry3D::origin() const
{
    return BinID( inlrg_.start, trcnrrg_.start );
}


bool Survey::Geometry3D::isRightHandSystem() const
{
    const auto& xtransf = b2c_.getTransform( true );
    const auto& ytransf = b2c_.getTransform( false );
    const auto det = xtransf.b * ytransf.c - xtransf.c * ytransf.b;
    return det < 0;
}


void Survey::Geometry3D::snap( BinID& binid, SnapDir snapdir ) const
{
    snapPos( binid.inl(), inlRange(), snapdir );
    snapTrcNr( binid.crl(), snapdir );
}


void Survey::Geometry3D::snapStep( BinID& stp ) const
{
    Geometry::snapStep( stp.inl(), inlRange() );
    Geometry::snapStep( stp.crl(), crlRange() );
}


Survey::Geometry::dist_type Survey::Geometry3D::inlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(1,0) );
    return c00.distTo<dist_type>( c10 );
}


Survey::Geometry::dist_type Survey::Geometry3D::crlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c01 = transform( BinID(0,1) );
    return c00.distTo<dist_type>( c01 );
}


Survey::Geometry::dist_type Survey::Geometry3D::averageTrcDist() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(inlrg_.step,0) );
    const Coord c01 = transform( BinID(0,trcnrrg_.step) );
    return ( c00.distTo<dist_type>(c10) + c00.distTo<dist_type>(c01) ) * 0.5;
}


void Survey::Geometry3D::getMapInfo( const IOPar& iop )
{
    b2c_.usePar( iop );
    auto& inlrg = inlRange();
    auto& crlrg = crlRange();
    iop.get( sKey::FirstInl(), inlrg.start );
    iop.get( sKey::FirstCrl(), crlrg.start );
    iop.get( sKey::StepInl(), inlrg.step );
    iop.get( sKey::StepCrl(), crlrg.step );
    iop.get( sKey::LastInl(), inlrg.stop );
    iop.get( sKey::LastCrl(), crlrg.stop );
}


void Survey::Geometry3D::putMapInfo( IOPar& iop ) const
{
    b2c_.fillPar( iop );
    SI().getCoordSystem()->fillPar( iop );
    const auto& inlrg = inlRange();
    const auto& crlrg = crlRange();
    iop.set( sKey::FirstInl(), inlrg.start );
    iop.set( sKey::FirstCrl(), crlrg.start );
    iop.set( sKey::StepInl(), inlrg.step );
    iop.set( sKey::StepCrl(), crlrg.step );
    iop.set( sKey::LastInl(), inlrg.stop );
    iop.set( sKey::LastCrl(), crlrg.stop );
}


void Survey::Geometry3D::setTransform( const Pos::IdxPair2Coord& b2c )
{
    b2c_ = b2c;
}


void Survey::Geometry3D::setRanges( const pos_steprg_type& inlrg,
		const pos_steprg_type& crlrg, const z_steprg_type& zrg )
{
    inlrg_ = inlrg;
    trcnrrg_ = crlrg;
    zrg_ = zrg;
}


Coord3 Survey::Geometry3D::oneStepTranslation( const Coord3& planenormal ) const
{
    Coord3 translation( 0, 0, 0 );

    if ( fabs(planenormal.z_) > 0.5 )
	translation.z_ = zrg_.step;
    else
    {
	Coord norm2d = Coord(planenormal.x_,planenormal.y_);
	norm2d.normalize();

	if ( fabs(norm2d.dot(b2c_.inlDir())) > 0.5 )
	    translation.x_ = inlDistance();
	else
	    translation.y_ = crlDistance();
    }

    return translation;
}


static bool rendersSamePositions( const Survey::Geometry3D& sg1,
	const Survey::Geometry3D& sg2, const BinID& bid )
{
    const Coord coord1( sg1.transform(bid) );
    const Coord coord2( sg2.transform(bid) );
    return coord1.sqDistTo(coord2) < 1;
}


Survey::Geometry::RelationType Survey::Geometry3D::compare(
				const Geometry3D& oth, bool usezrg ) const
{
    if ( !(b2c_ == oth.b2c_) // this check is rather lenient
      || !rendersSamePositions( *this, oth, BinID(inlrg_.start,trcnrrg_.start) )
      || !rendersSamePositions( *this, oth, BinID(inlrg_.start,trcnrrg_.stop) )
      || !rendersSamePositions( *this, oth, BinID(inlrg_.stop,trcnrrg_.start) )
      || !rendersSamePositions( *this, oth, BinID(inlrg_.stop,trcnrrg_.stop) ) )
	return UnRelated;

    const auto myinlrg = inlRange();
    const auto mycrlrg = crlRange();
    const auto myzrg = zRange();
    const auto othinlrg = oth.inlRange();
    const auto othcrlrg = oth.crlRange();
    const auto othzrg = oth.zRange();
    if ( myinlrg == othinlrg && mycrlrg == othcrlrg &&
	    (!usezrg || myzrg.isEqual(othzrg,1e-3)) )
	return Identical;
    if ( myinlrg.includes(othinlrg) && mycrlrg.includes(othcrlrg) &&
	    (!usezrg || myzrg.includes(othzrg)) )
	return SuperSet;
    if ( othinlrg.includes(myinlrg) && othcrlrg.includes(mycrlrg) &&
	    (!usezrg || othzrg.includes(myzrg)) )
	return SubSet;

    return Related;
}


//-- 2D

#define mL2DPos(idx) data_.positions().get( idx )


Survey::Geometry2D::Geometry2D()
    : Geometry(GeomID())
    , data_(*new Line2DData)
    , objectChanged(this)
{
    trcnrrg_.start = trcnrrg_.stop = 0; trcnrrg_.step = 1;
    zrg_ = SI().zRange();
}



Survey::Geometry2D::Geometry2D( const char* lnm )
    : Geometry(GeomID())
    , data_(*new Line2DData(lnm))
    , objectChanged(this)
{
    setFromLineData();
    zrg_ = SI().zRange();
}


Survey::Geometry2D::Geometry2D( Line2DData* l2d )
    : Geometry(GeomID())
    , data_(l2d ? *l2d : *new Line2DData)
    , objectChanged(this)
{
    setFromLineData();
}


Survey::Geometry2D::Geometry2D( const Geometry2D& oth )
    : Geometry(oth)
    , data_(*new Line2DData(oth.data_))
    , spnrs_(oth.spnrs_)
    , avgtrcdist_(oth.avgtrcdist_)
    , linelength_(oth.linelength_)
    , objectChanged(this)
{
}


Survey::Geometry2D::~Geometry2D()
{
    delete &data_;
}


bool Survey::Geometry2D::isEmpty() const
{
    return data_.isEmpty();
}


bool Survey::Geometry2D::isPresent( GeomID geomid )
{
    return GM().get2DGeometry( geomid );
}


void Survey::Geometry2D::getGeomIDs( GeomIDSet& gids )
{
    GM().list2D( gids );
}


const Survey::Geometry2D& Survey::Geometry2D::get( GeomID geomid )
{
    const auto* g2d = GM().get2DGeometry( geomid );
    return g2d ? *g2d : dummy();
}


const Survey::Geometry2D& Survey::Geometry2D::get( const char* linenm )
{
    const auto* g2d = GM().get2DGeometry( linenm );
    return g2d ? *g2d : dummy();
}


void Survey::Geometry2D::setFromLineData()
{
    trcnrrg_ = data_.trcNrRange();
    zrg_ = data_.zRange();
    spnrs_.setSize( data_.size(), mUdf(spnr_type) );

    const auto sz = size();
    if ( sz < 1 )
	avgtrcdist_ = linelength_ = mUdf(dist_type);
    else
    {
	linelength_ = 0; size_type nrpos = 0;
	for ( int idx=1; idx<data_.positions().size(); idx++ )
	{
	    const auto dist = mL2DPos(idx).coord_.distTo<dist_type>(
			      mL2DPos(idx-1).coord_ );
	    linelength_ += dist;
	    if ( !mIsZero(dist,0.001) )
		nrpos++;
	}
	avgtrcdist_ = linelength_;
	if ( nrpos > 1 )
	    avgtrcdist_ /= nrpos;
    }
}


const OD::String& Survey::Geometry2D::name() const
{
    return data_.lineName();
}


size_type Survey::Geometry2D::size() const
{
    return data_.size();
}


idx_type Survey::Geometry2D::indexOf( trcnr_type trcnr ) const
{
    return data_.indexOf( trcnr );
}


trcnr_type Survey::Geometry2D::trcNr( idx_type idx ) const
{
    return data_.validIdx( idx ) ? mL2DPos(idx).nr_ : mUdf(trcnr_type);
}


Bin2D Survey::Geometry2D::bin2D( idx_type idx ) const
{
    return Bin2D( geomID(), trcNr(idx) );
}


bool Survey::Geometry2D::includes( trcnr_type trcnr ) const
{
    return indexOf(trcnr) >= 0;
}


bool Survey::Geometry2D::includes( const Bin2D& b2d ) const
{
    return b2d.geomID() == geomID() && includes( b2d.trcNr() );
}


Coord Survey::Geometry2D::getCoord( trcnr_type trcnr ) const
{
    return getCoordByIdx( indexOf(trcnr) );
}


Coord Survey::Geometry2D::getCoordByIdx( idx_type idx ) const
{
    return idx < 0 ? Coord::udf() : mL2DPos( idx ).coord_;
}


spnr_type Survey::Geometry2D::getSPNr( trcnr_type trcnr ) const
{
    const auto idx = indexOf( trcnr );
    return idx < 0 ? mUdf(spnr_type) : spnrs_.get( idx );
}


bool Survey::Geometry2D::findSP( spnr_type reqspnr, trcnr_type& trcnr ) const
{
    static const spnr_type eps = (spnr_type)0.001;

    int posidx = -1;
    for ( int idx=0; idx<spnrs_.size(); idx++ )
    {
	const auto spnr = spnrs_[idx];
	if ( mIsEqual(spnr,reqspnr,eps) )
	    { posidx = idx; break; }
    }
    if ( posidx < 0 )
	return false;

    trcnr = mL2DPos( posidx ).nr_;
    return true;
}


void Survey::Geometry2D::getInfo( trcnr_type trcnr, Coord& crd,
				  spnr_type& spnr ) const
{
    const auto idx = indexOf( trcnr );
    if ( idx < 0 )
	{ crd.setUdf(); mSetUdf(spnr); }
    else
	{ crd = mL2DPos( idx ).coord_; spnr = spnrs_.get( idx ); }
}


trcnr_type Survey::Geometry2D::nearestTracePosition( const Coord& crd,
							dist_type* dist ) const
{
    PosInfo::Line2DPos pos;
    return data_.getPos(crd,pos,dist) ? pos.nr_ : mUdf(trcnr_type);
}


trcnr_type Survey::Geometry2D::tracePosition( const Coord& crd,
					    dist_type maxdist ) const
{
    PosInfo::Line2DPos pos;
    return data_.getPos(crd,pos,maxdist) ? pos.nr_ : mUdf(trcnr_type);
}


Bin2D Survey::Geometry2D::findNearestTrace( const Coord& coord,
					    dist_type* dist )
{
    GeomIDSet geomids;
    list2D( geomids );

    Bin2D best_b2d; dist_type best_dist = mUdf(dist_type);
    for ( auto geomid : geomids )
    {
	dist_type linedist = mUdf(dist_type);
	const auto tnr = get(geomid).nearestTracePosition( coord, &linedist );
	if ( linedist < best_dist )
	{
	    best_b2d = Bin2D( geomid, tnr );
	    best_dist = linedist;
	}

    }

    if ( dist )
	*dist = best_dist;
    return best_b2d;
}


void Survey::Geometry2D::getSampling( TrcKeyZSampling& tkzs ) const
{
    auto& hs = tkzs.hsamp_;
    hs.setIs2D();
    hs.start_.inl() = hs.stop_.inl() = geomID().getI();
    hs.step_.inl() = 1;
    hs.start_.crl() = trcNrRange().start;
    hs.stop_.crl() = trcNrRange().stop;
    hs.step_.crl() = trcNrRange().step;
    tkzs.zsamp_ = zRange();
}


void Survey::Geometry2D::setEmpty() const
{
    auto& self = *const_cast<Geometry2D*>( this );
    self.data_.setEmpty();
    self.spnrs_.erase();
    self.setFromLineData();
}


void Survey::Geometry2D::setName( const char* nm )
{
    data_.setLineName( nm );
}


void Survey::Geometry2D::add( const Coord& crd, int trcnr, spnr_type spnr )
{
    PosInfo::Line2DPos pos( trcnr );
    pos.coord_ = crd;
    data_.add( pos );
    spnrs_ += spnr;
}


void Survey::Geometry2D::commitChanges() const
{
    auto& self = *const_cast<Geometry2D*>( this );
    self.setFromLineData();
    self.objectChanged.trigger();
}


Survey::Geometry::RelationType Survey::Geometry2D::compare(
				const Geometry2D& geom, bool usezrg ) const
{
    const Line2DData& mydata = data();
    const Line2DData& otherdata = geom.data();
    if ( !mydata.coincidesWith(otherdata) )
	return UnRelated;

    const auto mytrcrg = mydata.trcNrRange();
    const auto othtrcrg = otherdata.trcNrRange();
    const auto myzrg = mydata.zRange();
    const auto othzrg = otherdata.zRange();
    if ( mytrcrg == othtrcrg && (!usezrg || myzrg.isEqual(othzrg,1e-3)) )
	return Identical;
    if ( mytrcrg.includes(othtrcrg) && (!usezrg || myzrg.includes(othzrg)) )
	return SuperSet;
    if ( othtrcrg.includes(mytrcrg) && (!usezrg || othzrg.includes(myzrg)) )
	return SubSet;

    return Related;
}


Survey::Geometry2D& Survey::Geometry2D::dummy()
{
    static Geometry2D ret;
    return ret;
}
