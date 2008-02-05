/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          August 2006
 RCS:           $Id: vismarchingcubessurface.cc,v 1.12 2008-02-05 22:02:43 cvskris Exp $
________________________________________________________________________

-*/

#include "vismarchingcubessurface.h"

#include "explicitmarchingcubes.h"
#include "marchingcubes.h"
#include "visgeomindexedshape.h"

#include <Inventor/nodes/SoShapeHints.h>

mCreateFactoryEntry( visBase::MarchingCubesSurface );

namespace visBase
{

MarchingCubesSurface::MarchingCubesSurface()
    : VisualObjectImpl( true )
    , hints_( new SoShapeHints )
    , side_( 0 )
    , surface_( new ExplicitMarchingCubesSurface( 0 ) )
    , shape_( GeomIndexedShape::create() )
{
    addChild( hints_ );

    shape_->ref();
    shape_->removeSwitch();
    addChild( shape_->getInventorNode() );
    shape_->setSelectable( false );

    shape_->setSurface( surface_ );
    shape_->setMaterial( 0 );
    renderOneSide( 0 );
}


MarchingCubesSurface::~MarchingCubesSurface()
{
    shape_->unRef();

    delete surface_;
}


void MarchingCubesSurface::setRightHandSystem( bool yn )
{ shape_->setRightHandSystem( yn ); }


void MarchingCubesSurface::renderOneSide( int side )
{
    side_ = side;
    updateHints();
}


void MarchingCubesSurface::updateHints() 
{
    hints_->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;

    if ( side_==0 )
    {
	hints_->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    }
    else if ( side_==1 )
    {
	hints_->shapeType = SoShapeHints::SOLID;
    }
    else
    {
	hints_->shapeType = SoShapeHints::SOLID;
    }
}


void MarchingCubesSurface::setSurface( ::MarchingCubesSurface& ns )
{
    surface_->setSurface( &ns );
    touch( true );
}


void MarchingCubesSurface::touch( bool forall, TaskRunner* tr )
{ shape_->touch( forall, tr ); }


::MarchingCubesSurface* MarchingCubesSurface::getSurface()
{ return surface_->getSurface(); }


const ::MarchingCubesSurface* MarchingCubesSurface::getSurface() const
{ return surface_->getSurface(); }


void MarchingCubesSurface::setScales(const SamplingData<float>& xrg,
				     const SamplingData<float>& yrg,
				     const SamplingData<float>& zrg)
{ surface_->setAxisScales( xrg, yrg, zrg ); }


const SamplingData<float>& MarchingCubesSurface::getScale( int dim ) const
{
    return surface_->getAxisScale( dim );
}
    

}; // namespace visBase
