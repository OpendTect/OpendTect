/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vispolyline.cc,v 1.6 2003-09-22 08:26:24 kristofer Exp $";

#include "vispolyline.h"

#include "viscoord.h"

#include "Inventor/nodes/SoLineSet.h"
#include "Inventor/nodes/SoIndexedLineSet.h"

mCreateFactoryEntry( visBase::PolyLine );
mCreateFactoryEntry( visBase::IndexedPolyLine );

visBase::PolyLine::PolyLine()
    : VertexShape( new SoLineSet )
    , lineset( dynamic_cast<SoLineSet*>( shape ) )
{ }


int visBase::PolyLine::size() const { return coords->size(); }


void visBase::PolyLine::addPoint( const Coord3& pos )
{
    setPoint( size(), pos );
}


void visBase::PolyLine::setPoint(int idx, const Coord3& pos )
{
    if ( idx>size() ) return;
    coords->setPos( idx, pos );
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


visBase::IndexedPolyLine::IndexedPolyLine()
    : IndexedShape( new SoIndexedLineSet )
{ }

