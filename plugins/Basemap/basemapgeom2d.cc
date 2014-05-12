/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : January 2014
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemapgeom2d.h"

#include "bendpointfinder.h"
#include "ioman.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "survgeom2d.h"

namespace Basemap
{

Geom2DObject::Geom2DObject( const MultiID& mid )
    : BaseMapObject(0)
    , mid_(mid)
{
    setMultiID( mid );
}


Geom2DObject::~Geom2DObject()
{}


void Geom2DObject::setMultiID( const MultiID& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !ioobj ) return;

    const BufferString& linename = ioobj->name();
    setName( linename );

    Pos::GeomID geomid = Survey::GM().getGeomID( linename );
    if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	return;

//    const PosInfo::Line2DData geom( linename );
//    const TypeSet<PosInfo::Line2DPos>& positions = geom.positions();
//    BendPointFinder2D bpf( positions, 1.0 );
//    bpf.execute();
}


void Geom2DObject::updateGeometry()
{ changed.trigger(); }


int Geom2DObject::nrShapes() const
{ return 1; }

const char* Geom2DObject::getShapeName( int ) const
{ return 0; }

void Geom2DObject::getPoints( int, TypeSet<Coord>& crds ) const
{
}

} // namespace Basemap
