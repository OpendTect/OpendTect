/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "visvolumedisplay.h"

#include "visboxdragger.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarchingcubessurface.h"
#include "vismaterial.h"
#include "visselman.h"
#include "vistransform.h"
#include "visvolorthoslice.h"
#include "visvolrenscalarfield.h"
#include "visvolren.h"

#include "array3dfloodfill.h"
#include "arrayndimpl.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "cubesampling.h"
#include "ioman.h"
#include "iopar.h"
#include "marchingcubes.h"
#include "picksettr.h"
#include "pickset.h"
#include "sorting.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "zaxistransformer.h"

#include <fstream>

#define mVisMCSurf visBase::MarchingCubesSurface
mCreateFactoryEntry( visSurvey::VolumeDisplay );


namespace visSurvey {

const char* VolumeDisplay::sKeyVolumeID()	{ return "Cube ID"; }
const char* VolumeDisplay::sKeyVolRen()		{ return "Volren"; }
const char* VolumeDisplay::sKeyInline()		{ return "Inline"; } 
const char* VolumeDisplay::sKeyCrossLine()	{ return "Crossline"; }
const char* VolumeDisplay::sKeyTime()		{ return "Z-slice"; }

const char* VolumeDisplay::sKeyNrSlices()	{ return "Nr of slices"; }
const char* VolumeDisplay::sKeySlice()		{ return "SliceID "; }
const char* VolumeDisplay::sKeyTexture()	{ return "TextureID"; }

const char* VolumeDisplay::sKeyNrIsoSurfaces()	{ return "Nr Isosurfaces"; }
const char* VolumeDisplay::sKeyIsoValueStart()	{ return "Iso Value"; }
const char* VolumeDisplay::sKeyIsoOnStart()	{ return "Iso Surf On "; }
const char* VolumeDisplay::sKeySurfMode()	{ return "Surf Mode"; }
const char* VolumeDisplay::sKeySeedsMid()	{ return "Surf Seeds Mid"; }
const char* VolumeDisplay::sKeySeedsAboveIsov()	{ return "Above IsoVal"; }


static CubeSampling getInitCubeSampling( const CubeSampling& csin )
{
    CubeSampling cs(false);
    cs.hrg.start.inl = (5*csin.hrg.start.inl+3*csin.hrg.stop.inl)/8;
    cs.hrg.start.crl = (5*csin.hrg.start.crl+3*csin.hrg.stop.crl)/8;
    cs.hrg.stop.inl = (3*csin.hrg.start.inl+5*csin.hrg.stop.inl)/8;
    cs.hrg.stop.crl = (3*csin.hrg.start.crl+5*csin.hrg.stop.crl)/8;
    cs.zrg.start = ( 5*csin.zrg.start + 3*csin.zrg.stop ) / 8;
    cs.zrg.stop = ( 3*csin.zrg.start + 5*csin.zrg.stop ) / 8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = csin.zrg.snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = csin.zrg.snap( cs.zrg.stop ); cs.zrg.stop = z1;
    return cs;
}


VolumeDisplay::VolumeDisplay()
    : VisualObjectImpl(true)
    , boxdragger_(visBase::BoxDragger::create())
    , isinited_(0)
    , scalarfield_(0)
    , volren_(0)
    , as_(*new Attrib::SelSpec)
    , cache_(0)
    , cacheid_(DataPack::cNoID())
    , slicemoving(this)
    , voltrans_(visBase::Transformation::create())
    , allowshading_(false)
    , datatransform_(0)
    , datatransformer_(0)
    , csfromsession_(true)
    , eventcatcher_( 0 )
    , onoffstatus_( true )
{
    boxdragger_->ref();
    boxdragger_->setBoxTransparency( 0.7 );
    addChild( boxdragger_->getInventorNode() );
    boxdragger_->finished.notify( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    getMaterial()->setColor( Color::White() );
    getMaterial()->setAmbience( 0.3 );
    getMaterial()->setDiffIntensity( 0.8 );
    getMaterial()->change.notify(mCB(this,VolumeDisplay,materialChange) );
    voltrans_->ref();
    addChild( voltrans_->getInventorNode() );
    voltrans_->setRotation( Coord3(0,1,0), M_PI_2 );

    scalarfield_ = visBase::VolumeRenderScalarField::create();
    scalarfield_->ref(); //Don't add it here, do that in getInventorNode

    CubeSampling sics = SI().sampling( true );
    CubeSampling cs = getInitCubeSampling( sics );
    setCubeSampling( cs );
}


VolumeDisplay::~VolumeDisplay()
{
    setSceneEventCatcher( 0 );

    if ( getMaterial() )
	getMaterial()->change.remove( mCB(this,VolumeDisplay,materialChange) );

    delete &as_;
    DPM( DataPackMgr::CubeID() ).release( cacheid_ );
    if ( cache_ ) cache_->unRef();

    TypeSet<int> children;
    getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	removeChild( children[idx] );

    boxdragger_->finished.remove( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    boxdragger_->unRef();
    voltrans_->unRef();
    scalarfield_->unRef();

    setZAxisTransform( 0,0 );
}


void VolumeDisplay::setMaterial( visBase::Material* nm )
{
    getMaterial()->change.remove(mCB(this,VolumeDisplay,materialChange) );
    visBase::VisualObjectImpl::setMaterial( nm );
    if ( nm )
	getMaterial()->change.notify( mCB(this,VolumeDisplay,materialChange) );
    materialChange( 0 );
}

#define mSetMaterialProp( prop ) \
    isosurfaces_[idx]->getMaterial()->set##prop( \
	    getMaterial()->get##prop() )
void VolumeDisplay::materialChange( CallBacker* )
{
    if ( !getMaterial() )
	return;

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	mSetMaterialProp( Ambience );
	mSetMaterialProp( DiffIntensity );
	mSetMaterialProp( SpecIntensity );
	mSetMaterialProp( EmmIntensity );
	mSetMaterialProp( Shininess );
	mSetMaterialProp( Transparency );
    }
}


void VolumeDisplay::updateIsoSurfColor()
{
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( mIsUdf( isosurfsettings_[idx].isovalue_) )
	    continue;

	const float val = isosurfsettings_[idx].isovalue_;
	Color col;
	if ( mIsUdf(val) )
	    col = getColTabSequence( 0 )->undefColor();
	else
	{
	    const float mappedval =
		scalarfield_->getColTabMapper().position( val );
	    col = getColTabSequence( 0 )->color( mappedval );
	}

	isosurfaces_[idx]->getMaterial()->setColor( col );
    }
}


bool VolumeDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( zat == datatransform_ )
	return true;

    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,VolumeDisplay,dataTransformCB) );
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    delete datatransformer_;
    datatransformer_ = 0;

    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,VolumeDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* VolumeDisplay::getZAxisTransform() const
{ return datatransform_; }


void VolumeDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	isosurfaces_[idx]->setRightHandSystem( yn );
}


void VolumeDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    if ( cache_ ) setDataVolume( 0, cache_, 0 );
}


void VolumeDisplay::updateRanges( bool updateic, bool updatez )
{
    if ( !datatransform_ ) return;

    const CubeSampling defcs( true );
    if ( csfromsession_ != defcs )
	setCubeSampling( csfromsession_ );
    else
    {
	const CubeSampling csin = scene_ ? scene_->getCubeSampling()
					 : getCubeSampling( 0 );
	CubeSampling cs = getInitCubeSampling( csin );
	setCubeSampling( cs );
    }
}


void VolumeDisplay::getChildren( TypeSet<int>&res ) const
{
    res.erase();
    for ( int idx=0; idx<slices_.size(); idx++ )
	res += slices_[idx]->id();
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	res += isosurfaces_[idx]->id();
    if ( volren_ ) res += volren_->id();
}


void VolumeDisplay::showManipulator( bool yn )
{ boxdragger_->turnOn( yn ); }


bool VolumeDisplay::isManipulatorShown() const
{ return boxdragger_->isOn(); }


bool VolumeDisplay::isManipulated() const
{
    return getCubeSampling(true,true,0) != getCubeSampling(false,true,0);
}


bool VolumeDisplay::canResetManipulation() const
{ return true; }


void VolumeDisplay::resetManipulation()
{
    const Coord3 center = voltrans_->getTranslation();
    const Coord3 width = voltrans_->getScale();
    boxdragger_->setCenter( center );
    boxdragger_->setWidth( Coord3(width.z, width.y, -width.x) );
}


void VolumeDisplay::acceptManipulation()
{
    setCubeSampling( getCubeSampling(true,true,0) );
}


int VolumeDisplay::addSlice( int dim )
{
    visBase::OrthogonalSlice* slice = visBase::OrthogonalSlice::create();
    slice->ref();
    slice->setMaterial(0);
    slice->setDim(dim);
    slice->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
    slices_ += slice;

    slice->setName( dim==cTimeSlice() ? sKeyTime() : 
	    	   (dim==cCrossLine() ? sKeyCrossLine() : sKeyInline()) );

    addChild( slice->getInventorNode() );
    const CubeSampling cs = getCubeSampling( 0 );
    const Interval<float> defintv(-0.5,0.5);
    slice->setSpaceLimits( defintv, defintv, defintv );
    if ( cache_ )
    {
	const Array3D<float>& arr = cache_->getCube(0);
	slice->setVolumeDataSize( arr.info().getSize(2),
				    arr.info().getSize(1),
				    arr.info().getSize(0) );
    }

    return slice->id();
}


void VolumeDisplay::removeChild( int displayid )
{
    if ( volren_ && displayid==volren_->id() )
    {
	VisualObjectImpl::removeChild( volren_->getInventorNode() );
	volren_->unRef();
	volren_ = 0;
	return;
    }

    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild( slices_[idx]->getInventorNode() );
	    slices_[idx]->motion.remove( mCB(this,VolumeDisplay,sliceMoving) );
	    slices_[idx]->unRef();
	    slices_.remove(idx,false);
	    return;
	}
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( isosurfaces_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild(
		    isosurfaces_[idx]->getInventorNode() );
	    isosurfaces_[idx]->unRef();
	    isosurfaces_.remove(idx,false);
	    isosurfsettings_.remove(idx,false);
	    return;
	}
    }
}


