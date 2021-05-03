/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/



#include "visvolorthoslice.h"

#include "iopar.h"
#include "visdepthtabplanedragger.h"
#include "vistexturerect.h"
#include "vistexturechannels.h"

#include <osgGeo/LayeredTexture>

mCreateFactoryEntry( visBase::OrthogonalSlice );

namespace visBase
{

OrthogonalSlice::OrthogonalSlice()
    : VisualObjectImpl( false )
    , slice_( TextureRectangle::create() )
    , dragger_( DepthTabPlaneDragger::create() )
    , motion(this)
    , xdatasz_(0), ydatasz_(0), zdatasz_(0)
    , curdim_( 0 )
{
    dragger_->ref();
    dragger_->setMaterial(0);
    dragger_->removeScaleTabs();
    dragger_->motion.notify( mCB(this,OrthogonalSlice,draggerMovementCB) );
    addChild( dragger_->osgNode() );
    addChild( slice_->osgNode() );

    slice_->swapTextureAxes();
    slice_->setPickable( false );

    for ( int dim=0; dim<3; dim++ )
	slicenr_[dim] = 0;
    
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
    dragger_->setSpaceLimits( x, y, z );
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
{ return curdim_; }


void OrthogonalSlice::setDim( int dim )
{
    if ( dim<0 || dim>2 )
	return;

    TextureChannels* channels = slice_->getTextureChannels();
    if ( channels )
    {
	const osgGeo::ImageDataOrder dataorder = dim==2 ? osgGeo::TRS :
						 dim==1 ? osgGeo::SRT :
						 osgGeo::STR;

	osgGeo::LayeredTexture* laytex = channels->getOsgTexture();

	for ( int channel=0; channel<channels->nrChannels(); channel++ )
	{
	    const TypeSet<int>& osgids = *channels->getOsgIDs( channel );
	    for ( int idx=0; laytex && idx<osgids.size(); idx++ )
		laytex->setDataLayerImageOrder( osgids[idx], dataorder );
	}
    }

    curdim_ = dim;

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


void OrthogonalSlice::setSliceNr( int nr, int dim )
{
    if ( dim<0 || dim>2 )
	dim = curdim_;

    TextureChannels* channels = slice_->getTextureChannels();
    if ( channels && dim==curdim_ )
    {
	osgGeo::LayeredTexture* laytex = channels->getOsgTexture();

	for ( int channel=0; channel<channels->nrChannels(); channel++ )
	{
	    const TypeSet<int>& osgids = *channels->getOsgIDs( channel );
	    for ( int idx=0; laytex && idx<osgids.size(); idx++ )
		laytex->setDataLayerSliceNr( osgids[idx], nr );
	}
    }

    slicenr_[dim] = nr;
}


int OrthogonalSlice::getSliceNr( int dim ) const
{ return dim<0 || dim>2 ? slicenr_[dim] : slicenr_[curdim_]; }


NotifierAccess& OrthogonalSlice::dragStart()
{ return dragger_->started; }


NotifierAccess& OrthogonalSlice::dragFinished()
{ return dragger_->finished; }


void OrthogonalSlice::draggerMovementCB( CallBacker* cb )
{
    float draggerpos = (float) dragger_->center()[curdim_];

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

    slice_->setCenter( dragger_->center() );
    Coord3 width = dragger_->size();
    width[curdim_] = 0.0;
    slice_->setWidth( width );

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


void OrthogonalSlice::enablePicking( bool yn )
{
    slice_->setPickable( yn );
}


bool OrthogonalSlice::isPickingEnabled() const
{ return slice_->isPickable(); }


void OrthogonalSlice::setTextureChannels( visBase::TextureChannels* channels )
{
    slice_->setTextureChannels( channels );
    setDim( curdim_ );
    setSliceNr( slicenr_[curdim_], curdim_ );
}


visBase::TextureChannels* OrthogonalSlice::getTextureChannels()
{
    return slice_->getTextureChannels();
}


void OrthogonalSlice::setDisplayTransformation( const mVisTrans* trans )
{
    slice_->setDisplayTransformation( trans );
    if ( dragger_ )
	dragger_->setDisplayTransformation( trans );
}


const mVisTrans* OrthogonalSlice::getDisplayTransformation() const
{
    return slice_->getDisplayTransformation();
}


} // namespace visBase
