/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
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
    , styleChanged(this)
    , zvalueChanged(this)
    , nameChanged(this)
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
    zvalueChanged.trigger();
}


int BaseMapObject::nrShapes() const
{
    return 0;
}

const char* BaseMapObject::shapeName( int ) const
{
    return "";
}


bool BaseMapObject::getBoundingBox( BoundingBox& bbox ) const
{
    return false;
}


BaseMapObject::Alignment BaseMapObject::alignment( int shapeidx ) const
{
    return Alignment();
}


Color BaseMapObject::color() const
{
    Color fillcolor = fillColor( 0 );
    if ( fillcolor != Color::NoColor() )
	return fillcolor;

    const LineStyle* ls = lineStyle( 0 );
    if ( ls && ls->color_ != Color::NoColor() )
	return ls->color_;

    const MarkerStyle* ms = markerStyle( 0 );
    if ( ms && ms->color_ != Color::NoColor() )
	return ms->color_;

    return Color::NoColor();
}


const OD::RGBImage* BaseMapObject::createImage( Coord& origin,Coord& p11 ) const
{
    return 0;
}


const OD::RGBImage* BaseMapObject::createPreview( int approxdiagonal ) const
{
    return 0;
}


bool BaseMapObject::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name() );
    par.set( sKey::Type(), type() );
    par.set( sKey::Depth(), depth() );
    return true;
}


bool BaseMapObject::usePar( const IOPar& par, const TaskRunnerProvider& trprov )
{
    BufferString nm, typ;
    if ( par.get(sKey::Name(),nm) )
	setName( nm );
    if ( par.get(sKey::Type(),typ) )
	setType( typ );

    int dpth = 0;
    if ( par.get(sKey::Depth(),dpth) )
	setDepth( dpth );

    return doUsePar( par, trprov );
}


BaseMap::BaseMap()
{
    mDefineStaticLocalObject( Threads::Atomic<int>, vwrid, (0) );
    viewerobjid_ = Presentation::ViewerObjID::get( vwrid++ );
}



BaseMapMarkers::BaseMapMarkers()
    : BaseMapObject( 0 )
{
    typenm_ = "Markers";
}


BaseMapMarkers::~BaseMapMarkers()
{
}


void BaseMapMarkers::setMarkerStyle( int, const MarkerStyle& ms )
{
    markerstyle_ = ms;
}


void BaseMapMarkers::getPoints( int shapeidx, TypeSet<Coord>& res ) const
{
    res = positions_;
}


void BaseMapMarkers::updateGeometry()
{
    changed.trigger();
}
