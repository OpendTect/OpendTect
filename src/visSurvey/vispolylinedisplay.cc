/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vispolylinedisplay.h"

#include "survinfo.h"
#include "vismaterial.h"
#include "vispolyline.h"


namespace visSurvey
{

PolyLineDisplay::PolyLineDisplay()
    : VisualObjectImpl(true)
{
    polyline_ = visBase::PolyLine::create();
    addChild( polyline_->osgNode() );
}


PolyLineDisplay::~PolyLineDisplay()
{
    removeChild( polyline_->osgNode() );
}


void PolyLineDisplay::fillPolyLine( const TypeSet<Coord>& coords )
{
    for ( int idx=0; idx<coords.size(); idx++ )
	fillPolyLine( Coord3(coords[idx], SI().sampling(0).zsamp_.start) );
}


void PolyLineDisplay::fillPolyLine( const Coord3& pos )
{ polyline_->addPoint( pos ); }


void PolyLineDisplay::setDisplayTransformation( const mVisTrans* nt )
{ polyline_->setDisplayTransformation( nt ); }


const mVisTrans* PolyLineDisplay::getDisplayTransformation() const
{ return polyline_->getDisplayTransformation(); }


void PolyLineDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( polyline_ )
	polyline_->setPixelDensity( dpi );

}


void PolyLineDisplay::setColor( OD::Color col )
{
    getMaterial()->setColor( col );
}


OD::Color PolyLineDisplay::getColor() const
{
    return getMaterial()->getColor();
}

} // namespace visSurvey
