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
#include "iopar.h"
#include "vismaterial.h"
#include "valseries.h"
#include "viscolortab.h"
#include "visdataman.h"
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


VolumeRenderScalarField::VolumeRenderScalarField()
//    : dummytexture_( 255 )
    : indexcache_( 0 )
    , ownsindexcache_( true )
    , datacache_( 0 )
    , ownsdatacache_( true )
    , sz0_( 1 )
    , sz1_( 1 )
    , sz2_( 1 )
//    , blendcolor_( Color::White() )
    , material_( 0 )
    , useshading_( true )
    , sequence_( ColTab::Sequence( ColTab::defSeqName() ) )
    , osgvoltile_( new osgVolume::VolumeTile() )
    , osgvolume_( new osgVolume::Volume() )
    , osgvolroot_( new osg::Switch() )
    , osgimagelayer_( new osgVolume::ImageLayer() )
    , osgvoldata_( new osg::Image() )
    , osgtransfunc_( new osg::TransferFunction1D() )
{
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
    osgVolume::TransparencyProperty* transparency =
			new osgVolume::TransparencyProperty( 1.0 );
//    osgVolume::IsoSurfaceProperty* isosurface =
//			new osgVolume::IsoSurfaceProperty( 0.99 );

    osgtransfunc_->allocate( mNrColors );
    osg::ref_ptr<osgVolume::TransferFunctionProperty> transferfunction =
		    new osgVolume::TransferFunctionProperty( osgtransfunc_ );

    osgVolume::CompositeProperty* compprop = new osgVolume::CompositeProperty;
    compprop->addProperty( alpha );
    compprop->addProperty( sampledensity );
    compprop->addProperty( movingsampledensity );
    compprop->addProperty( transparency );

    compprop->addProperty( transferfunction );
//    compprop->addProperty( isosurface );
//    compprop->addProperty( new osgVolume::LightingProperty );
    osgimagelayer_->addProperty( compprop );

    enableTextureInterpolation( true );
}


