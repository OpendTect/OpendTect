/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vispolyline.cc,v 1.10 2005-02-04 14:31:34 kristofer Exp $";

#include "vispolyline.h"

#include "viscoord.h"

#include "SoIndexedLineSet3D.h"

#include "Inventor/nodes/SoLineSet.h"
#include "Inventor/nodes/SoIndexedLineSet.h"

namespace visBase
{

mCreateFactoryEntry( PolyLine );
mCreateFactoryEntry( IndexedPolyLine );
mCreateFactoryEntry( IndexedPolyLine3D );

PolyLine::PolyLine()
    : VertexShape( new SoLineSet )
    , lineset( dynamic_cast<SoLineSet*>( shape ) )
{ }


int PolyLine::size() const { return coords->size(); }


void PolyLine::addPoint( const Coord3& pos )
{
    setPoint( size(), pos );
}


void PolyLine::setPoint(int idx, const Coord3& pos )
{
    if ( idx>size() ) return;
    coords->setPos( idx, pos );
    lineset->numVertices.setValue( size() );
}


Coord3 PolyLine::getPoint( int idx ) const 
{ return coords->getPos( idx ); }


void PolyLine::removePoint( int idx )
{
    lineset->numVertices.setValue( size()-1 );
    for ( int idy=idx; idy<size()-1; idy++ )
    {
	coords->setPos( idy, coords->getPos( idy+1 ) );
    }

    coords->removePos( size()-1 );
}


IndexedPolyLine::IndexedPolyLine()
    : IndexedShape( new SoIndexedLineSet )
{ }


IndexedPolyLine3D::IndexedPolyLine3D()
    : IndexedShape( new SoIndexedLineSet3D )
{ }


float IndexedPolyLine3D::getRadius() const
{
    return reinterpret_cast<SoIndexedLineSet3D*>(shape)->radius.getValue();
}


void IndexedPolyLine3D::setRadius(float nv)
{
    reinterpret_cast<SoIndexedLineSet3D*>(shape)->radius.setValue(nv);
}

}; // namespace visBase
