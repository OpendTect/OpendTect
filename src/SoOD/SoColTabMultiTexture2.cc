/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "SoColTabMultiTexture2.h"

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#if COIN_MAJOR_VERSION <= 3
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#endif
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
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
#include <Inventor/threads/SbThread.h>
#include <Inventor/threads/SbCondVar.h>
#include <Inventor/C/glue/gl.h>

#include "limits.h"
#include "SoOD.h"

class SoColTabMultiTextureProcessor
{
public:
    				SoColTabMultiTextureProcessor( int nrthreads );
    void			process( const SoColTabMultiTexture2&,
				         unsigned char* res,
				         int nc, int sz,
				         const unsigned char* coltab,
				         int coltabnc, int nrcolors );

				~SoColTabMultiTextureProcessor();

protected:
    bool			prepare( int idx );
    bool			process( int start, int stop );
    static void*		threadFunc( void* data );

    //Variables for this process (set in process(...) )
    const SoColTabMultiTexture2*	texture_;
    unsigned char*		output_;
    const unsigned char*	coltab_;
    int				coltabnc_;
    int				nc_;
    int				sz_;
    int				totalnumcolors_;
    bool			imageisinitialized_;

    //Variables for this layer (set in prepare)
    int				imagenc_;
    const unsigned char*	imagedata_;
    int				coltabstart_;
    int				imagenumcolors_;
    SoColTabMultiTexture2::Operator	imageoper_;
    bool			imagemask_[4];
    unsigned char		opacity_;

    //Thread management variables
    SbList<int>			threadstarts_;
    SbList<int>			threadstops_;
    SbMutex			threadrangemutex_;

    SbList<SbThread*>		threads_;

    SbMutex			threadmutex_;
    SbCondVar			threadcondvar_;
    SbList<bool>		startthread_;
    int				threadfinishcount_;
    bool			terminatethreads_;
};


SoColTabMultiTextureProcessor::SoColTabMultiTextureProcessor( int nrthreads )
    : terminatethreads_( false )
{
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	startthread_.append( false );

	threadrangemutex_.lock();
	SbThread* thread = SbThread::create( threadFunc, this );

	threadrangemutex_.lock();
	threadstarts_.append( -1 );
	threadstops_.append( -1 );
	threads_.append( thread );
	threadrangemutex_.unlock();
    }
}


SoColTabMultiTextureProcessor::~SoColTabMultiTextureProcessor()
{
    threadmutex_.lock();
    terminatethreads_ = true;
    threadcondvar_.wakeAll();
    threadmutex_.unlock();

    /* Cannot destroy since mutex is deleted when function goes out of scope,
       and the threads need the mutex when exiting. */
    for ( int idx=0; idx<threads_.getLength(); idx++ )
	SbThread::join( threads_[idx] ); 
}




void SoColTabMultiTextureProcessor::process( const SoColTabMultiTexture2& mt,
		  unsigned char* res, int nc, int sz, const unsigned char* ctab,
		  int ctabnc, int totalnumcols )
{
    texture_ = &mt;
    output_ = res;
    coltab_ = ctab;
    coltabnc_ = ctabnc;
    nc_ = nc;
    sz_ = sz;
    totalnumcolors_ = totalnumcols;
    coltabstart_ = 0;
    imageisinitialized_ = false;

    const int nrthreads = sz_>(threads_.getLength()*1000) ?
			  threads_.getLength() : 1;

    if ( nrthreads>1 )
    {
	const int idealnrperthread = sz/nrthreads;

	threadrangemutex_.lock();
	for ( int idx=threads_.getLength()-1; idx>=0; idx--)
	{
	    threadstarts_[idx] = -1;
	    threadstops_[idx] = -1;
	}

	int threadstart = 0;
	for ( int idx=0; idx<nrthreads; idx++ )
	{
	    if ( threadstart>=sz )
		break;

	    int threadstop = threadstart+idealnrperthread-1;
	    if ( idx==nrthreads-1 || threadstop>=sz )
		threadstop = sz-1;

	    threadstarts_[idx] = threadstart;
	    threadstops_[idx] = threadstop;
	    threadstart = threadstop+1;
	}

	threadrangemutex_.unlock();
    }

    for ( int iimg=0; iimg<texture_->image.getNum(); iimg++ )
    {
	if ( !prepare(iimg) )
	    continue;
	
	if ( nrthreads>1 )
	{
	    threadmutex_.lock();
	    for ( int idx=nrthreads-1; idx>=0; idx-- )
		startthread_[idx]=true;

	    threadfinishcount_ = nrthreads;

	    threadcondvar_.wakeAll();

	    while ( true )
	    {
		if ( threadfinishcount_ )
		    threadcondvar_.wait( threadmutex_ );
		if ( !threadfinishcount_ )
		{
		    threadmutex_.unlock();
		    break;
		}
	    }
	}
	else
	{
	    process( 0, sz_-1 );
	}

	coltabstart_ += imagenumcolors_; 
	imageisinitialized_ = true;
    }
}


