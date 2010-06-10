/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          October 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoColorTableTexture.cc,v 1.1 2010-06-10 09:35:26 cvsranojay Exp $";


#include "SoColorTableTexture.h"
#include "SoTextureComposerElement.h"
#include "SoTextureComposer.h"

#include <Inventor/misc/SoGLImage.h>
#include <Inventor/C/glue/gl.h>
#include <SoTextureChannelSetElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#if COIN_MAJOR_VERSION <= 3
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#endif
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/actions/SoCallbackAction.h"
#include "Inventor/actions/SoRayPickAction.h"
#include "Inventor/sensors/SoFieldSensor.h"
#include "Inventor/SbImage.h"

SO_NODE_SOURCE( SoColorTableTexture );

void SoColorTableTexture::initClass()
{
    SO_NODE_INIT_CLASS(SoColorTableTexture, SoNode, "Node");
    SO_ENABLE( SoGLRenderAction, SoCacheElement );
    SO_ENABLE( SoGLRenderAction, SoTextureImageElement );
    SO_ENABLE( SoGLRenderAction, SoGLTextureEnabledElement );
    SO_ENABLE( SoGLRenderAction, SoTextureComposerElement );

    SO_ENABLE( SoCallbackAction, SoTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoTextureEnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureOverrideElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureEnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureComposerElement );

    SO_ENABLE( SoRayPickAction, SoTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoTextureEnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureOverrideElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureEnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureComposerElement );

#if COIN_MAJOR_VERSION <= 3
    SO_ENABLE( SoRayPickAction, SoTexture3EnabledElement );
    SO_ENABLE( SoCallbackAction, SoTexture3EnabledElement );
    SO_ENABLE( SoGLRenderAction, SoGLTexture3EnabledElement );
#endif
}


SoColorTableTexture::SoColorTableTexture()
    : matchinfo_( 0 )
    , ti_(-1)
    , glimage_( new SoGLImage )
    , needregeneration_( true )
{
    SO_NODE_CONSTRUCTOR( SoColorTableTexture );
    SO_NODE_ADD_FIELD( image, (SbVec2s(0,0),4,0,SoSFImage::NO_COPY) );

    sensor_ = new SoFieldSensor( fieldChangeCB, this );
    sensor_->attach( &image );
}


void SoColorTableTexture::fieldChangeCB( void* data, SoSensor* )
{
    SoColorTableTexture* ptr = (SoColorTableTexture*) data;
    ptr->needregeneration_ = true;
}


SoColorTableTexture::~SoColorTableTexture()
{
    delete sensor_;
    delete matchinfo_;
    glimage_->unref();
}


#define mFastDim	2
#define mMidDim		1
#define mSlowDim	0

void SoColorTableTexture::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    if ( SoTextureOverrideElement::getImageOverride(state) )
	return;

    const SoElement* transinfoelem = state->getConstElement(
	    SoTextureComposerElement::getClassStackIndex() );

    const int unit = SoTextureUnitElement::get( state );
    const char ti = SoTextureComposerElement::getTransparencyInfo( state );

    if ( ti_!=ti )
    {
	ti_ = ti;

	const uint32_t flags = SoTextureComposerInfo::getGLImageFlags(
				glimage_->getFlags(), ti );

	glimage_->setFlags( flags );
    }

    const float quality = SoTextureQualityElement::get(state);

    if ( needregeneration_ )
    {
	int nc;
	SbVec2s size;
	const unsigned char* bytes = image.getValue( size, nc );
	glimage_->setData( bytes,
		size,nc,SoGLImage::CLAMP,
		SoGLImage::CLAMP, quality,false,0 );
	needregeneration_ = false;
    }

    const SoTextureImageElement::Model glmodel =
	SoTextureImageElement::MODULATE;
    const cc_glglue* glue =
	cc_glglue_instance( SoGLCacheContextElement::get(state) );

    const int maxunits = cc_glglue_max_texture_units(glue);

    if ( unit==0 )
    {
	SoGLTextureImageElement::set( state, this, glimage_,
				      glmodel, SbColor(1,1,1) );
#if COIN_MAJOR_VERSION <= 3
	SoGLTexture3EnabledElement::set(state, this, false );
	SoGLTextureEnabledElement::set(state, this, quality > 0.0f );
#else
	SoGLMultiTextureEnabledElement::set( state, this, unit, quality > 0.0f);
#endif
	if ( isOverride() )
	    SoTextureOverrideElement::setImageOverride( state, true );
    }
    else if ( unit<maxunits )
    {
	SoGLMultiTextureImageElement::set( state, this, unit,
					   glimage_, glmodel,
					   SbColor(1,1,1) );
	SoGLMultiTextureEnabledElement::set( state, this, unit, quality > 0.0f);
    }

    if ( needregeneration_ )
    {
	SoCacheElement::setInvalid( true );
	if ( state->isCacheOpen() )
	    SoCacheElement::invalidate(state);
    }
}


void SoColorTableTexture::doAction( SoAction* action )
{
    SoState * state = action->getState();
    const int unit = SoTextureUnitElement::get( state );
    if ( !unit && SoTextureOverrideElement::getImageOverride(state) )
	return;

    int nc;
    SbVec2s size;
    const unsigned char* bytes = image.getValue( size, nc );

    if ( !bytes )
    {
	static const unsigned char dummytex[] = {0xff,0xff,0xff,0xff};
	bytes = dummytex;
	size = SbVec2s(2,2);
	nc = 1;
    } 

    if ( !unit )
    {
#if COIN_MAJOR_VERSION <= 3
	SoTexture3EnabledElement::set(state, this, false );
#endif
	if ( size!=SbVec2s(0,0) )
	{
	    SoTextureImageElement::set(state, this, size,
		    nc, bytes, 0, 0, SoTextureImageElement::MODULATE,
		    SbColor( 1, 1, 1 ) );

#if COIN_MAJOR_VERSION <= 3
	    SoTextureEnabledElement::set(state, this, true );
#else
	    SoMultiTextureEnabledElement::set(state, this, unit, true );
#endif
	}
	else
	{
	    SoTextureImageElement::setDefault(state, this);
#if COIN_MAJOR_VERSION <= 3
	    SoTextureEnabledElement::set(state, this, false );
#else
	    SoMultiTextureEnabledElement::set(state, this, unit, false );
#endif
	}

	if ( isOverride() )
	{
	    SoTextureOverrideElement::setImageOverride(state, true );
	}
    }
    else
    {
	if ( size!=SbVec2s(0,0) )
	{
	    SoMultiTextureImageElement::set(state, this, unit,
		size, nc, bytes,
		SoTextureImageElement::CLAMP,
		SoTextureImageElement::CLAMP,
		SoTextureImageElement::MODULATE,
		SbColor( 1, 1, 1 ) );

	    SoMultiTextureEnabledElement::set(state, this, unit, true );
	}
	else
	{
	    SoMultiTextureImageElement::setDefault(state, this, unit);
	    SoMultiTextureEnabledElement::set(state, this, unit, false );
	}
    }
}


void SoColorTableTexture::callback( SoCallbackAction* action )
{ doAction(action); }


void SoColorTableTexture::rayPick( SoRayPickAction* action )
{ doAction(action); }
