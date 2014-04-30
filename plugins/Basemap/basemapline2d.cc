/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : January 2014
-*/

static const char* rcsID mUsedVar = "$Id: basemappi.cc 34122 2014-04-10 18:29:49Z nanne.hemstra@dgbes.com $";


#include "basemapline2d.h"

namespace Basemap
{

Line2DObject::Line2DObject( const MultiID& mid )
    : BaseMapObject(0)
    , mid_(mid)
{
}


Line2DObject::~Line2DObject()
{}


void Line2DObject::updateGeometry()
{ changed.trigger(); }


int Line2DObject::nrShapes() const
{ return 1; }

const char* Line2DObject::getShapeName( int ) const
{ return 0; }

void Line2DObject::getPoints( int, TypeSet<Coord>& crds ) const
{
}

} // namespace Basemap
