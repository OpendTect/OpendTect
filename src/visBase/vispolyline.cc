/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vispolyline.cc,v 1.8 2004-07-23 12:57:22 kristofer Exp $";

#include "vispolyline.h"

#include "viscoord.h"

#include "SoIndexedPolyLine3D.h"

#include "Inventor/nodes/SoLineSet.h"
#include "Inventor/nodes/SoIndexedLineSet.h"

mCreateFactoryEntry( visBase::PolyLine );
mCreateFactoryEntry( visBase::IndexedPolyLine );
mCreateFactoryEntry( visBase::IndexedPolyLine3D );

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


visBase::IndexedPolyLine3D::IndexedPolyLine3D()
    : IndexedShape( new SoIndexedPolyLine3D )
{ }


float visBase::IndexedPolyLine3D::getRadius() const
{
    return reinterpret_cast<SoIndexedPolyLine3D*>(shape)->radius.getValue();
}


void visBase::IndexedPolyLine3D::setRadius(float nv)
{
    reinterpret_cast<SoIndexedPolyLine3D*>(shape)->radius.setValue(nv);
}