VolumeRenderScalarField::~VolumeRenderScalarField()
{
    if ( ownsindexcache_ ) delete [] indexcache_;
    if ( ownsdatacache_ ) delete datacache_;

    osgvolroot_->unref();
    osgvoltile_->unref();
    osgvolume_->unref();
    osgimagelayer_->unref();
    osgvoldata_->unref();
    osgtransfunc_->unref();

    setMaterial( 0 );
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


/* Modified code from OpenSceneGraph/examples/osgvolume/osgvolume.cpp:
   Flipped signs of y and z dimensions. */

static osg::Matrix getLocatorMatrix( const Coord3& trans, const Coord3& rotvec,
				     double rotangle, const Coord3& scale ) 
{
    Coord3 fabsscale( fabs(scale.x), -fabs(scale.y), -fabs(scale.z) );

    osg::Matrix mat = osg::Matrix::scale( Conv::to<osg::Vec3d>(fabsscale) );
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

    mat.preMult( osg::Matrix::scale( scale.x<0.0 ? -1.0 : 1.0,
				     scale.y>0.0 ? -1.0 : 1.0,
				     scale.z>0.0 ? -1.0 : 1.0 ) );

    mat.preMult( osg::Matrix::translate( scale.x<0.0 ? -1.0 : 0.0,
					 scale.y>0.0 ? -1.0 : 0.0,
					 scale.z>0.0 ? -1.0 : 0.0) );

    osgimagelayer_->setLocator( new osgVolume::Locator( mat) );
}


void VolumeRenderScalarField::setROIVolumeTransform( const Coord3& trans,
				    const Coord3& rotvec, double rotangle,
				    const Coord3& scale ) 
{
    osg::Matrix mat = getLocatorMatrix( trans, rotvec, rotangle, scale );
    osgvoltile_->setLocator( new osgVolume::Locator(mat) );
    osgvoltile_->setDirty( true );
}


void VolumeRenderScalarField::useShading( bool yn )
{
    if ( useshading_==yn && osgvoltile_->getVolumeTechnique() )
	return;

    if ( yn )
    {
	osgGeo::RayTracedTechnique* rtt = new osgGeo::RayTracedTechnique;
	rtt->setCustomShader( osg::Shader::FRAGMENT,
			      rtt->volumeTfFragDepthCode() );
	osgvoltile_->setVolumeTechnique( rtt );
    }
    else
	osgvoltile_->setVolumeTechnique( new osgGeo::FixedFunctionTechnique );

    useshading_ = yn;
    updateVolumeSlicing();

//    Does this have an equivalent in OSG?
//    if ( !useshading_ )
//	SetEnvVar( "CVR_DISABLE_PALETTED_FRAGPROG", "1" );
}


void VolumeRenderScalarField::updateVolumeSlicing()
{
    mDynamicCastGet( osgGeo::FixedFunctionTechnique*, fft,
		     osgvoltile_->getVolumeTechnique() );
    if ( fft )
	fft->setNumSlices( 8*mMAX(sz0_,mMAX(sz1_,sz2_)) );     // Empirical
}


bool VolumeRenderScalarField::turnOn( bool yn )
{
    const bool wason = isOn();
    osgvolroot_->setValue( 0, yn );
    return wason;
/*
    if ( !voldata_ ) return false;
    const bool wason = isOn();
     if ( !yn )
	 voldata_->setVolumeData( SbVec3s(1,1,1),
			    &dummytexture_, SoVolumeData::UNSIGNED_BYTE );
     else if ( indexcache_ )
	 voldata_->setVolumeData( SbVec3s(sz2_,sz1_,sz0_),
				 indexcache_, SoVolumeData::UNSIGNED_BYTE );
    return wason;
*/
}


bool VolumeRenderScalarField::isOn() const
{
    return osgvolroot_->getValue(0);
/*
    if ( !voldata_ )
	return false;

    SbVec3s size;
    void* ptr;
    SoVolumeData::DataType dt;
    return voldata_->getVolumeData(size,ptr,dt) && ptr==indexcache_;
*/
}


void VolumeRenderScalarField::setScalarField( const Array3D<float>* sc,
					      bool mine, TaskRunner* tr )
{
    if ( !sc )
    {
	turnOn( false );
	if ( ownsdatacache_ ) delete datacache_;
	datacache_ = 0;
	return;
    }

    const bool isresize = sc->info().getSize(0)!=sz0_ ||
			  sc->info().getSize(1)!=sz1_ ||
			  sc->info().getSize(2)!=sz2_;

    const od_int64 totalsz = sc->info().getTotalSz();

    bool doset = false;
    if ( isresize )
    {
	sz0_ = sc->info().getSize( 0 );
	sz1_ = sc->info().getSize( 1 );
	sz2_ = sc->info().getSize( 2 );
	doset = true;

	if ( ownsindexcache_ ) delete [] indexcache_;
	indexcache_ = 0;

	updateVolumeSlicing();
    }

    if ( ownsdatacache_ ) delete datacache_;

    ownsdatacache_ = mine;
    datacache_ = sc->getStorage();
    if ( !datacache_ || !datacache_->arr() )
    {
	MultiArrayValueSeries<float,float>* myvalser =
	    new MultiArrayValueSeries<float,float>( totalsz );
	if ( !myvalser || !myvalser->isOK() )
	    delete myvalser;
	else
	{
	    sc->getAll( *myvalser );

	    datacache_ = myvalser;
	    ownsdatacache_ = true;
	}
    }

    //TODO: if 8-bit data & some flags, use data itself
    if ( mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	//mapper_.setData( datacache_, totalsz, tr );
	clipData( tr );

    makeIndices( doset, tr );
}


void VolumeRenderScalarField::setColTabSequence( const ColTab::Sequence& s,
						 TaskRunner* tr )
{
    sequence_ = s;
    makeColorTables();
}


const ColTab::Mapper& VolumeRenderScalarField::getColTabMapper()
{ return mapper_; }


const ColTab::Sequence& VolumeRenderScalarField::getColTabSequence()
{ return sequence_; }


void VolumeRenderScalarField::setColTabMapperSetup( const ColTab::MapperSetup&
	ms, TaskRunner* tr )
{
    if ( mapper_.setup_ == ms )
	return;

//    const bool autoscalechange = mapper_.setup_.type_ != ms.type_;

    mapper_.setup_ = ms;

    /*if ( autoscalechange )
	mapper_.setup_.triggerAutoscaleChange();
    else
	mapper_.setup_.triggerRangeChange();*/

    if ( mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	clipData( tr );

    makeIndices( false, tr );
}

/*
void VolumeRenderScalarField::setBlendColor( const Color& col )
{
    blendcolor_ = col;
    makeColorTables();
}


const Color& VolumeRenderScalarField::getBlendColor() const
{ return blendcolor_; }
*/

const TypeSet<float>& VolumeRenderScalarField::getHistogram() const
{ return histogram_; }


/*
SoNode* VolumeRenderScalarField::gtInvntrNode()
{
    if ( !voldata_ )
    {
	voldata_ = new SoVolumeData;
	root_->addChild( voldata_ );

	setVolumeSize( Interval<float>(-0.5,0.5), Interval<float>(-0.5,0.5),
		       Interval<float>(-0.5,0.5) );
	voldata_->setVolumeData( SbVec3s(1,1,1),
			    &dummytexture_, SoVolumeData::UNSIGNED_BYTE );
	if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	    voldata_->usePalettedTexture = FALSE;

	transferfunc_ = new SoTransferFunction;
	makeColorTables();

	root_->addChild( transferfunc_ );
    }

    return root_;
}
*/

void VolumeRenderScalarField::clipData( TaskRunner* tr )
{
    if ( !datacache_ )
	return;

    const od_int64 totalsz = sz0_*sz1_*sz2_;
    mapper_.setData( datacache_, totalsz, tr );
    mapper_.setup_.triggerRangeChange();
}


void VolumeRenderScalarField::makeColorTables()
{
    osg::TransferFunction1D::ColorMap colmap;
    for ( int idx=0; idx<mNrColors-1; idx++ )
    {
	const Color col = sequence_.color( float(idx)/(mNrColors-2) );
	colmap[ float(idx)/(mNrColors-1) ] = Conv::to<osg::Vec4>( col );
    }

    colmap[ 1.0 ] = Conv::to<osg::Vec4>( sequence_.undefColor() );
    osgtransfunc_->assign( colmap );

    if ( !useshading_ )
	makeIndices( false, 0 );

/*
    if ( !transferfunc_ )
	return;

    const float redfactor = (float) blendcolor_.r()/(255*255);
    const float greenfactor = (float) blendcolor_.g()/(255*255);
    const float bluefactor = (float) blendcolor_.b()/(255*255);
    const float opacityfactor = (float) (255-blendcolor_.t())/(255*255);

    const bool didnotify = transferfunc_->colorMap.enableNotify( false );
    int cti = 0;

    // Fill up positions 0 to 253 with the color of defined values and
    // positions 254 (and 255) with undef color (this is a workaround for a
    // bug in SIMVoleon - it does not take in color at position mNrColors-1
    // (that is, at 255)).

    for ( int idx=0; idx<mNrColors-2; idx++ )
    {
	const float relval = ((float) idx)/(mNrColors-2);
	const ::Color col = sequence_.color( relval );
	transferfunc_->colorMap.set1Value( cti++, col.r()*redfactor );
	transferfunc_->colorMap.set1Value( cti++, col.g()*greenfactor );
	transferfunc_->colorMap.set1Value( cti++, col.b()*bluefactor );
	transferfunc_->colorMap.set1Value( cti++, 1.0f-col.t()*opacityfactor );
    }

    const ::Color col = sequence_.undefColor();

    for ( char count=0; count<2; count++ )
    {
        transferfunc_->colorMap.set1Value( cti++, col.r()*redfactor );
	transferfunc_->colorMap.set1Value( cti++, col.g()*greenfactor );
	transferfunc_->colorMap.set1Value( cti++, col.b()*bluefactor );
	transferfunc_->colorMap.set1Value( cti++, 1.0f-col.t()*opacityfactor );
    }

    transferfunc_->predefColorMap = SoTransferFunction::NONE;

    transferfunc_->colorMap.enableNotify(didnotify);
    transferfunc_->colorMap.touch();
    */
}


void VolumeRenderScalarField::makeIndices( bool doset, TaskRunner* tr )
{
    if ( !datacache_ )
	return;

    const od_int64 totalsz = sz0_*sz1_*sz2_;

    if ( !indexcache_ )
    {
	indexcache_ = new unsigned char[totalsz];
	ownsindexcache_ = true;
    }

    // Reverse the index order, because osgVolume turns out to perform well
    // for one sense of the coordinate system only, and OD uses the other. A
    // transform in visSurvey::VolumeDisplay does the geometrical mirroring.
        ColTab::MapperTask<unsigned char> indexer( mapper_, totalsz,
         mNrColors-1, *datacache_, indexcache_+totalsz-1, -1 );
//    ColTab::MapperTask<unsigned char> indexer( mapper_, totalsz,
//				    mNrColors-1, *datacache_, indexcache_ );

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
	histogram_.setSize( mNrColors-1, 0 );
	for ( int idx=mNrColors-2; idx>=0; idx-- )
	    histogram_[idx] = (float) histogram[idx]/max;
    }

    if ( !useshading_ )
    {
	if ( sz2_!=osgvoldata_->s() || sz1_!=osgvoldata_->t() ||
	     sz0_!=osgvoldata_->r() )
	{
	    osgvoldata_->allocateImage( sz2_, sz1_, sz0_, GL_RGBA,
					GL_UNSIGNED_BYTE );
	}

	unsigned char coltab[1024];
	for ( int idx=0; idx<=255; idx++ )
	{
	    const Color col =
		Conv::to<Color>( osgtransfunc_->getColor(float(idx)/255) );
	    coltab[4*idx+0] = col.r();
	    coltab[4*idx+1] = col.g();
	    coltab[4*idx+2] = col.b();
	    coltab[4*idx+3] = 255-col.t();
	}

	unsigned char* ptr = osgvoldata_->data();
	const int nrvoxels = sz2_ * sz1_ * sz0_;

	for ( int idx=0; idx<nrvoxels; idx++ )
	{
	    OD::memCopy( ptr , coltab+4*indexcache_[idx], 4 );
	    ptr += 4;
	}
    }
    else if ( doset )
    {
	osgvoldata_->setImage( sz2_, sz1_, sz0_, GL_LUMINANCE, GL_LUMINANCE,
		GL_UNSIGNED_BYTE, indexcache_, osg::Image::NO_DELETE, 1 );
    }
    else
	osgvoldata_->dirty();

    osgvoltile_->setDirty( true );
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



const char* VolumeRenderScalarField::writeVolumeFile( od_ostream& strm ) const
{
    if ( !indexcache_ )
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

    vh.width = hton_uint32(sz2_);
    vh.height = hton_uint32(sz1_);
    vh.images = hton_uint32(sz0_);
    vh.bits_per_voxel = hton_uint32(8);

    if ( !strm.addBin( &vh, sizeof(struct VolFileHeader) ) )
	return writeerr;

    const od_int64 totalsz = sz0_*sz1_*sz2_;
    if ( !strm.addBin( indexcache_, totalsz ) )
	return writeerr;

    return 0;
}


} // namespace visBase
