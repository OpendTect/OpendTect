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

Geom2DObject::Geom2DObject()
    : BaseMapObject(0)
    , mid_(MultiID::udf())
{}


Geom2DObject::~Geom2DObject()
{}


void Geom2DObject::setMultiID( const MultiID& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !ioobj ) return;

    const OD::String& linename = ioobj->name();
    setName( linename );

    Pos::GeomID geomid = Survey::GM().getGeomID( linename );
    if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	return;

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(geomid))
    if ( !geom2d ) return;

    const TypeSet<PosInfo::Line2DPos>& positions = geom2d->data().positions();
    BendPointFinder2DGeom bpf( positions, 10.0 );
    bpf.execute();

    bptcoords_.erase();
    for ( int idx=0; idx<bpf.bendPoints().size(); idx++ )
    {
	const int posidx = bpf.bendPoints()[idx];
	bptcoords_ += positions[posidx].coord_;
    }
}


float Geom2DObject::getTextRotation() const
{
    const float dy = (float) (bptcoords_[1].y - bptcoords_[0].y);
    const float dx = (float) (bptcoords_[1].x - bptcoords_[0].x);
    return Math::toDegrees( Math::Atan2(dy,dx) );
}

void Geom2DObject::setLineStyle( int shapeidx, const LineStyle& ls )
{
    ls_ = ls;
    stylechanged.trigger();
}


void Geom2DObject::updateGeometry()
{ changed.trigger(); }


int Geom2DObject::nrShapes() const
{ return 1; }


const char* Geom2DObject::getShapeName( int ) const
{ return name().buf(); }


void Geom2DObject::getPoints( int, TypeSet<Coord>& crds ) const
{ crds = bptcoords_; }


} // namespace Basemap
