/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          18-4-1996
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "survgeom.h"
#include "survinfo.h"

using namespace Survey;

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


const Geometry* GeometryManager::getGeomety(int geomid) const
{
    if ( geomid==cDefault3DGeom() )
	return SI().get3DGeometry( false );
    
    return 0;
}


const Geometry* GeometryManager::getGeomety(const MultiID&) const
{
    return 0;
}


Coord GeometryManager::transform( const TraceID& tid ) const
{
    RefMan<const Geometry> geom = getGeomety( tid.geomid_ );
    return geom ? geom->toCoord( tid.line_, tid.trcnr_ ) : Coord::udf();
}


void GeometryManager::addGeometry(Survey::Geometry* g)
{
    g->ref();
    geometries_ += g;
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
