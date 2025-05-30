/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvolumedisplay.h"

#include "array3dfloodfill.h"
#include "arrayndimpl.h"
#include "attribsel.h"
#include "color.h"
#include "flatview.h"
#include "marchingcubes.h"
#include "paralleltask.h"
#include "pickset.h"
#include "picksettr.h"
#include "od_ostream.h"
#include "settings.h"
#include "uistrings.h"
#include "vismaterial.h"
#include "visselman.h"
#include "zaxistransformer.h"

#include <QTimer>

/* OSG-TODO: Port VolrenDisplay volren_ and set of OrthogonalSlice slices_
   to OSG in case of prolongation. */


#define mVisMCSurf visBase::MarchingCubesSurface
#define mDefaultBoxTransparency 0.75


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


static TrcKeyZSampling getInitTrcKeyZSampling( const TrcKeyZSampling& csin )
{
    TrcKeyZSampling cs(false);
    cs.hsamp_.survid_ = csin.hsamp_.survid_;
    cs.hsamp_.start_.inl() =
	(5*csin.hsamp_.start_.inl()+3*csin.hsamp_.stop_.inl())/8;
    cs.hsamp_.start_.crl() =
	(5*csin.hsamp_.start_.crl()+3*csin.hsamp_.stop_.crl())/8;
    cs.hsamp_.stop_.inl() =
	(3*csin.hsamp_.start_.inl()+5*csin.hsamp_.stop_.inl())/8;
    cs.hsamp_.stop_.crl() =
	(3*csin.hsamp_.start_.crl()+5*csin.hsamp_.stop_.crl())/8;
    cs.zsamp_.start_ = ( 5*csin.zsamp_.start_ + 3*csin.zsamp_.stop_ ) / 8.f;
    cs.zsamp_.stop_ = ( 3*csin.zsamp_.start_ + 5*csin.zsamp_.stop_ ) / 8.f;
    SI().snap( cs.hsamp_.start_ );
    SI().snap( cs.hsamp_.stop_ );
    float z0 = csin.zsamp_.snap( cs.zsamp_.start_ ); cs.zsamp_.start_ = z0;
    float z1 = csin.zsamp_.snap( cs.zsamp_.stop_ ); cs.zsamp_.stop_ = z1;
    return cs;
}


// VolumeDisplay::AttribData

VolumeDisplay::AttribData::AttribData()
    : as_(*new Attrib::SelSpec)
    , selspec_(new TypeSet<Attrib::SelSpec>(1,Attrib::SelSpec()))
{}


VolumeDisplay::AttribData::~AttribData()
{
    delete &as_;
    delete selspec_;
}


// VolumeDisplay

VolumeDisplay::VolumeDisplay()
    : visBase::VisualObjectImpl(true)
    , csfromsession_(true)
    , boxMoving(this)
{
    ref();
    boxdragger_ = visBase::BoxDragger::create();
    texturecs_.hsamp_.setGeomID( Survey::default3DGeomID() );
    addChild( boxdragger_->osgNode() );

    boxdragger_->setBoxTransparency( mDefaultBoxTransparency );
    mAttachCB( boxdragger_->started, VolumeDisplay::draggerStartCB );
    mAttachCB( boxdragger_->motion, VolumeDisplay::draggerMoveCB );
    mAttachCB( boxdragger_->finished, VolumeDisplay::draggerFinishCB );

    updateRanges( true, true );

    scalarfield_ = visBase::VolumeRenderScalarField::create();
    setChannels2RGBA( visBase::ColTabTextureChannel2RGBA::create().ptr() );
    addAttrib();

    addChild( scalarfield_->osgNode() );

    getMaterial()->setColor( OD::Color::White() );
    getMaterial()->setAmbience( 0.3 );
    getMaterial()->setDiffIntensity( 0.8 );
    mAttachCB( getMaterial()->change, VolumeDisplay::materialChange );
    scalarfield_->setMaterial( getMaterial() );

    TrcKeyZSampling sics = SI().sampling( true );
    TrcKeyZSampling cs = getInitTrcKeyZSampling( sics );
    setTrcKeyZSampling( cs );

    int buttonkey = OD::NoButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyVolDepthKey(), buttonkey );
    boxdragger_->setPlaneTransDragKeys( true, buttonkey );
    buttonkey = OD::ShiftButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyVolPlaneKey(), buttonkey );
    boxdragger_->setPlaneTransDragKeys( false, buttonkey );

    bool useindepthtransforresize = true;
    mSettUse( getYN, "dTect.MouseInteraction", sKeyInDepthVolResize(),
	      useindepthtransforresize );
    boxdragger_->useInDepthTranslationForResize( useindepthtransforresize );

    showManipulator( boxdragger_->isOn() );
    unRefNoDelete();
}


VolumeDisplay::~VolumeDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( nullptr );

    deepErase( attribs_ );

    TypeSet<VisID> children;
    getChildren( children );
    for ( const auto& child : children )
	removeChild( child );

    boxdragger_ = nullptr;
    scalarfield_ = nullptr;
    setZAxisTransform( nullptr, nullptr );
}


void VolumeDisplay::setMaterial( visBase::Material* mt )
{
    if ( material_ )
	mDetachCB( getMaterial()->change, VolumeDisplay::materialChange );

    visBase::VisualObjectImpl::setMaterial( mt );
    if ( material_ )
	mAttachCB( getMaterial()->change, VolumeDisplay::materialChange );

    materialChange( nullptr );
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
	OD::Color col;
	if ( mIsUdf(val) )
	    col = getColTabSequence( 0 )->undefColor();
	else
	{
	    // TODO: adapt to multi-attrib
	    const float mappedval =
		scalarfield_->getColTabMapper(0).position( val );
	    col = getColTabSequence( 0 )->color( mappedval );
	}

	isosurfaces_[idx]->getMaterial()->setColor( col );
    }
}


bool VolumeDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( zat == datatransform_.ptr() )
	return true;

    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	{
	    mDetachCB( *datatransform_->changeNotifier(),
		       VolumeDisplay::dataTransformCB );
	}

	datatransform_ = nullptr;
    }

    deleteAndNullPtr( datatransformer_ );
    datatransform_ = zat;

    if ( datatransform_ )
    {
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	{
	    mAttachCB( *datatransform_->changeNotifier(),
		       VolumeDisplay::dataTransformCB );
	}
    }

    return true;
}


const ZAxisTransform* VolumeDisplay::getZAxisTransform() const
{
    return datatransform_.ptr();
}


void VolumeDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    boxdragger_->setRightHandSystem( yn );
    scalarfield_->setRightHandSystem( yn );
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	isosurfaces_[idx]->setRightHandSystem( yn );
}


void VolumeDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int attrib=0; attrib<attribs_.size(); attrib++ )
    {
	if ( attribs_[attrib]->cache_ )
	    setVolumeDataPack( attrib,attribs_[attrib]->cache_.getNonConstPtr(),
			     nullptr );
    }
}


void VolumeDisplay::setScene( Scene* sc )
{
    SurveyObject::setScene( sc );
    if ( sc )
	updateRanges( false, false );
}


void VolumeDisplay::updateRanges( bool updateic, bool updatez )
{
    if ( !datatransform_ )
	return;

    const TrcKeyZSampling defcs( true );
    if ( csfromsession_ != defcs )
	setTrcKeyZSampling( csfromsession_ );
    else
    {
	const TrcKeyZSampling& csin = scene_ ? scene_->getTrcKeyZSampling()
					  : getTrcKeyZSampling( false, 0 );
	TrcKeyZSampling cs = getInitTrcKeyZSampling( csin );
	setTrcKeyZSampling( cs );
    }
}


void VolumeDisplay::getChildren( TypeSet<VisID>&res ) const
{
    res.erase();
    for ( int idx=0; idx<slices_.size(); idx++ )
	res += slices_[idx]->id();
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	res += isosurfaces_[idx]->id();
//    if ( volren_ ) res += volren_->id();
}


void VolumeDisplay::showManipulator( bool yn )
{
    boxdragger_->turnOn( yn );
    scalarfield_->enableTraversal(visBase::cDraggerIntersecTraversalMask(),!yn);
}


bool VolumeDisplay::isManipulatorShown() const
{ return boxdragger_->isOn(); }


bool VolumeDisplay::isManipulated() const
{
    return ismanip_ &&
	(!isinited_ || !texturecs_.includes(getTrcKeyZSampling(true,true,0)) );
}


bool VolumeDisplay::canResetManipulation() const
{ return true; }


void VolumeDisplay::resetManipulation()
{
    ismanip_ = false;
}


void VolumeDisplay::acceptManipulation()
{
    setTrcKeyZSampling( getTrcKeyZSampling(true,true,0) );
    ismanip_ = false;
}


void VolumeDisplay::draggerStartCB( CallBacker* )
{
    updateDraggerLimits( false );
}


void VolumeDisplay::draggerMoveCB( CallBacker* )
{
    TrcKeyZSampling cs = getTrcKeyZSampling(true,true,0);
    if ( scene_ )
	cs.limitTo( scene_->getTrcKeyZSampling() );

    QTimer::singleShot( 0, [this, cs] {

	const Coord3 center(
	    ( cs.hsamp_.start_.inl() + cs.hsamp_.stop_.inl() ) / 2.0,
	    ( cs.hsamp_.start_.crl() + cs.hsamp_.stop_.crl() ) / 2.0,
	    ( cs.zsamp_.start_ + cs.zsamp_.stop_ ) / 2.0 );

	const Coord3 width(
	    cs.hsamp_.stop_.inl() - cs.hsamp_.start_.inl(),
	    cs.hsamp_.stop_.crl() - cs.hsamp_.start_.crl(),
	    cs.zsamp_.stop_ - cs.zsamp_.start_ );

	boxdragger_->setCenter( center );
	boxdragger_->setWidth( width );

	setTrcKeyZSampling( cs, true );
	if ( keepdraggerinsidetexture_ )
	{
	    boxdragger_->setBoxTransparency( 1.0 );
	    boxdragger_->showScaleTabs( false );
	}

	boxMoving.trigger();
	ismanip_ = true;
    });
}


void VolumeDisplay::draggerFinishCB( CallBacker* )
{
    QTimer::singleShot( 0, [this] {
	boxdragger_->setBoxTransparency( mDefaultBoxTransparency );
	boxdragger_->showScaleTabs( true );
    });
}


VisID VolumeDisplay::addSlice( int dim )
{
    RefMan<visBase::OrthogonalSlice> slice = visBase::OrthogonalSlice::create();
    slice->setMaterial( nullptr );
    slice->setDim( dim );
    mAttachCB( slice->motion, VolumeDisplay::sliceMoving );
    slices_ += slice.ptr();

    slice->setUiName( dim==cTimeSlice() ? uiStrings::sTime() :
		   (dim==cCrossLine()
		    ? uiStrings::sCrossline()
		    : uiStrings::sInline()) );

    addChild( slice->osgNode() );
    const TrcKeyZSampling cs = getTrcKeyZSampling( false, 0 );
    const Interval<float> defintv(-0.5,0.5);
    slice->setSpaceLimits( defintv, defintv, defintv );
    // TODO: adapt to multi-attrib
    if ( attribs_[0]->cache_ )
    {
	const Array3D<float>& arr = attribs_[0]->cache_->data();
	slice->setVolumeDataSize( arr.info().getSize(2),
				  arr.info().getSize(1),
				  arr.info().getSize(0) );
    }

    return slice->id();
}


void VolumeDisplay::removeChild( const VisID& displayid )
{
/*
    if ( volren_ && displayid==volren_->id() )
    {
	VisualObjectImpl::removeChild( volren_->osgNode() );
	volren_ = nullptr;
	return;
    }
*/

    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild( slices_[idx]->osgNode() );
	    mDetachCB( slices_[idx]->motion, VolumeDisplay::sliceMoving );
	    slices_.removeSingle(idx,false);
	    return;
	}
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( isosurfaces_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild( isosurfaces_[idx]->osgNode() );
	    isosurfaces_.removeSingle(idx,false);
	    isosurfsettings_.removeSingle(idx,false);
	    return;
	}
    }
}


void VolumeDisplay::showVolRen( bool yn )
{
/*
    if ( yn && !volren_ )
    {
	volren_ = visBase::VolrenDisplay::create();
	volren_->setMaterial( nullptr );
	addChild( volren_->osgNode() );
	volren_->setName( sKeyVolRen() );
    }

    if ( volren_ ) volren_->turnOn( yn );
*/
}


bool VolumeDisplay::isVolRenShown() const
{
    return false;
//    return volren_ && volren_->isOn();
}


