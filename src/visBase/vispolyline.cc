/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vispolyline.cc,v 1.2 2002-04-26 13:00:08 kristofer Exp $";

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


void visBase::PolyLine::addPoint( const Geometry::Pos& pos )
{
    int idx = size();
    coords->point.setNum( idx+1 );
    coords->point.set1Value( idx, pos.x, pos.y, pos.z );
    lineset->numVertices.setValue( size() );
}


void visBase::PolyLine::insertPoint( int idx, const Geometry::Pos& pos )
{
    if ( idx==size() ) return addPoint( pos );

    coords->point.insertSpace( idx, 1 );
    coords->point.set1Value( idx, pos.x, pos.y, pos.z );
    lineset->numVertices.setValue( size() );
}


Geometry::Pos visBase::PolyLine::getPoint( int idx ) const 
{
    if ( idx<size() && idx>=0 )
    {
	SbVec3f pos = coords->point[idx];
	Geometry::Pos res ( pos[0], pos[1], pos[2] );
	return res;
    }

    return Geometry::Pos( mUndefValue, mUndefValue, mUndefValue );
}


void visBase::PolyLine::removePoint( int idx )
{
    if ( idx<size() && idx>=0 )
    {
	coords->point.deleteValues( idx, 1 );
	lineset->numVertices.setValue( size() );
    }
}
