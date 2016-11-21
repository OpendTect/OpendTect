/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/


#include "basemapimpl.h"
#include "coord.h"
#include "task.h"
#include "keystrs.h"
#include "uistrings.h"


BaseMapObject::BaseMapObject( const char* nm )
    : NamedMonitorable(nm)
    , changed(this)
    , leftClicked(this)
    , rightClicked(this)
    , stylechanged(this)
    , zvaluechanged(this)
    , namechanged(this)
    , depth_(0)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, treeitmid, (1000) );
    id_ = treeitmid++;
}


BaseMapObject::~BaseMapObject()
{
    sendDelNotif();
}


void BaseMapObject::setDepth( int val )
{
    depth_ = val;
    zvaluechanged.trigger();
}


int BaseMapObject::nrShapes() const
{ return 0; }

const char* BaseMapObject::getShapeName( int ) const
{ return 0; }

void BaseMapObject::getPoints( int, TypeSet<Coord>& ) const
{ }

bool BaseMapObject::getBoundingBox( BoundingBox& bbox ) const
{ return false; }

OD::Alignment BaseMapObject::getAlignment( int shapeidx ) const
{ return OD::Alignment(); }

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


void BaseMapMarkers::setMarkerStyle( int, const OD::MarkerStyle2D& ms )
{
    if ( markerstyle_==ms )
	return;

    markerstyle_ = ms;
}


void BaseMapMarkers::getPoints( int shapeidx, TypeSet<Coord>& res ) const
{ res = positions_; }

void BaseMapMarkers::updateGeometry()
{ changed.trigger(); }


BaseMap::BaseMap()
    : OD::PresentationManagedViewer()
{
    mDefineStaticLocalObject( Threads::Atomic<int>, vwrid, (0) );
    viewerobjid_ = OD::ViewerObjID::get( vwrid++ );
}