float VolumeDisplay::defaultIsoValue() const
{
    // TODO: adapt to multi-attrib
    return attribs_[0]->cache_ ? getColTabMapperSetup(0)->range_.center()
			       : mUdf(float);
}


VisID VolumeDisplay::addIsoSurface( TaskRunner* taskr, bool updateisosurface )
{
    RefMan<mVisMCSurf> isosurface = mVisMCSurf::create();
    isosurface->setRightHandSystem( righthandsystem_ );
    RefMan<MarchingCubesSurface> surface = new MarchingCubesSurface();
    isosurface->setSurface( *surface, taskr );
    isosurface->setUiName( ::toUiString("Iso surface") );

    isosurfaces_ += isosurface.ptr();
    IsosurfaceSetting setting;
    setting.isovalue_ = defaultIsoValue();
    isosurfsettings_ += setting;

    if ( updateisosurface )
	updateIsoSurface( isosurfaces_.size()-1, taskr );

    //add before the volume transform
    addChild( isosurface->osgNode() );
    materialChange( nullptr ); //updates new surface's material
    return isosurface->id();
}


VisID VolumeDisplay::volRenID() const
{
    return VisID::udf();
//    return volren_ ? volren_->id() : -1;
}


#define mSetVolumeTransform( name, center, width, cs, extrasteps ) \
\
    const Coord3 center( (cs.hsamp_.start_.inl() + cs.hsamp_.stop_.inl())/2.0, \
			 (cs.hsamp_.start_.crl() + cs.hsamp_.stop_.crl())/2.0, \
			 (cs.zsamp_.start_ + cs.zsamp_.stop_)/2.0 ); \
\
    const Coord3 width( cs.hsamp_.stop_.inl() - cs.hsamp_.start_.inl(), \
			cs.hsamp_.stop_.crl() - cs.hsamp_.start_.crl(), \
			cs.zsamp_.stop_ - cs.zsamp_.start_ ); \
{ \
    const Coord3 step( cs.hsamp_.step_.inl(), cs.hsamp_.step_.crl(), \
		       cs.zsamp_.step_ ); \
\
    Coord3 trans( center ); \
    mVisTrans::transform( displaytrans_.ptr(), trans ); \
    Coord3 scale( width + extrasteps*step ); \
    mVisTrans::transformSize( displaytrans_.ptr(), scale ); \
    trans += 0.5 * scale; \
    scale = Coord3( scale.z_, -scale.y_, -scale.x_ ); \
    scalarfield_->set##name##Transform( trans, Coord3(0,1,0), M_PI_2, scale ); \
}

void VolumeDisplay::setTrcKeyZSampling( const TrcKeyZSampling& desiredcs )
{
    setTrcKeyZSampling( desiredcs, false );
}


void VolumeDisplay::setTrcKeyZSampling( const TrcKeyZSampling& desiredcs,
					bool dragmode )
{
    TrcKeyZSampling cs( desiredcs );

    if ( dragmode )
	cs.limitTo( texturecs_ );
    else if ( scene_ )
	cs.limitTo( scene_->getTrcKeyZSampling() );

    updateDraggerLimits( dragmode );
    mSetVolumeTransform( ROIVolume, center, width, cs, 0 )

    if ( dragmode )
	return;

    TrcKeyZSampling texcs = scalarfield_->getMultiAttribTrcKeyZSampling();
    if ( !texcs.isDefined() || texcs.isEmpty() )
	texcs = cs;

    mSetVolumeTransform( TexVolume, texcenter, texwidth, texcs, 1 )

    texturecs_ = cs;
    scalarfield_->turnOn( false );

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	isosurfaces_[idx]->getSurface()->removeAll();
	isosurfaces_[idx]->touch( false );
    }

    boxdragger_->setCenter( center );
    boxdragger_->setWidth( width );
}


void VolumeDisplay::updateDraggerLimits( bool dragmode )
{
    const TrcKeyZSampling curcs = getTrcKeyZSampling( true, true, 0 );

    if ( !dragmode )
    {
	keepdraggerinsidetexture_ = false;
	draggerstartcs_ = curcs;
    }

    if ( curcs!=draggerstartcs_ && texturecs_.includes(curcs) &&
	 scalarfield_->isOn() )
    {
	keepdraggerinsidetexture_ = true;
    }

    TrcKeyZSampling limcs( texturecs_ );
    if ( !keepdraggerinsidetexture_ && scene_ )
	limcs = scene_->getTrcKeyZSampling();

    const StepInterval<float> inlrg( sCast(float,limcs.hsamp_.start_.inl()),
				     sCast(float,limcs.hsamp_.stop_.inl()),
				     sCast(float,limcs.hsamp_.step_.inl()) );
    const StepInterval<float> crlrg( sCast(float,limcs.hsamp_.start_.crl()),
				     sCast(float,limcs.hsamp_.stop_.crl()),
				     sCast(float,limcs.hsamp_.step_.crl()) );

    boxdragger_->setSpaceLimits( inlrg, crlrg, limcs.zsamp_ );

    const int minvoxwidth = 1;
    boxdragger_->setWidthLimits(
	Interval<float>( sCast(float,minvoxwidth*limcs.hsamp_.step_.inl()),
			 mUdf(float) ),
	Interval<float>( sCast(float,minvoxwidth*limcs.hsamp_.step_.crl()),
			 mUdf(float) ),
	Interval<float>( minvoxwidth*limcs.zsamp_.step_, mUdf(float) ) );

    boxdragger_->setDragCtrlSpacing( inlrg, crlrg, limcs.zsamp_ );
}


float VolumeDisplay::getValue( int attrib, const Coord3& pos ) const
{
    if ( !attribs_.validIdx(attrib) || !attribs_[attrib]->cache_ )
	return mUdf(float);

    const BinID bid( SI().transform(pos) );
    const TrcKeyZSampling& samp = attribs_[attrib]->cache_->sampling();
    const int inlidx = samp.inlIdx( bid.inl() );
    const int crlidx = samp.crlIdx( bid.crl() );
    const int zidx = samp.zsamp_.getIndex( pos.z_ );

    const Array3DImpl<float>& array = attribs_[attrib]->cache_->data();
    const float val = array.info().validPos(inlidx,crlidx,zidx) ?
	array.get( inlidx, crlidx, zidx ) : mUdf(float);
    return val;
}


float VolumeDisplay::isoValue( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    return idx<0 ? mUdf(float) : isosurfsettings_[idx].isovalue_;
}