void VolumeDisplay::showVolRen( bool yn )
{
    if ( yn && !volren_ )
    {
	volren_ = visBase::VolrenDisplay::create();
	volren_->ref();
	volren_->setMaterial(0);
	addChild( volren_->getInventorNode() );
	volren_->setName( sKeyVolRen() );
    }

    if ( volren_ ) volren_->turnOn( yn );
}


bool VolumeDisplay::isVolRenShown() const
{ return volren_ && volren_->isOn(); }


float VolumeDisplay::defaultIsoValue() const
{
    return  cache_ ? getColTabMapperSetup(0)->range_.center() : mUdf(float);
}


int VolumeDisplay::addIsoSurface( TaskRunner* tr, bool updateisosurface )
{
    mVisMCSurf* isosurface = mVisMCSurf::create();
    isosurface->ref();
    isosurface->setRightHandSystem( righthandsystem_ );
    mDeclareAndTryAlloc( RefMan<MarchingCubesSurface>, surface,
	    		 MarchingCubesSurface() );
    isosurface->setSurface( *surface, tr );
    isosurface->setName( "Iso surface" );

    isosurfaces_ += isosurface;
    IsosurfaceSetting setting;
    setting.isovalue_ = defaultIsoValue();
    isosurfsettings_ += setting;

    if ( updateisosurface )   
       	updateIsoSurface( isosurfaces_.size()-1, tr );

    //Insert before the volume transform
    insertChild( childIndex(voltrans_->getInventorNode()),
	    		    isosurface->getInventorNode() );
    materialChange( 0 ); //updates new surface's material
    return isosurface->id();
}


int VolumeDisplay::volRenID() const
{ return volren_ ? volren_->id() : -1; }

    
void VolumeDisplay::setCubeSampling( const CubeSampling& cs )
{
    const Interval<float> xintv( cs.hrg.start.inl, cs.hrg.stop.inl );
    const Interval<float> yintv( cs.hrg.start.crl, cs.hrg.stop.crl );
    const Interval<float> zintv( cs.zrg.start, cs.zrg.stop );
    voltrans_->setTranslation( 
	    	Coord3(xintv.center(),yintv.center(),zintv.center()) );
    voltrans_->setRotation( Coord3( 0, 1, 0 ), M_PI_2 );
    voltrans_->setScale( Coord3(-zintv.width(),yintv.width(),xintv.width()) );
    scalarfield_->setVolumeSize( Interval<float>(-0.5,0.5),
	    		    Interval<float>(-0.5,0.5),
			    Interval<float>(-0.5,0.5) );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setSpaceLimits( Interval<float>(-0.5,0.5), 
				     Interval<float>(-0.5,0.5),
				     Interval<float>(-0.5,0.5) );

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	isosurfaces_[idx]->getSurface()->removeAll();
	isosurfaces_[idx]->touch( false );
    }

    if ( scalarfield_ ) scalarfield_->turnOn( false );

    resetManipulation();
}


float VolumeDisplay::getValue( const Coord3& pos_ ) const
{
    if ( !cache_ ) return mUdf(float);
    const BinIDValue bidv( SI().transform(pos_), pos_.z );
    float val;
    if ( !cache_->getValue(0,bidv,&val,false) )
	return mUdf(float);

    return val;
}


float VolumeDisplay::isoValue( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    return idx<0 ? mUdf(float) : isosurfsettings_[idx].isovalue_;
}


void VolumeDisplay::setIsoValue( const mVisMCSurf* mcd, float nv, 
				 TaskRunner* tr )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 )
	return;

    isosurfsettings_[idx].isovalue_ = nv;
    updateIsoSurface( idx, tr );
}


mVisMCSurf* VolumeDisplay::getIsoSurface( int idx ) 
{ return isosurfaces_.validIdx(idx) ? isosurfaces_[idx] : 0; }


const int VolumeDisplay::getNrIsoSurfaces()
{ return isosurfaces_.size(); }


char VolumeDisplay::isFullMode( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return -1;

    return isosurfsettings_[idx].mode_;
}


void VolumeDisplay::setFullMode( const mVisMCSurf* mcd, bool full )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 )
	return;
    
    isosurfsettings_[idx].mode_ = full;
}


char VolumeDisplay::seedAboveIsovalue( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
            return -1;

    return isosurfsettings_[idx].seedsaboveisoval_;
}


void VolumeDisplay::setSeedAboveIsovalue( const mVisMCSurf* mcd, bool above )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
            return;

    isosurfsettings_[idx].seedsaboveisoval_ = above;
}


MultiID  VolumeDisplay::getSeedsID( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return MultiID();
    
    return isosurfsettings_[idx].seedsid_;
}


