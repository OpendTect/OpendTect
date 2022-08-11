/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2005
________________________________________________________________________

-*/

#include "visimagedisplay.h"

#include "odimage.h"
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
    , rgbimage_(0)
{
    group_->ref();
    addChild( group_->osgNode() );
}


ImageDisplay::~ImageDisplay()
{
    removeChild( group_->osgNode() );
    group_->unRef();
    if ( rgbimage_ )
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
	mDynamicCastGet(const visBase::ImageRect*,imagerect,group_->getObject(idx));
	if ( imagerect && evi.pickedobjids.isPresent(imagerect->id()) )
	    return idx;
    }

    return -1;
}


void ImageDisplay::setPosition( int idx, const Pick::Location& pick, bool add )
{
    const int size = set_ ? set_->disp_.pixsize_ : 100;

    const Coord3 topleft(-size,0,size);
    const Coord3 bottomright(size,0,-size);

    mDynamicCastGet(visBase::ImageRect*,imagerect,group_->getObject(idx));
    if ( imagerect )
    {
	imagerect->setRGBImage( *rgbimage_ );
	imagerect->setPick( pick );
	imagerect->setCornerPos( topleft, bottomright );
    }
    else
    {
	visBase::ImageRect* imgrect =
	    static_cast<visBase::ImageRect*>( createLocation() );

	if ( !rgbimage_ )
	    return;

	imgrect->setRGBImage( *rgbimage_ );
	imgrect->setPick( pick );
	imgrect->setCornerPos( topleft, bottomright );
	group_->addObject( imgrect );
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
    const int size = set_ ? set_->disp_.pixsize_ : 100;

    const Coord3 topleft(-size,0,size);
    const Coord3 bottomright(size,0,-size);

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::ImageRect*,imagerect,group_->getObject(idx));
	if ( imagerect )
	    imagerect->setCornerPos( topleft, bottomright );
    }
}

} //namespace visSurvey
