/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kris
 Date:          2013
________________________________________________________________________

-*/

#include "survgeom2d.h"
#include "survgeom3d.h"
#include "posinfo2dsurv.h"
#include "coordsystem.h"

#include "keystrs.h"
#include "dbkey.h"
#include "survinfo.h"
#include "task.h"

static Pos::GeomID cSIGeomID = -99;
static Pos::GeomID cSyntheticSurveyID = -100;

mImplClassFactory(Survey::GeometryReader,factory);
mImplClassFactory(Survey::GeometryWriter,factory);
const TrcKey::SurvID Survey::GeometryManager::surv2did_ = 0;

static PtrMan<Survey::GeometryManager> theinst = 0;

const Survey::GeometryManager& Survey::GM()
{
    return *theinst.createIfNull();
}


Survey::Geometry::Geometry()
    : id_(mUdfGeomID)
    , sampling_(false)
{
    trcnrrg_ = sampling_.hsamp_.trcRange();
    zrg_ = sampling_.zsamp_;
}


Survey::Geometry::~Geometry()
{}


const Survey::Geometry& Survey::Geometry::default3D()
{
    return *GM().getGeometry( GM().default3DSurvID() );
}


bool Survey::Geometry::isCompatibleWith( const Geometry& geom ) const
{
    return compare( geom, false ) != UnRelated;
}


TrcKey Survey::Geometry::getTrace( const Coord& crd, float maxdist ) const
{
    float dist;
    const TrcKey tk = nearestTrace( crd,  &dist );
    return tk.isUdf() || dist>maxdist ? TrcKey::udf() : tk;
}


bool Survey::Geometry::includes( const TrcKey& tk ) const
{
    return tk.geomID() == id()
	&& includes( tk.lineNr(), tk.trcNr() );
}


Coord Survey::Geometry::toCoord( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( tk.geomID() );
    return geom ? geom->toCoord( tk.binID() ) : Coord::udf();
}


bool Survey::Geometry::exists( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( tk.geomID() );
    return geom && geom->includes( tk.binID() );
}


Pos::SurvID Survey::Geometry::getSurvID() const
{
    return is2D() ? Survey::GeometryManager::get2DSurvID()
		  : (Pos::SurvID)id();
}


#define mGetConstGeom(varnm,geomid) \
    ConstRefMan<Geometry> varnm = GM().getGeometry( geomid );


Survey::GeometryManager::GeometryManager()
    : hasduplnms_(false)
{}

Survey::GeometryManager::~GeometryManager()
{ deepUnRef( geometries_ ); }

int Survey::GeometryManager::nrGeometries() const
{ return geometries_.size(); }


TrcKey::SurvID Survey::GeometryManager::default3DSurvID() const
{
    return cSIGeomID;
}


TrcKey::SurvID Survey::GeometryManager::synthSurvID() const
{
    return cSyntheticSurveyID;
}


const Survey::Geometry* Survey::GeometryManager::getGeometry(
						Geometry::ID geomid ) const
{
    return const_cast<GeometryManager*>( this )->getGeometry( geomid );
}


Survey::Geometry* Survey::GeometryManager::getGeometry( Geometry::ID geomid )
{
    if ( geomid == cSIGeomID )
	return &SI().gt3DGeom();
    else if ( mIsUdfGeomID(geomid) )
	return 0;

    const int idx = indexOf( geomid );
    if ( idx >= 0 )
	return geometries_[idx];

    pErrMsg( "Geometry not present" );
    return 0;
}


const Survey::Geometry3D* Survey::GeometryManager::getGeometry3D(
							Pos::SurvID sid ) const
{
    const TrcKey tk( sid, BinID(0,0) );
    const Geometry* geom = getGeometry( tk.geomID() );
    return geom ? geom->as3D() : 0;
}


const Survey::Geometry* Survey::GeometryManager::getGeometry(
						const DBKey& dbky ) const
{
    if ( dbky.hasValidObjID() )
	return getGeometry( dbky.objID().getI() );

    return 0;
}


const Survey::Geometry* Survey::GeometryManager::getGeometry(
						const char* nm ) const
{
    const auto* sigeom = SI().s3dgeom_;
    if ( sigeom && sigeom->hasName(nm) )
	return sigeom;

    for ( const auto* geom : geometries_ )
	if ( geom->hasName(nm) )
	    return geom;

    return 0;
}


