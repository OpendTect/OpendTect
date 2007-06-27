/*+
* COPYRIGHT: (C) dGB Beheer B.V.
* AUTHOR   : H. Payraudeau
* DATE     : November 2005
-*/


static const char* rcsID = "$Id: vispolylinedisplay.cc,v 1.2 2007-06-27 10:41:05 cvsraman Exp $";

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
	fillPolyLine( Coord3(coords[idx], SI().sampling(0).zrg.start) );
    }
}


void PolyLineDisplay::fillPolyLine( const Coord3& pos )
{
	polyline_->addPoint( pos );
}


void PolyLineDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    polyline_->setDisplayTransformation( nt );
}


visBase::Transformation* PolyLineDisplay::getDisplayTransformation()
{ return polyline_->getDisplayTransformation(); }

}//namespace
