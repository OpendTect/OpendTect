/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kris
 Date:          2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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


const GeometryManager& GM()
{
    mDefineStaticLocalObject( GeometryManager*, theinst, = 0 );
    if ( !theinst )
	{ theinst = new GeometryManager; }
    return *theinst;
}


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
    return GM().getGeomID( tk ) == getID()
	&& includes( tk.lineNr(), tk.trcNr() );
}


Coord Geometry::toCoord( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( GM().getGeomID(tk)  );
    return geom ? geom->toCoord( tk.pos() ) : Coord::udf();
}


bool Geometry::exists( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( GM().getGeomID(tk) );
    return geom && geom->includes( tk.pos() );
}


#define mGetConstGeom(varnm,geomid) \
    ConstRefMan<Geometry> varnm = GM().getGeometry( geomid );


GeometryManager::GeometryManager()
{ hasduplnms_ = hasDuplicateLineNames(); }

GeometryManager::~GeometryManager()
{ deepUnRef( geometries_ ); }

int GeometryManager::nrGeometries() const
{ return geometries_.size(); }

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
	RefMan<Geometry3D> rm = SI().get3DGeometry( false );
	Geometry3D* survicsys = rm.ptr();
	rm.set( 0, false );
	survicsys->setID( cSIGeomID );
	const_cast<GeometryManager*>(this)->addGeometry( *survicsys );
    }
}


TrcKey::SurvID GeometryManager::default3DSurvID() const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( !geometries_[idx]->is2D() )
	    return geometries_[idx]->getID();

    return cUndefGeomID();
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


Geometry::ID GeometryManager::getGeomID( const TrcKey& tk ) const
{
    Geometry::ID geomid = tk.survID();
    if ( geomid == surv2did_ )
	geomid = tk.lineNr();
    return geomid;
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


Coord GeometryManager::toCoord( const TrcKey& tk ) const
{
    const Geometry::ID geomid = getGeomID( tk );
    mGetConstGeom(geom,geomid);
    return geom ? geom->toCoord( tk.lineNr(), tk.trcNr() ) : Coord::udf();
}


TrcKey GeometryManager::traceKey( Geometry::ID geomid, Pos::LineID lid,
				  Pos::TraceID tid ) const
{
    mGetConstGeom(geom,geomid);
    if ( !geom )
	return TrcKey::udf();

    if ( geom->is2D() )
	return TrcKey( TrcKey::std2DSurvID(), geomid,  tid );

    return TrcKey( geomid, lid, tid );
}


TrcKey GeometryManager::traceKey( Geometry::ID geomid, Pos::TraceID tid ) const
{
    mGetConstGeom(geom,geomid);
    if ( !geom || !geom->is2D() )
	return TrcKey::udf();

    return TrcKey( TrcKey::std2DSurvID(), geomid,  tid );
}


void GeometryManager::addGeometry( Survey::Geometry& geom )
{
    geom.ref();
    geometries_ += &geom;
}


bool GeometryManager::fetchFrom2DGeom( uiString& errmsg )
{
    fillGeometries(0);
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
	{
	    addGeometry( geom );
	    geom.unRef();
	}

	return true;
    }
    else
	return false;
}

/*
IOObj* GeometryManager::createEntry( const char* name, const bool is2d )
{
    if ( is2d )
    {
	PtrMan<GeometryWriter> geomwriter = GeometryWriter::factory()
				    .create( sKey::TwoD() );

	return geomwriter->createEntry( name );
    }
    else
	return 0;
}
*/

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
    deepUnRef( geometries_ );
    ensureSIPresent();
    PtrMan<GeometryReader> geomreader = GeometryReader::factory()
				        .create(sKey::TwoD());
    return geomreader ? geomreader->read( geometries_, taskrunner ) : false;
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

} // namespace Survey
