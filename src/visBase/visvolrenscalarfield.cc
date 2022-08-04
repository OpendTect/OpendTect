/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2004
-*/


#include "visvolrenscalarfield.h"

#include "arraynd.h"
#include "draw.h"
#include "envvars.h"
#include "genc.h"
#include "iopar.h"
#include "vismaterial.h"
#include "valseriesimpl.h"
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
    : indexcache_( 0 )
    , indexcachestep_( 0 )
    , ownsindexcache_( false )
    , datacache_( 0 )
    , ownsdatacache_( false )
    , datatkzs_( false )
    , resizecache_( 0 )
    , ownsresizecache_( false )
{}


VolumeRenderScalarField::AttribData::~AttribData()
{
    clearDataCache();
    clearResizeCache();
    clearIndexCache();
}


bool VolumeRenderScalarField::AttribData::isInVolumeCache() const
{
    return indexcachestep_==4;
}


void VolumeRenderScalarField::AttribData::clearDataCache()
{
    if ( ownsdatacache_ && datacache_ )
	delete datacache_;

    datacache_ = 0;
    ownsdatacache_ = false;
}


void VolumeRenderScalarField::AttribData::clearResizeCache()
{
    if ( ownsresizecache_ && resizecache_ )
	delete resizecache_;

    resizecache_ = 0;
    ownsresizecache_ = false;
}


void VolumeRenderScalarField::AttribData::clearIndexCache()
{
    if ( ownsindexcache_ && indexcache_ )
	delete [] indexcache_;

    indexcache_ = 0;
    indexcachestep_ = 0;
    ownsindexcache_ = false;
}


#define mCheckAttribStore( attr ) \
    if ( attr<0 || attr>3 ) \
	return; \
    while ( !attribs_.validIdx(attr) ) \
	attribs_ += new AttribData(); \


VolumeRenderScalarField::VolumeRenderScalarField()
    : material_( 0 )
    , channels2rgba_( 0 )
    , isrgba_( false )
    , useshading_( true )
    , isrighthandsystem_( DataObject::isRightHandSystem() )
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


void VolumeRenderScalarField::setRightHandSystem( bool yn )
{ isrighthandsystem_ = yn; }


bool VolumeRenderScalarField::isRightHandSystem() const
{ return isrighthandsystem_; }


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
    const osg::Vec4 rgbabordercolor( 0.0, 0.0, 0.0, 1.0 );

    if ( raytt_ )
    {
	if ( isrgba_ )
	{
	    raytt_->setFragShaderType( osgGeo::RayTracedTechnique::RGBA );
	    raytt_->setBorderColor( rgbabordercolor );
	}
	else
	{
	    raytt_->setFragShaderType( osgGeo::RayTracedTechnique::ColTab );
	    raytt_->setColTabValueChannel( 0 );
	    raytt_->setColTabUndefChannel( 3 );
	    raytt_->setColTabUndefValue( 1.0 );
	    raytt_->invertColTabUndefChannel( true );
	    raytt_->setBorderColor( osg::Vec4d(1.0,1.0,1.0,0.0) );
	}
    }
    else
    {
	mDynamicCastGet( osgGeo::FixedFunctionTechnique*, fft,
			 osgvoltile_->getVolumeTechnique() );
	if ( fft && isrgba_ )
	    fft->setBorderColor( rgbabordercolor );
    }
}


