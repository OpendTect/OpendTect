/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kris
 Date:          2013
________________________________________________________________________

-*/

#include "survgeom2d.h"
#include "posinfo2dsurv.h"

#include "keystrs.h"
#include "multiid.h"
#include "survinfo.h"
#include "task.h"


namespace Survey
{

static Pos::GeomID cSIGeomID = -1;

mImplFactory(GeometryReader,GeometryReader::factory);
mImplFactory(GeometryWriter,GeometryWriter::factory);
const TrcKey::SurvID GeometryManager::surv2did_ = 0;

static PtrMan<GeometryManager> theinst = 0;

const GeometryManager& GM()
{
    return *theinst.createIfNull();
}


bool is2DGeom( Pos::GeomID geomid )
{ return geomid >= 0; }

bool is3DGeom( Pos::GeomID geomid )
{ return geomid == -1; }

bool isSynthetic( Pos::GeomID geomid )
{ return geomid == -2; }

Pos::GeomID default3DGeomID()
{ return Geometry::default3D().getID(); }

bool isValidGeomID( Pos::GeomID geomid )
{ return geomid != -999 && !mIsUdf(geomid); }


Geometry::Geometry()
    : id_(mUdf(ID))
{
}


Geometry::~Geometry()
{
}


const Geometry& Geometry::default3D()
{
    return *GM().getGeometry( GM().default3DSurvID() );
}


TrcKey Geometry::getTrace( const Coord& crd, float maxdist ) const
{
    float dist;
    const TrcKey tk = nearestTrace( crd,  &dist );
    return tk.isUdf() || dist>maxdist ? TrcKey::udf() : tk;
}


bool Geometry::includes( const TrcKey& tk ) const
{
    return tk.geomID() == getID()
	&& includes( tk.lineNr(), tk.trcNr() );
}


Coord Geometry::toCoord( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( tk.geomID() );
    return geom ? geom->toCoord( tk.pos() ) : Coord::udf();
}


bool Geometry::exists( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( tk.geomID() );
    return geom && geom->includes( tk.pos() );
}


Pos::SurvID Geometry::getSurvID() const
{
    return is2D()
	? Survey::GeometryManager::get2DSurvID()
	: (Pos::SurvID) getID();
}


const Geometry2D* Geometry::as2D() const
{
    return const_cast<Geometry*>( this )->as2D();
}



#define mGetConstGeom(varnm,geomid) \
    ConstRefMan<Geometry> varnm = GM().getGeometry( geomid );


GeometryManager::GeometryManager()
    : hasduplnms_(false)
{}

GeometryManager::~GeometryManager()
{ deepUnRef( geometries_ ); }

int GeometryManager::nrGeometries() const
{ return geometries_.size(); }


bool GeometryManager::isUsable( Pos::GeomID geomid ) const
{
    auto* geom = getGeometry( geomid );
    if ( !geom )
	return false;

    return geom->as3D() || (geom->as2D() && !geom->as2D()->isEmpty());
}


void GeometryManager::ensureSIPresent() const
{
    bool has3d = false;
    for ( int idx=0; idx<geometries_.size(); idx++ )
    {
	const bool is2d = geometries_[idx]->is2D();
	if ( !is2d )
	{
	    has3d = true;
	    break;
	}
    }

    if ( !has3d )
    {
	RefMan<Geometry3D> survicsys = SI().get3DGeometry( false );
	survicsys->setID( cSIGeomID );
	const_cast<GeometryManager*>(this)->addGeometry( *survicsys );
    }
}


TrcKey::SurvID GeometryManager::default3DSurvID() const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( !geometries_[idx]->is2D() )
	    return geometries_[idx]->getID();

    return cSIGeomID;
}


const Geometry* GeometryManager::getGeometry( Geometry::ID geomid ) const
{
    const int idx = indexOf( geomid );
    return idx<0 ? 0 : geometries_[idx];
}


Geometry* GeometryManager::getGeometry( Geometry::ID geomid )
{
    const int idx = indexOf( geomid );
    return idx<0 ? 0 : geometries_[idx];
}


