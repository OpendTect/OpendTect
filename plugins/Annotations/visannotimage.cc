/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visannotimage.h"

#include "pickset.h"
#include "viscoord.h"
#include "vistexturecoords.h"
#include "visfaceset.h"
#include "visforegroundlifter.h"
#include "visimage.h"
#include "vistransform.h"


namespace Annotations
{

mCreateFactoryEntry( ImageDisplay );
mCreateFactoryEntry( Image );

Image::Image()
    : visBase::VisualObjectImpl( true )
    , position_( visBase::Transformation::create() )
    , transform_( 0 )
    , shape_( 0 )
    , lifter_( visBase::ForegroundLifter::create() )
{
    setMaterial( 0 );
    position_->ref();
    addChild( position_->getInventorNode() );

    lifter_->ref();
    lifter_->setLift(0.8);
    addChild( lifter_->getInventorNode() );
}


Image::~Image()
{
    shape_->unRef();
    position_->unRef();
    lifter_->unRef();
    if ( transform_ ) transform_->unRef();
}


void Image::setShape( visBase::FaceSet* fs )
{
    if ( shape_ )
    {
	removeChild( shape_->getInventorNode() );
	shape_->unRef();
    }

    shape_ = fs;

    if ( shape_ )
    {
	shape_->ref();
	addChild( shape_->getInventorNode() );
    }
}


void Image::setDisplayTransformation( const mVisTrans* trans )
{
    Coord3 pos = position_->getTranslation();
    if ( transform_ )
    {
	pos = transform_->transformBack(pos);
	transform_->unRef();
    }
    transform_ = trans;
    if ( transform_ )
    {
	transform_->ref();
	pos = transform_->transform( pos );
    }

    position_->setTranslation( pos );
}


void Image::setPick( const Pick::Location& loc )
{
    const Coord3 displaypos =
	transform_ ? transform_->transform(loc.pos_) : loc.pos_;
    position_->setTranslation( displaypos );
    position_->setRotation( Coord3(0,0,1), loc.dir_.phi );
}


// ImageDisplay
ImageDisplay::ImageDisplay()
    : shape_( visBase::FaceSet::create() )
    , image_( visBase::Image::create() )
    , needFileName( this )
{
    image_->ref();
    image_->replaceMaterial(true);
    insertChild( childIndex( group_->getInventorNode() ),
	    	image_->getInventorNode());

    shape_->ref();
    shape_->removeSwitch();
    shape_->setVertexOrdering(
		visBase::VertexShape::cCounterClockWiseVertexOrdering() );
    shape_->setSelectable( false );

    visBase::Coordinates* facecoords = shape_->getCoordinates();
    facecoords->setPos( 0, Coord3(-1,-1,0) );
    facecoords->setPos( 1, Coord3(-1,1,0) );
    facecoords->setPos( 2, Coord3(1,1,0) );
    facecoords->setPos( 3, Coord3(1,-1,0) );

    shape_->setCoordIndex(0,3);
    shape_->setCoordIndex(1,2);
    shape_->setCoordIndex(2,1);
    shape_->setCoordIndex(3,0);
    shape_->setCoordIndex(4,-1);

    visBase::TextureCoords* texturecoords = visBase::TextureCoords::create();
    shape_->setTextureCoords( texturecoords );
    texturecoords->setCoord( 0, Coord3(0,0,0) );
    texturecoords->setCoord( 1, Coord3(0,1,0) );
    texturecoords->setCoord( 2, Coord3(1,1,0) );
    texturecoords->setCoord( 3, Coord3(1,0,0) );
    shape_->setTextureCoordIndex( 0, 3 );
    shape_->setTextureCoordIndex( 1, 2 );
    shape_->setTextureCoordIndex( 2, 1 );
    shape_->setTextureCoordIndex( 3, 0 );
    shape_->setTextureCoordIndex( 4, -1 );
}


ImageDisplay::~ImageDisplay()
{
    shape_->unRef();
    image_->unRef();

    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ImageDisplay,updateCoords) );
}


bool ImageDisplay::setFileName( const char* nm )
{
    image_->setFileName( nm );
    return true;
}


const char* ImageDisplay::getFileName() const
{
    return image_->getFileName();
}


void ImageDisplay::setSet( Pick::Set* set )
{
    LocationDisplay::setSet( set );
    updateCoords();
}


void ImageDisplay::setScene( visSurvey::Scene* scene )
{
    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ImageDisplay,updateCoords) );

    visSurvey::SurveyObject::setScene( scene );

    if ( scene_ )
	scene_->zstretchchange.notify( mCB(this,ImageDisplay,updateCoords) );

    updateCoords();
}


visBase::VisualObject* ImageDisplay::createLocation() const
{
    if ( !getFileName() )
	const_cast<ImageDisplay*>(this)->needFileName.trigger();

    Image* res = Image::create();
    res->setShape( shape_ );

    return res;
}


void ImageDisplay::dispChg( CallBacker* cb )
{
    LocationDisplay::dispChg( cb );
    updateCoords();
}


int ImageDisplay::isMarkerClick(const TypeSet<int>& path) const
{
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( Image*, image, group_->getObject(idx) );
	if ( !image )
	    continue;
	
	if ( path.indexOf(group_->getObject(idx)->id())!=-1 )
	    return idx;
    }

    return -1;
}


void ImageDisplay::setPosition( int idx, const Pick::Location& pick )
{
    mDynamicCastGet( Image*, image, group_->getObject(idx) );
    image->setPick( pick );
}


void ImageDisplay::updateCoords(CallBacker*)
{
    const float zscale = scene_ ? scene_->getZStretch()*scene_->getZScale() : 1;
    const float size = set_ ? set_->disp_.pixsize_ : 100;
    visBase::Coordinates* facecoords = shape_->getCoordinates();
    facecoords->setPos( 0, Coord3(-size,0,-2*size/zscale) );
    facecoords->setPos( 1, Coord3(-size,0,2*size/zscale) );
    facecoords->setPos( 2, Coord3(size,0,2*size/zscale) );
    facecoords->setPos( 3, Coord3(size,0,-2*size/zscale) );
}

} //namespace