void VolumeDisplay::setSeedsID( const mVisMCSurf* mcd, MultiID mid )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return;

    isosurfsettings_[idx].seedsid_ = mid;
}


bool VolumeDisplay::updateSeedBasedSurface( int idx, TaskRunner* tr )
{
    if ( idx<0 || idx>=isosurfaces_.size() || !cache_ ||
	 mIsUdf(isosurfsettings_[idx].isovalue_) ||
	 isosurfsettings_[idx].seedsid_.isEmpty() )
	return false;

    Pick::Set seeds;
    if ( Pick::Mgr().indexOf(isosurfsettings_[idx].seedsid_)!=-1 )
	seeds = Pick::Mgr().get( isosurfsettings_[idx].seedsid_ );
    else
    {
	BufferString ermsg;
	if ( !PickSetTranslator::retrieve( seeds,
		    IOM().get(isosurfsettings_[idx].seedsid_), true, ermsg ) )
	    return false;
    }

    const Array3D<float>& data = cache_->getCube(0);
    if ( !data.isOK() )
	return false;
    
    Array3DImpl<float> newarr( data.info() );
    Array3DFloodfill<float> ff( data, isosurfsettings_[idx].isovalue_,
	    isosurfsettings_[idx].seedsaboveisoval_, newarr );    
    ff.useInputValue( true );

    CubeSampling cs = getCubeSampling(true,true,0);
    cs.normalise();
    for ( int seedidx=0; seedidx<seeds.size(); seedidx++ )
    {
	const Coord3 pos =  seeds[seedidx].pos;
	const BinID bid = SI().transform( pos );
	const int i = cs.inlIdx( bid.inl );
	const int j = cs.crlIdx( bid.crl );
	const int k = cs.zIdx( pos.z );
	ff.addSeed( i, j, k );
    }

    if ( !ff.execute() )
	return false;

    isosurfaces_[idx]->getSurface()->setVolumeData( 0, 0, 0, newarr,
	    isosurfsettings_[idx].isovalue_, tr );
    return true;
}


int VolumeDisplay::getIsoSurfaceIdx( const mVisMCSurf* mcd ) const
{
    return isosurfaces_.indexOf( mcd );
}


void VolumeDisplay::updateIsoSurface( int idx, TaskRunner* tr )
{
    if ( !cache_ || !cache_->getCube(0).isOK() || 
	 mIsUdf(isosurfsettings_[idx].isovalue_) )
	isosurfaces_[idx]->getSurface()->removeAll();
    else
    {
	isosurfaces_[idx]->getSurface()->removeAll(); 
	isosurfaces_[idx]->setBoxBoundary(
		cache_->cubeSampling().hrg.inlRange().stop,
		cache_->cubeSampling().hrg.crlRange().stop,
		cache_->cubeSampling().zrg.stop );
	isosurfaces_[idx]->setScales(
		cache_->inlsampling_, cache_->crlsampling_,
		SamplingData<float>(cache_->z0_*cache_->zstep_,cache_->zstep_) );
	if ( isosurfsettings_[idx].mode_ )
    	    isosurfaces_[idx]->getSurface()->setVolumeData( 0, 0, 0,
		    cache_->getCube(0), isosurfsettings_[idx].isovalue_, tr );
	else
	{
	    if ( !updateSeedBasedSurface( idx, tr ) )
		return;
	}

    }

    updateIsoSurfColor();
    isosurfaces_[idx]->touch( false, tr );
}


void VolumeDisplay::manipMotionFinishCB( CallBacker* )
{
    if ( scene_ && scene_->getZAxisTransform() )
	return;

    CubeSampling cs = getCubeSampling( true, true, 0 );
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;

    Interval<int> inlrg( cs.hrg.start.inl, cs.hrg.stop.inl );
    Interval<int> crlrg( cs.hrg.start.crl, cs.hrg.stop.crl );
    Interval<float> zrg( cs.zrg.start, cs.zrg.stop );
    SI().checkInlRange( inlrg, true );
    SI().checkCrlRange( crlrg, true );
    SI().checkZRange( zrg, true );
    if ( inlrg.start == inlrg.stop ||
	 crlrg.start == crlrg.stop ||
	 mIsEqual(zrg.start,zrg.stop,1e-8) )
    {
	resetManipulation();
	return;
    }
    else
    {
	cs.hrg.start.inl = inlrg.start; cs.hrg.stop.inl = inlrg.stop;
	cs.hrg.start.crl = crlrg.start; cs.hrg.stop.crl = crlrg.stop;
	cs.zrg.start = zrg.start; cs.zrg.stop = zrg.stop;
    }

    const Coord3 newwidth( cs.hrg.stop.inl - cs.hrg.start.inl,
			   cs.hrg.stop.crl - cs.hrg.start.crl,
			   cs.zrg.stop - cs.zrg.start );
    boxdragger_->setWidth( newwidth );
    const Coord3 newcenter( 0.5*(cs.hrg.stop.inl + cs.hrg.start.inl),
			    0.5*(cs.hrg.stop.crl + cs.hrg.start.crl),
			    0.5*(cs.zrg.stop + cs.zrg.start) );
    boxdragger_->setCenter( newcenter );
}