void VolumeDisplay::setIsoValue( const mVisMCSurf* mcd, float nv,
				 TaskRunner* taskr )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 )
	return;

    isosurfsettings_[idx].isovalue_ = nv;
    updateIsoSurface( idx, taskr );
}


mVisMCSurf* VolumeDisplay::getIsoSurface( int idx )
{ return isosurfaces_.validIdx(idx) ? isosurfaces_[idx] : nullptr; }


int VolumeDisplay::getNrIsoSurfaces()
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

    isosurfsettings_[idx].mode_ = full ? 1 : 0;
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


bool VolumeDisplay::updateSeedBasedSurface( int idx, TaskRunner* taskr )
{
    // TODO: adapt to multi-attrib
    if ( idx<0 || idx>=isosurfaces_.size() || !attribs_[0]->cache_ ||
	 mIsUdf(isosurfsettings_[idx].isovalue_) ||
	 isosurfsettings_[idx].seedsid_.isUdf() )
	return false;

    uiString errmsg;
    ConstRefMan<Pick::Set> seeds =
		    Pick::getSet( isosurfsettings_[idx].seedsid_, errmsg );
    if ( !seeds )
	return false;

    // TODO: adapt to multi-attrib
    const Array3D<float>& data = attribs_[0]->cache_->data();
    if ( !data.isOK() )
	return false;

    Array3DImpl<float> newarr( data.info() );
    Array3DFloodfill<float> ff( data, isosurfsettings_[idx].isovalue_,
	    isosurfsettings_[idx].seedsaboveisoval_, newarr );
    ff.useInputValue( true );

    TrcKeyZSampling cs = getTrcKeyZSampling(true,true,0);
    cs.normalize();
    for ( int seedidx=0; seedidx<seeds->size(); seedidx++ )
    {
	const Pick::Location& loc = seeds->get( seedidx );
	const BinID bid = SI().transform( loc.pos() );
	const int i = cs.inlIdx( bid.inl() );
	const int j = cs.crlIdx( bid.crl() );
	const int k = cs.zIdx( loc.z() );
	ff.addSeed( i, j, k );
    }

    if ( !ff.execute() )
	return false;

    if ( !isosurfsettings_[idx].seedsaboveisoval_ )
    {
	const float outsideval = -1.f;
	const float threshold = isosurfsettings_[idx].isovalue_;
	float* newdata = newarr.getData();
	if ( newdata )
	{
	    for ( od_int64 idy=newarr.info().getTotalSz()-1; idy>=0; idy-- )
		newdata[idy] = mIsUdf(newdata[idy]) ? outsideval
						    : threshold-newdata[idy];
	}
	else if ( newarr.getStorage() )
	{
	    ValueSeries<float>* newstor = newarr.getStorage();
	    for ( od_int64 idy=newarr.info().getTotalSz()-1; idy>=0; idy-- )
	    {
		newstor->setValue(idy, mIsUdf(newstor->value(idy))
			? outsideval : threshold-newstor->value(idy) );
	    }

	}
	else
	{
	    for ( int id0=0; id0<newarr.info().getSize(0); id0++ )
	    {
		for ( int idy=0; idy<newarr.info().getSize(1); idy++ )
		{
		    for ( int idz=0; idz<newarr.info().getSize(2); idz++ )
		    {
			float val = newarr.get(id0,idy,idz);
			val = mIsUdf(val) ? outsideval : threshold-val;
			newarr.set( id0, idy, idz, val );
		    }
		}
	    }
	}
    }

    const float threshold = isosurfsettings_[idx].seedsaboveisoval_
	? isosurfsettings_[idx].isovalue_ : 0;

    isosurfaces_[idx]->getSurface()->setVolumeData( 0, 0, 0, newarr,
						    threshold, taskr );
    return true;
}


int VolumeDisplay::getIsoSurfaceIdx( const mVisMCSurf* mcd ) const
{
    return isosurfaces_.indexOf( mcd );
}


mDefParallelCalc3Pars( PtrDataFlipper, tr("Flipping data"),
	const float*, inpdata, float*, newdata, float, threshold )
mDefParallelCalcBody( , newdata_[idx] = threshold_ - inpdata_[idx]; , )


mDefParallelCalc3Pars( VSDataFlipper, tr("Flipping data"),
	const ValueSeries<float>*, arrstor, ValueSeries<float>*, newstor,
	float, threshold )
mDefParallelCalcBody( ,
	newstor_->setValue( idx, threshold_ - arrstor_->value(idx) ); , )

mDefParallelCalc3Pars( ArrDataFlipper, tr("Flipping data"),
	const Array3D<float>&, inpdata, Array3D<float>&, newdata,
	float, threshold )
mDefParallelCalcBody(
	const Array2DInfoImpl info2d( inpdata_.info().getSize(0),
				      inpdata_.info().getSize(1) );
	ArrayNDIter iter( info2d );
	iter.setGlobalPos( start );
	,
	const int iidx = iter[0];
	const int iidy = iter[1];
	const int idz = sCast(int,idx);
	newdata_.set( iidx, iidy, idz, threshold_-inpdata_.get(iidx,iidy,idz));
	, )


