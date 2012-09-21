/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "basemapimpl.h"
#include "position.h"

BaseMapObject::BaseMapObject( const char* nm )
    : NamedObject(nm)
    , changed(this)
{}


int BaseMapObject::nrShapes() const
{ return 0; }


const char* BaseMapObject::getShapeName( int ) const
{ return 0; }


void BaseMapObject::getPoints( int, TypeSet<Coord>& ) const
{ }


const OD::RGBImage* BaseMapObject::getImage(Coord& origin,Coord& p11) const
{ return 0; }


const OD::RGBImage* BaseMapObject::getPreview(int approxdiagonal) const
{ return 0; }


BaseMapMarkers::BaseMapMarkers()
    : BaseMapObject( 0 )
{}


BaseMapMarkers::~BaseMapMarkers()
{ }


void BaseMapMarkers::setMarkerStyle( const MarkerStyle2D& ms )
{
    if ( markerstyle_==ms )
	return;
    
    markerstyle_ = ms;
}


void BaseMapMarkers::getPoints( int shapeidx, TypeSet<Coord>& res) const
{
    res = positions_;
}


void BaseMapMarkers::updateGeometry()
{
    changed.trigger();
}
