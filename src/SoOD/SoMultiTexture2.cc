/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: SoMultiTexture2.cc,v 1.1 2005-12-16 17:57:43 cvskris Exp $
________________________________________________________________________

-*/

#include "SoMultiTexture2.h"

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
//#include <Inventor/elements/SoTextureScalePolicyElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGLBigImage.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/threads/SbMutex.h>
#include <Inventor/C/glue/gl.h>


SO_NODE_SOURCE( SoMultiTexture2 );

void SoMultiTexture2::initClass()
{
    SO_NODE_INIT_CLASS( SoMultiTexture2, SoNode, "Node" );

    SO_ENABLE(SoGLRenderAction, SoGLTextureImageElement);
    SO_ENABLE(SoGLRenderAction, SoGLTextureEnabledElement);
    SO_ENABLE(SoGLRenderAction, SoGLTexture3EnabledElement);

    SO_ENABLE(SoGLRenderAction, SoGLMultiTextureImageElement);
    SO_ENABLE(SoGLRenderAction, SoGLMultiTextureEnabledElement);

    SO_ENABLE(SoCallbackAction, SoTextureImageElement);
    SO_ENABLE(SoCallbackAction, SoTextureEnabledElement);
    SO_ENABLE(SoCallbackAction, SoMultiTextureEnabledElement);
    SO_ENABLE(SoCallbackAction, SoMultiTextureImageElement);
    SO_ENABLE(SoCallbackAction, SoTexture3EnabledElement);

    SO_ENABLE(SoRayPickAction, SoTextureImageElement);
    SO_ENABLE(SoRayPickAction, SoTextureEnabledElement);
    SO_ENABLE(SoRayPickAction, SoTexture3EnabledElement);
    SO_ENABLE(SoRayPickAction, SoMultiTextureEnabledElement);
    SO_ENABLE(SoRayPickAction, SoMultiTextureImageElement);
}


SoMultiTexture2::SoMultiTexture2()
    : glimagevalid(false)
    , glimagemutex( new SbMutex )
    , glimage( 0 )
{
    SO_NODE_CONSTRUCTOR(SoMultiTexture2);

    SO_NODE_ADD_FIELD( image, (SbImage(0, SbVec2s(0,0), 0)) );
    SO_NODE_ADD_FIELD( numcolor, (0) );
    SO_NODE_ADD_FIELD( colors, (SbVec2s(0,0), 0, 0) );
    SO_NODE_ADD_FIELD( operation, (BLEND) );
    SO_NODE_ADD_FIELD( component, (RED|GREEN|BLUE|OPACITY) );
    SO_NODE_ADD_FIELD( blendColor, (0, 0, 0) );
    SO_NODE_ADD_FIELD( wrapS, (SoTexture2::REPEAT) );
    SO_NODE_ADD_FIELD( wrapT, (SoTexture2::REPEAT) );
    SO_NODE_ADD_FIELD( model, (SoTexture2::MODULATE) );

    SO_NODE_SET_SF_ENUM_TYPE( wrapS, Wrap);
    SO_NODE_SET_SF_ENUM_TYPE( wrapT, Wrap);

    SO_NODE_SET_MF_ENUM_TYPE( operation, Operator);


    SO_NODE_DEFINE_ENUM_VALUE( Wrap, SoTexture2::REPEAT );
    SO_NODE_DEFINE_ENUM_VALUE( Wrap, SoTexture2::CLAMP );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::MODULATE );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::DECAL );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::BLEND );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::REPLACE );
    SO_NODE_SET_SF_ENUM_TYPE( model, Model );
   
#define mImageSensor( var ) \
    var##sensor =  new SoFieldSensor(imageChangeCB, this); \
    var##sensor->attach(&var)

    mImageSensor( numcolor );
    mImageSensor( image );
    mImageSensor( colors );
    mImageSensor( operation );
    mImageSensor( component );
}


SoMultiTexture2::~SoMultiTexture2()
{
    delete imagesensor;
    delete numcolorsensor;
    delete colorssensor;
    delete operationsensor;
    delete componentsensor;

    delete glimagemutex;
    delete [] imagedata;
}

static SoGLImage::Wrap
translateWrap(const SoTexture2::Wrap wrap)
{
      if (wrap == SoTexture2::REPEAT) return SoGLImage::REPEAT;
        return SoGLImage::CLAMP;
}