void VolumeDisplay::updateIsoSurface( int idx, TaskRunner* taskr )
{
    // TODO:: adapt to multi-attrib
    const RegularSeisDataPack* cache = attribs_[0]->cache_.ptr();
    if ( !cache || cache->isEmpty() ||
	 mIsUdf(isosurfsettings_[idx].isovalue_) )
	isosurfaces_[idx]->getSurface()->removeAll();
    else
    {
	const TrcKeyZSampling& samp = cache->sampling();
	isosurfaces_[idx]->getSurface()->removeAll();
	isosurfaces_[idx]->setBoxBoundary(
		    sCast(float,samp.hsamp_.inlRange().stop_),
		    sCast(float,samp.hsamp_.crlRange().stop_),
		    samp.zsamp_.stop_ );

	const SamplingData<float> inlsampling(
		    sCast(float,samp.hsamp_.inlRange().start_),
		    sCast(float,samp.hsamp_.inlRange().step_) );

	const SamplingData<float> crlsampling(
		    sCast(float,samp.hsamp_.crlRange().start_),
		    sCast(float,samp.hsamp_.crlRange().step_) );

	SamplingData<float> zsampling ( samp.zsamp_.start_, samp.zsamp_.step_ );
	isosurfaces_[idx]->setScales( inlsampling, crlsampling, zsampling );

	if ( isosurfsettings_[idx].mode_ )
	{
	    const Array3D<float>& arr = cache->data();
	    if ( !arr.isOK() )  return;

	    const od_int64 size = arr.info().getTotalSz();
	    PtrMan< Array3D<float> > newarr =
		new Array3DImpl<float>(arr.info());
	    if ( !newarr->isOK() )
		return;

	    const float threshold = isosurfsettings_[idx].isovalue_;
	    const float* data = arr.getData();
	    if ( data && newarr->getData() )
	    {
		PtrDataFlipper adf( size, data, newarr->getData(), threshold );
		if ( !adf.execute() )
		    return;
	    }
	    else if ( arr.getStorage() && newarr->getStorage() )
	    {
		VSDataFlipper vdf( size, arr.getStorage(), newarr->getStorage(),
			threshold );
		if ( !vdf.execute() )
		    return;
	    }
	    else
	    {
		const int nrz = arr.info().getSize( 2 );
		if ( nrz > 0 )
		{
		    ArrDataFlipper adf( size / nrz, arr, *newarr, threshold );
		    if ( !adf.execute() )
			return;
		}
	    }

	    isosurfaces_[idx]->getSurface()->setVolumeData( 0, 0, 0, *newarr,
							    0, taskr );
	}
	else
	{
	    if ( !updateSeedBasedSurface(idx,taskr) )
		return;
	}
    }

    updateIsoSurfColor();
    isosurfaces_[idx]->touch( false, taskr );
}


uiString VolumeDisplay::getManipulationString() const
{
    uiString str;
    getObjectInfo( str );
    return str;
}


void VolumeDisplay::getObjectInfoText( uiString& info, bool compact ) const
{
    const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true, 0 );
    const BufferString formatstr( compact
				  ? "%1-%2 / %3-%4 / %5-%6"
				  : "Inl: %1-%2, Crl: %3-%4, %7: %5-%6");
    info = ::toUiString( formatstr.str() ).arg( tkzs.hsamp_.start_.inl() )
					  .arg( tkzs.hsamp_.stop_.inl() )
					  .arg( tkzs.hsamp_.start_.crl() )
					  .arg( tkzs.hsamp_.stop_.crl() );
    const Scene* scene = getScene();
    const ZDomain::Info& datazdom = scene ? scene->zDomainInfo()
					  : SI().zDomainInfo();
    const ZDomain::Info& displayzdom = datazdom.isDepth()
			? ZDomain::DefaultDepth( true ) : datazdom;
    ZSampling zrg = tkzs.zsamp_;
    zrg.scale( FlatView::Viewer::userFactor(datazdom,&displayzdom) );
    info.arg( ::toUiStringDec(zrg.start_,0) );
    info.arg( ::toUiStringDec(zrg.stop_,0) );
    if ( !compact )
	info.arg( displayzdom.getLabel() );
}


void VolumeDisplay::getObjectInfo( uiString& info ) const
{
    getObjectInfoText( info, false );
}


void VolumeDisplay::getTreeObjectInfo( uiString& info ) const
{ getObjectInfoText( info, true ); }


void VolumeDisplay::sliceMoving( CallBacker* cb )
{
    mDynamicCastGet(visBase::OrthogonalSlice*,slice,cb)
    if ( !slice )
	return;

    slicename_ = slice->name();
    sliceposition_ = slicePosition( slice );
}


float VolumeDisplay::slicePosition( visBase::OrthogonalSlice* slice ) const
{
    if ( !slice )
	return 0.f;

    const int dim = slice->getDim();
    float slicepos = slice->getPosition();

    float pos;
    if ( dim == 2 )
	pos = sCast( float, SI().inlRange(true).snap(slicepos) );
    else if ( dim == 1 )
	pos = sCast( float, SI().crlRange(true).snap(slicepos) );
    else
	pos = slicepos;

    return pos;
}


void VolumeDisplay::setSlicePosition( visBase::OrthogonalSlice* slice,
					const TrcKeyZSampling& cs )
{
    if ( !slice ) return;

    const int dim = slice->getDim();
    float pos = 0;
    Interval<float> rg;
    int nrslices = 0;
    slice->getSliceInfo( nrslices, rg );
    if ( dim == 2 )
	pos = sCast(float,cs.hsamp_.inlRange().start_);
    else if ( dim == 1 )
	pos = sCast(float,cs.hsamp_.crlRange().start_);
    else
	pos = sCast(float,cs.zsamp_.start_);

//    pos -= (float) voltrans_->getTranslation()[2-dim];
//    pos /= (float) -voltrans_->getScale()[dim];

    const float slicenr =  nrslices ? (pos-rg.start_)*nrslices/rg.width() : 0;
    const float draggerpos = slicenr /(nrslices-1) *rg.width() + rg.start_;
    Coord3 center(0,0,0);
    center[dim] = sCast(double,draggerpos);
    slice->setCenter( center, false );
    slice->motion.trigger();
}


const TypeSet<float>* VolumeDisplay::getHistogram( int attrib ) const
{ return &scalarfield_->getHistogram( attrib ); }


SurveyObject::AttribFormat VolumeDisplay::getAttributeFormat( int ) const
{ return SurveyObject::Cube; }


const Attrib::SelSpec* VolumeDisplay::getSelSpec(
				int attrib, int version ) const
{
    const TypeSet<Attrib::SelSpec>* specs = getSelSpecs( attrib );
    if ( !specs || specs->isEmpty() )
	return nullptr;

    if ( !specs->validIdx(version) )
	version = 0;

    return specs->validIdx(version) ? &specs->get( version )
				    : &specs->first();
}


const TypeSet<Attrib::SelSpec>* VolumeDisplay::getSelSpecs( int attrib ) const
{
    return attribs_.validIdx(attrib) ? attribs_[attrib]->selspec_ : nullptr;
}


void VolumeDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    setSelSpecs( attrib, TypeSet<Attrib::SelSpec>(1,as) );
}


void VolumeDisplay::setSelSpecs( int attrib, const TypeSet<Attrib::SelSpec>& as)
{
    SurveyObject::setSelSpecs( attrib, as );
    if ( !attribs_.validIdx(attrib) || *attribs_[attrib]->selspec_==as )
	return;

    *attribs_[attrib]->selspec_ = as;
    attribs_[attrib]->cache_ = nullptr;

    TrcKeyZSampling emptytkzs( false );
    emptytkzs.hsamp_.survid_ = s3dgeom_->geomSystem();
    scalarfield_->setScalarField( attrib, nullptr, true, emptytkzs, 0 );

    updateAttribEnabling();

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );
}


