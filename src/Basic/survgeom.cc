/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          18-4-1996
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "survgeom.h"

#include "multiid.h"
#include "surv2dgeom.h"
#include "survinfo.h"

using namespace Survey;


mImplFactory(GeometryReader,GeometryReader::factory);
mImplFactory(GeometryWriter,GeometryWriter::factory);

static GeometryManager* theinst = 0;

GeometryManager& Survey::GMAdmin()
{
    if( !theinst )
    { theinst = new GeometryManager(); }

    return *theinst;
}


Geometry::Geometry()
    : geomid_( -2 )
{}


Geometry::~Geometry()
{}


TraceID Geometry::getTrace(Coord const& crd, float maxdist ) const
{
    float dist;
    TraceID trcid = nearestTrace( crd,  &dist );
    
    if ( trcid.isUdf() || dist>maxdist )
	return TraceID::udf();
    
    return trcid;
}


GeometryManager::GeometryManager()
{
}


GeometryManager::~GeometryManager()
{
    deepUnRef( geometries_ );
}


const Geometry* GeometryManager::getGeometry(int geomid) const
{
    if ( geomid==cDefault3DGeom() )
	return SI().get3DGeometry( false );

    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( geometries_[idx]->getGeomID() == geomid )
	    return geometries_[idx];
    
    return 0;
}


const Geometry* GeometryManager::getGeometry( const MultiID& mid ) const
{
    if ( mid.nrKeys() == 2 )
	return getGeometry( mid.ID(1) );

    return 0;
}


int GeometryManager::getGeomID( const char* name ) const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
    {
	if ( !geometries_[idx]->is2D() )
	    return -cDefault3DGeom();
	else
	{
	    if ( ((Geometry2D*)geometries_[idx])->data().lineName() == name)
		return geometries_[idx]->getGeomID();
	}
    }

    return -1;
}


const char* GeometryManager::getName( const int geomid ) const
{
    if ( !getGeometry(geomid)->is2D() )
    {}//
    else
	return ( (Geometry2D*)getGeometry(geomid) )->data().lineName();

    return 0;
}


Coord GeometryManager::toCoord( const TraceID& tid ) const
{
    RefMan<const Geometry> geom = getGeometry( tid.geomid_ );
    return geom ? geom->toCoord( tid.line_, tid.trcnr_ ) : Coord::udf();
}


void GeometryManager::addGeometry(Survey::Geometry* g)
{
    g->ref();
    geometries_ += g;
}


bool GeometryManager::fetchFrom2DGeom()
{
    GeometryWriter* geomwriter =GeometryWriter::factory().create("2D");
    BufferStringSet lsname;
    S2DPOS().getLineSets( lsname );
    for ( int idx=0; idx<lsname.size(); idx++ )
    {
	BufferStringSet lname;
	S2DPOS().getLines( lname, lsname.get(idx).buf() );
	for ( int idx2=0; idx2<lname.size(); idx2++ )
	{
	    Geometry2D* geom2d = new Geometry2D();
	    geom2d->ref();
	    geom2d->data().setLineName( lname.get(idx2) );
	    S2DPOS().getGeometry(geom2d->data());
	    BufferString newlnm = lsname.get(idx);
	    newlnm.add( "_" );
	    newlnm.add( lname.get(idx2) );
	    geom2d->data().setLineName( newlnm );
	    geomwriter->write( geom2d );
	    addGeometry( geom2d );
	    geom2d->unRef();
	}
    }
    return true;
}


bool GeometryManager::write( Geometry* geom)
{
    if ( geom->is2D() )
    {
    GeometryWriter* geomwriter =GeometryWriter::factory().create( "2D" );
    geom->ref();
    geomwriter->write( geom );
    addGeometry( geom );
    geom->unRef();
    return true;
    }
    else
	return false;
}


int GeometryManager::createEntry( const char* name, const bool is2d )
{
    if ( is2d )
    {
	GeometryWriter* geomwriter = GeometryWriter::factory().create("2D");
	return geomwriter->createEntry( name );
    }
    else
	return cDefault3DGeom();
}


BinID InlCrlSystem::transform( const Coord& c ) const
{
    StepInterval<int> inlrg, crlrg;
    cs_.hrg.get( inlrg, crlrg );
    return b2c_.transformBack( c, &inlrg, &crlrg );
}


Coord InlCrlSystem::transform( const BinID& b ) const
{ return b2c_.transform(b); }


Coord3 InlCrlSystem::oneStepTranslation( const Coord3& planenormal ) const
{
    Coord3 translation( 0, 0, 0 );
    
    if ( fabs(planenormal.z) > 0.5 )
    {
	translation.z = SI().zStep();
    }
    else
    {
	Coord norm2d = planenormal;
	norm2d.normalize();
	
	if ( fabs(norm2d.dot(SI().binID2Coord().rowDir())) > 0.5 )
	    translation.x = inlDistance();
	else
	    translation.y = crlDistance();
    }
    
    return translation;
}


float InlCrlSystem::inlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(1,0) );
    return (float) c00.distTo(c10);
}


float InlCrlSystem::crlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c01 = transform( BinID(0,1) );
    return (float) c00.distTo(c01);
}