void SoMultiTexture2::GLRender( SoGLRenderAction * action )
{
    SoState * state = action->getState();

    if ( SoTextureOverrideElement::getImageOverride(state) )
	return;

    const float quality = SoTextureQualityElement::get(state);

    const cc_glglue* glue =
			cc_glglue_instance(SoGLCacheContextElement::get(state));

    const bool needbig = false; //Hack since SoTextureScalePolicyElement is gone
    const SoType glimagetype = glimage ?
	glimage->getTypeId() : SoType::badType();

    glimagemutex->lock();
    if ( !glimagevalid ||
	 ( needbig && glimagetype!=SoGLBigImage::getClassTypeId() ) ||
	 ( !needbig && glimagetype!=SoGLImage::getClassTypeId() ) )
    {
	if ( needbig && glimagetype!=SoGLBigImage::getClassTypeId() )
	{
	    if ( glimage ) glimage->unref(state);
	    glimage = new SoGLBigImage();
	}
	else if ( !needbig && glimagetype!=SoGLImage::getClassTypeId() )
	{
	    if ( glimage ) glimage->unref(state);
	    glimage = new SoGLImage();
	}

	if ( imagedata ) delete [] imagedata;
	imagedata = createImage(imagesize,imagenc);
	if ( imagedata && imagesize[0] && imagesize[1] )
	{
	    glimage->setData( imagedata, imagesize, imagenc,
		    translateWrap((SoTexture2::Wrap)wrapS.getValue()),
		    translateWrap((SoTexture2::Wrap)wrapT.getValue()),
		    quality);
	    glimagevalid = true;
	    SoCacheElement::setInvalid(true);
	    if ( state->isCacheOpen() )
		SoCacheElement::invalidate(state);
	}
    }

    if ( glimage && glimage->getTypeId() == SoGLBigImage::getClassTypeId() )
	SoCacheElement::invalidate(state);

    glimagemutex->unlock();

    SoTextureImageElement::Model glmodel =
	(SoTextureImageElement::Model) this->model.getValue();

    if ( glmodel==SoTextureImageElement::REPLACE)
    {
	if ( !cc_glglue_glversion_matches_at_least(glue, 1, 1, 0) )
	{
	    static int didwarn = 0;
	    if ( !didwarn )
	    {
		SoDebugError::postWarning("SoMultiTexture2::GLRender",
				"Unable to use the GL_REPLACE texture model. "
				"Your OpenGL version is < 1.1. "
				"Using GL_MODULATE instead.");
		didwarn = 1;
	    }

	    glmodel = SoTextureImageElement::MODULATE;
	}
    }

    doAction( action );
}


void SoMultiTexture2::doAction( SoAction* action )
{
    SoState * state = action->getState();
    const cc_glglue* glue =
			cc_glglue_instance(SoGLCacheContextElement::get(state));
    const float quality = SoTextureQualityElement::get(state);

    const int unit = SoTextureUnitElement::get(state);
    const int maxunits = cc_glglue_max_texture_units(glue);
    if ( !unit )
    {
	SoTextureImageElement::Model glmodel =
	    (SoTextureImageElement::Model) model.getValue();
	SoGLTextureImageElement::set( state, this, glimagevalid ? glimage : 0,
				      glmodel, blendColor.getValue() );
	SoGLTexture3EnabledElement::set(state, this, FALSE);
	SoGLTextureEnabledElement::set(state, this, glimagevalid && quality>0 );

	if ( isOverride() )
	    SoTextureOverrideElement::setImageOverride(state, TRUE);
    }
    else if ( unit<maxunits )
    {
	 SoGLMultiTextureEnabledElement::set( state, this, unit,
		 			      glimagevalid && quality>0 );
    }
}


void SoMultiTexture2::callback(SoCallbackAction * action)
{
    doAction( action );
}


void SoMultiTexture2::rayPick(SoRayPickAction * action)
{
    doAction( action );
}


#define mCondErrRet( cond, text )\
if ( cond ) \
{\
    static bool didwarn = 0;\
    if ( !didwarn )\
    {\
	SoDebugError::postWarning("SoMultiTexture2::createImage", text );\
	didwarn = true;\
    }\
\
    return 0;\
}


