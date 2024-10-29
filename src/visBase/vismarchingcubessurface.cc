/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismarchingcubessurface.h"

#include "explicitmarchingcubes.h"
#include "marchingcubes.h"
#include "samplingdata.h"
#include "viscoord.h"
#include "visgeomindexedshape.h"
#include "vistransform.h"

mCreateFactoryEntry( visBase::MarchingCubesSurface );

namespace visBase
{

MarchingCubesSurface::MarchingCubesSurface()
    : VisualObjectImpl(true)
    , surface_(new ExplicitMarchingCubesSurface(0))
    , sectionlocation_( mUdf(float) )
    , xrg_( mUdf(float), mUdf(float), 0 )
    , yrg_( mUdf(float), mUdf(float), 0 )
    , zrg_( mUdf(float), mUdf(float), 0 )
{
    ref();
    shape_ = GeomIndexedShape::create();
    addChild( shape_->osgNode() );
    shape_->setSelectable( false );
    shape_->setMaterial( nullptr );
    shape_->useOsgNormal( true );

    setRenderMode( RenderBothSides );
    unRefNoDelete();
}


MarchingCubesSurface::~MarchingCubesSurface()
{
    surface_->removeAll( false );
    delete surface_;
}


void MarchingCubesSurface::setRightHandSystem( bool yn )
{ shape_->setRightHandSystem( yn ); }


void MarchingCubesSurface::setRenderMode( RenderMode mode )
{
    shape_->setRenderMode( mode );
}


bool MarchingCubesSurface::setSurface( ::MarchingCubesSurface& ns,
       TaskRunner* tr	)
{
    surface_->setSurface( &ns );
    shape_->setSurface( surface_,tr );
    return touch( true, tr );
}


bool MarchingCubesSurface::touch( bool forall, TaskRunner* tr )
{ return shape_->touch( forall, tr ); }


::MarchingCubesSurface* MarchingCubesSurface::getSurface()
{ return surface_->getSurface(); }


const ::MarchingCubesSurface* MarchingCubesSurface::getSurface() const
{ return surface_->getSurface(); }


void MarchingCubesSurface::setScales(const SamplingData<float>& xrg,
				     const SamplingData<float>& yrg,
				     const SamplingData<float>& zrg)
{
    xrg_.start_ = xrg.start_; xrg_.step_ = xrg.step_;
    yrg_.start_ = yrg.start_; yrg_.step_ = yrg.step_;
    zrg_.start_ = zrg.start_; zrg_.step_ = zrg.step_;

    updateDisplayRange();
}

void MarchingCubesSurface::enableSection( char sec )
{
    if ( displaysection_==sec )
	return;

    displaysection_ = sec;
    updateDisplayRange();
}


char MarchingCubesSurface::enabledSection() const
{ return displaysection_; }


void MarchingCubesSurface::setSectionPosition( float pos )
{
    if ( sectionlocation_==pos )
	return;

    sectionlocation_ = pos;
    if ( displaysection_>=0 )
	updateDisplayRange();
}


float MarchingCubesSurface::getSectionPosition()
{
    return sectionlocation_;
}


void MarchingCubesSurface::setBoxBoundary( float maxx, float maxy, float maxz )
{
    if ( xrg_.stop_==maxx && yrg_.stop_==maxy && zrg_.stop_==maxz )
	return;

    xrg_.stop_ = maxx;
    yrg_.stop_ = maxy;
    zrg_.stop_ = maxz;
    if ( displaysection_>=0 )
	updateDisplayRange();
}


void MarchingCubesSurface::setDisplayTransformation( const mVisTrans* trans )
{
   transform_ = trans;
    shape_->setDisplayTransformation( trans );

}


const mVisTrans* MarchingCubesSurface::getDisplayTransformation() const
{
    return transform_.ptr();
}


void MarchingCubesSurface::getTransformCoord( Coord3& pos )
{
    Coord3 postoset = pos;
    if ( postoset.isDefined() )
	Transformation::transform( transform_.ptr(), postoset );

    pos =  postoset;
}

void MarchingCubesSurface::updateDisplayRange()
{

    if ( mIsUdf(sectionlocation_) || mIsUdf(xrg_.start_) ||
	 mIsUdf(yrg_.start_) || mIsUdf(zrg_.start_) || mIsUdf(xrg_.stop_) ||
	 mIsUdf(yrg_.stop_) || mIsUdf(zrg_.stop_) )
	return;

    if ( !displaysection_ )
    {
	if ( sectionlocation_>xrg_.stop_ )
	    xrg_.start_ = xrg_.stop_;
	else if ( sectionlocation_>xrg_.start_ )
	    xrg_.start_ = sectionlocation_;

	xrg_.step_ = 0;
    }
    else if ( displaysection_==1 )
    {
	if ( sectionlocation_>yrg_.stop_ )
	    yrg_.start_ = yrg_.stop_;
	else if ( sectionlocation_>yrg_.start_ )
	    yrg_.start_ = sectionlocation_;

	yrg_.step_ = 0;
    }
    else if ( displaysection_==2 )
    {
	if ( sectionlocation_>zrg_.stop_ )
	    zrg_.start_ = zrg_.stop_;
	else if ( sectionlocation_>zrg_.start_ )
	    zrg_.start_ = sectionlocation_;

	zrg_.step_ = 0;
    }

}


const SamplingData<float> MarchingCubesSurface::getScale( int dim ) const
{
    if ( dim == 0 ) return SamplingData<float>( xrg_);
    else if ( dim == 1) return SamplingData<float> ( yrg_ );
    else if ( dim == 2) return SamplingData<float> ( zrg_ );
    else return SamplingData<float>( 0, 1 );
}


} // namespace visBase
