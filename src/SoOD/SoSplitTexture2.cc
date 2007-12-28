/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2006
 RCS:           $Id: SoSplitTexture2.cc,v 1.7 2007-12-28 22:34:23 cvskris Exp $
________________________________________________________________________

-*/


#include "SoSplitTexture2.h"

#include <Inventor/misc/SoGLImage.h>
#include <Inventor/C/glue/gl.h>
#include <SoSplitTexture2Element.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/actions/SoCallbackAction.h"
#include "Inventor/actions/SoRayPickAction.h"
#include "Inventor/sensors/SoFieldSensor.h"

#include <Inventor/system/gl.h>

SO_NODE_SOURCE( SoSplitTexture2 );

void SoSplitTexture2::initClass()
{
    SO_NODE_INIT_CLASS(SoSplitTexture2, SoNode, "Node");

    SO_ENABLE(SoGLRenderAction, SoSplitTexture2Element);
}


SoSplitTexture2::SoSplitTexture2()
{
    SO_NODE_CONSTRUCTOR( SoSplitTexture2 );
    SO_NODE_ADD_FIELD( image, (SbVec2s(0,0),0,0,SoSFImage::COPY));
}


SoSplitTexture2::~SoSplitTexture2()
{
}


void SoSplitTexture2::GLRender( SoGLRenderAction* action )
{
    SbVec2s sz;
    int nrcomp;
    const unsigned char* values = image.getValue( sz, nrcomp );

    SoState* state = action->getState();
    const int unit = SoTextureUnitElement::get( state );
    SoSplitTexture2Element::set( state, this, unit, sz, nrcomp, values );
}


SO_NODE_SOURCE( SoSplitTexture2Part );

void SoSplitTexture2Part::initClass()
{
    SO_NODE_INIT_CLASS(SoSplitTexture2Part, SoNode, "Node");
    SO_ENABLE( SoGLRenderAction, SoSplitTexture2Element);
    SO_ENABLE( SoGLRenderAction, SoCacheElement );
    SO_ENABLE( SoGLRenderAction, SoTextureImageElement );
    SO_ENABLE( SoGLRenderAction, SoGLTexture3EnabledElement );
    SO_ENABLE( SoGLRenderAction, SoGLTextureEnabledElement );

    SO_ENABLE( SoCallbackAction, SoTexture3EnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoTextureEnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureOverrideElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureEnabledElement );

    SO_ENABLE( SoRayPickAction, SoTexture3EnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoTextureEnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureOverrideElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureEnabledElement );
}



SoSplitTexture2Part::SoSplitTexture2Part()
    : imagedata_( 0 )
    , glimage_( new SoGLImage )
    , needregeenration_( true )
{
    SO_NODE_CONSTRUCTOR( SoSplitTexture2Part );
    SO_NODE_ADD_FIELD( origin, (SbVec2i32(0,0) ) );
    SO_NODE_ADD_FIELD( size, (SbVec2i32(0,0) ) );
    SO_NODE_ADD_FIELD( textureunits, (0) );

    originsensor_ = new SoFieldSensor( fieldChangeCB, this );
    originsensor_->attach( &origin );
}


SoSplitTexture2Part::~SoSplitTexture2Part()
{
    delete [] imagedata_;
    if ( glimage_ ) glimage_->unref();
    delete originsensor_;
}

void SoSplitTexture2Part::fieldChangeCB( void* data, SoSensor* )
{
    SoSplitTexture2Part* ptr = (SoSplitTexture2Part*) data;
    ptr->needregeenration_ = true;
}


#define mFastDim	1
#define mSlowDim	0

void SoSplitTexture2Part::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    if ( SoTextureOverrideElement::getImageOverride(state) )
	return;

    for ( int idx=textureunits.getNum()-1; idx>=0; idx-- )
	GLRenderUnit( textureunits[idx], state );
}


void SoSplitTexture2Part::doAction( SoAction* action )
{
    SoState * state = action->getState();

    for ( int idx=textureunits.getNum()-1; idx>=0; idx-- )
	doActionUnit( textureunits[idx], state );
}


void SoSplitTexture2Part::callback( SoCallbackAction* action )
{ doAction(action); }


void SoSplitTexture2Part::rayPick( SoRayPickAction* action )
{ doAction(action); }


