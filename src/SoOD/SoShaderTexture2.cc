/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "SoShaderTexture2.h"

//#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#if COIN_MAJOR_VERSION <= 3
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#endif
#include <Inventor/elements/SoTextureQualityElement.h>
#include "Inventor/elements/SoGLDisplayList.h"
#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/sensors/SoFieldSensor.h"
#include "Inventor/nodes/SoTextureUnit.h"
#include "Inventor/SbImage.h"

//#define GL_GLEXT_PROTOTYPES

#include <Inventor/system/gl.h>

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
    if ( glimage_ ) glimage_->unref();
    delete imagesensor_;
}


int SoShaderTexture2::getMaxSize()
{
    static int maxsize = -1;
    if ( maxsize!=-1 )
	return maxsize;

    GLint maxr;
    glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &maxr);
    if ( glGetError()==GL_NO_ERROR )
    {
	maxsize = maxr;
	return maxsize;
    }

    return 2048; //conservative default
}


void SoShaderTexture2::GLRender( SoGLRenderAction* action )
{
    SoCacheElement::invalidate(action->getState());

    SoState * state = action->getState();
    const int unit = SoTextureUnitElement::get(state);

    //SoGLTextureEnabledElement::enableRectange();
  

    if ( !glimage_ || !glimagevalid_ )
    {
	int nc;
	SbVec2s size;
	const unsigned char* bytes = image.getValue( size, nc );
	if ( !bytes )
	    return;

	const float quality = SoTextureQualityElement::get(state);

	if ( !glimage_ )
	{
	    glimage_ = new SoGLImage;
	}

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
#if COIN_MAJOR_VERSION <= 3
	SoGLTexture3EnabledElement::set( state, this, false );
	SoGLTextureEnabledElement::set( state, this, true );	  
#else
	SoGLMultiTextureEnabledElement::set(state, this, unit, true );
#endif

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