bool SoColTabMultiTextureProcessor::prepare( int idx )
{
    SbVec2i32 lsz;
    imagedata_ = texture_->image[idx].getValue( lsz,imagenc_ );

    opacity_ = idx>=texture_->opacity.getNum()
	? 255
	: texture_->opacity[idx];

    if ( !opacity_ )
	return false;

    imagenumcolors_ = idx>=texture_->numcolor.getNum()
	? 0
	: texture_->numcolor[idx];

    imageoper_ = idx>=texture_->operation.getNum()
	    ? SoColTabMultiTexture2::BLEND
	    : (SoColTabMultiTexture2::Operator) texture_->operation[idx];

    
    if ( imageoper_!=SoColTabMultiTexture2::REPLACE )
    {
	if ( !idx )
	{
	    static bool didwarn = false;
	    if ( !didwarn )
	    {
		SoDebugError::postWarning("SoColTabMultiTexture2::createImage",
				    "Operator != REPLACE is invalid for "
				    "first texture. Using REPLACE." );
		didwarn = 1;
	    }

	    imageoper_ = SoColTabMultiTexture2::REPLACE;
	}

	if ( !imageisinitialized_ )
	    imageoper_ = SoColTabMultiTexture2::REPLACE;
    }


    const short comp = idx>=texture_->component.getNum()
	    ? SoColTabMultiTexture2::RED | SoColTabMultiTexture2::GREEN |
	      SoColTabMultiTexture2::BLUE | (nc_==4?SoColTabMultiTexture2::OPACITY:0)
	    : texture_->component[idx];
    if ( !texture_->enabled[idx] || !comp )
	return false;

    imagemask_[0] = comp & (SoColTabMultiTexture2::RED^-1);
    imagemask_[1] = comp & (SoColTabMultiTexture2::GREEN^-1);
    imagemask_[2] = comp & (SoColTabMultiTexture2::BLUE^-1);
    imagemask_[3] = comp & (SoColTabMultiTexture2::OPACITY^-1);

    return true;
}


void* SoColTabMultiTextureProcessor::threadFunc( void* data )
{
    SoColTabMultiTextureProcessor* thisp =
	reinterpret_cast<SoColTabMultiTextureProcessor*>( data );

    const int mythreadnr = thisp->threads_.getLength();
    thisp->threadrangemutex_.unlock();

    thisp->threadmutex_.lock();
    while ( true )
    {
	if ( !thisp->startthread_[mythreadnr] && !thisp->terminatethreads_ )
	    thisp->threadcondvar_.wait( thisp->threadmutex_ );

	if ( thisp->startthread_[mythreadnr] )
	{
	    thisp->threadmutex_.unlock();
	    const int start = thisp->threadstarts_[mythreadnr];
	    if ( start!=-1 )
		thisp->process( start, thisp->threadstops_[mythreadnr] );

	    thisp->startthread_[mythreadnr] = false;

	    thisp->threadmutex_.lock();
	    thisp->threadfinishcount_--;
	    if ( !thisp->threadfinishcount_ )
		thisp->threadcondvar_.wakeAll();
	    continue;
	}

	if ( thisp->terminatethreads_ )
	{
	    thisp->threadmutex_.unlock();
	    break;
	}
    }

    return 0;
}


