/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2006
 RCS:           $Id: SoSplitTexture2.cc,v 1.1 2007-11-16 21:39:05 cvskris Exp $
________________________________________________________________________

-*/


#include "SoSplitTexture2.h"

//#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/misc/SoGLImage.h>
//#include <Inventor/elements/SoCacheElement.h>
//#include <Inventor/elements/SoTextureUnitElement.h>
//#include <Inventor/elements/SoTextureOverrideElement.h>
//#include <Inventor/elements/SoGLMultiTextureImageElement.h>
//#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoTextureImageElement.h>
//#include <Inventor/elements/SoGLTextureEnabledElement.h>
//#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
//#include "Inventor/elements/SoGLDisplayList.h"
#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/sensors/SoFieldSensor.h"
//#include "Inventor/nodes/SoTextureUnit.h"
//#include "Inventor/SbImage.h"

//#define GL_GLEXT_PROTOTYPES

#include <Inventor/system/gl.h>

SO_NODE_SOURCE( SoSplitTexture2 );

void SoSplitTexture2::initClass()
{
    SO_NODE_INIT_CLASS(SoSplitTexture2, SoNode, "Node");
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

    SoTextureImageElement::set( action->getState(), this, sz, nrcomp, values,
	    SoTextureImageElement::CLAMP_TO_BORDER,
	    SoTextureImageElement::CLAMP_TO_BORDER,
	    SoTextureImageElement::MODULATE, SbColor( 1, 1, 1 ) );
}


SO_NODE_SOURCE( SoSplitTexture2Part );

void SoSplitTexture2Part::initClass()
{
    SO_NODE_INIT_CLASS(SoSplitTexture2Part, SoNode, "Node");
}



SoSplitTexture2Part::SoSplitTexture2Part()
    : imagedata_( 0 )
    , glimage_( new SoGLImage )
    , needregeenration_( true )
{
    SO_NODE_CONSTRUCTOR( SoSplitTexture2Part );
    SO_NODE_ADD_FIELD( origin, (SbVec2i32(0,0) ) );
    SO_NODE_ADD_FIELD( size, (SbVec2i32(0,0) ) );
    SO_NODE_ADD_FIELD( borders, (true) );

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

    const SbVec2i32 origsz = size.getValue();
    const SbVec2i32 origstart = origin.getValue();

    const SbVec2s sz = borders.getValue()
	? SbVec2s(origsz[0]+2,origsz[1]+2) : SbVec2s(origsz[0],origsz[1]);

    const SbVec2s start = borders.getValue()
	? SbVec2s(origstart[0]-1,origstart[1]-1)
	: SbVec2s(origstart[0],origstart[1]);

    const int bufsize = sz[0]*sz[1];
    if ( !imagedata_ || bufsize!=imagesize_ )
    {
	imagesize_ = bufsize;
	delete [] imagedata_;
	imagedata_ = new unsigned char[imagesize_];

	if ( !imagedata_ )
	    return;

	needregeenration_ = true;
    }

    SbVec2s imagesize;
    int numcomponents;
    const unsigned char* imagedata = SoTextureImageElement::getImage( state,
	    imagesize, numcomponents );

    if ( needregeenration_ )
    {
	for ( int idx=sz[mSlowDim]-1; idx>=0; idx-- )
	{
	    int srcslowidx = start[mSlowDim]+idx;
	    if ( srcslowidx<0 )
		srcslowidx = 0;
	    else if ( srcslowidx>=imagesize[mSlowDim] )
		srcslowidx = imagesize[mSlowDim]-1;

	    int srcfastidx = start[mFastDim];
	    if ( srcfastidx<0 )
		srcfastidx = 0;
	    else if ( srcfastidx>=imagesize[mFastDim] )
		srcfastidx = imagesize[mFastDim]-1;

	    int srcfaststop = start[mFastDim]+sz[mFastDim];
	    if ( srcfaststop<0 )
		srcfaststop = 0;
	    else if ( srcfaststop>=imagesize[mFastDim] )
		srcfaststop = imagesize[mFastDim]-1;

	    const unsigned char* src = imagedata +
		(srcslowidx * imagesize[mFastDim] + srcfastidx) * numcomponents;

	    const int cpsize = numcomponents * ( srcfaststop-srcfastidx+1 );
	    memcpy( imagedata_+idx*sz[mSlowDim]*numcomponents, src, cpsize );
	}

	needregeenration_ = false;
    }

    const float quality = SoTextureQualityElement::get(state);
    glimage_->setData( imagedata_, sz, numcomponents, SoGLImage::REPEAT,
	    	       SoGLImage::REPEAT, quality, borders.getValue(), state );
}