TrcKeyZSampling VolumeDisplay::getTrcKeyZSampling( bool displayspace,
							int attrib ) const
{
    return getTrcKeyZSampling(true,displayspace,attrib);
}


bool VolumeDisplay::setVolumeDataPack( int attrib, VolumeDataPack* voldp,
				       TaskRunner* taskr )
{
    mDynamicCastGet(RegularSeisDataPack*,regsdp,voldp);
    if ( !attribs_.validIdx(attrib) || !regsdp || regsdp->isEmpty() )
	return false;

    TrcKeyZSampling tkzs = regsdp->sampling();

    const Array3D<float>* usedarray = nullptr;
    bool arrayismine = true;
    if ( alreadyTransformed(attrib) || !datatransform_ )
	usedarray = &regsdp->data();
    else
    {
	datatransform_->setDataFromZDomainInfo( regsdp->zDomain() );
	if ( !datatransformer_ )
	    datatransformer_ = new ZAxisTransformer( *datatransform_, true );

//	datatransformer_->setInterpolate( !isClassification(attrib) );
	datatransformer_->setInterpolate( true );
	datatransformer_->setInput( regsdp->data(), tkzs );
	tkzs.zsamp_ = getTrcKeyZSampling(true,true,0).zsamp_;
	datatransformer_->setOutputRange( tkzs );

	if ( !TaskRunner::execute(taskr,*datatransformer_) )
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

    tkzs.hsamp_.survid_ = s3dgeom_->geomSystem();
    scalarfield_->setScalarField( attrib, usedarray, !arrayismine, tkzs, taskr);

    setTrcKeyZSampling( getTrcKeyZSampling(true,true,0) );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setVolumeDataSize( usedarray->info().getSize(2),
					 usedarray->info().getSize(1),
					 usedarray->info().getSize(0) );

    if ( attribs_[attrib]->cache_.ptr() != regsdp )
	attribs_[attrib]->cache_ = regsdp;

    isinited_ = true;
    updateAttribEnabling();

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );

    return true;
}


ConstRefMan<DataPack> VolumeDisplay::getDataPack( int attrib ) const
{
    return getVolumeDataPack( attrib );
}


ConstRefMan<VolumeDataPack> VolumeDisplay::getVolumeDataPack( int attrib ) const
{
    return attribs_.validIdx(attrib) ? attribs_[attrib]->cache_ : nullptr;
}


void VolumeDisplay::getMousePosInfo( const visBase::EventInfo&,
				     Coord3& pos, BufferString& val,
				     uiString& info ) const
{
    info.setEmpty();
    val = "undef";
    Coord3 attribpos = pos;
    ConstRefMan<ZAxisTransform> datatrans = getZAxisTransform();
    if ( datatrans ) //TODO check for allready transformed data.
    {
        attribpos.z_ = datatrans->transformBack( pos );
	if ( !attribpos.isDefined() )
	    return;
    }

    val = getValue( 0, attribpos ); // TODO: adapt to multi-attrib
}


TrcKeyZSampling VolumeDisplay::getTrcKeyZSampling( bool manippos,
						   bool displayspace,
						   int attrib ) const
{
    TrcKeyZSampling res = texturecs_;
    if ( manippos )
    {
	Coord3 center = boxdragger_->center();
	Coord3 width = boxdragger_->width();

        res.hsamp_.start_ = BinID( mNINT32( center.x_ - width.x_/2 ),
                                   mNINT32( center.y_ - width.y_/2 ) );

        res.hsamp_.stop_ = BinID( mNINT32( center.x_ + width.x_/2 ),
                                  mNINT32( center.y_ + width.y_/2 ) );

	res.hsamp_.step_ = BinID( SI().inlStep(), SI().crlStep() );

        res.zsamp_.start_ = (float) ( center.z_ - width.z_/2 );
        res.zsamp_.stop_ = (float) ( center.z_ + width.z_/2 );
	res.zsamp_.step_ = SI().zStep();

	SI().snap( res.hsamp_.start_ );
	SI().snap( res.hsamp_.stop_ );

	if ( !datatransform_ )
	{
	    SI().snapZ( res.zsamp_.start_ );
	    SI().snapZ( res.zsamp_.stop_ );
	}
	else
	{
	    ZSampling zrg = datatransform_->getZInterval( false );
	    if ( scene_ )
		zrg.step_ = scene_->getTrcKeyZSampling().zsamp_.step_;

	    zrg.snap( res.zsamp_.start_ );
	    zrg.snap( res.zsamp_.stop_ );
	}
    }

    const bool alreadytf = alreadyTransformed( attrib );
    if ( alreadytf )
    {
	if ( scene_ )
	    res.zsamp_.step_ = scene_->getTrcKeyZSampling().zsamp_.step_;
	else if ( datatransform_ )
	    res.zsamp_.step_ = datatransform_->getZInterval( false ).step_;
	return res;
    }

    if ( datatransform_ )
    {
	if ( !displayspace )
	{
	    res.zsamp_ = datatransform_->getZInterval( true );
	}
	else
	{
	    if ( scene_ )
		res.zsamp_.step_ = scene_->getTrcKeyZSampling().zsamp_.step_;
	    else
		res.zsamp_.step_ =
			datatransform_->getZInterval( false ).step_;
	}
    }
    else
	res.zsamp_.step_ = SI().zRange(true).step_;

    return res;
}


bool VolumeDisplay::allowsPicks() const
{
    return !isVolRenShown();
}


SurveyObject* VolumeDisplay::duplicate( TaskRunner* taskr ) const
{
    auto* vd = new VolumeDisplay;

    TypeSet<VisID> children;
    vd->getChildren( children );
    for ( const auto& child : children )
	vd->removeChild( child );

    vd->setZAxisTransform( datatransform_.getNonConstPtr(), taskr );
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	const VisID sliceid = vd->addSlice( slices_[idx]->getDim() );
	mDynamicCastGet(visBase::OrthogonalSlice*,slice,
			visBase::DM().getObject(sliceid))
	slice->setSliceNr( slices_[idx]->getSliceNr() );
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	vd->addIsoSurface();
	vd->isosurfsettings_[idx] = isosurfsettings_[idx];
    }

    vd->allowShading( usesShading() );
    vd->showVolRen( isVolRenShown() );

    vd->setTrcKeyZSampling( getTrcKeyZSampling(false,true,0) );

    for ( int attrib=0; attrib<attribs_.size(); attrib++ )
    {
	while ( attrib >= vd->nrAttribs() )
	    vd->addAttrib();

	vd->setSelSpecs( attrib, *attribs_[attrib]->selspec_ );
	vd->setVolumeDataPack( attrib,
			     attribs_[attrib]->cache_.getNonConstPtr(), taskr );
	vd->setColTabMapperSetup( attrib, *getColTabMapperSetup( attrib ),
				  taskr );
	vd->setColTabSequence( attrib,
			*getChannels2RGBA()->getSequence(attrib), taskr );
    }

    return vd;
}


void VolumeDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	mDetachCB( eventcatcher_->eventhappened,
		   VolumeDisplay::updateMouseCursorCB );
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
	mAttachCB( eventcatcher_->eventhappened,
		   VolumeDisplay::updateMouseCursorCB );
    }
}


bool VolumeDisplay::isSelected() const
{
    return visBase::DM().selMan().selected().indexOf( id()) != -1;
}


void VolumeDisplay::updateMouseCursorCB( CallBacker* cb )
{
    if ( !isManipulatorShown() || !isOn() || isLocked() )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
	initAdaptiveMouseCursor( cb, id(),
		    boxdragger_->getPlaneTransDragKeys(false), mousecursor_ );
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
{
    mDynamicCastGet( const visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() )
    return !rgba2rgba;
}


void VolumeDisplay::setColTabSequence( int attrib, const ColTab::Sequence& seq,
					TaskRunner* )
{
    if ( getChannels2RGBA() )
    {
	getChannels2RGBA()->setSequence( attrib, seq );
	scalarfield_->makeColorTables( attrib );
	updateIsoSurfColor();
    }
}


const ColTab::Sequence* VolumeDisplay::getColTabSequence( int attrib ) const
{
    return getChannels2RGBA() ? getChannels2RGBA()->getSequence(attrib)
			      : nullptr;
}


void VolumeDisplay::setColTabMapperSetup( int attrib,
					  const ColTab::MapperSetup& ms,
					  TaskRunner* taskr )
{
    scalarfield_->setColTabMapperSetup( attrib, ms, taskr );
    updateIsoSurfColor();
}


const ColTab::MapperSetup* VolumeDisplay::getColTabMapperSetup( int attrib,
						    int /* version */ ) const
{
    return &scalarfield_->getColTabMapper( attrib ).setup_;
}


bool VolumeDisplay::turnOn( bool yn )
{
    onoffstatus_ = yn;
    updateAttribEnabling();

    return VisualObjectImpl::turnOn( isAnyAttribEnabled() && yn );
}


bool VolumeDisplay::isOn() const
{ return onoffstatus_; }


void VolumeDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    SurveyObject::fillPar( par );
    const TrcKeyZSampling cs = getTrcKeyZSampling(false,true,0);
    cs.fillPar( par );
}


bool VolumeDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar(par) ||
	 !SurveyObject::usePar(par) )
	return false;

    PtrMan<IOPar> texturepar = par.subselect( sKeyTexture() );
    if ( texturepar ) //old format (up to 4.0)
    {
	ColTab::MapperSetup mappersetup;
	ColTab::Sequence sequence;

	mappersetup.usePar(*texturepar);
	sequence.usePar(*texturepar );
	setColTabMapperSetup( 0, mappersetup, nullptr );
	setColTabSequence( 0, sequence, nullptr );
	if ( !(*attribs_[0]->selspec_)[0].usePar(par) )
	    return false;
    }

    VisID volid;
    if ( par.get(sKeyVolumeID(),volid) )
    {
	RefMan<visBase::DataObject> dataobj = visBase::DM().getObject( volid );
	if ( !dataobj )
	    return false;
/*
	mDynamicCastGet(visBase::VolrenDisplay*,vr,dataobj.ptr());
	if ( !vr ) return -1;

	{
	    if ( childIndex(volren_->osgNode())!=-1 )
		VisualObjectImpl::removeChild(volren_->osgNode());
	}

	volren_ = vr;
	addChild( volren_->osgNode() );
*/
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
	VisID sliceid;
	par.get( str, sliceid );
	RefMan<visBase::DataObject> dataobj = visBase::DM().getObject(sliceid);
	if ( !dataobj )
	    return false;

	mDynamicCastGet(visBase::OrthogonalSlice*,os,dataobj.ptr())
	if ( !os )
	    return false;

	mAttachCB( os->motion, VolumeDisplay::sliceMoving );
	slices_ += os;
	addChild( os->osgNode() );
	// set correct dimensions ...
	if ( os->name() == StringView(sKeyInline()) )
	    os->setDim( cInLine() );
	else if ( os->name() == StringView(sKeyCrossLine()) )
	    os->setDim( cCrossLine() );
	else if ( os->name() == StringView(sKeyTime()) )
	    os->setDim( cTimeSlice() );
    }

    TrcKeyZSampling cs;
    if ( cs.usePar(par) )
    {
	csfromsession_ = cs;
	setTrcKeyZSampling( cs );
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
		addIsoSurface( nullptr, false );
		isosurfsettings_[idx].isovalue_ = isovalue;
	    }

	    str = sKeyIsoOnStart(); str += idx;
	    bool status = true;
	    par.getYN( str, status );
	    isosurfaces_[idx]->turnOn( status );

	    str = sKeySurfMode(); str += idx;
	    int smode;
	    par.get( str, smode );
	    isosurfsettings_[idx].mode_ = sCast( char, smode );

	    str = sKeySeedsAboveIsov(); str += idx;
	    int aboveisov;
	    par.get( str, aboveisov );
	    isosurfsettings_[idx].seedsaboveisoval_ = sCast( char, aboveisov );

	    str = sKeySeedsMid(); str += idx;
	    MultiID mid;
	    par.get( str, mid );
	    isosurfsettings_[idx].seedsid_ = mid;
	}
    }

    return true;
}


bool VolumeDisplay::writeVolume( int attrib, const char* filename ) const
{
    if ( !attribs_.validIdx(attrib) || !scalarfield_ )
	return false;

    od_ostream strm( filename );
    if ( !strm.isOK() )
    {
	errmsg_ = "Cannot open file";
	return false;
    }

    errmsg_ = scalarfield_->writeVolumeFile( attrib, strm );
    return errmsg_.str();
}


