/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basemap.h"

#include "coord.h"
#include "keystrs.h"
#include "task.h"



BasemapObject::BasemapObject( const char* nm )
    : NamedCallBacker(nm)
    , leftClicked(this)
    , rightClicked(this)
    , changed(this)
    , stylechanged(this)
    , zvalueChanged(this)
    , nameChanged(this)
    , depth_(0)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, bmobjid, (1000) );
    id_.set( bmobjid++ );
}


BasemapObject::~BasemapObject()
{
}


void BasemapObject::setDepth( int val )
{
    depth_ = val;
    zvalueChanged.trigger();
}


int BasemapObject::nrShapes() const
{ return 0; }

const char* BasemapObject::getShapeName( int ) const
{ return 0; }

void BasemapObject::getPoints( int, TypeSet<Coord>& ) const
{}

bool BasemapObject::getBoundingBox( BoundingBox& ) const
{ return false; }

Alignment BasemapObject::getAlignment( int ) const
{ return Alignment(); }

FontData BasemapObject::getFont( int ) const
{ return FontData(); }

Coord BasemapObject::getTextOffset( int ) const
{ return Coord(0., 0.); }

Coord BasemapObject::getTextPos(int shapeidx) const
{ return Coord::udf(); }

OD::Color BasemapObject::getColor() const
{
    if ( getFillColor(0) != OD::Color::NoColor() )
	return getFillColor( 0 );
    else if ( getLineStyle(0) )
	return getLineStyle(0)->color_;
    else if ( getMarkerStyle(0) )
	return getMarkerStyle(0)->color_;
    return OD::Color::NoColor();
}


const OD::RGBImage* BasemapObject::createImage( Coord& origin,Coord& p11 ) const
{
    return nullptr;
}


const OD::RGBImage* BasemapObject::createPreview( int approxdiagonal ) const
{
    return nullptr;
}


bool BasemapObject::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name() );
    par.set( sKey::Type(), getType() );
    par.set( sKey::Depth(), getDepth() );
    return true;
}


bool BasemapObject::usePar( const IOPar& par, TaskRunner* )
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
