/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visvolrenscalarfield.h"

#include "arraynd.h"
#include "draw.h"
#include "envvars.h"
#include "genc.h"
#include "iopar.h"
#include "vismaterial.h"
#include "valseries.h"
#include "visdataman.h"
#include "visrgbatexturechannel2rgba.h"
#include "od_ostream.h"

#include "ostream"

#include <stdint.h>
#include <osgVolume/Volume>
#include <osgVolume/VolumeTile>
#include <osg/TransferFunction>
#include <osg/CullFace>

#include <osgGeo/VolumeTechniques>


mCreateFactoryEntry( visBase::VolumeRenderScalarField );

namespace visBase
{

#define mNrColors 256

#define mTransparencyBendPower (100.0/255.0)
/* By nature the cloud-like transparancy of volumes tends to be responsive
   for the highest transparancies only. This setting at least guarantees that
   all discrete values of the transparency byte will be adressed at the high
   side of the transparency range after mapping the 1-100 UI slider scale. */


VolumeRenderScalarField::AttribData::AttribData()
    : sz0_( 1 )
    , sz1_( 1 )
    , sz2_( 1 )
    , indexcache_( 0 )
    , indexcachestep_( 0 )
    , ownsindexcache_( false )
    , datacache_( 0 )
    , ownsdatacache_( false )
{}


VolumeRenderScalarField::AttribData::~AttribData()
{
    if ( ownsindexcache_ && indexcache_ )
	delete [] indexcache_;

    if ( ownsdatacache_ && datacache_ )
	delete datacache_;
}


od_int64 VolumeRenderScalarField::AttribData::totalSz() const
{
    return sz0_ * sz1_ * sz2_;
}


bool VolumeRenderScalarField::AttribData::isInVolumeCache() const
{
    return indexcachestep_==4;
}


#define mCheckAttribStore( attr ) \
    if ( attr<0 || attr>3 ) \
	return; \
    while ( !attribs_.validIdx(attr) ) \
	attribs_ += new AttribData();


VolumeRenderScalarField::VolumeRenderScalarField()
    : material_( 0 )
    , channels2rgba_( 0 )
    , isrgba_( false )
    , useshading_( true )
    , raytt_( 0 )
    , osgvoltile_( new osgVolume::VolumeTile() )
    , osgvolume_( new osgVolume::Volume() )
    , osgvolroot_( new osg::Switch() )
    , osgimagelayer_( new osgVolume::ImageLayer() )
    , osgvoldata_( new osg::Image() )
    , osgtransfunc_( new osg::TransferFunction1D() )
    , osgtransprop_( new osgVolume::TransparencyProperty(1.0) )
{
    attribs_ += new AttribData();	// Need at least one for dummy returns

    setOsgNode( osgvolroot_ );
    osgvolroot_->ref();
    osgvoltile_->ref();
    osgvolume_->ref();
    osgimagelayer_->ref();
    osgvoldata_->ref();
    osgtransfunc_->ref();

    osgimagelayer_->setImage( osgvoldata_ );
    osgvolume_->addChild( osgvoltile_ );
    osgvoltile_->setLayer( osgimagelayer_ );
    osgvolroot_->addChild( osgvolume_ );

    osgVolume::AlphaFuncProperty* alpha =
			new osgVolume::AlphaFuncProperty( 0.0 );
    osgVolume::SampleDensityProperty* sampledensity =
			new osgVolume::SampleDensityProperty( 0.005 );
    osgVolume::SampleDensityWhenMovingProperty* movingsampledensity =
			new osgVolume::SampleDensityWhenMovingProperty( 0.005 );
//    osgVolume::IsoSurfaceProperty* isosurface =
//			new osgVolume::IsoSurfaceProperty( 0.99 );

    osgtransfunc_->allocate( mNrColors );
    osg::ref_ptr<osgVolume::TransferFunctionProperty> transferfunction =
		    new osgVolume::TransferFunctionProperty( osgtransfunc_ );

    osgVolume::CompositeProperty* compprop = new osgVolume::CompositeProperty;
    compprop->addProperty( alpha );
    compprop->addProperty( sampledensity );
    compprop->addProperty( movingsampledensity );
    compprop->addProperty( osgtransprop_ );

    compprop->addProperty( transferfunction );
//    compprop->addProperty( isosurface );
//    compprop->addProperty( new osgVolume::LightingProperty );
    osgimagelayer_->addProperty( compprop );

    enableTextureInterpolation( true );
    allowShading( useshading_ );
}


VolumeRenderScalarField::~VolumeRenderScalarField()
{
    deepErase( attribs_ );

    osgvolroot_->unref();
    osgvoltile_->unref();
    osgvolume_->unref();
    osgimagelayer_->unref();
    osgvoldata_->unref();
    osgtransfunc_->unref();

    setMaterial( 0 );
    setChannels2RGBA( 0 );
}


void VolumeRenderScalarField::setChannels2RGBA(
					visBase::TextureChannel2RGBA* tc2rgba )
{
    if ( channels2rgba_ )
	channels2rgba_->unRef();

    channels2rgba_ = tc2rgba;

    if ( channels2rgba_ )
	channels2rgba_->ref();

    mDynamicCast( visBase::RGBATextureChannel2RGBA*, isrgba_, channels2rgba_ );
    updateFragShaderType();
}


TextureChannel2RGBA* VolumeRenderScalarField::getChannels2RGBA()
{
    return channels2rgba_;
}


const TextureChannel2RGBA* VolumeRenderScalarField::getChannels2RGBA() const
{
    return channels2rgba_;
}


bool VolumeRenderScalarField::textureInterpolationEnabled() const
{
    return osgimagelayer_->getMagFilter()==osg::Texture::LINEAR;
}


void VolumeRenderScalarField::enableTextureInterpolation( bool yn )
{
/*  Mipmap is said not to do a good job, and that shows unless colormap
    is non-transparent and sample density is set very high.
    osgimagelayer_->setMinFilter( yn ? osg::Texture::LINEAR_MIPMAP_LINEAR :
				       osg::Texture::NEAREST_MIPMAP_NEAREST );
*/
    osgimagelayer_->setMinFilter( yn ? osg::Texture::LINEAR :
				       osg::Texture::NEAREST );
    osgimagelayer_->setMagFilter( yn ? osg::Texture::LINEAR :
				       osg::Texture::NEAREST );
    osgvoltile_->setDirty( true );
}


/* Modified code from OpenSceneGraph/examples/osgvolume/osgvolume.cpp */

static osg::Matrix getLocatorMatrix( const Coord3& trans, const Coord3& rotvec,
				     double rotangle, const Coord3& scale )
{
    osg::Matrix mat = osg::Matrix::scale( Conv::to<osg::Vec3d>(scale) );
    mat *= osg::Matrix::rotate(
			osg::Quat(rotangle,Conv::to<osg::Vec3d>(rotvec)) );
    mat *= osg::Matrix::translate( Conv::to<osg::Vec3d>(trans) );

    return mat;
}


void VolumeRenderScalarField::setTexVolumeTransform( const Coord3& trans,
				    const Coord3& rotvec, double rotangle,
				    const Coord3& scale )
{
    osg::Matrix mat = getLocatorMatrix( trans, rotvec, rotangle, scale );
    if ( !isRightHandSystem() )
    {
	mat.preMult( osg::Matrix::scale(-1.0, 1.0, 1.0) );
	mat.preMult( osg::Matrix::translate(-1.0, 0.0, 0.0) );
    }

    osgimagelayer_->setLocator( new osgVolume::Locator(mat) );
}


void VolumeRenderScalarField::setROIVolumeTransform( const Coord3& trans,
				    const Coord3& rotvec, double rotangle,
				    const Coord3& scale )
{
    osg::Matrix mat = getLocatorMatrix( trans, rotvec, rotangle, scale );
    osgvoltile_->setLocator( new osgVolume::Locator(mat) );
    osgvoltile_->setDirty( true );
}


bool VolumeRenderScalarField::isShadingSupported()
{
    return osgGeo::RayTracedTechnique::isShadingSupported();
}


void VolumeRenderScalarField::allowShading( bool yn )
{
    if ( osgvoldata_->data() )	// ignore calls after initialization
	return;

    if ( yn && !isShadingSupported() )
	yn = false;

    if ( useshading_==yn && osgvoltile_->getVolumeTechnique() )
	return;

    if ( yn )
    {
	raytt_ = new osgGeo::RayTracedTechnique( true );
	updateFragShaderType();
	osgvoltile_->setVolumeTechnique( raytt_ );
    }
    else
    {
	osgvoltile_->setVolumeTechnique( new osgGeo::FixedFunctionTechnique );
	raytt_ = 0;
    }

    useshading_ = yn;
    updateVolumeSlicing();
    updateTransparencyRescaling();
}


bool VolumeRenderScalarField::usesShading() const
{
    return useshading_;
}


void VolumeRenderScalarField::updateFragShaderType()
{
    if ( raytt_ )
    {
	if ( isrgba_ )
	    raytt_->setFragShaderType( osgGeo::RayTracedTechnique::RGBA );
	else
	{
	    raytt_->setFragShaderType( osgGeo::RayTracedTechnique::ColTab );
	    raytt_->setColTabValueChannel( 0 );
	    raytt_->setColTabUndefChannel( 3 );
	    raytt_->setColTabUndefValue( 1.0 );
	    raytt_->invertColTabUndefChannel( true );
	}
    }
}


void VolumeRenderScalarField::updateVolumeSlicing()
{
    mDynamicCastGet( osgGeo::FixedFunctionTechnique*, fft,
		     osgvoltile_->getVolumeTechnique() );
    if ( !fft )
	return;

    int maxlen = 1;
    for ( int idx=0; idx<attribs_.size(); idx++ )
    {
	maxlen = mMAX( attribs_[idx]->sz0_,
		       mMAX( attribs_[idx]->sz1_,
			     mMAX( attribs_[idx]->sz2_, maxlen ) ) );
    }

    fft->setNumSlices( 8*maxlen );     // Empirical
}


bool VolumeRenderScalarField::turnOn( bool yn )
{
    const bool wason = isOn();
    osgvolroot_->setValue( 0, yn );
    return wason;
}


bool VolumeRenderScalarField::isOn() const
{
    return osgvolroot_->getValue(0);
}


void VolumeRenderScalarField::setScalarField( int attr,
					      const Array3D<float>* sc,
					      bool mine, TaskRunner* tr )
{
    mCheckAttribStore( attr );

    if ( !sc )
    {
	if ( attribs_[attr]->ownsdatacache_ )
	    delete attribs_[attr]->datacache_;
	attribs_[attr]->datacache_ = 0;
	attribs_[attr]->ownsdatacache_ = false;
	return;
    }

    const bool isresize = sc->info().getSize(0)!=attribs_[attr]->sz0_ ||
			  sc->info().getSize(1)!=attribs_[attr]->sz1_ ||
			  sc->info().getSize(2)!=attribs_[attr]->sz2_;

    const od_int64 totalsz = sc->info().getTotalSz();

    bool doset = false;
    if ( isresize )
    {
	attribs_[attr]->sz0_ = sc->info().getSize( 0 );
	attribs_[attr]->sz1_ = sc->info().getSize( 1 );
	attribs_[attr]->sz2_ = sc->info().getSize( 2 );
	doset = true;

	if ( attribs_[attr]->ownsindexcache_ )
	    delete [] attribs_[attr]->indexcache_;
	attribs_[attr]->indexcache_ = 0;
	attribs_[attr]->indexcachestep_ = 0;
	attribs_[attr]->ownsindexcache_ = false;

	updateVolumeSlicing();
    }

    if ( attribs_[attr]->ownsdatacache_ )
	delete attribs_[attr]->datacache_;

    attribs_[attr]->ownsdatacache_ = mine;
    attribs_[attr]->datacache_ = sc->getStorage();
    if ( !attribs_[attr]->datacache_ || !attribs_[attr]->datacache_->arr() )
    {
	MultiArrayValueSeries<float,float>* myvalser =
	    new MultiArrayValueSeries<float,float>( totalsz );
	if ( !myvalser || !myvalser->isOK() )
	    delete myvalser;
	else
	{
	    sc->getAll( *myvalser );

	    attribs_[attr]->datacache_ = myvalser;
	    attribs_[attr]->ownsdatacache_ = true;
	}
    }

    //TODO: if 8-bit data & some flags, use data itself
    if ( attribs_[attr]->mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
//	attribs_[attr]->mapper_.setData(attribs_[attr]->datacache_,totalsz,tr);
	clipData( attr, tr );

    makeIndices( attr, doset, tr );
}


const ColTab::Mapper& VolumeRenderScalarField::getColTabMapper( int attr )
{
    return attribs_[attribs_.validIdx(attr) ? attr : 0]->mapper_;
}


void VolumeRenderScalarField::setColTabMapperSetup( int attr,
				const ColTab::MapperSetup& ms, TaskRunner* tr )
{
    mCheckAttribStore( attr );

    if ( attribs_[attr]->mapper_.setup_ == ms )
	return;

//    const bool autoscalechange =
//	attribs_[attr]->mapper_.setup_.type_ != ms.type_;

    attribs_[attr]->mapper_.setup_ = ms;

    /*if ( autoscalechange )
	attribs_[attr]->mapper_.setup_.triggerAutoscaleChange();
    else
	attribs_[attr]->mapper_.setup_.triggerRangeChange();*/

    if ( attribs_[attr]->mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	clipData( attr, tr );

    makeIndices( attr, false, tr );
}


const TypeSet<float>& VolumeRenderScalarField::getHistogram( int attr ) const
{
    return attribs_[attribs_.validIdx(attr) ? attr : 0]->histogram_;
}


void VolumeRenderScalarField::clipData( int attr, TaskRunner* tr )
{
    mCheckAttribStore( attr );

    if ( !attribs_[attr]->datacache_ )
	return;

    attribs_[attr]->mapper_.setData( attribs_[attr]->datacache_,
				     attribs_[attr]->totalSz(), tr );
    attribs_[attr]->mapper_.setup_.triggerRangeChange();
}


void VolumeRenderScalarField::makeColorTables( int attr )
{
    mCheckAttribStore( attr );

    const ColTab::Sequence* sequence =
		getChannels2RGBA() ? getChannels2RGBA()->getSequence(attr) : 0;

    if ( !sequence || isrgba_ )
	return;

    osg::TransferFunction1D::ColorMap colmap;
    for ( int idx=0; idx<mNrColors-1; idx++ )
    {
	const float idxfrac = mCast(float,idx) / (mNrColors-2);
	const Color col = sequence->color( idxfrac );
	colmap[ mCast(float,idx)/(mNrColors-1) ] = Conv::to<osg::Vec4>( col );
    }

    colmap[1.0] = Conv::to<osg::Vec4>( sequence->undefColor() );
    osgtransfunc_->assign( colmap );
    if ( raytt_ )
	raytt_->setColTabUndefColor( colmap[1.0] );

    if ( !useshading_ )
	makeIndices( attr, false, 0 );
}


// May be parallelized if necessary
#define mStepwiseDataIteration( operation ) \
    while ( (nrbytes--) > 0 ) \
	{ operation; destptr += deststep; srcptr += srcstep; }

static void copyDataStepwise( od_int64 nrbytes,
			      unsigned char* srcptr, int srcstep,
			      unsigned char* destptr, int deststep,
			      bool mutual=false )
{
    if ( mutual )
	mStepwiseDataIteration( Swap(*destptr,*srcptr) )
    else
	mStepwiseDataIteration( *destptr=*srcptr )
}


static void addDataStepwise( od_int64 nrbytes,
			     unsigned char* srcptr, int srcstep,
			     unsigned char* destptr, int deststep )
{
    mStepwiseDataIteration( *destptr+=*srcptr )
}


void VolumeRenderScalarField::setDefaultRGBAValue( int channel )
{
    if ( channel>=0 && channel<=3 && osgvoldata_->data() )
    {
	unsigned char channeldefault = mRounded( unsigned char,
	    255.0 * osgGeo::RayTracedTechnique::getChannelDefaults()[channel] );

	copyDataStepwise( osgvoldata_->getTotalSizeInBytes()/4,
			  &channeldefault, 0, osgvoldata_->data()+channel, 4 );

	osgvoldata_->dirty();
    }
}


void VolumeRenderScalarField::makeIndices( int attr, bool doset, TaskRunner* tr)
{
    mCheckAttribStore( attr );

    if ( !attribs_[attr]->datacache_ )
	return;

    const od_int64 totalsz = attribs_[attr]->totalSz();

    if ( !useshading_ || isrgba_ )
    {
	if ( attribs_[attr]->sz2_!=osgvoldata_->s() ||
	     attribs_[attr]->sz1_!=osgvoldata_->t() ||
	     attribs_[attr]->sz0_!=osgvoldata_->r() )
	{
	    osgvoldata_->allocateImage( attribs_[attr]->sz2_,
					attribs_[attr]->sz1_,
					attribs_[attr]->sz0_,
					GL_RGBA, GL_UNSIGNED_BYTE );
	    if ( isrgba_ && !useshading_ )
	    {
		for ( int channel=0; channel<4; channel++ )
		    setDefaultRGBAValue( channel );
	    }
	}
    }

    const bool attribenabled = getChannels2RGBA() &&
			       getChannels2RGBA()->isEnabled(attr);

    const bool usevolcache = isrgba_ && (useshading_ || attribenabled);
    const bool hasundefchannel = !isrgba_ && useshading_;

    if ( !attribs_[attr]->indexcache_ )
    {
	const int offset = raytt_ ? raytt_->getSourceChannel(attr) : attr;
	attribs_[attr]->indexcachestep_ = usevolcache ? 4 :
					  hasundefchannel ? 2 : 1;
	const od_int64 nrbytes = totalsz * attribs_[attr]->indexcachestep_;
	attribs_[attr]->indexcache_ = usevolcache ? osgvoldata_->data()+offset
						  : new unsigned char[nrbytes];
	attribs_[attr]->ownsindexcache_ = !usevolcache;
    }

    if ( usevolcache && !attribenabled )
	setDefaultRGBAValue( attr );

    // Reverse the index order, because osgVolume turns out to perform well
    // for one sense of the coordinate system only, and OD uses the other. A
    // transform in visSurvey::VolumeDisplay does the geometrical mirroring.
    const int idxstep = -attribs_[attr]->indexcachestep_;
    unsigned char* idxptr = attribs_[attr]->indexcache_ - (totalsz-1)*idxstep;
//    const int idxstep = attribs_[attr]->indexcachestep_;
//    unsigned char* idxptr = attribs_[attr]->indexcache_;
    unsigned char* udfptr = hasundefchannel ? idxptr+1 : 0;
    ColTab::MapperTask<unsigned char> indexer( attribs_[attr]->mapper_,
			    totalsz, mNrColors-1, *attribs_[attr]->datacache_,
			    idxptr, idxstep, udfptr, idxstep );

    if ( tr ? !tr->execute(indexer) : !indexer.execute() )
	return;

    int max = 0;
    const unsigned int* histogram = indexer.getHistogram();
    for ( int idx=mNrColors-2; idx>=0; idx-- )
    {
	if ( histogram[idx]>max )
	    max = histogram[idx];
    }

    if ( max )
    {
	attribs_[attr]->histogram_.setSize( mNrColors-1, 0 );
	for ( int idx=mNrColors-2; idx>=0; idx-- )
	    attribs_[attr]->histogram_[idx] = (float) histogram[idx] / max;
    }

    unsigned char one = 1;
    if ( isrgba_ && attr!=3 )
    {
	// wrap indices by 1 to move RGBA undef color to default Vec4(0,0,0,1)
	addDataStepwise( totalsz, &one, 0, attribs_[attr]->indexcache_,
			 attribs_[attr]->indexcachestep_ );
    }

    if ( !useshading_ && !isrgba_ )
    {
	unsigned char coltab[1024];
	for ( int idx=0; idx<=255; idx++ )
	{
	    const Color col = Conv::to<Color>(
			    osgtransfunc_->getColor( mCast(float,idx)/255 ) );
	    coltab[4*idx+0] = col.r();
	    coltab[4*idx+1] = col.g();
	    coltab[4*idx+2] = col.b();
	    coltab[4*idx+3] = 255-col.t();
	}

	unsigned char* ptr = osgvoldata_->data();
	for ( od_int64 idx=0; idx<totalsz; idx++ )
	{
	    OD::memCopy( ptr , coltab+4*attribs_[attr]->indexcache_[idx], 4 );
	    ptr += 4;
	}
    }
    else if ( doset && !isrgba_ )
    {
	osgvoldata_->setImage( attribs_[attr]->sz2_, attribs_[attr]->sz1_,
			       attribs_[attr]->sz0_, GL_LUMINANCE_ALPHA,
			       GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
			       attribs_[attr]->indexcache_,
			       osg::Image::NO_DELETE, 1 );
    }
    else
	osgvoldata_->dirty();

    osgvoltile_->setDirty( true );
}


void VolumeRenderScalarField::enableAttrib( int attr, bool yn )
{
    mCheckAttribStore( attr );

    if ( raytt_ )
    {
	raytt_->enableDestChannel( attr, yn );
    }
    else if ( isrgba_ && osgvoldata_->data() && attribs_[attr]->indexcache_ &&
	      yn!=attribs_[attr]->isInVolumeCache() )
    {
	const od_int64 totalsz = attribs_[attr]->totalSz();
	const int deststep = yn ? 4 : 1;
	unsigned char* destptr = yn ? osgvoldata_->data()+attr
				    : new unsigned char[totalsz];

	copyDataStepwise( totalsz, attribs_[attr]->indexcache_,
			  attribs_[attr]->indexcachestep_, destptr, deststep );
	if ( yn )
	    delete [] attribs_[attr]->indexcache_;
	else
	    setDefaultRGBAValue( attr );

	attribs_[attr]->indexcache_ = destptr;
	attribs_[attr]->indexcachestep_ = deststep;
	attribs_[attr]->ownsindexcache_ = !yn;

	osgvoldata_->dirty();
    }
}


void VolumeRenderScalarField::swapAttribs( int attr0, int attr1 )
{
    if ( attr0 == attr1 )
	return;

    mCheckAttribStore( attr0 );
    mCheckAttribStore( attr1 );
    attribs_.swap( attr0, attr1 );

    if ( raytt_ )
    {
	const int sourcechannel0 = raytt_->getSourceChannel( attr0 );
	const int sourcechannel1 = raytt_->getSourceChannel( attr1 );
	raytt_->setSourceChannel( attr0, sourcechannel1 );
	raytt_->setSourceChannel( attr1, sourcechannel0 );
	return;
    }

    if ( attribs_[attr0]->isInVolumeCache() )
	Swap( attr0, attr1 );
    else if ( !attribs_[attr1]->isInVolumeCache() )
	return;

    const bool swapvoldata = attribs_[attr0]->isInVolumeCache();

    copyDataStepwise( osgvoldata_->getTotalSizeInBytes()/4,
		      osgvoldata_->data()+attr0, 4,
		      osgvoldata_->data()+attr1, 4, swapvoldata );

    attribs_[attr1]->indexcache_ = osgvoldata_->data() + attr1;

    if ( swapvoldata )
	attribs_[attr0]->indexcache_ = osgvoldata_->data() + attr0;
    else
	setDefaultRGBAValue( attr0 );

    osgvoldata_->dirty();
}


void VolumeRenderScalarField::setAttribTransparency( int attr,
						     unsigned char trans )
{
    const float rescaledtrans = pow( mCast(float,trans)/255.0,
				     mTransparencyBendPower );
    osgtransprop_->setValue( 1.0 - rescaledtrans );

    /* As long as only single layer textures are supported, setting OSG's
    transparency property will do. The FixedFunctionTechnique ignores this
    property. However, its material properties transparency already provides
    the same cloud-like effect as a result of its multi-plane implementation.
    That material properties transparency provides a much "thinner" effect
    for the projective implementation of the RayTracingTechnique. */
}


void VolumeRenderScalarField::setMaterial( Material* newmat )
{
    if ( material_ == newmat )
	return;

    if ( material_ )
    {
	material_->detachStateSet( osgvolume_->getOrCreateStateSet() );
	material_->unRef();
    }

    if ( !newmat )
	return;

    newmat->ref();
    newmat->attachStateSet( osgvolume_->getOrCreateStateSet() );
    material_ = newmat;

    osg::StateAttribute* attr = osgvolume_->getStateSet()->getAttribute(
					    osg::StateAttribute::MATERIAL );

    osgvolume_->getStateSet()->setAttribute( attr,
					    osg::StateAttribute::OVERRIDE );
    updateTransparencyRescaling();
}


void VolumeRenderScalarField::updateTransparencyRescaling()
{
    if ( material_ )
    {
	const float bendpower = useshading_ ? 1.0 : mTransparencyBendPower;
	material_->rescaleTransparency( bendpower );
    }
}


struct VolFileHeader
{
    uint32_t	magic_number;
    uint32_t	header_length;
    uint32_t	width;
    uint32_t	height;
    uint32_t	images;
    uint32_t	bits_per_voxel;
    uint32_t	index_bits;
    float	scaleX, scaleY, scaleZ;
    float	rotX, rotY, rotZ;
};

static uint32_t hton_uint32(uint32_t value)
{
#ifdef __islittle__
    SwapBytes( &value, 4 );
#endif
    return value;
}

static float hton_float(float value)
{
#ifdef __islittle__
    SwapBytes( &value, 4 );
#endif
    return value;
}


const char* VolumeRenderScalarField::writeVolumeFile( int attr,
						      od_ostream& strm ) const
{
    if ( !attribs_.validIdx(attr) || !attribs_[attr]->indexcache_ )
	return "Nothing to write";

    const char* writeerr = "Cannot write to stream";

    struct VolFileHeader vh = {
	hton_uint32(0x0b7e7759), // magic_number
	hton_uint32(sizeof(struct VolFileHeader)),
	0, 0, 0, // whatever -- these are replaced
	hton_uint32(8), hton_uint32(0),
	hton_float(1.0f), hton_float(1.0f), hton_float(1.0f),
	hton_float(0.0f), hton_float(0.0f), hton_float(0.0f)
      };

    vh.width = hton_uint32( attribs_[attr]->sz2_ );
    vh.height = hton_uint32( attribs_[attr]->sz1_ );
    vh.images = hton_uint32( attribs_[attr]->sz0_ );
    vh.bits_per_voxel = hton_uint32(8);

    if ( strm.addBin(vh).isBad() )
	return writeerr;

    const unsigned char* indexcacheptr = attribs_[attr]->indexcache_;
    const int indexcachestep = attribs_[attr]->indexcachestep_;

    if ( indexcachestep!=1 )
    {
	for ( int count=attribs_[attr]->totalSz(); count>0; count-- )
	{
	    if ( !strm.addBin(indexcacheptr,1) )
		return writeerr;

	    indexcacheptr += indexcachestep;
	}
    }
    else if ( !strm.addBin(indexcacheptr,attribs_[attr]->totalSz()) )
	return writeerr;

    return 0;
}


} // namespace visBase

