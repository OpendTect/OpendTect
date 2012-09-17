/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : H. Payraudeau
* DATE     : November 2005
-*/


static const char* rcsID = "$Id: vispolylinedisplay.cc,v 1.4 2011/12/16 15:57:21 cvskris Exp $";

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


void PolyLineDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    polyline_->setDisplayTransformation( nt );
}


const mVisTrans* PolyLineDisplay::getDisplayTransformation() const
{ return polyline_->getDisplayTransformation(); }

}//namespace
