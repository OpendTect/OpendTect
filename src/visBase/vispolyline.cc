/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Apr 2002
 RCS:           $Id: vispolyline.cc,v 1.14 2008-05-19 21:15:03 cvskris Exp $
________________________________________________________________________

-*/

#include "vispolyline.h"
#include "viscoord.h"
#include "visdrawstyle.h"

#include "SoIndexedLineSet3D.h"

#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>

mCreateFactoryEntry( visBase::PolyLine );
mCreateFactoryEntry( visBase::IndexedPolyLine );
mCreateFactoryEntry( visBase::IndexedPolyLine3D );

namespace visBase
{

PolyLine::PolyLine()
    : VertexShape( new SoLineSet )
    , lineset( dynamic_cast<SoLineSet*>( shape_ ) )
    , drawstyle_(0)

{ }


int PolyLine::size() const { return coords_->size(); }


void PolyLine::setLineStyle( const LineStyle& lst )
{
    if ( !drawstyle_ ) 
    {
	drawstyle_ = DrawStyle::create();
	insertNode( drawstyle_->getInventorNode() );
    }

    drawstyle_->setLineStyle( lst );
}

void PolyLine::addPoint( const Coord3& pos )
{
    setPoint( size(), pos );
}


void PolyLine::setPoint(int idx, const Coord3& pos )
{
    if ( idx>size() ) return;
    coords_->setPos( idx, pos );
    lineset->numVertices.setValue( size() );
}


Coord3 PolyLine::getPoint( int idx ) const 
{ return coords_->getPos( idx ); }


void PolyLine::removePoint( int idx )
{
    lineset->numVertices.setValue( size()-1 );
    for ( int idy=idx; idy<size()-1; idy++ )
    {
	coords_->setPos( idy, coords_->getPos( idy+1 ) );
    }

    coords_->removePos( size()-1 );
}


IndexedPolyLine::IndexedPolyLine()
    : IndexedShape( new SoIndexedLineSet )
{ }


IndexedPolyLine3D::IndexedPolyLine3D()
    : IndexedShape( new SoIndexedLineSet3D )
{ }


float IndexedPolyLine3D::getRadius() const
{
    return reinterpret_cast<SoIndexedLineSet3D*>(shape_)->radius.getValue();
}


void IndexedPolyLine3D::setRadius(float nv,bool fixedonscreen)
{
    reinterpret_cast<SoIndexedLineSet3D*>(shape_)->radius.setValue(nv);
    reinterpret_cast<SoIndexedLineSet3D*>(shape_)->screenSize.setValue(fixedonscreen);
}

}; // namespace visBase
