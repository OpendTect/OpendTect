/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2006
 RCS:           $Id: SoShaderTexture2.cc,v 1.1 2006-12-28 22:17:58 cvskris Exp $
________________________________________________________________________

-*/


#include "SoShaderTexture2.h"

//#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include "Inventor/elements/SoGLDisplayList.h"
#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/sensors/SoFieldSensor.h"
#include "Inventor/nodes/SoTextureUnit.h"
#include "Inventor/SbImage.h"

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>


SoGLShaderImage::SoGLShaderImage()
    : texname_( GL_INVALID_VALUE )
    , list_( 0 ) 
    , nc_( 0 )
    , size_( 0, 0 )
{ }

SoGLShaderImage::~SoGLShaderImage()
{
    if ( list_ ) list_->unref();
}

bool SoGLShaderImage::setTexture( const unsigned char* bytes,
				  const SbVec2s& size,
				  int nc, Wrap wrapS, Wrap wrapT, int unit )
{
    glPushAttrib( GL_ENABLE_BIT | GL_TEXTURE_BIT );
    if ( texname_==GL_INVALID_VALUE )
    {
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glGenTextures(1, &texname_);
    }

    if ( texname_==GL_INVALID_VALUE )
	return false;

    GLenum format = GL_RGBA;
    if ( nc==1 ) format = GL_RED;
    else if ( nc==3 ) format = GL_RGB;

    if ( unit==0 ) glActiveTexture(GL_TEXTURE0);
    else if ( unit==1 ) glActiveTexture(GL_TEXTURE1);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texname_ );

    SoGLImage::setData( NULL, size, nc, wrapS, wrapT );
    if ( size==size_ && nc==nc_ )
    {
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 9, 9, size[0], size[1],
			format, GL_UNSIGNED_BYTE, bytes );
    }
    else
    {
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, format, size[0], size[1], 0,
		      format, GL_UNSIGNED_BYTE, bytes );
    }

    glPopAttrib();

    size_ = size_;
    nc_ = nc;

    return true;
}


GLenum translateWrap( SoGLImage::Wrap wrap )
{
    return wrap==SoGLImage::CLAMP ? GL_CLAMP : GL_REPEAT;
}


SoGLDisplayList* SoGLShaderImage::getGLDisplayList(SoState * state)
{
    if ( !list_ )
    {
	list_ = new SoGLDisplayList( state, SoGLDisplayList::DISPLAY_LIST );
	list_->ref();
	list_->open( state ); //hm - does also execution ...
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texname_ );
	//glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	//glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	//glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S,
			 //translateWrap(getWrapS()));
	//glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T,
			 //translateWrap(getWrapT()) );
	list_->close(state);
    }

    return list_;
}



SO_NODE_SOURCE( SoShaderTexture2 );

void SoShaderTexture2::initClass()
{
    SO_NODE_INIT_CLASS(SoShaderTexture2, SoNode, "Node");
}


SoShaderTexture2::SoShaderTexture2()
    : glimage_( 0 )
    , imagesensor_( 0 )
    , glimagevalid_( false )
{
    SO_NODE_CONSTRUCTOR( SoShaderTexture2 );
    SO_NODE_ADD_FIELD( image, (SbVec2s(0,0),0,0,SoSFImage::COPY));

    imagesensor_ =  new SoFieldSensor(imageChangeCB, this);
    imagesensor_->attach( &image );
}


SoShaderTexture2::~SoShaderTexture2()
{
    glimage_->unref();
    delete imagesensor_;
}


void SoShaderTexture2::GLRender( SoGLRenderAction* action )
{
    SoCacheElement::invalidate(action->getState());

    SoState * state = action->getState();
    const int unit = SoTextureUnitElement::get(state);

    //SoGLTextureEnabledElement::enableRectange();
  

    if ( !glimage_ || !glimagevalid_ )
    {
	if ( !glimage_ )
	{
	    glimage_ = new SoGLImage;
	}

	int nc;
	SbVec2s size;
	const unsigned char* bytes = image.getValue( size, nc );
	const float quality = SoTextureQualityElement::get(state);

	glimage_->setData(bytes,size,nc,SoGLImage::CLAMP,SoGLImage::CLAMP, quality );
	glimage_->setFlags( SoGLImage::RECTANGLE | SoGLImage::NO_MIPMAP |
			    SoGLImage::COMPRESSED | SoGLImage::USE_QUALITY_VALUE );
	glimagevalid_ = true;
    }

    if ( !unit && SoTextureOverrideElement::getImageOverride(state) )
	return;

    const int maxunits = SoTextureUnit::getMaxTextureUnit();

    if ( !unit ) 
    {  
	SoGLTextureImageElement::set( state, this, glimage_,
				  SoTextureImageElement::REPLACE, SbColor() );
	SoGLTexture3EnabledElement::set( state, this, false );
	SoGLTextureEnabledElement::set( state, this, true );	  

	if ( isOverride() ) 
	    SoTextureOverrideElement::setImageOverride( state, true );
    }
    else if (unit<maxunits) 
    {
	SoGLMultiTextureImageElement::set( state, this, unit,
					   glimage_,
					   SoTextureImageElement::REPLACE,
					   SbColor() );
	  
	SoGLMultiTextureEnabledElement::set(state, this, unit, true );
    }
    else 
    {
      // do nothing; warning triggered already in textureunit 
    }
}


void SoShaderTexture2::imageChangeCB( void* data, SoSensor* )
{
    SoShaderTexture2* ptr = (SoShaderTexture2*) data;
    ptr->glimagevalid_ = false;
}