BufferString VolumeDisplay::getManipulationString() const
{
    BufferString str = slicename_; str += ": "; str += sliceposition_;
    return str;
}


void VolumeDisplay::sliceMoving( CallBacker* cb )
{
    mDynamicCastGet( visBase::OrthogonalSlice*, slice, cb );
    if ( !slice ) return;

    slicename_ = slice->name();
    sliceposition_ = slicePosition( slice );
    slicemoving.trigger();
}


float VolumeDisplay::slicePosition( visBase::OrthogonalSlice* slice ) const
{
    if ( !slice ) return 0;
    const int dim = slice->getDim();
    float slicepos = slice->getPosition();
    slicepos *= -voltrans_->getScale()[dim];

    float pos;    
    if ( dim == 2 )
    {
	slicepos += voltrans_->getTranslation()[0];
	pos = SI().inlRange(true).snap(slicepos);
    }
    else if ( dim == 1 )
    {
	slicepos += voltrans_->getTranslation()[1];
	pos = SI().crlRange(true).snap(slicepos);
    }
    else
    {
	slicepos += voltrans_->getTranslation()[2];
	pos = slicepos;
    }

    return pos;
}


void VolumeDisplay::setSlicePosition( visBase::OrthogonalSlice* slice, 
					const CubeSampling& cs ) 
{
    if ( !slice ) return;

    const int dim = slice->getDim();
    float pos = 0;
    Interval<float> rg; 
    int nrslices = 0;
    slice->getSliceInfo( nrslices, rg );
    if ( dim == 2 )
	pos = (float)cs.hrg.inlRange().start;
    else if ( dim == 1 )
	pos = (float)cs.hrg.crlRange().start;
    else
	pos = (float)cs.zrg.start;

    pos -= voltrans_->getTranslation()[2-dim];
    pos /= -voltrans_->getScale()[dim];

    float slicenr =  nrslices ? (pos-rg.start)*nrslices/rg.width() : 0;
    float draggerpos = slicenr /(nrslices-1) *rg.width() + rg.start;
    Coord3 center(0,0,0);
    center[dim] = draggerpos;
    slice->setCenter( center, false );
    slice->motion.trigger();
}


const TypeSet<float>* VolumeDisplay::getHistogram( int attrib ) const
{ return attrib ? 0 : &scalarfield_->getHistogram(); }


SurveyObject::AttribFormat VolumeDisplay::getAttributeFormat( int ) const
{ return visSurvey::SurveyObject::Cube; }


const Attrib::SelSpec* VolumeDisplay::getSelSpec( int attrib ) const
{ return attrib ? 0 : &as_; }


void VolumeDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    if ( attrib || as_==as ) return;
    as_ = as;
    if ( cache_ ) cache_->unRef();
    cache_ = 0;

    DPM( DataPackMgr::CubeID() ).release( cacheid_ );
    cacheid_ = DataPack::cNoID();

    scalarfield_->setScalarField( 0, true, 0 );

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );
}


CubeSampling VolumeDisplay::getCubeSampling( int attrib ) const
{ return getCubeSampling(true,false,attrib); }