visBase::OrthogonalSlice* VolumeDisplay::getSelectedSlice() const
{
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->isSelected() )
	    return const_cast<visBase::OrthogonalSlice*>( slices_[idx] );
    }

    return nullptr;
}


TrcKeyZSampling
	VolumeDisplay::sliceSampling(visBase::OrthogonalSlice* slice) const
{
    TrcKeyZSampling cs( false );
    if ( !slice )
	return cs;

    cs = getTrcKeyZSampling(false,true,0);
    float pos = slicePosition( slice );
    if ( slice->getDim() == cTimeSlice() )
	cs.zsamp_.limitTo( Interval<float>( pos, pos ) );
    else if ( slice->getDim() == cCrossLine() )
	cs.hsamp_.setCrlRange( Interval<int>( mNINT32(pos), mNINT32(pos) ) );
    else if ( slice->getDim() == cInLine() )
	cs.hsamp_.setInlRange( Interval<int>( mNINT32(pos), mNINT32(pos) ) );
    return cs;
}


void VolumeDisplay::setDisplayTransformation( const mVisTrans* t )
{
    const bool voldisplayed = scalarfield_->isOn();
    TrcKeyZSampling cs = getTrcKeyZSampling( false, true, 0 );

    displaytrans_ = t;
    boxdragger_->setDisplayTransformation( t );

    setTrcKeyZSampling( cs );
    scalarfield_->turnOn( voldisplayed );
}


bool VolumeDisplay::canEnableTextureInterpolation() const
{ return true; }


bool VolumeDisplay::textureInterpolationEnabled() const
{ return scalarfield_->textureInterpolationEnabled(); }


void VolumeDisplay::enableTextureInterpolation( bool yn )
{ scalarfield_->enableTextureInterpolation( yn ); }


bool VolumeDisplay::canUseVolRenShading()
{
    return visBase::VolumeRenderScalarField::isShadingSupported();
}


void VolumeDisplay::allowShading( bool yn )
{
    scalarfield_->allowShading( yn );
}


bool VolumeDisplay::usesShading() const
{ return scalarfield_->usesShading(); }


bool VolumeDisplay::canAddAttrib( int nr ) const
{
    if ( !getChannels2RGBA() )
	return false;

    mDynamicCastGet( const visBase::ColTabTextureChannel2RGBA*, coltabtc2rgba,
		     getChannels2RGBA() )
    if ( coltabtc2rgba )	// Multiple coltab textures not yet supported
	return nrAttribs()+nr <= 1;

    return nrAttribs()+nr <= getChannels2RGBA()->maxNrChannels();
}


bool VolumeDisplay::canRemoveAttrib() const
{
    if ( !getChannels2RGBA() )
	return false;

    return nrAttribs() > getChannels2RGBA()->minNrChannels();
}


int VolumeDisplay::nrAttribs() const
{ return attribs_.size(); }


bool VolumeDisplay::addAttrib()
{
    if ( !canAddAttrib() )
	return false;

    const int attrib = attribs_.size();
    attribs_ += new AttribData();
    getChannels2RGBA()->notifyChannelInsert( attrib );
    enableAttrib( attrib, isAttribEnabled(attrib) );
    return true;
}


bool VolumeDisplay::removeAttrib( int attrib )
{
    if ( !canRemoveAttrib() || !attribs_.validIdx(attrib) )
	return false;

    getChannels2RGBA()->notifyChannelRemove( attrib );
    delete attribs_.removeSingle( attrib );
    updateAttribEnabling();
    turnOn( onoffstatus_ );
    return true;
}


void VolumeDisplay::updateAttribEnabling()
{
    bool showvolren = false;

    if ( getChannels2RGBA() )
    {
	for ( int idx=0; idx<attribs_.size(); idx++ )
	{
	    bool yn = getChannels2RGBA()->isEnabled( idx );

	    if ( !attribs_[idx]->cache_ )
		yn = false;

	    scalarfield_->enableAttrib( idx, yn );
	    showvolren = showvolren || yn;
	}
    }

    scalarfield_->turnOn( onoffstatus_ && showvolren );
}


void VolumeDisplay::enableAttrib( int attrib, bool yn )
{
    if ( getChannels2RGBA() )
	getChannels2RGBA()->setEnabled( attrib, yn );

    updateAttribEnabling();
    turnOn( onoffstatus_ );
}


bool VolumeDisplay::isAttribEnabled( int attrib ) const
{
    return getChannels2RGBA() ? getChannels2RGBA()->isEnabled(attrib) : false;
}


bool VolumeDisplay::swapAttribs( int attrib0, int attrib1 )
{
    if ( !attribs_.validIdx(attrib0) || !attribs_.validIdx(attrib1) ||
	 attrib0==attrib1 || !getChannels2RGBA() )
	return false;

    getChannels2RGBA()->swapChannels( attrib0, attrib1 );
    attribs_.swap( attrib0, attrib1 );

    scalarfield_->swapAttribs( attrib0, attrib1 );

    updateAttribEnabling();
    return true;
}


void VolumeDisplay::setAttribTransparency( int attrib, unsigned char trans )
{
    mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*,
		     coltab2rgba, getChannels2RGBA() )
    if ( coltab2rgba )
	coltab2rgba->setTransparency( attrib, trans );

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() )
    if ( rgba2rgba )
	rgba2rgba->setTransparency( trans );

    scalarfield_->setAttribTransparency( attrib, trans );
}


unsigned char VolumeDisplay::getAttribTransparency( int attrib ) const
{
    mDynamicCastGet( const visBase::ColTabTextureChannel2RGBA*,
		     coltab2rgba, getChannels2RGBA() )
    if ( coltab2rgba )
	return coltab2rgba->getTransparency( attrib );

    mDynamicCastGet( const visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() )
    if ( rgba2rgba )
	return rgba2rgba->getTransparency();

    return 0;
}


bool VolumeDisplay::setChannels2RGBA( visBase::TextureChannel2RGBA* tc2rgba )
{
    if ( scalarfield_ )
	scalarfield_->setChannels2RGBA( tc2rgba );

    return scalarfield_;
}


visBase::TextureChannel2RGBA* VolumeDisplay::getChannels2RGBA()
{
    return scalarfield_ ? scalarfield_->getChannels2RGBA() : nullptr;
}


const visBase::TextureChannel2RGBA* VolumeDisplay::getChannels2RGBA() const
{
    return scalarfield_ ? scalarfield_->getChannels2RGBA() : nullptr;
}

} // namespace visSurvey
