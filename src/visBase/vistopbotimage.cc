/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          June 2009
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "vistopbotimage.h"
#include "viscoord.h"
#include "vistransform.h"
#include "vismaterial.h"
#include "iopar.h"
#include "keystrs.h"

#include <osgGeo/TexturePlane>
#include <osgGeo/LayeredTexture>
#include <osg/Image>
#include <osgDB/ReadFile>

mCreateFactoryEntry( visBase::TopBotImage );

namespace visBase
{

    const char* TopBotImage::sKeyTopLeftCoord()     { return "TopLeft"; }
    const char*	TopBotImage::sKeyBottomRightCoord() { return "BotRight"; } 
    const char*	TopBotImage::sKeyFileNameStr()      { return sKey::FileName(); }

TopBotImage::TopBotImage()
    : VisualObjectImpl(true)
    , trans_(0)
    , laytex_( new osgGeo::LayeredTexture )
    , texplane_( new osgGeo::TexturePlaneNode )
{
    laytex_->ref();
    texplane_->ref();
    layerid_ = laytex_->addDataLayer();
    laytex_->addProcess( new osgGeo::IdentityLayerProcess(*laytex_, layerid_) );
    texplane_->setLayeredTexture( laytex_ );
    addChild( texplane_ );

    setTransparency( 0.0 );
}


TopBotImage::~TopBotImage()
{
    if ( trans_ ) trans_->unRef();
    laytex_->unref();
    texplane_->unref();
}


void TopBotImage::setPos( const Coord3& c1, const Coord3& c2 )
{
    pos0_ = c1;
    pos1_ = c2;
    updateCoords();
}


void TopBotImage::updateCoords()
{
    Coord3 pos0, pos1;
    Transformation::transform( trans_, pos0_, pos0 );
    Transformation::transform( trans_, pos1_, pos1 );

    const Coord3 dif = pos1 - pos0;
    texplane_->setWidth( osg::Vec3(dif.x, -dif.y, 0.0) );

    const Coord3 avg = 0.5 * (pos0+pos1);
    texplane_->setCenter( osg::Vec3(avg.x, avg.y, avg.z) );
}


void TopBotImage::setDisplayTransformation( const mVisTrans* trans )
{
    if ( trans_ ) trans_->unRef();
    trans_ = trans;
    if ( trans_ ) trans_->ref();
    updateCoords();
}


void TopBotImage::setTransparency( float val )
{ getMaterial()->setTransparency( val ); }


float TopBotImage::getTransparency() const
{ return getMaterial()->getTransparency(); }


void TopBotImage::setImageFilename( const char* fnm )
{
    filenm_ = fnm;
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile( fnm );
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


const char* TopBotImage::getImageFilename() const
{ return filenm_.buf(); }


void TopBotImage::fillPar( IOPar& iopar ) const
{
    iopar.set( sKeyTopLeftCoord(), pos0_ );
    iopar.set( sKeyBottomRightCoord(), pos1_ );
    iopar.set( sKeyFileNameStr(), filenm_  );
}


bool TopBotImage::usePar( const IOPar& iopar )
{
    Coord3 ltpos;
    Coord3 brpos;
    iopar.get( sKeyTopLeftCoord(), ltpos );
    iopar.get( sKeyBottomRightCoord(), brpos );
    iopar.get( sKeyFileNameStr(), filenm_  );
   
    setPos( ltpos, brpos );  
    setImageFilename( filenm_ );
    return true;
}

} //namespace visBase