bool VolumeDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				   TaskRunner* tr )
{
    if ( attrib>0 ) return false;

    DataPackMgr& dpman = DPM( DataPackMgr::CubeID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    const bool res = setDataVolume( attrib, cdp ? &cdp->cube() : 0, tr );
    if ( !res )
    {
	dpman.release( dpid );
	return false;
    }

    const DataPack::ID oldid = cacheid_;
    cacheid_ = dpid;

    dpman.release( oldid );
    return true;
}


bool VolumeDisplay::setDataVolume( int attrib,
				   const Attrib::DataCubes* attribdata,
       				   TaskRunner* tr )
{
    if ( attrib || !attribdata )
	return false;

    const Array3D<float>* usedarray = 0;
    bool arrayismine = true;
    if ( alreadyTransformed(attrib) || !datatransform_ )
	usedarray = &attribdata->getCube(0);
    else
    {
	if ( !datatransformer_ )
	    mTryAlloc( datatransformer_,ZAxisTransformer(*datatransform_,true));

//	datatransformer_->setInterpolate( !isClassification(attrib) );
	datatransformer_->setInterpolate( true );
	datatransformer_->setInput( attribdata->getCube(0),
				    attribdata->cubeSampling() );
	datatransformer_->setOutputRange( getCubeSampling(true,true,0) );

	if ( (tr && !tr->execute(*datatransformer_)) ||
             !datatransformer_->execute() )
	{
	    pErrMsg( "Transform failed" );
	    return false;
	}

	usedarray = datatransformer_->getOutput( true );
	if ( !usedarray )
	{
	    pErrMsg( "No output from transform" );
	    return false;
	}

	arrayismine = false;
    }

    scalarfield_->setScalarField( usedarray, !arrayismine, tr );

    setCubeSampling( getCubeSampling(true,true,0) );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setVolumeDataSize( usedarray->info().getSize(2),
					 usedarray->info().getSize(1),
					 usedarray->info().getSize(0) );

    scalarfield_->turnOn( true );

    if ( cache_ != attribdata )
    {
	if ( cache_ ) cache_->unRef();
	cache_ = attribdata;
	cache_->ref();
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );

    return true;
}


const Attrib::DataCubes* VolumeDisplay::getCacheVolume( int attrib ) const
{ return attrib ? 0 : cache_; }


DataPack::ID VolumeDisplay::getDataPackID( int attrib ) const
{ return attrib==0 ? cacheid_ : DataPack::cNoID(); }


void VolumeDisplay::getMousePosInfo( const visBase::EventInfo&,
				     Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "";
    val = "undef";
    Coord3 attribpos = pos;
    RefMan<const ZAxisTransform> datatrans = getZAxisTransform();
    if ( datatrans ) //TODO check for allready transformed data.
    {
	attribpos.z = datatrans->transformBack( pos );
	if ( !attribpos.isDefined() )
	    return;
    }

    if ( !isManipulatorShown() )
	val = getValue( attribpos );
}


CubeSampling VolumeDisplay::getCubeSampling( bool manippos, bool displayspace,
					     int attrib ) const
{
    CubeSampling res;
    if ( manippos )
    {
	Coord3 center_ = boxdragger_->center();
	Coord3 width_ = boxdragger_->width();

	res.hrg.start = BinID( mNINT32( center_.x - width_.x / 2 ),
			      mNINT32( center_.y - width_.y / 2 ) );

	res.hrg.stop = BinID( mNINT32( center_.x + width_.x / 2 ),
			     mNINT32( center_.y + width_.y / 2 ) );

	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = center_.z - width_.z / 2;
	res.zrg.stop = center_.z + width_.z / 2;
    }
    else
    {
	const Coord3 transl = voltrans_->getTranslation();
	Coord3 scale = voltrans_->getScale();
	double dummy = scale.x; scale.x=scale.z; scale.z = dummy;

	res.hrg.start = BinID( mNINT32(transl.x+scale.x/2),
			       mNINT32(transl.y+scale.y/2) );
	res.hrg.stop = BinID( mNINT32(transl.x-scale.x/2),
			       mNINT32(transl.y-scale.y/2) );
	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = transl.z+scale.z/2;
	res.zrg.stop = transl.z-scale.z/2;
    }

    const bool alreadytf = alreadyTransformed( attrib );
    if ( alreadytf )
    {
	if ( scene_ )
	    res.zrg.step = scene_->getCubeSampling().zrg.step;
	else if ( datatransform_ )
	    res.zrg.step = datatransform_->getGoodZStep();
	return res;
    }

    if ( datatransform_ )
    {
	if ( !displayspace )
	{
	    res.zrg.setFrom( datatransform_->getZInterval(true) );
	    res.zrg.step = SI().zRange(true).step;
	}
	else
	{
	    if ( scene_ )
		res.zrg.step = scene_->getCubeSampling().zrg.step;
	    else
		res.zrg.step = datatransform_->getGoodZStep();
	}
    }
    else
	res.zrg.step = SI().zRange(true).step;

    return res;
}


bool VolumeDisplay::allowsPicks() const
{
    return !isVolRenShown();
}


visSurvey::SurveyObject* VolumeDisplay::duplicate( TaskRunner* tr ) const
{
    VolumeDisplay* vd = create();

    SoNode* node = vd->getInventorNode();

    TypeSet<int> children;
    vd->getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	vd->removeChild( children[idx] );

    vd->setZAxisTransform( const_cast<ZAxisTransform*>(datatransform_), tr );
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	const int sliceid = vd->addSlice( slices_[idx]->getDim() );
	mDynamicCastGet(visBase::OrthogonalSlice*,slice,
			visBase::DM().getObject(sliceid));
	slice->setSliceNr( slices_[idx]->getSliceNr() );
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	const int isosurfid = vd->addIsoSurface();
	mDynamicCastGet( mVisMCSurf*, isosurface,
			 visBase::DM().getObject(isosurfid) );
	vd->isosurfsettings_[idx] = isosurfsettings_[idx];
    }

    vd->init();
    vd->showVolRen( isVolRenShown() );

    vd->setCubeSampling( getCubeSampling(false,true,0) );

    vd->setSelSpec( 0, as_ );
    vd->setDataVolume( 0, cache_, tr );
    return vd;
}


void VolumeDisplay::init()
{
    isinited_ = true;
    scalarfield_->useShading( allowshading_ );

    const int voltransidx = childIndex( voltrans_->getInventorNode() );
    insertChild( voltransidx+1, scalarfield_->getInventorNode() );

    scalarfield_->turnOn( true );

    if ( !slices_.size() )
    {
	addSlice( cInLine() );
	addSlice( cCrossLine() );
	addSlice( cTimeSlice() );
    }
    
    if ( !volren_ )
    {
	showVolRen( true );
	showVolRen( false );
    }
}


