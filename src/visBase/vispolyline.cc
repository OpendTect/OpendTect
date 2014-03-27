/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vispolyline.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismaterial.h"

#include <osgGeo/PolyLine>
#include <osg/Node>
#include <osg/Geode>

mCreateFactoryEntry( visBase::PolyLine );
mCreateFactoryEntry( visBase::PolyLine3D );

namespace visBase
{

PolyLine::PolyLine()
    : VertexShape( Geometry::PrimitiveSet::LineStrips, true )
    , coordrange_( Geometry::RangePrimitiveSet::create() )
    , drawstyle_( 0 )
{
    addPrimitiveSet( coordrange_ );
    coordrange_->ref();
}


PolyLine::~PolyLine()
{ coordrange_->unRef(); }


int PolyLine::size() const { return coords_->size(); }


void PolyLine::addPoint( const Coord3& pos )
{
    setPoint( size(), pos );
}


void PolyLine::setPoint(int idx, const Coord3& pos )
{
    if ( idx>size() ) return;
    coords_->setPos( idx, pos );
    coordrange_->setRange( Interval<int>( 0, size()-1 ) );
}


Coord3 PolyLine::getPoint( int idx ) const
{ return coords_->getPos( idx ); }


void PolyLine::removePoint( int idx )
{
    coordrange_->setRange( Interval<int>( 0, size()-2 ) );
    for ( int idy=idx; idy<size()-1; idy++ )
    {
	coords_->setPos( idy, coords_->getPos( idy+1 ) );
    }

    coords_->removePos( size()-1 );
}


void PolyLine::removeAllPoints()
{
   for ( int idx=size()-1; idx>=0; idx-- )
       removePoint( idx );
}


void PolyLine::setLineStyle( const LineStyle& lst )
{
    if ( !drawstyle_ )
	drawstyle_ = addNodeState( new DrawStyle );

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


void PolyLine::setDisplayTransformation( const mVisTrans* trans)
{
    VertexShape::setDisplayTransformation( trans );
}


PolyLine3D::PolyLine3D()
    : VertexShape( Geometry::PrimitiveSet::Other, false )
{
    node_ = osgpoly_ = new osgGeo::PolyLineNode;
    osgpoly_->ref();
    setOsgNode( node_ );
    osgpoly_->setVertexArray( coords_->osgArray() );
}


void PolyLine3D::setLineStyle( const LineStyle& lst )
{
    lst_ = lst_;
    osgpoly_->setRadius( lst.width_ );
}


void PolyLine3D::setCoordinates( Coordinates* coords )
{
    VertexShape::setCoordinates( coords );
    if ( coords && coords->osgArray() )
        osgpoly_->setVertexArray( coords->osgArray() );
}


void PolyLine3D::setResolution( int res )
{
    osgpoly_->setResolution( res );
}


int PolyLine3D::getResolution() const
{
    return osgpoly_->getResolution();
}


const LineStyle& PolyLine3D::lineStyle() const
{
    return lst_;
}


void PolyLine3D::addPrimitiveSetToScene( osg::PrimitiveSet* ps )
{
    osgpoly_->addPrimitiveSet( ps );
}


void PolyLine3D::removePrimitiveSetFromScene( const osg::PrimitiveSet* ps )
{
    const int idx = osgpoly_->getPrimitiveSetIndex( ps );
    osgpoly_->removePrimitiveSet( idx );
}


void PolyLine3D::touchPrimitiveSet( int idx )
{
    osgpoly_->touchPrimitiveSet( idx );
}


void PolyLine3D::setDisplayTransformation( const mVisTrans* trans)
{
    VertexShape::setDisplayTransformation( trans );
    setCoordinates( coords_ );
}

}; // namespace visBase