void SoSplitTexture2Part::GLRenderUnit( int unit, SoState* state )
{
    const SbVec2i32 origsz = size.getValue();
    const SbVec2i32 origstart = origin.getValue();

    const SbVec2s sz( origsz[0], origsz[1] );

    const SbVec2s start( origstart[0], origstart[1] );

    SbVec2s sourcesize;
    int numcomponents;
    const unsigned char* sourcedata = SoSplitTexture2Element::get( state, unit,
	    sourcesize, numcomponents );

    if ( !sourcedata )
	return;

    const int bufsize = sz[0]*sz[1];
    if ( !imagedata_ || bufsize!=imagesize_ )
    {
	imagesize_ = bufsize;
	delete [] imagedata_;
	imagedata_ = new unsigned char[imagesize_*numcomponents];
	numcomp_ = numcomponents;

	if ( !imagedata_ )
	    return;

	needregeenration_ = true;
    }

    const float quality = SoTextureQualityElement::get(state);

    if ( needregeenration_ )
    {
	for ( int idx=sz[mSlowDim]-1; idx>=0; idx-- )
	{
	    int srcslowidx = start[mSlowDim]+idx;
	    if ( srcslowidx<0 )
		srcslowidx = 0;
	    else if ( srcslowidx>=sourcesize[mSlowDim] )
		srcslowidx = sourcesize[mSlowDim]-1;

	    const unsigned char* srcptr = sourcedata +
		(srcslowidx*sourcesize[mFastDim]) * numcomponents;
	    unsigned char* dstptr =
		imagedata_ + (sz[mFastDim] * idx) * numcomponents;

	    for ( int idy=0; idy<sz[mFastDim]; idy++ )
	    {
		int srcfastidx = start[mFastDim]+idy;
		if ( !srcfastidx )
		{
		    int copysize = sz[mFastDim]-idy;
		    if ( copysize>sourcesize[mFastDim] )
			copysize = sourcesize[mFastDim];
		    if ( copysize>0 )
		    {
			memcpy( dstptr+idy*numcomponents, srcptr,
				copysize*numcomponents );
			idy += (copysize-1);
			continue;
		    }
		}

		if ( srcfastidx>=sourcesize[mFastDim] )
		    srcfastidx = sourcesize[mFastDim]-1;
		else if ( srcfastidx<0 )
		    srcfastidx = 0;

		memcpy( dstptr+idy*numcomponents,
			srcptr+srcfastidx*numcomponents, numcomponents );
	    }
	}

	glimage_->setData( imagedata_, sz, numcomponents,
			SoGLImage::CLAMP,
			   SoGLImage::CLAMP, quality, false,
			   state );
	needregeenration_ = false;
	SoCacheElement::setInvalid( true );
	if ( state->isCacheOpen() )
	    SoCacheElement::invalidate(state);
    }

    const SoTextureImageElement::Model glmodel =
	SoTextureImageElement::MODULATE;
    const cc_glglue* glue =
	cc_glglue_instance( SoGLCacheContextElement::get(state) );

    const int maxunits = cc_glglue_max_texture_units(glue);

    if ( !unit )
    {
	SoGLTextureImageElement::set( state, this, glimage_, glmodel,
				      SbColor(1,1,1));
	SoGLTexture3EnabledElement::set( state, this, FALSE);
	SoGLTextureEnabledElement::set( state, this,
					!needregeenration_ && quality > 0.0f);
	if ( isOverride() )
	    SoTextureOverrideElement::setImageOverride( state, true );
    }
    else if ( unit<maxunits )
    {
	SoGLMultiTextureImageElement::set( state, this, unit, glimage_, glmodel,
					   SbColor(1,1,1) );
	SoGLMultiTextureEnabledElement::set( state, this, unit,
				!needregeenration_ && quality > 0.0f);
    }
}


void SoSplitTexture2Part::doActionUnit( int unit, SoState* state )
{
    if ( !unit && SoTextureOverrideElement::getImageOverride(state) )
	return;

    SbVec2s sz( size.getValue()[0], size.getValue()[1] );
    const unsigned char* bytes = imagedata_;
    int nc = numcomp_;

    if ( !bytes )
    {
	static const unsigned char dummytex[] = {0xff,0xff,0xff,0xff};
	bytes = dummytex;
	sz = SbVec2s(2,2);
	nc = 1;
    } 

    if ( !unit )
    {
	SoTexture3EnabledElement::set(state, this, false );
	if ( sz!=SbVec2s(0,0) )
	{
	    SoTextureImageElement::set(state, this, sz, nc, bytes,
				    0, 0, SoTextureImageElement::MODULATE,
				    SbColor( 1, 1, 1 ) );
	    SoTextureEnabledElement::set(state, this, true );
	}
	else
	{
	    SoTextureImageElement::setDefault(state, this);
	    SoTextureEnabledElement::set(state, this, false );
	}
	if ( isOverride() )
	{
		SoTextureOverrideElement::setImageOverride(state, true );
	}
    }
    else
    {
	if ( sz!=SbVec2s(0,0) )
	{
	    SoMultiTextureImageElement::set(state, this, unit,
		sz, nc, bytes, SoTextureImageElement::CLAMP,
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