bool SoColTabMultiTextureProcessor::process( int start, int stop )
{
    for ( int idy=start; idy<=stop; idy++ )
    {
	unsigned char pixelcolor[4];
	if ( imagenumcolors_ )
	{
	    int index = imagedata_[idy];
	    if ( index>=imagenumcolors_ ) index=imagenumcolors_-1;

	    index += coltabstart_;
	    if ( index>=totalnumcolors_ ) index=totalnumcolors_-1;

	    memcpy( pixelcolor, coltab_+(index*coltabnc_), coltabnc_ );
	    if ( coltabnc_==3 ) pixelcolor[3] = 0;
	}
	else
	{
	    if ( imagenc_==1 )
	    {
		pixelcolor[0] = pixelcolor[1] =
		pixelcolor[2] = pixelcolor[3] = imagedata_[idy];
	    }
	    else
	    {
		if ( imagenc_==3 )
		{
		    memcpy( pixelcolor, imagedata_+idy*3, 3 );
		    pixelcolor[3] = 0;
		}
		else
		    memcpy( pixelcolor, imagedata_+idy*4, 4 );
	    }
	}

	unsigned char* oldval = output_ + idy*nc_;

	pixelcolor[3] = (int) (pixelcolor[3]*opacity_)/255;
	unsigned char invtrans = (int) pixelcolor[3];
	unsigned char trans = 255-invtrans;

	for ( int comp=nc_-1; comp>=0; comp-- )
	{
	    if ( !imagemask_[comp] )
		continue;

	    if ( imageoper_==SoColTabMultiTexture2::BLEND )
	    {
		if ( comp==3 )
		    continue;

		oldval[comp] = (int)((int)oldval[comp] * trans +
				     (int)pixelcolor[comp] * invtrans)/255;
	    }
	    else if ( imageoper_==SoColTabMultiTexture2::ADD )
		oldval[comp] = ((int)oldval[comp] + pixelcolor[comp]) >> 1;
	    else 
		oldval[comp] =  pixelcolor[comp];
	}
    }

    return true;
}


SO_NODE_SOURCE( SoColTabMultiTexture2 );

void SoColTabMultiTexture2::initClass()
{
    SO_NODE_INIT_CLASS( SoColTabMultiTexture2, SoNode, "Node" );

    SO_ENABLE(SoGLRenderAction, SoGLTextureImageElement);
    SO_ENABLE(SoGLRenderAction, SoGLTextureEnabledElement);

    SO_ENABLE(SoGLRenderAction, SoGLMultiTextureImageElement);
    SO_ENABLE(SoGLRenderAction, SoGLMultiTextureEnabledElement);

    SO_ENABLE(SoCallbackAction, SoTextureImageElement);
    SO_ENABLE(SoCallbackAction, SoTextureEnabledElement);
    SO_ENABLE(SoCallbackAction, SoMultiTextureEnabledElement);
    SO_ENABLE(SoCallbackAction, SoMultiTextureImageElement);

#if COIN_MAJOR_VERSION <= 3
    SO_ENABLE(SoCallbackAction, SoTexture3EnabledElement);
    SO_ENABLE(SoGLRenderAction, SoGLTexture3EnabledElement);
    SO_ENABLE(SoRayPickAction, SoTexture3EnabledElement);
#endif

    SO_ENABLE(SoRayPickAction, SoTextureImageElement);
    SO_ENABLE(SoRayPickAction, SoTextureEnabledElement);
    SO_ENABLE(SoRayPickAction, SoMultiTextureEnabledElement);
    SO_ENABLE(SoRayPickAction, SoMultiTextureImageElement);
}


