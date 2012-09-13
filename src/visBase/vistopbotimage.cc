/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          June 2009
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: vistopbotimage.cc,v 1.11 2012-09-13 14:31:57 cvskris Exp $";


#include "vistopbotimage.h"
#include "viscoord.h"
#include "vistristripset.h"
#include "visimage.h"
#include "vistransform.h"
#include "vismaterial.h"
#include "vistexturecoords.h"
#include "iopar.h"
#include "keystrs.h"


mCreateFactoryEntry( visBase::TopBotImage );

namespace visBase
{

    const char* TopBotImage::sKeyTopLeftCoord()     { return "TopLeft"; }
    const char*	TopBotImage::sKeyBottomRightCoord() { return "BotRight"; } 
    const char*	TopBotImage::sKeyFileNameStr()      { return sKey::FileName(); } 	

TopBotImage::TopBotImage()
    : VisualObjectImpl(true)
    , trans_(0)
    , imgshape_( visBase::TriangleStripSet::create() )
    , image_( visBase::Image::create() )
{
    image_->ref();
    image_->replaceMaterial(false);
    addChild( image_->getInventorNode() );

    imgshape_->ref();
    imgshape_->removeSwitch();
    imgshape_->setVertexOrdering(
		visBase::VertexShape::cCounterClockWiseVertexOrdering() );
    if ( doOsg() )
	addChild( imgshape_->osgNode() );
    else
	addChild( imgshape_->getInventorNode() );
   
    visBase::TextureCoords* texturecoords = visBase::TextureCoords::create();
    imgshape_->setTextureCoords( texturecoords );
    texturecoords->setCoord( 0, Coord3(0,1,0) );
    texturecoords->setCoord( 1, Coord3(0,0,0) );
    texturecoords->setCoord( 2, Coord3(1,0,0) );
    texturecoords->setCoord( 3, Coord3(1,1,0) );
    
    RefMan<Geometry::IndexedPrimitiveSet> triangles =
	Geometry::IndexedPrimitiveSet::create( false );
    const int indices[] = { 3, 2, 0, 1 };
    triangles->set( indices, 4 );
    
    imgshape_->addPrimitiveSet( triangles );
}


TopBotImage::~TopBotImage()
{
    imgshape_->unRef();
    image_->unRef();
    if ( trans_ ) trans_->unRef();
}


void TopBotImage::setPos( const Coord& c1, const Coord& c2, float z )
{
    pos0_ = Coord3( c1.x, c1.y, z );
    pos1_ = Coord3( c1.x, c2.y, z);
    pos2_ = Coord3( c2.x, c2.y, z );
    pos3_ = Coord3( c2.x, c1.y, z ); 
    updateCoords();
}


void TopBotImage::updateCoords()
{
    visBase::Coordinates* facecoords = imgshape_->getCoordinates();
   
    facecoords->setPos( 0, trans_ ? trans_->transform(pos0_) : pos0_ );
    facecoords->setPos( 1, trans_ ? trans_->transform(pos1_) : pos1_ ); 
    facecoords->setPos( 2, trans_ ? trans_->transform(pos2_) : pos2_ );
    facecoords->setPos( 3, trans_ ? trans_->transform(pos3_) : pos3_ );
    imgshape_->dirtyCoordinates();
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
    image_->setFileName( fnm );
}


const char* TopBotImage::getImageFilename() const
{ return filenm_.buf(); }


void TopBotImage::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( iopar, saveids );
     
    iopar.set( sKeyTopLeftCoord(), pos0_ );
    iopar.set( sKeyBottomRightCoord(), pos2_ );
    iopar.set( sKeyFileNameStr(), filenm_  );
}


int TopBotImage::usePar( const IOPar& iopar )
{
    int res = VisualObjectImpl::usePar( iopar );
    if ( res!=1 ) return res;
    
    Coord3 ltpos;
    Coord3 brpos;
    iopar.get( sKeyTopLeftCoord(), ltpos );
    iopar.get( sKeyBottomRightCoord(), brpos );
    iopar.get( sKeyFileNameStr(), filenm_  );
   
    setPos( ltpos, brpos, (float) ltpos.z );  
    setImageFilename( filenm_ );
    return 1;
}

} //namespace visBase

