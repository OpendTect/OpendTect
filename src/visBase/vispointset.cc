/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vispointset.h"

#include "datapointset.h"
#include "viscoord.h"
#include "vismaterial.h"

mCreateFactoryEntry( visBase::PointSet );

namespace visBase
{

PointSet::PointSet()
    : VertexShape(Geometry::PrimitiveSet::Points,true)
{
    ref();
    RefMan<DrawStyle> drawstyle = DrawStyle::create();
    drawstyle_ = addNodeState( drawstyle.ptr() );
    // drawstyle_->setDrawStyle( DrawStyle::Points ); Gives pErrMsg
    drawstyle_->setPointSize( 5.0 );

    RefMan<Material> newmat = Material::create();
    setMaterial( newmat.ptr() );
    getMaterial()->setColorMode( Material::Diffuse );
    setColorBindType( VertexShape::BIND_PER_VERTEX );
    unRefNoDelete();
}


PointSet::~PointSet()
{
}


int PointSet::size() const
{
    return coords_->size();
}


void PointSet::setPointSize( int sz )
{
    drawstyle_->setPointSize( (float)sz );
    requestSingleRedraw();
}


int PointSet::getPointSize() const
{
    return mNINT32( drawstyle_->getPointSize() );
}


int PointSet::addPoint( const Coord3& pos )
{
     coords_->addPos( pos );
     return size()-1;
}


const Coord3 PointSet::getPoint( int idx ) const
{
    return coords_->getPos( idx );
}


const Coord3 PointSet::getPoint( int idx, bool scenespace ) const
{
    return coords_->getPos( idx, scenespace );
}


void PointSet::removeAllPoints()
{
    coords_->setEmpty();
}


void PointSet::setDisplayTransformation( const mVisTrans* trans )
{
     VertexShape::setDisplayTransformation( trans );
}


void PointSet::clear()
{
    removeAllPoints();
    removeAllPrimitiveSets();
    getMaterial()->clear();
}

} // namespace visBase
