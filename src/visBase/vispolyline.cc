/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vispolyline.cc,v 1.19 2011-02-22 19:53:41 cvskris Exp $";

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
    return ((SoIndexedLineSet3D*) shape_)->radius.getValue();
}


void IndexedPolyLine3D::setRadius(float nv,bool fixedonscreen,float maxdisplaysize)
{
    ((SoIndexedLineSet3D*) shape_)->radius.setValue(nv);
    ((SoIndexedLineSet3D*) shape_)->screenSize.setValue(fixedonscreen);
    ((SoIndexedLineSet3D*) shape_)->maxRadius.setValue(maxdisplaysize);
}


void IndexedPolyLine3D::setRightHandSystem( bool yn )
{
    //((SoIndexedLineSet3D*) shape_)->rightHandSystem.setValue( yn );
}


bool IndexedPolyLine3D::isRightHandSystem() const
//{ return ((SoIndexedLineSet3D*) shape_)->rightHandSystem.getValue(); }
{ return true; }


}; // namespace visBase