Survey::Geometry::ID Survey::GeometryManager::getGeomID(
						const char* lnnm ) const
{
    for ( const auto* geom : geometries_ )
	if ( geom->is2D() && geom->hasName(lnnm) )
	    return geom->id();

    return mUdfGeomID;
}


Survey::Geometry::ID Survey::GeometryManager::getGeomID( const char* lsnm,
						     const char* lnnm ) const
{
    if ( !hasduplnms_ )
	return getGeomID( lnnm );

    BufferString newlnm = lsnm;
    newlnm.add( "-" );
    newlnm.add( lnnm );
    return getGeomID( newlnm.buf() );
}


const char* Survey::GeometryManager::getName( Geometry::ID geomid ) const
{
    mGetConstGeom(geom,geomid);
    return geom ? geom->name().str() : 0;
}


bool Survey::GeometryManager::is2D( Geometry::ID geomid ) const
{
    mGetConstGeom(geom,geomid);
    return geom ? geom->is2D() : false;
}


Coord Survey::GeometryManager::toCoord( const TrcKey& tk ) const
{
    mGetConstGeom(geom,tk.geomID());
    return geom ? geom->toCoord( tk.lineNr(), tk.trcNr() ) : Coord::udf();
}


TrcKey Survey::GeometryManager::traceKey( Geometry::ID geomid, Pos::LineID lid,
					  Pos::TraceID tid ) const
{
    mGetConstGeom(geom,geomid);
    if ( !geom )
	return TrcKey::udf();

    if ( geom->is2D() )
	return TrcKey( geomid,	tid );

    return TrcKey( geomid, BinID(lid, tid) );
}


TrcKey Survey::GeometryManager::traceKey( Geometry::ID geomid,
					  Pos::TraceID tid ) const
{
    mGetConstGeom(geom,geomid);
    if ( !geom || !geom->is2D() )
	return TrcKey::udf();

    return TrcKey( geomid,  tid );
}


void Survey::GeometryManager::addGeometry( Geometry& geom )
{
    geom.ref();
    geometries_ += &geom;
}


bool Survey::GeometryManager::fetchFrom2DGeom( uiString& errmsg )
{
    fillGeometries(0);
    PtrMan<GeometryWriter> geomwriter =
		GeometryWriter::factory().create( sKey::TwoD() );
    BufferStringSet lsnames;
    S2DPOS().getLineSets( lsnames );
    bool fetchedgeometry = false;
    for ( int lsidx=0; lsidx<lsnames.size(); lsidx++ )
    {
	BufferStringSet lnames;
	S2DPOS().getLines( lnames, lsnames.get(lsidx).buf() );
	S2DPOS().setCurLineSet( lsnames.get(lsidx).buf() );
	for ( int lidx=0; lidx<lnames.size(); lidx++ )
	{
	    Pos::GeomID geomid = GM().getGeomID( lsnames.get(lsidx),
						 lnames.get(lidx) );
	    if ( !mIsUdfGeomID(geomid) )
		continue;

	    fetchedgeometry = true;
	    PosInfo::Line2DData* data = new PosInfo::Line2DData;
	    data->setLineName( lnames.get(lidx) );
	    if ( !S2DPOS().getGeometry( *data ) )
	    {
		delete data;
		continue;
	    }

	    if ( hasduplnms_ )
	    {
		BufferString newlnm = lsnames.get( lsidx );
		newlnm.add( "-" );
		newlnm.add( lnames.get(lidx) );
		data->setLineName( newlnm );
	    }

	    RefMan<Geometry2D> geom2d = new Geometry2D( data );
	    uiString errormsg;
	    PosInfo::Line2DKey l2dkey =
		S2DPOS().getLine2DKey( lsnames.get(lsidx), lnames.get(lidx) );
	    const char* crfromstr = l2dkey.toString();
	    if ( !geomwriter->write(*geom2d,errormsg,crfromstr) )
	    {
		errmsg = tr(
		    "Unable to convert 2D geometries to OD5.0 format.\n%1").
								arg( errormsg );
		return false;
	    }
	}
    }

    if ( fetchedgeometry )
	fillGeometries(0);

    return true;
}


bool Survey::GeometryManager::hasDuplicateLineNames()
{
    BufferStringSet lsnames;
    S2DPOS().getLineSets( lsnames );
    BufferStringSet linenames;
    for ( int lsidx=0; lsidx<lsnames.size(); lsidx++ )
    {
	BufferStringSet lnames;
	S2DPOS().getLines( lnames, lsnames.get(lsidx).buf() );
	for ( int totalidx=0; totalidx<linenames.size(); totalidx++ )
	{
	    for ( int lidx=0; lidx<lnames.size(); lidx++ )
	    {
		if ( lnames.get(lidx) == linenames.get(totalidx) )
		    return true;
	    }
	}

	linenames.add( lnames, false );
    }

    return false;
}