SoNode* VolumeDisplay::gtInvntrNode()
{
    if ( !isinited_ )
	init();

    return VisualObjectImpl::gtInvntrNode();
}


void VolumeDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
	    mCB(this,VolumeDisplay,updateMouseCursorCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
	    mCB(this,VolumeDisplay,updateMouseCursorCB) );
    }
}


bool VolumeDisplay::isSelected() const
{
    return visBase::DM().selMan().selected().indexOf( id()) != -1;
}


void VolumeDisplay::updateMouseCursorCB( CallBacker* cb )
{
    char newstatus = 1; // 1=pan, 2=tabs
    if ( cb )
    {
	mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
	if ( eventinfo.pickedobjids.indexOf(boxdragger_->id())==-1 )
	    newstatus = 0;
	else
	{
	    //TODO determine if tabs
	}
    }

    if ( !isSelected() || !isOn() || isLocked() )
	newstatus = 0;

    if ( !newstatus ) mousecursor_.shape_ = MouseCursor::NotSet;
    else if ( newstatus==1 ) mousecursor_.shape_ = MouseCursor::SizeAll;
}


VolumeDisplay::IsosurfaceSetting::IsosurfaceSetting()
{
    mode_ = 1;
    seedsaboveisoval_ = -1;
    seedsid_ = MultiID();
}


bool VolumeDisplay::IsosurfaceSetting::operator==( 
	const IsosurfaceSetting& ns ) const
{
    return mode_==ns.mode_ && seedsaboveisoval_==ns.seedsaboveisoval_ &&
	   seedsid_==ns.seedsid_ && 
	   mIsEqual(isovalue_, ns.isovalue_, (isovalue_+ns.isovalue_)/2000 );
}


VolumeDisplay::IsosurfaceSetting& VolumeDisplay::IsosurfaceSetting::operator=(
       const IsosurfaceSetting& ns )
{
    mode_ = ns.mode_;
    seedsaboveisoval_ = ns.seedsaboveisoval_;
    seedsid_ = ns.seedsid_;
    isovalue_ = ns.isovalue_;

    return *this;
}


bool VolumeDisplay::canSetColTabSequence() const
{ return true; }


void VolumeDisplay::setColTabSequence( int attr, const ColTab::Sequence& seq,
       					TaskRunner* tr )
{
    scalarfield_->setColTabSequence( seq, tr );
    updateIsoSurfColor();
}


const ColTab::Sequence* VolumeDisplay::getColTabSequence( int attrib ) const
{
    return &scalarfield_->getColTabSequence();
}


void VolumeDisplay::setColTabMapperSetup( int attrib,
					  const ColTab::MapperSetup& ms,
       					  TaskRunner* tr )
{
    scalarfield_->setColTabMapperSetup( ms, tr );
    updateIsoSurfColor();
}


const ColTab::MapperSetup* VolumeDisplay::getColTabMapperSetup( int, int ) const
{
    return &scalarfield_->getColTabMapper().setup_;
}


void VolumeDisplay::turnOn( bool yn )
{
    onoffstatus_ = yn;
    
    VisualObjectImpl::turnOn( isAttribEnabled( 0 ) && yn );
}


bool VolumeDisplay::isOn() const
{ return onoffstatus_; }


void VolumeDisplay::fillPar( IOPar& par, TypeSet<int>& saveids) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    const CubeSampling cs = getCubeSampling(false,true,0);
    cs.fillPar( par );

    if ( volren_ )
    {
	int volid = volren_->id();
	par.set( sKeyVolumeID(), volid );
	if ( saveids.indexOf( volid )==-1 ) saveids += volid;
    }

    const int nrslices = slices_.size();
    par.set( sKeyNrSlices(), nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( sKeySlice(), idx );
	const int sliceid = slices_[idx]->id();
	par.set( str, sliceid );
	if ( saveids.indexOf(sliceid) == -1 ) saveids += sliceid;
    }

    const int nrisosurfaces = isosurfaces_.size();
    par.set( sKeyNrIsoSurfaces(), nrisosurfaces );
    for ( int idx=0; idx<nrisosurfaces; idx++ )
    {
	BufferString str( sKeyIsoValueStart() ); str += idx;
	par.set( str, isosurfsettings_[idx].isovalue_ );

	str = sKeyIsoOnStart(); str += idx;
	par.setYN( str, isosurfaces_[idx]->isOn() );

	str = sKeySurfMode(); str += idx;
	par.set( str, isosurfsettings_[idx].mode_ );

	str = sKeySeedsAboveIsov(); str += idx;
	par.set( str, isosurfsettings_[idx].seedsaboveisoval_ );

	str = sKeySeedsMid(); str += idx;
	par.set( str, isosurfsettings_[idx].seedsid_ );
    }

    fillSOPar( par, saveids );
}


int VolumeDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    if ( !getMaterial() )
	visBase::VisualObjectImpl::setMaterial( visBase::Material::create() );

    PtrMan<IOPar> texturepar = par.subselect( sKeyTexture() );
    if ( texturepar ) //old format (up to 4.0)
    {
	ColTab::MapperSetup mappersetup;
	ColTab::Sequence sequence;

	mappersetup.usePar(*texturepar);
	sequence.usePar(*texturepar );
	setColTabMapperSetup( 0, mappersetup, 0 );
	setColTabSequence( 0, sequence, 0 );
	if ( !as_.usePar(par) ) return -1;
    }
    else
    {
	res = useSOPar( par );
	if ( res!=1 )
	    return res;
    }

    int volid;
    if ( par.get(sKeyVolumeID(),volid) )
    {
	RefMan<visBase::DataObject> dataobj = visBase::DM().getObject( volid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::VolrenDisplay*,vr,dataobj.ptr());
	if ( !vr ) return -1;
	if ( volren_ )
	{
	    if ( childIndex(volren_->getInventorNode())!=-1 )
		VisualObjectImpl::removeChild(volren_->getInventorNode());
	    volren_->unRef();
	}
	volren_ = vr;
	volren_->ref();
	addChild( volren_->getInventorNode() );
    }

    while ( slices_.size() )
	removeChild( slices_[0]->id() );

    while ( isosurfaces_.size() )
	removeChild( isosurfaces_[0]->id() );

    int nrslices = 0;
    par.get( sKeyNrSlices(), nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( sKeySlice(), idx );
	int sliceid;
	par.get( str, sliceid );
	RefMan<visBase::DataObject> dataobj = visBase::DM().getObject(sliceid);
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::OrthogonalSlice*,os,dataobj.ptr())
	if ( !os ) return -1;
	os->ref();
	os->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
	slices_ += os;
	addChild( os->getInventorNode() );
	// set correct dimensions ...
	if ( !strcmp(os->name(),sKeyInline()) )
	    os->setDim( cInLine() );
	else if ( !strcmp(os->name(),sKeyCrossLine()) )
	    os->setDim( cCrossLine() );
	else if ( !strcmp(os->name(),sKeyTime()) )
	    os->setDim( cTimeSlice() );
    }

    CubeSampling cs;
    if ( cs.usePar(par) )
    {
	csfromsession_ = cs;
	setCubeSampling( cs );
    }

    int nrisosurfaces;
    if ( par.get( sKeyNrIsoSurfaces(), nrisosurfaces ) )
    {
	for ( int idx=0; idx<nrisosurfaces; idx++ )
	{
	    BufferString str( sKeyIsoValueStart() ); str += idx;
	    float isovalue;
	    if ( par.get( str, isovalue ) )
	    {
		addIsoSurface( 0, false );
		isosurfsettings_[idx].isovalue_ = isovalue;
	    }

	    str = sKeyIsoOnStart(); str += idx;
	    bool status = true;
	    par.getYN( str, status );
	    isosurfaces_[idx]->turnOn( status );
	    
	    str = sKeySurfMode(); str += idx;
	    int smode;
	    par.get( str, smode );
	    isosurfsettings_[idx].mode_ = smode;
	    
	    str = sKeySeedsAboveIsov(); str += idx;
	    int aboveisov;
	    par.get( str, aboveisov );
	    isosurfsettings_[idx].seedsaboveisoval_ = aboveisov;
    
	    str = sKeySeedsMid(); str += idx;
	    MultiID mid;
	    par.get( str, mid );
	    isosurfsettings_[idx].seedsid_ = mid;
	}
    }

    return 1;
}


bool VolumeDisplay::writeVolume( const char* filename ) const
{
    if ( !scalarfield_ )
	return false;

    std::ofstream strm( filename );
    if ( !strm )
    {
	errmsg_ = "Cannot open file";
	return false;
    }

    errmsg_ = scalarfield_->writeVolumeFile( strm );
    return errmsg_.str();
}


visBase::OrthogonalSlice* VolumeDisplay::getSelectedSlice() const
{
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->isSelected() )
	    return const_cast<visBase::OrthogonalSlice*>( slices_[idx] );
    }
    return 0;
}


CubeSampling VolumeDisplay::sliceSampling(visBase::OrthogonalSlice* slice) const
{
    CubeSampling cs(false); 
    if ( !slice ) return cs; 
    cs = getCubeSampling(false,true,0);
    float pos = slicePosition( slice ); 
    if ( slice->getDim() == cTimeSlice() )
	cs.zrg.limitTo( Interval<float>( pos, pos ) );
    else if ( slice->getDim() == cCrossLine() )
	cs.hrg.setCrlRange( Interval<int>( mNINT32(pos), mNINT32(pos) ) );
    else if ( slice->getDim() == cInLine() )
	cs.hrg.setInlRange( Interval<int>( mNINT32(pos), mNINT32(pos) ) );
    return cs;
}

} // namespace visSurvey
