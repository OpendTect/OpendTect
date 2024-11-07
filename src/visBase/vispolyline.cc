/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vispolyline.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismaterial.h"

#include <osgGeo/PolyLine>
#include <osg/Node>
#include <osg/Geode>
#include <osg/LineStipple>

mCreateFactoryEntry( visBase::PolyLine );
mCreateFactoryEntry( visBase::PolyLine3D );

namespace visBase
{

PolyLine::PolyLine()
    : VertexShape(Geometry::PrimitiveSet::LineStrips,true)
{
    ref();
    coordrange_ = Geometry::RangePrimitiveSet::create();
    addPrimitiveSet( coordrange_.ptr() );
    unRefNoDelete();
}


PolyLine::~PolyLine()
{
}


int PolyLine::size() const { return coords_->size(); }


void PolyLine::addPoint( const Coord3& pos )
{
    setPoint( size(), pos );
}


void PolyLine::setPoint(int idx, const Coord3& pos )
{
    if ( idx>size() ) return;
    coords_->setPos( idx, pos );
    if ( coordrange_ )
	coordrange_->setRange( Interval<int>( 0, size()-1 ) );
}


Coord3 PolyLine::getPoint( int idx ) const
{ return coords_->getPos( idx ); }


void PolyLine::removePoint( int idx )
{
    if ( coordrange_ )
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


void PolyLine::setLineStyle( const OD::LineStyle& lst )
{
    if ( !drawstyle_ )
    {
	RefMan<DrawStyle> drawstyle = DrawStyle::create();
	drawstyle_ = addNodeState( drawstyle.ptr() );
    }

    drawstyle_->setLineStyle( lst );
    if ( getMaterial() )
	getMaterial()->setColor( lst.color_ );

    setAttribAndMode( drawstyle_->getLineStipple() );

    if ( lst.width_ == 0 )
	turnOn( false );
    else
	turnOn( true );
}


const OD::LineStyle& PolyLine::lineStyle() const
{
    if ( drawstyle_ )
	return drawstyle_->lineStyle();

    static OD::LineStyle ls;
    ls.color_ = getMaterial()->getColor();
    return ls;
}


void PolyLine::setDisplayTransformation( const mVisTrans* trans)
{
    VertexShape::setDisplayTransformation( trans );
}


PolyLine3D::PolyLine3D()
    : VertexShape( Geometry::PrimitiveSet::Other, false )
    , pixeldensity_( getDefaultPixelDensity() )
{
    node_ = osgpoly_ = new osgGeo::PolyLineNode;
    refOsgPtr( osgpoly_ );
    setOsgNode( node_ );
    osgpoly_->setVertexArray( coords_->osgArray() );
}


PolyLine3D::~PolyLine3D()
{
    unRefOsgPtr( osgpoly_ );
}


void PolyLine3D::setLineStyle( const OD::LineStyle& lst )
{
    lst_ = lst;
    updateRadius();
}


void PolyLine3D::updateRadius()
{
    const float factor = pixeldensity_ / getDefaultPixelDensity();
    osgpoly_->setRadius( lst_.width_ * factor );
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


void PolyLine3D::setPixelDensity( float dpi )
{
    VertexShape::setPixelDensity( dpi );

    pixeldensity_ = dpi;
    updateRadius();
}


const OD::LineStyle& PolyLine3D::lineStyle() const
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


void PolyLine3D::removeAllPrimitiveSetsFromScene()
{
    osgpoly_->removeAllPrimitiveSets();
}


void PolyLine3D::touchPrimitiveSet( int idx )
{
    osgpoly_->touchPrimitiveSet( idx );
}


void PolyLine3D::setDisplayTransformation( const mVisTrans* trans)
{
    VertexShape::setDisplayTransformation( trans );
    setCoordinates( coords_.ptr() );
}




} // namespace visBase
