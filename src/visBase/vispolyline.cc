/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vispolyline.cc,v 1.3 2002-10-14 14:24:39 niclas Exp $";

#include "vispolyline.h"

#include "Inventor/nodes/SoLineSet.h"
#include "Inventor/nodes/SoCoordinate3.h"

mCreateFactoryEntry( visBase::PolyLine );

visBase::PolyLine::PolyLine()
    : lineset( new SoLineSet )
    , coords( new SoCoordinate3 )
    , VisualObjectImpl( true )
{
    addChild( coords );
    addChild( lineset );

    coords->point.deleteValues( 0, 1 );
}


int visBase::PolyLine::size() const { return coords->point.getNum(); }


void visBase::PolyLine::addPoint( const Coord3& pos )
{
    int idx = size();
    coords->point.setNum( idx+1 );
    coords->point.set1Value( idx, pos.x, pos.y, pos.z );
    lineset->numVertices.setValue( size() );
}


void visBase::PolyLine::insertPoint( int idx, const Coord3& pos )
{
    if ( idx==size() ) return addPoint( pos );

    coords->point.insertSpace( idx, 1 );
    coords->point.set1Value( idx, pos.x, pos.y, pos.z );
    lineset->numVertices.setValue( size() );
}


Coord3 visBase::PolyLine::getPoint( int idx ) const 
{
    if ( idx<size() && idx>=0 )
    {
	SbVec3f pos = coords->point[idx];
	Coord3 res ( pos[0], pos[1], pos[2] );
	return res;
    }

    return Coord3( mUndefValue, mUndefValue, mUndefValue );
}


void visBase::PolyLine::removePoint( int idx )
{
    if ( idx<size() && idx>=0 )
    {
	coords->point.deleteValues( idx, 1 );
	lineset->numVertices.setValue( size() );
    }
}