bool Survey::GeometryManager::write( Geometry& geom, uiString& errmsg )
{
    if ( geom.is2D() )
    {
	PtrMan<GeometryWriter> geomwriter =GeometryWriter::factory()
						    .create( sKey::TwoD() );
	geom.ref();
	if ( !geomwriter->write(geom,errmsg) )
	{
	    geom.unRef();
	    return false;
	}

	if ( indexOf(geom.id()) < 0 )
	    addGeometry( geom );
	geom.unRef();

	return true;
    }
    else
	return false;
}


Survey::Geometry::ID Survey::GeometryManager::addNewEntry( Geometry* geom,
							   uiString& errmsg )
{
    if ( !geom )
	return cUndefGeomID();
    if ( !geom->is2D() )
	return default3DSurvID();
    Geometry::ID geomid = getGeomID( geom->name() );
    if ( geomid!=cUndefGeomID() )
	return geomid;
    PtrMan<GeometryWriter> geomwriter =
	GeometryWriter::factory().create( sKey::TwoD() );

    Threads::Locker locker( lock_ );
    geomid = geomwriter->createNewGeomID( geom->name() );
    if ( !write(*geom,errmsg) )
	return cUndefGeomID();
    return geomid;
}


bool Survey::GeometryManager::removeGeometry( Geometry::ID geomid )
{
    const int index = indexOf( geomid );
    if ( geometries_.validIdx(index) )
    {
	Geometry* geom = geometries_[ index ];
	if ( !geom )
	    return false;

	geometries_.removeSingle( index );
	geom->unRef();
	return true;
    }

    return false;
}


int Survey::GeometryManager::indexOf( Geometry::ID geomid ) const
{
    if ( geomid == mUdfGeomID )
	return -1;

    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( geometries_[idx]->id() == geomid )
	    return idx;

    return -1;
}


bool Survey::GeometryManager::fillGeometries( TaskRunner* taskrunner )
{
    Threads::Locker locker( lock_ );
    deepUnRef( geometries_ );
    hasduplnms_ = hasDuplicateLineNames();
    PtrMan<GeometryReader> geomreader = GeometryReader::factory()
					.create(sKey::TwoD());
    return geomreader ? geomreader->read( geometries_, taskrunner ) : false;
}


bool Survey::GeometryManager::updateGeometries( TaskRunner* taskrunner )
{
    Threads::Locker locker( lock_ );
    PtrMan<GeometryReader> geomreader = GeometryReader::factory()
					.create(sKey::TwoD());
    return geomreader ? geomreader->updateGeometries( geometries_, taskrunner )
		      : false;
}


bool Survey::GeometryManager::getList( BufferStringSet& names,
			   TypeSet<Geometry::ID>& geomids, bool is2d ) const
{
    names.erase();
    geomids.erase();
    for ( const auto* geom : geometries_ )
    {
	if ( geom->is2D() == is2d )
	{
	    names.add( geom->name() );
	    geomids += geom->id();
	}
    }

    return true;
}


Survey::Geometry::ID Survey::GeometryManager::findRelated( const Geometry& ref,
					   Geometry::RelationType& reltype,
					   bool usezrg ) const
{
    int identicalidx=-1, supersetidx=-1, subsetidx=-1, relatedidx=-1;
    for ( int idx=0; identicalidx<0 && idx<geometries_.size(); idx++ )
    {
	Geometry::RelationType rt = geometries_[idx]->compare( ref, usezrg );
	switch( rt )
	{
	    case Geometry::Identical:	identicalidx = idx; break;
	    case Geometry::SuperSet:	supersetidx = idx;  break;
	    case Geometry::SubSet:	subsetidx = idx;    break;
	    case Geometry::Related:	relatedidx = idx;   break;
	    default: break;
	}
    }

    if ( identicalidx >= 0 )
    { reltype = Geometry::Identical; return geometries_[identicalidx]->id();}
    if ( supersetidx >= 0 )
    { reltype = Geometry::SuperSet; return geometries_[supersetidx]->id(); }
    if ( subsetidx >= 0 )
    { reltype = Geometry::SubSet; return geometries_[subsetidx]->id(); }
    if ( relatedidx >= 0 )
    { reltype = Geometry::Related; return geometries_[relatedidx]->id(); }

    reltype = Geometry::UnRelated; return mUdfGeomID;
}