void VolumeRenderScalarField::updateVolumeSlicing()
{
    mDynamicCastGet( osgGeo::FixedFunctionTechnique*, fft,
		     osgvoltile_->getVolumeTechnique() );
    if ( !fft )
	return;

    const TrcKeyZSampling matkzs = getMultiAttribTrcKeyZSampling();

    int maxlen = 1;
    for ( int idx=0; idx<attribs_.size(); idx++ )
    {
	maxlen = mMAX( matkzs.nrLines(),
		       mMAX( matkzs.nrTrcs(),
			     mMAX( matkzs.nrZ(), maxlen ) ) );
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
    // for API preservation only

    TrcKeyZSampling dummy( true );
    const int nrlinesdiff = sc->info().getSize(0) - dummy.nrLines();
    dummy.hsamp_.stop_.lineNr() += nrlinesdiff * dummy.hsamp_.step_.lineNr();
    const int nrtrcsdiff = sc->info().getSize(1) - dummy.nrTrcs();
    dummy.hsamp_.stop_.trcNr() += nrtrcsdiff * dummy.hsamp_.step_.trcNr();
    const int nrzdiff = sc->info().getSize(2) - dummy.nrZ();
    dummy.zsamp_.stop += nrzdiff * dummy.zsamp_.step;

    setScalarField( attr, sc, mine, dummy, tr );
}


void VolumeRenderScalarField::setScalarField( int attr,
				const Array3D<float>* sc, bool mine,
				const TrcKeyZSampling& tkzs, TaskRunner* tr )
{
    mCheckAttribStore( attr );

    attribs_[attr]->clearDataCache();
    attribs_[attr]->clearResizeCache();

    if ( !sc )
	return;

    if ( sc->info().getSize(0)!=tkzs.nrLines() ||
	 sc->info().getSize(1)!=tkzs.nrTrcs() ||
	 sc->info().getSize(2)!=tkzs.nrZ() )
    {
	pErrMsg( "Unexpected volume data sampling mismatch" );
	return;
    }

    const TrcKeyZSampling oldmatkzs = getMultiAttribTrcKeyZSampling();

    attribs_[attr]->ownsdatacache_ = mine;
    attribs_[attr]->datacache_ = sc->getStorage();
    attribs_[attr]->datatkzs_ = tkzs;

    if ( !attribs_[attr]->datacache_ || !attribs_[attr]->datacache_->arr() )
    {
	MultiArrayValueSeries<float,float>* myvalser =
		    new MultiArrayValueSeries<float,float>( tkzs.totalNr() );

	if ( !myvalser || !myvalser->isOK() )
	    delete myvalser;
	else
	{
	    sc->getAll( *myvalser );

	    attribs_[attr]->datacache_ = myvalser;
	    attribs_[attr]->ownsdatacache_ = true;
	}
    }

    if ( oldmatkzs != getMultiAttribTrcKeyZSampling() )
    {
	for ( int attridx=0; attridx<attribs_.size(); attridx++ )
	{
	    // Reset datatkzs_ now. In case sc==0, the old datatkzs_ is kept
	    // until a multi-attrib resizecache_ update is needed anyway.
	    if ( !attribs_[attr]->datacache_ )
		attribs_[attr]->datatkzs_.setEmpty();
	}
    }

    if ( oldmatkzs == getMultiAttribTrcKeyZSampling() )
	updateResizeCache( attr, tr );
    else
    {
	for ( int attridx=0; attridx<attribs_.size(); attridx++ )
	{
	    attribs_[attridx]->clearIndexCache();
	    updateResizeCache( attridx, tr );
	}

	updateVolumeSlicing();
    }
}

class VolumeDataResizer : public ParallelTask
{
public:
VolumeDataResizer(const ValueSeries<float>& in,const TrcKeyZSampling& intkzs,
		  ValueSeries<float>& out,const TrcKeyZSampling& outtkzs)
    : in_( in )
    , intkzs_( intkzs )
    , out_( out )
    , outtkzs_( outtkzs )
{
    out_.setAll( mUdf(float) );
    worktkzs_ = outtkzs;
    worktkzs_.limitTo( intkzs, true );
    totalnr_ = worktkzs_.hsamp_.totalNr();
}

od_int64 nrIterations() const override		{ return totalnr_; }

bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    // TODO: sysMemCopy if possible & trilinear interpolation if necessary

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const BinID bid = worktkzs_.hsamp_.atIndex( idx );
	const int outinlidx = outtkzs_.hsamp_.lineIdx( bid.inl() );
	const int outcrlidx = outtkzs_.hsamp_.trcIdx( bid.crl() );
	const int outoffset =
		    (outcrlidx + outinlidx*outtkzs_.nrTrcs()) * outtkzs_.nrZ();

	const int nearestinl = bid.lineNr() + intkzs_.hsamp_.step_.lineNr()/2;
	const int ininlidx = intkzs_.hsamp_.lineIdx( nearestinl );
	const int nearestcrl = bid.trcNr() + intkzs_.hsamp_.step_.trcNr()/2;
	const int incrlidx = intkzs_.trcIdx( nearestcrl );
	const int inoffset =
		    (incrlidx + ininlidx*intkzs_.nrTrcs()) * intkzs_.nrZ();

	for ( int idz=0; idz<worktkzs_.nrZ(); idz++ )
	{
	    const float zval = worktkzs_.zsamp_.atIndex( idz );
	    const int inzidx = intkzs_.zsamp_.nearestIndex( zval );
	    const int outzidx = outtkzs_.zsamp_.nearestIndex( zval );

	    const float datavalue = in_.value( inoffset + inzidx );
	    out_.setValue( outoffset + outzidx, datavalue );
	}
    }
    return true;
}

protected:
    const ValueSeries<float>&	in_;
    const TrcKeyZSampling&	intkzs_;
    ValueSeries<float>&		out_;
    const TrcKeyZSampling&	outtkzs_;
    TrcKeyZSampling		worktkzs_;
    od_int64			totalnr_;
};