const Geometry3D* GeometryManager::getGeometry3D( Pos::SurvID sid ) const
{
    const TrcKey tk( sid, BinID(0,0) );
    const Geometry* geom = getGeometry( tk.geomID() );
    return geom ? geom->as3D() : 0;
}


static Geometry2D& dummyGeom2D()
{
    PosInfo::Line2DData* l2d = nullptr;
    static RefMan<Survey::Geometry2D> ret = new Survey::Geometry2D( l2d );
    return *ret;
}

const Geometry2D& GeometryManager::get2D( Pos::GeomID geomid ) const
{
    const Geometry* geom = getGeometry( geomid );
    return geom && geom->as2D() ? *geom->as2D(): dummyGeom2D();
}


const Geometry* GeometryManager::getGeometry( const MultiID& mid ) const
{
    if ( mid.nrKeys() == 2 )
	return getGeometry( mid.ID(1) );

    return 0;
}


const Geometry* GeometryManager::getGeometry( const char* nm ) const
{
    const FixedString namestr( nm );
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( namestr == geometries_[idx]->getName() )
	    return geometries_[idx];

    return 0;
}


Geometry::ID GeometryManager::getGeomID( const char* lnnm ) const
{
    const FixedString reqln( lnnm );
    for ( int idx=0; idx<geometries_.size(); idx++ )
    {
	if ( geometries_[idx]->is2D() && reqln==geometries_[idx]->getName() )
	    return geometries_[idx]->getID();
    }

    return cUndefGeomID();
}


Geometry::ID GeometryManager::getGeomID( const char* lsnm,
					 const char* lnnm ) const
{
    if ( !hasduplnms_ )
        return getGeomID( lnnm );

    BufferString newlnm = lsnm;
    newlnm.add( "-" );
    newlnm.add( lnnm );
    return getGeomID( newlnm.buf() );
}


const char* GeometryManager::getName( Geometry::ID geomid ) const
{
    mGetConstGeom(geom,geomid);
    return geom ? geom->getName() : 0;
}


StepInterval<float> GeometryManager::zRange( Pos::GeomID geomid ) const
{
    const Survey::Geometry* geom = getGeometry( geomid );
    StepInterval<float> zrg = SI().zRange();
    if ( geom && geom->as2D() )
	zrg = geom->as2D()->zRange();
    else if ( geom && geom->as3D() )
	zrg = geom->as3D()->zRange();

    return zrg;
}


Coord GeometryManager::toCoord( const TrcKey& tk ) const
{
    mGetConstGeom(geom,tk.geomID());
    return geom ? geom->toCoord( tk.lineNr(), tk.trcNr() ) : Coord::udf();
}


TrcKey GeometryManager::traceKey( Geometry::ID geomid, Pos::LineID lid,
				  Pos::TraceID tid ) const
{
    mGetConstGeom(geom,geomid);
    if ( !geom )
	return TrcKey::udf();

    if ( geom->is2D() )
	return TrcKey( geomid,	tid );

    return TrcKey( geomid, BinID(lid, tid) );
}


TrcKey GeometryManager::traceKey( Geometry::ID geomid, Pos::TraceID tid ) const
{
    mGetConstGeom(geom,geomid);
    if ( !geom || !geom->is2D() )
	return TrcKey::udf();

    return TrcKey( geomid,  tid );
}


TrcKey GeometryManager::nearestTrace( const Coord& crd, bool is2d,
				      float* dist ) const
{
    BufferStringSet nms; TypeSet<Pos::GeomID> geomids;
    const bool res = getList( nms, geomids, is2d );
    if ( !res )
	return TrcKey::udf();

    TrcKey tkatmin; float mindist = mUdf(float);
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const Geometry* geom = getGeometry( geomids[idx] );
	if ( !geom )
	    continue;

	float curdist = mUdf(float);
	TrcKey tk = geom->nearestTrace( crd, &curdist );
	if ( curdist < mindist )
	{
	    mindist = curdist;
	    tkatmin = tk;
	}
    }

    if ( dist )
	*dist = mindist;

    return tkatmin;
}


void GeometryManager::addGeometry( Survey::Geometry& geom )
{
    geom.ref();
    geometries_ += &geom;
}