SoColTabMultiTexture2::SoColTabMultiTexture2()
    : imagesensor_( 0 )
    , numcolorsensor_( 0 )
    , colorssensor_( 0 )
    , operationsensor_( 0 )
    , componentsensor_( 0 )
    , enabledsensor_( 0 )
    , opacitysensor_( 0 )
    , glimagemutex_( new SbMutex )
    , glimagevalid_(false)
    , glimage_( 0 )
    , imagedata_( 0 )
    , imagesize_( 0, 0 )
    , imagenc_( 0 )
    , nrthreads_( 1 )
{
    SO_NODE_CONSTRUCTOR(SoColTabMultiTexture2);

    SO_NODE_ADD_FIELD( image, (SbImagei32(0, SbVec2i32(0,0), 0)) );
    SO_NODE_ADD_FIELD( numcolor, (0) );
    SO_NODE_ADD_FIELD( opacity, (255) );
    SO_NODE_ADD_FIELD( colors, (SbVec2s(0,0), 0, 0) );
    SO_NODE_ADD_FIELD( operation, (BLEND) );
    SO_NODE_ADD_FIELD( component, (RED|GREEN|BLUE|OPACITY) );
    SO_NODE_ADD_FIELD( enabled, (true) );
    SO_NODE_ADD_FIELD( blendColor, (0, 0, 0) );
    SO_NODE_ADD_FIELD( wrapS, (SoTexture2::REPEAT) );
    SO_NODE_ADD_FIELD( wrapT, (SoTexture2::REPEAT) );
    SO_NODE_ADD_FIELD( model, (SoTexture2::MODULATE) );

    SO_NODE_SET_SF_ENUM_TYPE( wrapS, Wrap);
    SO_NODE_SET_SF_ENUM_TYPE( wrapT, Wrap);

    SO_NODE_DEFINE_ENUM_VALUE( Operator, BLEND );
    SO_NODE_DEFINE_ENUM_VALUE( Operator, ADD );
    SO_NODE_DEFINE_ENUM_VALUE( Operator, REPLACE );
    SO_NODE_SET_MF_ENUM_TYPE( operation, Operator);


    SO_NODE_DEFINE_ENUM_VALUE( Wrap, SoTexture2::REPEAT );
    SO_NODE_DEFINE_ENUM_VALUE( Wrap, SoTexture2::CLAMP );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::MODULATE );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::DECAL );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::BLEND );
    SO_NODE_DEFINE_ENUM_VALUE( Model, SoTexture2::REPLACE );
    SO_NODE_SET_SF_ENUM_TYPE( model, Model );
   
#define mImageSensor( var ) \
    var##sensor_ =  new SoFieldSensor(imageChangeCB, this); \
    var##sensor_->attach(&var)

    mImageSensor( numcolor );
    mImageSensor( image );
    mImageSensor( colors );
    mImageSensor( operation );
    mImageSensor( component );
    mImageSensor( enabled );
    mImageSensor( opacity );
}


SoColTabMultiTexture2::~SoColTabMultiTexture2()
{
    delete imagesensor_;
    delete numcolorsensor_;
    delete colorssensor_;
    delete operationsensor_;
    delete componentsensor_;
    delete enabledsensor_;
    delete opacitysensor_;

    delete glimagemutex_;
    delete [] imagedata_;
}


int SoColTabMultiTexture2::getMaxSize()
{
    int res = SoOD::maxTexture2DSize();
    if ( res!=-1 )
	return res;

    return 2048; //conservative default
}


static SoGLImage::Wrap
translateWrap(const SoTexture2::Wrap wrap)
{
      if (wrap == SoTexture2::REPEAT) return SoGLImage::REPEAT;
        return SoGLImage::CLAMP;
}