Coord Survey::Geometry3D::toCoord( int linenr, int tracenr ) const
{
    return transform( BinID(linenr,tracenr) );
}


TrcKey Survey::Geometry3D::nearestTrace( const Coord& crd, float* dist ) const
{
    const auto bid( nearestTracePosition(crd,dist) );
    return TrcKey( getSurvID(), bid );
}


BinID Survey::Geometry3D::nearestTracePosition( const Coord& crd,
						float* dist ) const
{
    const BinID bid( transform(crd) );
    if ( dist )
    {
	if ( sampling_.hsamp_.includes(bid) )
	{
	    const Coord projcoord( transform(bid) );
	    *dist = projcoord.distTo<float>( crd );
	}
	else
	{
	    const Coord nearcoord( transform(sampling_.hsamp_.getNearest(bid)));
	    *dist = (float)nearcoord.distTo<float>( crd );
	}
    }
    return bid;
}


bool Survey::Geometry3D::includes( int line, int tracenr ) const
{
    return sampling_.hsamp_.includes( BinID(line,tracenr) );
}


bool Survey::Geometry3D::isRightHandSystem() const
{
    const double xinl = b2c_.getTransform(true).b;
    const double xcrl = b2c_.getTransform(true).c;
    const double yinl = b2c_.getTransform(false).b;
    const double ycrl = b2c_.getTransform(false).c;

    const double det = xinl*ycrl - xcrl*yinl;
    return det < 0;
}


float Survey::Geometry3D::averageTrcDist() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(sampling_.hsamp_.step_.inl(),0) );
    const Coord c01 = transform( BinID(0,sampling_.hsamp_.step_.crl()) );
    return ( c00.distTo<float>(c10) + c00.distTo<float>(c01) )/2;
}


BinID Survey::Geometry3D::transform( const Coord& c ) const
{
    return BinID( b2c_.transformBack( c, sampling_.hsamp_.start_,
				      sampling_.hsamp_.step_ ) );
}


Coord Survey::Geometry3D::transform( const BinID& b ) const
{
    return b2c_.transform( b );
}


Survey::Geometry3D::Geometry3D()
    : zdomain_( ZDomain::SI() )
{
    sampling_.hsamp_.survid_ = id();
    inlrg_ = sampling_.hsamp_.lineRange();
}


Survey::Geometry3D::Geometry3D( const char* nm, const ZDomain::Def& zd )
    : name_( nm )
    , zdomain_( zd )
{
    sampling_.hsamp_.survid_ = id();
    inlrg_ = sampling_.hsamp_.lineRange();
}


static void doSnap( int& idx, int start, int step, int dir )
{
    if ( step < 2 )
	return;
    int rel = idx - start;
    int rest = rel % step;
    if ( !rest )
	return;

    idx -= rest;

    if ( !dir )
	dir = rest > step / 2 ? 1 : -1;
    if ( rel > 0 && dir > 0 )
	idx += step;
    else if ( rel < 0 && dir < 0 )
	idx -= step;
}


void Survey::Geometry3D::snap( BinID& binid, const BinID& rounding ) const
{
    const BinID stp = sampling_.hsamp_.step_;
    if ( stp.inl() == 1 && stp.crl() == 1 )
	return;
    doSnap( binid.inl(), sampling_.hsamp_.start_.inl(), stp.inl(),
	    rounding.inl() );
    doSnap( binid.crl(), sampling_.hsamp_.start_.crl(), stp.crl(),
	    rounding.crl() );
}

#define mSnapStep(ic) \
    rest = s.ic % stp.ic; \
    if ( rest ) \
    { \
	int hstep = stp.ic / 2; \
	bool upw = rounding.ic > 0 || (rounding.ic == 0 && rest > hstep); \
	s.ic -= rest; \
	if ( upw ) s.ic += stp.ic; \
    }

void Survey::Geometry3D::snapStep( BinID& s, const BinID& rounding ) const
{
    const BinID stp = sampling_.hsamp_.step_;
    if ( s.inl() < 0 )
	s.inl() = -s.inl();
    if ( s.crl() < 0 )
	s.crl() = -s.crl();
    if ( s.inl() < stp.inl() )
	s.inl() = stp.inl();
    if ( s.crl() < stp.crl() )
	s.crl() = stp.crl();
    if ( s == stp || (stp.inl() == 1 && stp.crl() == 1) )
	return;

    int rest;

    mSnapStep(inl())
    mSnapStep(crl())
}


