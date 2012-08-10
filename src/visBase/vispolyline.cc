/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: vispolyline.cc,v 1.26 2012-08-10 03:50:09 cvsaneesh Exp $";

#include "vispolyline.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismaterial.h"

#include "SoIndexedLineSet3D.h"
#include "SoLineSet3D.h"

#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>

mCreateFactoryEntry( visBase::PolyLine );
mCreateFactoryEntry( visBase::PolyLine3D );
mCreateFactoryEntry( visBase::IndexedPolyLine );
mCreateFactoryEntry( visBase::IndexedPolyLine3D );

namespace visBase
{

PolyLineBase::PolyLineBase( SoVertexShape* node )
    : VertexShape( node )
    , numvertices_( 0 )
{ }


int PolyLineBase::size() const { return coords_->size(); }


void PolyLineBase::addPoint( const Coord3& pos )
{
    setPoint( size(), pos );
}


void PolyLineBase::setPoint(int idx, const Coord3& pos )
{
    if ( idx>size() ) return;
    coords_->setPos( idx, pos );
    numvertices_->setValue( size() );
}


Coord3 PolyLineBase::getPoint( int idx ) const 
{ return coords_->getPos( idx ); }


void PolyLineBase::removePoint( int idx )
{
    numvertices_->setValue( size()-1 );
    for ( int idy=idx; idy<size()-1; idy++ )
    {
	coords_->setPos( idy, coords_->getPos( idy+1 ) );
    }

    coords_->removePos( size()-1 );
}


PolyLine::PolyLine()
    : PolyLineBase( new SoLineSet )
    , lineset_( dynamic_cast<SoLineSet*>( shape_ ) )
    , drawstyle_(0)
{
    numvertices_ = &lineset_->numVertices;
}



void PolyLine::setLineStyle( const LineStyle& lst )
{
    if ( !drawstyle_ ) 
    {
	drawstyle_ = DrawStyle::create();
	insertNode( drawstyle_->getInventorNode() );
    }

    drawstyle_->setLineStyle( lst );
    if ( getMaterial() )
	getMaterial()->setColor( lst.color_ );
}


const LineStyle& PolyLine::lineStyle() const
{
    if ( drawstyle_ )
	return drawstyle_->lineStyle();

    static LineStyle ls;
    ls.color_ = getMaterial()->getColor();
    return ls;
}


PolyLine3D::PolyLine3D()
    : PolyLineBase( new SoLineSet3D )
    , lineset_( dynamic_cast<SoLineSet3D*>( shape_ ) )
{
    numvertices_ = &lineset_->numVertices;
}



void PolyLine3D::setLineStyle( const LineStyle& lst )
{
    lineset_->radius = lst.width_*0.5f;
	//divided by 2 just like evry other radius in visBase
    getMaterial()->setColor( lst.color_ );
}


const LineStyle& PolyLine3D::lineStyle() const
{
    static LineStyle ls;
    ls.width_ = (int)(2*lineset_->radius.getValue());
    ls.color_ = getMaterial()->getColor();
    return ls;
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