void SoColTabMultiTexture2::GLRender( SoGLRenderAction * action )
{
    SoState * state = action->getState();

    if ( SoTextureOverrideElement::getImageOverride(state) )
	return;

    const float quality = SoTextureQualityElement::get(state);

    const cc_glglue* glue =
			cc_glglue_instance(SoGLCacheContextElement::get(state));

    const bool needbig = false; //Hack since SoTextureScalePolicyElement is gone
    const SoType glimagetype = glimage_ ?
	glimage_->getTypeId() : SoType::badType();

    glimagemutex_->lock();
    if ( !glimagevalid_ ||
	 ( needbig && glimagetype!=SoGLBigImage::getClassTypeId() ) ||
	 ( !needbig && glimagetype!=SoGLImage::getClassTypeId() ) )
    {
	if ( needbig && glimagetype!=SoGLBigImage::getClassTypeId() )
	{
	    if ( glimage_ ) glimage_->unref(state);
	    glimage_ = new SoGLBigImage();
	}
	else if ( !needbig && glimagetype!=SoGLImage::getClassTypeId() )
	{
	    if ( glimage_ ) glimage_->unref(state);
	    glimage_ = new SoGLImage();
	}

	if ( imagedata_ ) delete [] imagedata_;
	imagedata_ = createImage(imagesize_,imagenc_);
	if ( imagedata_ && imagesize_[0] && imagesize_[1] )
	{
	    glimage_->setData( imagedata_, imagesize_, imagenc_,
		    translateWrap((SoTexture2::Wrap)wrapS.getValue()),
		    translateWrap((SoTexture2::Wrap)wrapT.getValue()),
		    quality);
	    glimagevalid_ = true;
	    SoCacheElement::setInvalid(true);
	    if ( state->isCacheOpen() )
		SoCacheElement::invalidate(state);
	}
    }

    if ( glimage_ && glimage_->getTypeId() == SoGLBigImage::getClassTypeId() )
	SoCacheElement::invalidate(state);

    glimagemutex_->unlock();

    SoTextureImageElement::Model glmodel =
	(SoTextureImageElement::Model) this->model.getValue();

    if ( glmodel==SoTextureImageElement::REPLACE)
    {
	if ( !cc_glglue_glversion_matches_at_least(glue, 1, 1, 0) )
	{
	    static int didwarn = 0;
	    if ( !didwarn )
	    {
		SoDebugError::postWarning("SoColTabMultiTexture2::GLRender",
				"Unable to use the GL_REPLACE texture model. "
				"Your OpenGL version is < 1.1. "
				"Using GL_MODULATE instead.");
		didwarn = 1;
	    }

	    glmodel = SoTextureImageElement::MODULATE;
	}
    }

    const int unit = SoTextureUnitElement::get( state );
    const int maxunits = cc_glglue_max_texture_units( glue );
    if ( !unit )
    {
	SoGLTextureImageElement::set( state, this, glimagevalid_ ? glimage_ : 0,
				      glmodel, SbColor(0,0,0) );
#if COIN_MAJOR_VERSION <= 3
	SoGLTexture3EnabledElement::set( state, this, false );
	SoGLTextureEnabledElement::set(state, this, glimagevalid_ && quality );
#else
	SoGLMultiTextureEnabledElement::set( state, this, unit,
					     glimagevalid_ && quality );
#endif

	if ( isOverride() )
	    SoTextureOverrideElement::setImageOverride( state, true );
    }
    else if ( unit<maxunits )
    {
	SoGLMultiTextureImageElement::set( state, this, unit,
					   glimagevalid_ ? glimage_ : 0,
					   glmodel, SbColor(0,0,0));

	SoGLMultiTextureEnabledElement::set( state, this, unit,
					     glimagevalid_ && quality );
    }
}


void SoColTabMultiTexture2::doAction( SoAction* action )
{
    SoState * state = action->getState();

    const int unit = SoTextureUnitElement::get(state);
    if ( !unit && SoTextureOverrideElement::getImageOverride(state) )
	return;

    if ( !unit )
    {
#if COIN_MAJOR_VERSION <= 3
	SoTexture3EnabledElement::set(state, this, false );
#endif
	if ( imagesize_[0] && imagesize_[1] )
	{
	    SoTextureImageElement::set( state, this, imagesize_, imagenc_,
		    imagedata_,
		    (SoTextureImageElement::Wrap) wrapT.getValue(),
		    (SoTextureImageElement::Wrap) wrapS.getValue(),
		    (SoTextureImageElement::Model) model.getValue(),
		    SbColor(0,0,0) );
	    SoTextureEnabledElement::set(state, this, TRUE);
	}
	else
	{
	    SoTextureImageElement::setDefault(state, this);
	    SoTextureEnabledElement::set(state, this, false );
	}

	if ( isOverride() )
	{
	    SoTextureOverrideElement::setImageOverride(state, TRUE);
	}
    }
    else
    {
	if ( imagesize_[0] && imagesize_[1] )
	{
	    SoMultiTextureImageElement::set( state, this, unit, imagesize_,
		imagenc_, imagedata_,
		(SoTextureImageElement::Wrap) wrapT.getValue(),
		(SoTextureImageElement::Wrap) wrapS.getValue(),
		(SoTextureImageElement::Model) model.getValue(),
		SbColor( 0, 0, 0 ) );
	    SoMultiTextureEnabledElement::set( state, this, unit, TRUE);
	}
	else
	{
	    SoMultiTextureImageElement::setDefault( state, this, unit);
	    SoMultiTextureEnabledElement::set( state, this, unit, false );
	}
    }
}


