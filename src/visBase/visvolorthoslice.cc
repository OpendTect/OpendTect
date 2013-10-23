/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "visvolorthoslice.h"

#include "iopar.h"
#include "visdepthtabplanedragger.h"

/* OSG-TODO: Port SoOrthoSlice slice_ to OSG if this class is prolongated */

mCreateFactoryEntry( visBase::OrthogonalSlice );

namespace visBase
{

OrthogonalSlice::OrthogonalSlice()
    : VisualObjectImpl( false )
//    , slice_(new SoOrthoSlice)
    , dragger_(DepthTabPlaneDragger::create())
    , motion(this)
    , xdatasz_(0), ydatasz_(0), zdatasz_(0)
{
    dragger_->ref();
    dragger_->setMaterial(0);
    dragger_->removeScaleTabs();
    dragger_->motion.notify( mCB(this,OrthogonalSlice,draggerMovementCB) );
    addChild( dragger_->osgNode() );
    
//    slice_->alphaUse = SoOrthoSlice::ALPHA_AS_IS;
    
//    addChild( slice_ );
}


OrthogonalSlice::~OrthogonalSlice()
{
    dragger_->motion.remove( mCB(this, OrthogonalSlice, draggerMovementCB ));
    dragger_->unRef();
}


void OrthogonalSlice::setVolumeDataSize(int xsz, int ysz, int zsz)
{
    xdatasz_ = xsz; ydatasz_ = ysz; zdatasz_ = zsz;
    draggerMovementCB(0);
}


void OrthogonalSlice::setSpaceLimits( const Interval<float>& x,
				      const Interval<float>& y,
				      const Interval<float>& z )
{
    dragger_->setSpaceLimits( x,y,z );
    dragger_->setCenter( Coord3(x.center(),y.center(),z.center()) );
    dragger_->setSize( Coord3(x.width(),y.width(),z.width()) );
    draggerMovementCB(0);
}


void OrthogonalSlice::setCenter( const Coord3& newcenter, bool alldims )
{
    dragger_->setCenter( newcenter, alldims );
    draggerMovementCB(0);
}


visBase::DepthTabPlaneDragger* OrthogonalSlice::getDragger() const
{
    return dragger_;
}


int OrthogonalSlice::getDim() const
{
    return 0;
//    return slice_->axis.getValue();
}


void OrthogonalSlice::setDim( int dim )
{
/*
    if ( !dim )
	slice_->axis = SoOrthoSlice::X;
    else if ( dim==1 )
	slice_->axis = SoOrthoSlice::Y;
    else
	slice_->axis = SoOrthoSlice::Z;
*/
    dragger_->setDim( dim );
    draggerMovementCB(0);
}


float OrthogonalSlice::getPosition() const
{
    int nrslices;
    Interval<float> range;
    getSliceInfo( nrslices, range );

    if ( !nrslices ) return range.center();
    return (float)getSliceNr()/nrslices*range.width()+range.start;
}


void OrthogonalSlice::setSliceNr( int nr )
{
// slice_->sliceNumber = nr;
}

int  OrthogonalSlice::getSliceNr() const
{
    return 0;
//    return slice_->sliceNumber.getValue();
}


NotifierAccess& OrthogonalSlice::dragStart()
{ return dragger_->started; }


NotifierAccess& OrthogonalSlice::dragFinished()
{ return dragger_->finished; }


void OrthogonalSlice::draggerMovementCB( CallBacker* cb )
{
    const int dim = getDim();
    float draggerpos = (float) dragger_->center()[dim];

    int nrslices;
    Interval<float> range;
    getSliceInfo( nrslices, range );
    if ( !nrslices ) return;

    float slicenrf = (draggerpos-range.start)/range.width()*(nrslices-1);
    int slicenr = mNINT32(slicenrf);
    if ( slicenr>=nrslices ) slicenr=nrslices-1;
    else if ( slicenr<0 ) slicenr=0;

    if ( slicenr != getSliceNr() )
	setSliceNr( slicenr );

    if ( cb )
	motion.trigger();
}


void OrthogonalSlice::getSliceInfo( int& nrslices, Interval<float>& range) const
{
    Interval<float> xrange, yrange, zrange;
    dragger_->getSpaceLimits( xrange, yrange, zrange );

    const int dim = getDim();
    if ( dim == 0 )
    {
	nrslices = xdatasz_;
	range = xrange;
    }
    else if ( dim == 1 )
    {
	nrslices = ydatasz_;
	range = yrange;
    }
    else
    {
	nrslices = zdatasz_;
	range = zrange;
    }
}


void OrthogonalSlice::removeDragger()
{
    if ( dragger_ )
    {
	dragger_->motion.remove(mCB(this, OrthogonalSlice, draggerMovementCB));
	removeChild( dragger_->osgNode() );
	dragger_->unRef();
	dragger_ = 0;
    }
}


} // namespace visBase
