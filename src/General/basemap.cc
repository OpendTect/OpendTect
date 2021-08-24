/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/


#include "basemapimpl.h"
#include "coord.h"
#include "task.h"



BaseMapObject::BaseMapObject( const char* nm )
    : NamedCallBacker(nm)
    , changed(this)
    , leftClicked(this)
    , rightClicked(this)
    , stylechanged(this)
    , zvalueChanged(this)
    , nameChanged(this)
    , depth_(0)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, treeitmid, (1000) );
    id_ = treeitmid++;
}


BaseMapObject::~BaseMapObject()
{
}


void BaseMapObject::setDepth( int val )
{
    depth_ = val;
    zvalueChanged.trigger();
}


int BaseMapObject::nrShapes() const
{ return 0; }

const char* BaseMapObject::getShapeName( int ) const
{ return 0; }

void BaseMapObject::getPoints( int, TypeSet<Coord>& ) const
{}

bool BaseMapObject::getBoundingBox( BoundingBox& ) const
{ return false; }

Alignment BaseMapObject::getAlignment( int ) const
{ return Alignment(); }

Coord BaseMapObject::getTextPos(int shapeidx) const
{ return Coord::udf(); }

Color BaseMapObject::getColor() const
{
    if ( getFillColor(0) != Color::NoColor() )
	return getFillColor( 0 );
    else if ( getLineStyle(0) )
	return getLineStyle(0)->color_;
    else if ( getMarkerStyle(0) )
	return getMarkerStyle(0)->color_;
    return Color::NoColor();
}


const OD::RGBImage* BaseMapObject::createImage( Coord& origin,Coord& p11 ) const
{ return 0; }

const OD::RGBImage* BaseMapObject::createPreview( int approxdiagonal ) const
{ return 0; }

bool BaseMapObject::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name() );
    par.set( sKey::Type(), getType() );
    par.set( sKey::Depth(), getDepth() );
    return true;
}


bool BaseMapObject::usePar( const IOPar& par, TaskRunner* )
{
    BufferString nm, type;
    if ( par.get(sKey::Name(),nm) )
	setName( nm );
    if ( par.get(sKey::Type(),type) )
	setType( type );

    int depth = 0;
    if ( par.get(sKey::Depth(),depth) )
	setDepth( depth );

    return true;
}



BaseMapMarkers::BaseMapMarkers()
    : BaseMapObject( 0 )
{}


BaseMapMarkers::~BaseMapMarkers()
{ }


void BaseMapMarkers::setMarkerStyle( int, const MarkerStyle2D& ms )
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
{ changed.trigger(); }