const unsigned char* SoMultiTexture2::createImage( SbVec2s& size, int& nc) const
{
    const int numimages = image.getNum();
    if ( !numimages )
	return 0;

    SbVec2s coltabsz;
    int coltabnc;
    unsigned const char* coltab = colors.getValue( coltabsz, coltabnc );
    const int nrcolors = coltab ? coltabsz[0] * coltabsz[1] : 0;

    bool hastransperancy = false;
    int coltabstart = 0;
    for ( int idx=0; idx<numimages; idx++ )
    {
	const int numcolors = idx>=numcolor.getNum() ? 0 : numcolor[idx];
	const bool iscoltab = numcolors>0;

	SbVec2s lsz;
	int lnc;
	const unsigned char* bytes = image[idx].getValue(lsz,lnc);
	mCondErrRet( iscoltab && lnc!=1,
		     "Coltab image must be single component.");

	mCondErrRet( lnc!=1 && lnc!=3 && lnc!=4,
		     "Coltab must have either one, three or four components" );

	const short comp = idx>=component.getNum()
	    ? RED | GREEN | BLUE | (lnc==4?OPACITY:0)
	    : component[idx];
	const bool doopacity = comp & (OPACITY^-1);

	mCondErrRet( doopacity && lnc==3,
		     "Operations on opacity can ony be done with one or "
		      "four components");

	if ( !idx ) size = lsz;
	else mCondErrRet( size!=lsz, "Images have different size" );

	if ( !hastransperancy && doopacity )
	{
	    if ( iscoltab )
	    {
		if ( findTransperancy( coltab+coltabstart,
			    	       numcolors, coltabnc,
				       bytes, lsz[0]*lsz[1]) )
		    hastransperancy = true;
	    }
	    else
	    {
		if ( findTransperancy( bytes, lsz[0]*lsz[1], lnc, 0, 0 ) )
		    hastransperancy = true;
	    }
	}

	coltabstart += numcolors;
    }

    const int nrpixels = size[0]*size[1];
    nc = hastransperancy ? 4 : 3;

    unsigned char* res = new unsigned char[nrpixels*nc];
    memset( res, 255, nrpixels*nc );

    for ( int idx=0; idx<numimages; idx++ )
    {
	SbVec2s lsz; int lnc;
	const unsigned char* bytes = image[idx].getValue(lsz,lnc);

	const int numcolors = idx>=numcolor.getNum() ? 0 : numcolor[idx];
	const bool iscoltab = numcolors>0;

	Operator oper = idx>=operation.getNum()
	    ? BLEND : (Operator) operation[idx];

	if ( !idx && oper!=REPLACE )
	{
	    static bool didwarn = false;
	    if ( !didwarn )
	    {
		SoDebugError::postWarning("SoMultiTexture2::createImage",
				"Operator != REPLACE is invalid for "
				"first texture. Using REPLACE." );
		didwarn = 1;
	    }

	    oper = REPLACE;
	}

	const short comp = idx>=component.getNum()
	    ? RED | GREEN | BLUE | (nc==4?OPACITY:0)
	    : component[idx];
	const bool mask[] =
		{ comp & (RED^-1), comp & (GREEN^-1),
		  comp & (BLUE^-1), comp & (OPACITY^-1) };

	for ( int idy=0; idy<nrpixels; idy++ )
	{
	    unsigned char pixelcolor[4];
	    if ( iscoltab )
	    {
		int index = bytes[idy];
		if ( index>=numcolors ) index=numcolors-1;

		index += coltabstart;
		if ( index+coltabstart>=nrcolors ) index=nrcolors-1;

		memcpy( pixelcolor, coltab+(index*coltabnc), coltabnc );
		if ( coltabnc==3 ) pixelcolor[3] = 0;
	    }
	    else
	    {
		if ( lnc==1 )
		{
		    pixelcolor[0] = pixelcolor[1] =
	            pixelcolor[2] = pixelcolor[3] = bytes[idy];
		}
		else
		{
		    if ( lnc==3 )
		    {
			memcpy( pixelcolor, bytes+idy*3, 3 );
			pixelcolor[3] = 0;
		    }
		    else
			memcpy( pixelcolor, bytes+idy*4, 4 );
		}
	    }

	    unsigned char* oldval = res +idy*nc;

	    unsigned char invtrans = pixelcolor[3];
	    unsigned char trans = 255-invtrans;

	    for ( int comp=nc-1; comp>=0; comp-- )
	    {
		if ( !mask[comp] )
		    continue;

		if ( oper==BLEND )
		{
		    oldval[comp] = (int)((int)oldval[comp] * trans +
				         (int)pixelcolor[comp] * invtrans)/255;
		}
		else if ( oper==ADD )
		    oldval[comp] = (int)oldval[comp] + pixelcolor[comp] >> 1;
		else 
		    oldval[comp] =  pixelcolor[comp];
	    }
	}
    }

    return res;
}


bool SoMultiTexture2::findTransperancy( const unsigned char* colors, int ncol,
					int nc, const unsigned char* idxs,
					int nidx )
{
    if ( nc<4 ) return false;

    if ( idxs )
    {
	for ( int idx=0; idx<nidx; idx++ )
	{
	    unsigned char index = idxs[idx];
	    if ( index>=ncol )
		continue;

	    if ( colors[index*nc+3]!=255 )
		return true;
	}
    }
    else
    {
	for ( int idx=0; idx<ncol; idx++ )
	{
	    if ( colors[idx*nc+3]!=255 )
		return true;
	}
    }

    return false;
}







void SoMultiTexture2::imageChangeCB( void* data, SoSensor* )
{
    SoMultiTexture2* ptr = (SoMultiTexture2*) data;
    ptr->glimagevalid = false;
}