void SoColTabMultiTexture2::callback(SoCallbackAction * action)
{
    doAction( action );
}


void SoColTabMultiTexture2::rayPick(SoRayPickAction * action)
{
    doAction( action );
}


void SoColTabMultiTexture2::setNrThreads( int nr )
{ nrthreads_ = nr; }


#define mCondErrRet( cond, text )\
if ( cond ) \
{\
    static bool didwarn = 0;\
    if ( !didwarn )\
    {\
	SoDebugError::postWarning("SoColTabMultiTexture2::createImage", text );\
	didwarn = true;\
    }\
\
    return 0;\
}


const unsigned char* SoColTabMultiTexture2::createImage( SbVec2s& size, int& nc )
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
    bool nrimagesused = 0;
    for ( int idx=0; idx<numimages; idx++ )
    {
	SbVec2i32 lsz;
	int lnc;
	const unsigned char* bytes = image[idx].getValue( lsz,lnc );
	const short comp = idx>=component.getNum()
	    ? RED | GREEN | BLUE | (lnc==4?OPACITY:0)
	    : component[idx];

	const int numcolors = idx>=numcolor.getNum() ? 0 : numcolor[idx];
	const unsigned char opacityval =
	    idx>=opacity.getNum() ? 255 : opacity[idx];

	if ( !enabled[idx] || !comp || !opacityval ) //nothing to do
	{
	    coltabstart += numcolors;
	    continue;
	}

	const bool iscoltab = numcolors>0;

	mCondErrRet( iscoltab && lnc!=1,
		     "Coltab image must be single component.");

	mCondErrRet( lnc!=1 && lnc!=3 && lnc!=4,
		     "Coltab must have either one, three or four components" );

	const bool doopacity = comp & (OPACITY^-1);

	mCondErrRet( opacityval==255 && doopacity && lnc==3,
		     "Operations on opacity can ony be done with one or "
		      "four components");

	if ( !nrimagesused ) 
	{
	    // Check if size is within limits
	    if ( lsz[0] > SHRT_MAX || lsz[1] > SHRT_MAX )
	    {
		SoDebugError::postWarning( "SoColTabMultiTexture2::GLRender",
		    "Texture is too large to be rendered!" );
	    }

	    size = SbVec2s( lsz[0], lsz[1] );
	}
	else mCondErrRet( size[0]!=lsz[0] || size[1]!=lsz[1], 
		"Images have different size" );

	if ( !hastransperancy && doopacity )
	{
	    if ( opacityval!=255 )
		hastransperancy = true;
	    else if ( iscoltab )
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
	nrimagesused++;
    }

    if ( !nrimagesused )
	return 0;

    const int nrpixels = size[0]*size[1];
    nc = hastransperancy || !nrimagesused ? 4 : 3;

    unsigned char* res = new unsigned char[nrpixels*nc];
    static SoColTabMultiTextureProcessor processor( nrthreads_ );
    processor.process( *this, res, nc, nrpixels, coltab,
		       coltabnc, nrcolors );

    return res;
}


bool SoColTabMultiTexture2::findTransperancy( const unsigned char* colors, 
	int ncol, int nc, const unsigned char* idxs, int nidx )
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


void SoColTabMultiTexture2::imageChangeCB( void* data, SoSensor* )
{
    SoColTabMultiTexture2* ptr = (SoColTabMultiTexture2*) data;
    ptr->glimagevalid_ = false;
}