bool GeometryManager::fetchFrom2DGeom( uiString& errmsg )
{
    fillGeometries(0);
    if ( nrGeometries() > 1 ) // Already have new 2D geoms
	return true;

    PtrMan<GeometryWriter> geomwriter = GeometryWriter::factory()
				       .create(sKey::TwoD());
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
	    if ( geomid != GM().cUndefGeomID() )
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


bool GeometryManager::hasDuplicateLineNames()
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


bool GeometryManager::write( Geometry& geom, uiString& errmsg )
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

	if ( indexOf(geom.getID()) < 0 )
	    addGeometry( geom );
	geom.unRef();

	return true;
    }
    else
	return false;
}


Geometry::ID GeometryManager::addNewEntry( Geometry* geom, uiString& errmsg )
{
    if ( !geom )
	return cUndefGeomID();

    if ( !geom->is2D() )
	return default3DSurvID();

    Geometry::ID geomid = getGeomID( geom->getName() );
    if ( geomid!=cUndefGeomID() )
	return geomid;

    PtrMan<GeometryWriter> geomwriter =
	GeometryWriter::factory().create( sKey::TwoD() );

    Threads::Locker locker( lock_ );
    geomid = geomwriter->createNewGeomID( geom->getName() );
    if ( !write(*geom,errmsg) )
	return cUndefGeomID();

    return geomid;
}


bool GeometryManager::removeGeometry( Geometry::ID geomid )
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


int GeometryManager::indexOf( Geometry::ID geomid ) const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( geometries_[idx]->getID() == geomid )
	    return idx;

    return -1;
}


bool GeometryManager::fillGeometries( TaskRunner* taskrunner )
{
    Threads::Locker locker( lock_ );
    deepUnRef( geometries_ );
    ensureSIPresent();
    hasduplnms_ = hasDuplicateLineNames();
    PtrMan<GeometryReader> geomreader = GeometryReader::factory()
				        .create(sKey::TwoD());
    return geomreader ? geomreader->read( geometries_, taskrunner ) : false;
}


bool GeometryManager::updateGeometries( TaskRunner* taskrunner )
{
    Threads::Locker locker( lock_ );
    PtrMan<GeometryReader> geomreader = GeometryReader::factory()
					.create(sKey::TwoD());
    return geomreader ? geomreader->updateGeometries( geometries_, taskrunner )
		      : false;
}


bool GeometryManager::getList( BufferStringSet& names,
			       TypeSet<Geometry::ID>& geomids, bool is2d ) const
{
    names.erase();
    geomids.erase();
    for ( int idx=0; idx<geometries_.size(); idx++ )
    {
	if ( geometries_[idx]->is2D() == is2d )
	{
	    names.add( geometries_[idx]->getName() );
	    geomids += geometries_[idx]->getID();
	}
    }

    return true;
}


Geometry::ID GeometryManager::findRelated( const Geometry& ref,
					   Geometry::RelationType& reltype,
					   bool usezrg ) const
{
    int identicalidx=-1, supersetidx=-1, subsetidx=-1, relatedidx=-1;
    for ( int idx=0; identicalidx<0 && idx<geometries_.size(); idx++ )
    {
	Geometry::RelationType rt = geometries_[idx]->compare( ref, usezrg );
	switch(rt)
	{
	    case Geometry::Identical : identicalidx = idx; break;
	    case Geometry::SuperSet : supersetidx = idx; break;
	    case Geometry::SubSet : subsetidx = idx; break;
	    case Geometry::Related : relatedidx = idx; break;
	    default: break;
	}
    }

    if ( identicalidx >= 0 )
    { reltype = Geometry::Identical; return geometries_[identicalidx]->getID();}
    if ( supersetidx >= 0 )
    { reltype = Geometry::SuperSet; return geometries_[supersetidx]->getID(); }
    if ( subsetidx >= 0 )
    { reltype = Geometry::SubSet; return geometries_[subsetidx]->getID(); }
    if ( relatedidx >= 0 )
    { reltype = Geometry::Related; return geometries_[relatedidx]->getID(); }

    reltype = Geometry::UnRelated; return cUndefGeomID();
}


} // namespace Survey
