/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vispolyline.cc,v 1.4 2003-01-21 16:10:08 kristofer Exp $";

#include "vispolyline.h"

#include "viscoord.h"

#include "Inventor/nodes/SoLineSet.h"

mCreateFactoryEntry( visBase::PolyLine );

visBase::PolyLine::PolyLine()
    : VertexShape( new SoLineSet )
    , lineset( dynamic_cast<SoLineSet*>( shape ) )
{ }


int visBase::PolyLine::size() const { return coords->size(); }


void visBase::PolyLine::addPoint( const Coord3& pos )
{
    coords->addPos( pos );
    lineset->numVertices.setValue( size() );
}


Coord3 visBase::PolyLine::getPoint( int idx ) const 
{ return coords->getPos( idx ); }


void visBase::PolyLine::removePoint( int idx )
{
    lineset->numVertices.setValue( size()-1 );
    for ( int idy=idx; idy<size()-1; idy++ )
    {
	coords->setPos( idy, coords->getPos( idy+1 ) );
    }

    coords->removePos( size()-1 );
}
