/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visimagedisplay.h"

#include "odimage.h"
#include "pickset.h"

#include "visevent.h"
#include "vistransform.h"

namespace visSurvey
{

mCreateFactoryEntry( ImageDisplay );

// ImageDisplay
ImageDisplay::ImageDisplay()
    : group_(new visBase::DataObjectGroup)
    , needFileName(this)
    , rgbimage_(nullptr)
{
    addChild( group_->osgNode() );
}


ImageDisplay::~ImageDisplay()
{
    removeChild( group_->osgNode() );
    delete rgbimage_;
}


void ImageDisplay::setDisplayTransformation(const mVisTrans* tf )
{
    displaytransform_ = tf;
}


const mVisTrans* ImageDisplay::getDisplayTransformation() const
{
    return displaytransform_;
}


bool ImageDisplay::setFileName( const char* fnm )
{
    imagefnm_ = fnm;
    uiString errmsg;
    OD::RGBImage* img = OD::RGBImageLoader::loadRGBImage(fnm,errmsg);
    if ( !img )
    {
	pErrMsg( errmsg.getFullString() );
	return false;
    }

    setRGBImage( img );
    updateCoords();
    return true;
}


void ImageDisplay::setRGBImage( OD::RGBImage* img )
{
    if( !img->getData() )
	return;

    rgbimage_ = img;
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::ImageRect*,image,group_->getObject(idx));
	if ( image )
	    image->setRGBImage( *rgbimage_ );
    }
}


const char* ImageDisplay::getFileName() const
{
    return imagefnm_;
}


void ImageDisplay::setSet( Pick::Set* set )
{
    LocationDisplay::setSet( set );
    updateCoords();
}


void ImageDisplay::setScene( visSurvey::Scene* scene )
{
    visSurvey::SurveyObject::setScene( scene );
    updateCoords();
}


visBase::VisualObject* ImageDisplay::createLocation() const
{
    const StringView fnm = getFileName();
    if ( fnm.isEmpty() )
	const_cast<ImageDisplay*>(this)->needFileName.trigger();

    visBase::ImageRect* imagerct = visBase::ImageRect::create();
    imagerct->setDisplayTransformation( displaytransform_ );
    return imagerct;
}


void ImageDisplay::dispChg( CallBacker* cb )
{
    LocationDisplay::dispChg( cb );
    updateCoords();
}


int ImageDisplay::clickedMarkerIndex(const visBase::EventInfo& evi)const
{
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(const visBase::ImageRect*,imagerect,
			group_->getObject(idx));
	if ( imagerect && evi.pickedobjids.isPresent(imagerect->id()) )
	    return idx;
    }

    return -1;
}


static void getScaledImageDimensions( const OD::RGBImage* image,
			       double& width, double& height, int size )
{
    const double relsize( size );
    if ( !image )
    {
	width = height = relsize;
	return;
    }

    height = sCast(double,image->getSize(false));
    width = sCast(double,image->getSize(true));
    const double factor = mMAX( height, width ) / relsize;
    height /= factor;
    width /= factor;
}


void ImageDisplay::setPosition( int idx, const Pick::Location& pick, bool add )
{
    mDynamicCastGet(visBase::ImageRect*,imagerect,group_->getObject(idx));
    if ( !imagerect )
    {
	imagerect = sCast(visBase::ImageRect*,createLocation());
	group_->addObject( imagerect );
    }

    const int size = set_ ? set_->disp_.pixsize_ : 100;
    double width, height;
    getScaledImageDimensions( rgbimage_, width, height, size );
    const Coord3 topleft(-width,0,height);
    const Coord3 bottomright(width,0,-height);
    imagerect->setCornerPos( topleft, bottomright );
    imagerect->setPick( pick );
    if ( rgbimage_ )
	imagerect->setRGBImage( *rgbimage_ );
}


void ImageDisplay::removePosition( int idx )
{
    if ( idx >= group_->size() )
	return;

    group_->removeObject( idx );
}


void ImageDisplay::updateCoords( CallBacker* )
{
    const int size = set_ ? set_->disp_.pixsize_ : 100;

    double width, height;
    getScaledImageDimensions( rgbimage_, width, height, size );
    const Coord3 topleft(-width,0,height);
    const Coord3 bottomright(width,0,-height);

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::ImageRect*,imagerect,group_->getObject(idx));
	if ( imagerect )
	    imagerect->setCornerPos( topleft, bottomright );
    }
}

} // namespace visSurvey
