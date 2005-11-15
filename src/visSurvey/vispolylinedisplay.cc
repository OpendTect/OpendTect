/*+
* COPYRIGHT: (C) dGB Beheer B.V.
* AUTHOR   : H. Payraudeau
* DATE     : November 2005
-*/


static const char* rcsID = "$Id: vispolylinedisplay.cc,v 1.1 2005-11-15 16:16:57 cvshelene Exp $";

#include "vispolylinedisplay.h"
#include "survinfo.h"
#include "vispolyline.h"


mCreateFactoryEntry( visSurvey::PolyLineDisplay );

namespace visSurvey
{
    
PolyLineDisplay::PolyLineDisplay()
    : VisualObjectImpl(true)
{
    polyline_ = visBase::PolyLine::create();
    addChild( polyline_->getInventorNode() );
}


PolyLineDisplay::~PolyLineDisplay()
{
    removeChild( polyline_->getInventorNode() );
}


void PolyLineDisplay::fillPolyLine( const TypeSet<Coord>& coords )
{
    for ( int idx=0; idx<coords.size(); idx++ )
    {
	polyline_->addPoint( Coord3(coords[idx], SI().sampling(0).zrg.start) );
    }
}


void PolyLineDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    polyline_->setDisplayTransformation( nt );
}


visBase::Transformation* PolyLineDisplay::getDisplayTransformation()
{ return polyline_->getDisplayTransformation(); }

}//namespace
