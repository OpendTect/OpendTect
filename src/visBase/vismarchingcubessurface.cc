/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "vismarchingcubessurface.h"

#include "explicitmarchingcubes.h"
#include "marchingcubes.h"
#include "samplingdata.h"
#include "viscoord.h"
#include "visgeomindexedshape.h"

#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoMaterialBinding.h>

mCreateFactoryEntry( visBase::MarchingCubesSurface );

namespace visBase
{

MarchingCubesSurface::MarchingCubesSurface()
    : VisualObjectImpl( true )
    , surface_( new ExplicitMarchingCubesSurface( 0 ) )
    , shape_( GeomIndexedShape::create() )
    , displaysection_( -1 )
    , sectionlocation_( mUdf(float) )    
    , xrg_( mUdf(float), mUdf(float), 0 )
    , yrg_( mUdf(float), mUdf(float), 0 )
    , zrg_( mUdf(float), mUdf(float), 0 )
{
    SoMaterialBinding* materialbinding = new SoMaterialBinding;
    addChild( materialbinding );
    materialbinding->value = SoMaterialBinding::OVERALL;

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
    surface_->removeAll( false );
    delete surface_;
}


void MarchingCubesSurface::setRightHandSystem( bool yn )
{ shape_->setRightHandSystem( yn ); }


void MarchingCubesSurface::renderOneSide( int side )
{
    shape_->renderOneSide( side );
}


void MarchingCubesSurface::setSurface( ::MarchingCubesSurface& ns,
       TaskRunner* tr	)
{
    surface_->setSurface( &ns );
    touch( true, tr );
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
{
    xrg_.start = xrg.start; xrg_.step = xrg.step;
    yrg_.start = yrg.start; yrg_.step = yrg.step;
    zrg_.start = zrg.start; zrg_.step = zrg.step;

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
    if ( xrg_.stop==maxx && yrg_.stop==maxy && zrg_.stop==maxz )
	return;

     xrg_.stop = maxx;
     yrg_.stop = maxy;
     zrg_.stop = maxz;
    if ( displaysection_>=0 )    
	updateDisplayRange();
}


void MarchingCubesSurface::updateDisplayRange()
{
    if ( displaysection_==-1 )
    {    
	surface_->setAxisScales( SamplingData<float>(xrg_), 
		SamplingData<float>(yrg_), SamplingData<float>(zrg_) );   
	return;
    }

    if ( mIsUdf(sectionlocation_) || mIsUdf(xrg_.start) || 
	 mIsUdf(yrg_.start) || mIsUdf(zrg_.start) || mIsUdf(xrg_.stop) ||
	 mIsUdf(yrg_.stop) || mIsUdf(zrg_.stop) )
	return;
    
    if ( !displaysection_ )
    {
	if ( sectionlocation_>xrg_.stop )
    	    xrg_.start = xrg_.stop;
	else if ( sectionlocation_>xrg_.start )
	    xrg_.start = sectionlocation_;

	xrg_.step = 0;
    }
    else if ( displaysection_==1 )
    {
	if ( sectionlocation_>yrg_.stop )
    	    yrg_.start = yrg_.stop;
	else if ( sectionlocation_>yrg_.start )
	    yrg_.start = sectionlocation_;

	yrg_.step = 0;
    }
    else if ( displaysection_==2 )
    {
	if ( sectionlocation_>zrg_.stop )
    	    zrg_.start = zrg_.stop;
	else if ( sectionlocation_>zrg_.start )
	    zrg_.start = sectionlocation_;

	zrg_.step = 0;
    }

    surface_->setAxisScales( SamplingData<float>(xrg_), 
	    SamplingData<float>(yrg_), SamplingData<float>(zrg_) );
}


const SamplingData<float>& MarchingCubesSurface::getScale( int dim ) const
{
    return surface_->getAxisScale( dim );
}


}; // namespace visBase