void VolumeRenderScalarField::updateResizeCache( int attr, TaskRunner* tr )
{
    mCheckAttribStore( attr );

    attribs_[attr]->clearResizeCache();

    if ( !attribs_[attr]->datacache_ )
	return;

    const TrcKeyZSampling matkzs = getMultiAttribTrcKeyZSampling();
    if ( attribs_[attr]->datatkzs_ == matkzs )
	attribs_[attr]->resizecache_ = attribs_[attr]->datacache_;
    else
    {
	MultiArrayValueSeries<float,float>* myvalser =
		    new MultiArrayValueSeries<float,float>( matkzs.totalNr() );
	VolumeDataResizer resizer( *attribs_[attr]->datacache_,
				   attribs_[attr]->datatkzs_,
				   *myvalser, matkzs );
	resizer.execute();
	attribs_[attr]->resizecache_ = myvalser;
	attribs_[attr]->ownsresizecache_ = true;
    }


    //TODO: if 8-bit data & some flags, use data itself
    if ( attribs_[attr]->mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	clipData( attr, tr );

    makeIndices( attr, false, tr );
}


TrcKeyZSampling VolumeRenderScalarField::getMultiAttribTrcKeyZSampling() const
{
    TrcKeyZSampling res( false );

    for ( int attr=0; attr<attribs_.size(); attr++ )
    {
	const TrcKeyZSampling& tkzs = attribs_[attr]->datatkzs_;
	if ( !tkzs.isDefined() || tkzs.isEmpty() )
	    continue;

	if ( res.isEmpty() )
	    res = tkzs;
	else
	    res.include( tkzs );
    }

    return res;
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

    if ( !attribs_[attr]->resizecache_ )
	return;

    attribs_[attr]->mapper_.setData( *attribs_[attr]->resizecache_, tr );
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
	const OD::Color col = sequence->color( idxfrac );
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

    if ( !attribs_[attr]->resizecache_ )
	return;

    const TrcKeyZSampling matkzs = getMultiAttribTrcKeyZSampling();
    const od_int64 totalsz = matkzs.totalNr();

    const bool resize = matkzs.nrZ()!=osgvoldata_->s() ||
			matkzs.nrTrcs()!=osgvoldata_->t() ||
			matkzs.nrLines()!=osgvoldata_->r();

    const bool renewimage = resize ||
			    attribs_[attr]->indexcache_!=osgvoldata_->data();

    if ( !useshading_ || isrgba_ )
    {
	if ( resize )
	{
	    osgvoldata_->allocateImage( matkzs.nrZ(),
					matkzs.nrTrcs(),
					matkzs.nrLines(),
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
		    totalsz, mNrColors-1, *attribs_[attr]->resizecache_,
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
	    const OD::Color col = Conv::to<::OD::Color>(
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

	mDynamicCastGet( osgGeo::FixedFunctionTechnique*, fft,
			 osgvoltile_->getVolumeTechnique() );
	if ( fft )
	    fft->setBorderColor( osgtransfunc_->getColor(1.0) );
    }
    else if ( renewimage && !isrgba_ )
    {
	osgvoldata_->setImage( matkzs.nrZ(), matkzs.nrTrcs(),
			       matkzs.nrLines(), GL_LUMINANCE_ALPHA,
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
	const od_int64 totalsz = getMultiAttribTrcKeyZSampling().totalNr();
	const int deststep = yn ? 4 : 1;
	unsigned char* destptr = yn ? osgvoldata_->data()+attr
				    : new unsigned char[totalsz];

	copyDataStepwise( totalsz, attribs_[attr]->indexcache_,
			  attribs_[attr]->indexcachestep_, destptr, deststep );
	if ( yn )
	    attribs_[attr]->clearIndexCache();
	else
	    setDefaultRGBAValue( attr );

	attribs_[attr]->indexcache_ = destptr;
	attribs_[attr]->indexcachestep_ = deststep;
	attribs_[attr]->ownsindexcache_ = !yn;

	osgvoldata_->dirty();
    }

    requestSingleRedraw();
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
    requestSingleRedraw();

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

    const TrcKeyZSampling matkzs = getMultiAttribTrcKeyZSampling();

    vh.width = hton_uint32( matkzs.nrZ() );
    vh.height = hton_uint32( matkzs.nrTrcs() );
    vh.images = hton_uint32( matkzs.nrLines() );
    vh.bits_per_voxel = hton_uint32(8);

    if ( strm.addBin(vh).isBad() )
	return writeerr;

    const unsigned char* indexcacheptr = attribs_[attr]->indexcache_;
    const int indexcachestep = attribs_[attr]->indexcachestep_;

    if ( indexcachestep!=1 )
    {
	for ( int count=matkzs.totalNr(); count>0; count-- )
	{
	    if ( !strm.addBin(indexcacheptr,1) )
		return writeerr;

	    indexcacheptr += indexcachestep;
	}
    }
    else if ( !strm.addBin(indexcacheptr,matkzs.totalNr()) )
	return writeerr;

    return 0;
}


} // namespace visBase
