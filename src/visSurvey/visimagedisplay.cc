/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visimagedisplay.h"

#include "pickset.h"

#include "visevent.h"
#include "viscoord.h"
#include "vistexturecoords.h"
#include "vistransform.h"

namespace visSurvey
{

mCreateFactoryEntry( ImageDisplay );

// ImageDisplay
ImageDisplay::ImageDisplay()
    : group_(new visBase::DataObjectGroup)
    , needFileName(this)
{
    group_->ref();
    addChild( group_->osgNode() );
}


ImageDisplay::~ImageDisplay()
{
    removeChild( group_->osgNode() );
    group_->unRef();
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
    setImageDataFromFile( fnm );
    return true;
}


void ImageDisplay::setImageDataFromFile( const char* fnm )
{
    visBase::ImageRect* tmpimg = visBase::ImageRect::create();
    tmpimg->ref();
    tmpimg->setFileName( fnm );
    imagedata_ = tmpimg->getImageData();

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::ImageRect*,image,group_->getObject(idx));
	if ( image )
	    image->setImageData( imagedata_ );
    }
    
    tmpimg->unRef();
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
    const FixedString fnm = getFileName();
    if ( fnm.isEmpty() )
	const_cast<ImageDisplay*>(this)->needFileName.trigger();

    visBase::ImageRect* image = visBase::ImageRect::create();
    image->setDisplayTransformation( displaytransform_ );
    return image;
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
	mDynamicCastGet(visBase::ImageRect*,image,group_->getObject(idx));
	if ( image && evi.pickedobjids.isPresent(image->id()) )
	    return idx;
    }

    return -1;
}


void ImageDisplay::setPosition( int idx, const Pick::Location& pick )
{
    const int size = set_ ? set_->disp_.pixsize_ : 100;

    const Coord3 topleft(-size,0,size);
    const Coord3 bottomright(size,0,-size);

    mDynamicCastGet(visBase::ImageRect*,image,group_->getObject(idx));
    if ( image )
    {
	image->setCenterPos( pick.pos_ );
	image->setImageData( imagedata_ );
	image->setCornerPos( topleft, bottomright );
    }
    else
    {
	visBase::ImageRect* img =
	    static_cast<visBase::ImageRect*>( createLocation() );
	img->setCenterPos( pick.pos_ );
	img->setImageData( imagedata_ );
	img->setCornerPos( topleft, bottomright );
	group_->addObject( img );
    }
}


void ImageDisplay::removePosition( int idx )
{
    if ( idx >= group_->size() )
	return;

    group_->removeObject( idx );
}


void ImageDisplay::updateCoords(CallBacker*)
{
    //const float zscale = scene_ ? scene_->getZScale() : 1; TODO:
    const int size = set_ ? set_->disp_.pixsize_ : 100;

    const Coord3 topleft(-size,0,size);
    const Coord3 bottomright(size,0,-size);

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::ImageRect*,image,group_->getObject(idx));
	if ( image )
	    image->setCornerPos( topleft, bottomright );
    }
}

} //namespace visSurvey