void Survey::Geometry3D::snapZ( z_type& z, int dir ) const
{
    const StepInterval<z_type> zrg = sampling_.zsamp_;
    const z_type eps = 1e-8;

    if ( z < zrg.start + eps )
    { z = zrg.start; return; }
    if ( z > zrg.stop - eps )
    { z = zrg.stop; return; }

    const z_type relidx = zrg.getfIndex( z );
    int targetidx = mNINT32(relidx);
    const z_type zdiff = z - zrg.atIndex( targetidx );
    if ( !mIsZero(zdiff,eps) && dir )
	targetidx = (int)( dir < 0 ? Math::Floor(relidx) : Math::Ceil(relidx) );
    z = zrg.atIndex( targetidx );;
    if ( z > zrg.stop - eps )
	z = zrg.stop;
}


void Survey::Geometry3D::putMapInfo( IOPar& iop ) const
{
    b2c_.fillPar( iop );
    SI().getCoordSystem()->fillPar( iop );
    iop.set( sKey::FirstInl(), sampling_.hsamp_.start_.inl() );
    iop.set( sKey::FirstCrl(), sampling_.hsamp_.start_.crl() );
    iop.set( sKey::StepInl(), sampling_.hsamp_.step_.inl() );
    iop.set( sKey::StepCrl(), sampling_.hsamp_.step_.crl() );
    iop.set( sKey::LastInl(), sampling_.hsamp_.stop_.inl() );
    iop.set( sKey::LastCrl(), sampling_.hsamp_.stop_.crl() );
}


void Survey::Geometry3D::getMapInfo( const IOPar& iop )
{
    b2c_.usePar( iop );
    iop.get( sKey::FirstInl(), sampling_.hsamp_.start_.inl() );
    iop.get( sKey::FirstCrl(), sampling_.hsamp_.start_.crl() );
    iop.get( sKey::StepInl(), sampling_.hsamp_.step_.inl() );
    iop.get( sKey::StepCrl(), sampling_.hsamp_.step_.crl() );
    iop.get( sKey::LastInl(), sampling_.hsamp_.stop_.inl() );
    iop.get( sKey::LastCrl(), sampling_.hsamp_.stop_.crl() );
    trcnrrg_ = sampling_.hsamp_.trcRange();
    inlrg_ = sampling_.hsamp_.lineRange();
}


void Survey::Geometry3D::setGeomData( const Pos::IdxPair2Coord& b2c,
				const TrcKeyZSampling& cs, z_type zscl )
{
    b2c_ = b2c;
    sampling_ = cs;
    zscale_ = zscl;
    trcnrrg_ = sampling_.hsamp_.trcRange();
    inlrg_ = sampling_.hsamp_.lineRange();
    zrg_ = sampling_.zsamp_;
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


float Survey::Geometry3D::inlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(1,0) );
    return c00.distTo<float>(c10);
}


float Survey::Geometry3D::crlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c01 = transform( BinID(0,1) );
    return c00.distTo<float>(c01);
}


static bool rendersSamePositions( const Survey::Geometry3D& sg1,
	const Survey::Geometry3D& sg2, const BinID& bid )
{
    const Coord coord1( sg1.transform(bid) );
    const Coord coord2( sg2.transform(bid) );
    return coord1.sqDistTo(coord2) < 1;
}


Survey::Geometry::RelationType Survey::Geometry3D::compare(
				const Geometry& geom, bool usezrg ) const
{
    mDynamicCastGet( const Geometry3D*, geom3d, &geom );
    if ( !geom3d )
	return UnRelated;

    const Geometry3D& oth = *geom3d;

    if ( !(b2c_ == oth.b2c_) // this check is rather lenient
      || !rendersSamePositions( *this, oth, sampling_.hsamp_.start_ )
      || !rendersSamePositions( *this, oth, sampling_.hsamp_.start_ )
      || !rendersSamePositions( *this, oth, oth.sampling_.hsamp_.stop_ ) )
	return UnRelated;

    const StepInterval<int> myinlrg = inlRange();
    const StepInterval<int> mycrlrg = crlRange();
    const StepInterval<z_type> myzrg = zRange();
    const StepInterval<int> othinlrg = oth.inlRange();
    const StepInterval<int> othcrlrg = oth.crlRange();
    const StepInterval<z_type> othzrg = oth.zRange();
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
