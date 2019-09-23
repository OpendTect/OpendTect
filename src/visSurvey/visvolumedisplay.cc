/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/


#include "visvolumedisplay.h"

#include "visboxdragger.h"
#include "visevent.h"
#include "vismarchingcubessurface.h"
#include "vismaterial.h"
#include "visselman.h"
#include "visrgbatexturechannel2rgba.h"
#include "vistransform.h"
#include "visvolorthoslice.h"
#include "visvolrenscalarfield.h"

#include "array3dfloodfill.h"
#include "arrayndimpl.h"
#include "attribsel.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "marchingcubes.h"
#include "mousecursor.h"
#include "picksetmanager.h"
#include "od_ostream.h"
#include "seisdatapack.h"
#include "settings.h"
#include "survinfo.h"
#include "uistrings.h"
#include "zaxistransform.h"
#include "zaxistransformer.h"

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
    cs.hsamp_.start_.inl() =
	(5*csin.hsamp_.start_.inl()+3*csin.hsamp_.stop_.inl())/8;
    cs.hsamp_.start_.crl() =
	(5*csin.hsamp_.start_.crl()+3*csin.hsamp_.stop_.crl())/8;
    cs.hsamp_.stop_.inl() =
	(3*csin.hsamp_.start_.inl()+5*csin.hsamp_.stop_.inl())/8;
    cs.hsamp_.stop_.crl() =
	(3*csin.hsamp_.start_.crl()+5*csin.hsamp_.stop_.crl())/8;
    cs.zsamp_.start = ( 5*csin.zsamp_.start + 3*csin.zsamp_.stop ) / 8.f;
    cs.zsamp_.stop = ( 3*csin.zsamp_.start + 5*csin.zsamp_.stop ) / 8.f;

    SI().snap( cs.hsamp_.start_ );
    SI().snap( cs.hsamp_.stop_ );
    cs.zsamp_.start = csin.zsamp_.snap( cs.zsamp_.start );
    cs.zsamp_.stop = csin.zsamp_.snap( cs.zsamp_.stop );
    cs.zsamp_.limitTo( csin.zsamp_ );
    return cs;
}


VolumeDisplay::AttribData::AttribData()
    : as_(new Attrib::SelSpecList(1,Attrib::SelSpec()))
    , cache_(0)
{}


VolumeDisplay::AttribData::~AttribData()
{
    delete as_;
}


VolumeDisplay::VolumeDisplay()
    : VisualObjectImpl(true)
    , boxdragger_(visBase::BoxDragger::create())
    , isinited_(false)
    , scalarfield_(0)
//    , volren_(0)
    , boxMoving(this)
    , datatransform_(0)
    , datatransformer_(0)
    , csfromsession_(true)
    , eventcatcher_(0)
    , onoffstatus_(true)
    , ismanip_(false)
    , mousecursor_( *new MouseCursor )
{
    addChild( boxdragger_->osgNode() );

    boxdragger_->ref();
    boxdragger_->setBoxTransparency( mDefaultBoxTransparency );
    mAttachCB( boxdragger_->started, VolumeDisplay::draggerStartCB );
    mAttachCB( boxdragger_->motion, VolumeDisplay::draggerMoveCB );
    mAttachCB( boxdragger_->finished, VolumeDisplay::draggerFinishCB );

    updateRanges( true, true );

    scalarfield_ = visBase::VolumeRenderScalarField::create();
    scalarfield_->ref();
    setChannels2RGBA( visBase::ColTabTextureChannel2RGBA::create() );
    addAttrib();

    addChild( scalarfield_->osgNode() );

    getMaterial()->setColor( Color::White() );
    getMaterial()->setAmbience( 0.3 );
    getMaterial()->setDiffIntensity( 0.8 );
    mAttachCB( getMaterial()->change, VolumeDisplay::materialChange );
    scalarfield_->setMaterial( getMaterial() );

    const TrcKeyZSampling sics( OD::UsrWork );
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
}


VolumeDisplay::~VolumeDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );

    deepErase( attribs_ );

    TypeSet<int> children;
    getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	removeChild( children[idx] );

    boxdragger_->unRef();
    scalarfield_->unRef();

    setZAxisTransform( 0,0 );
    delete &mousecursor_;
}


void VolumeDisplay::setMaterial( visBase::Material* nm )
{
    if ( material_ )
	mDetachCB( material_->change, VolumeDisplay::materialChange );
    visBase::VisualObjectImpl::setMaterial( nm );
    if ( nm )
	mAttachCB( getMaterial()->change, VolumeDisplay::materialChange );
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
    const ColTab::Sequence& colseq = getColTabSequence( 0 );
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( mIsUdf( isosurfsettings_[idx].isovalue_) )
	    continue;

	const float val = isosurfsettings_[idx].isovalue_;
	Color col;
	if ( mIsUdf(val) )
	    col = colseq.undefColor();
	else
	{
	    // TODO: adapt to multi-attrib
	    const float mappedval =
		scalarfield_->getColTabMapper(0).seqPosition( val );
	    col = colseq.color( mappedval );
	}

	isosurfaces_[idx]->getMaterial()->setColor( col );
    }
}


bool VolumeDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tskr )
{
    if ( zat == datatransform_ )
	return true;

    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	{
	    mDetachCB( *datatransform_->changeNotifier(),
		       VolumeDisplay::dataTransformCB );
	}
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
	{
	    mAttachCB( *datatransform_->changeNotifier(),
		       VolumeDisplay::dataTransformCB );
	}
    }

    return true;
}


const ZAxisTransform* VolumeDisplay::getZAxisTransform() const
{ return datatransform_; }


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
	    setDataVolume( attrib, attribs_[attrib]->cache_, 0 );
    }
}


void VolumeDisplay::setScene( Scene* sc )
{
    SurveyObject::setScene( sc );
    if ( sc ) updateRanges( false, false );
}


void VolumeDisplay::updateRanges( bool updateic, bool updatez )
{
    if ( !datatransform_ ) return;

    const TrcKeyZSampling defcs( true );
    if ( csfromsession_ != defcs )
	setTrcKeyZSampling( csfromsession_ );
    else
    {
	const TrcKeyZSampling& csin = scene_ ? scene_->getTrcKeyZSampling()
					  : getTrcKeyZSampling( 0 );
	TrcKeyZSampling cs = getInitTrcKeyZSampling( csin );
	setTrcKeyZSampling( cs );
    }
}


void VolumeDisplay::getChildren( TypeSet<int>&res ) const
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

    const Coord3 center( (cs.hsamp_.start_.inl() + cs.hsamp_.stop_.inl())/2.0,
			 (cs.hsamp_.start_.crl() + cs.hsamp_.stop_.crl())/2.0,
			 (cs.zsamp_.start + cs.zsamp_.stop)/2.0 );

    const Coord3 width( cs.hsamp_.stop_.inl() - cs.hsamp_.start_.inl(),
			cs.hsamp_.stop_.crl() - cs.hsamp_.start_.crl(),
			cs.zsamp_.stop - cs.zsamp_.start );

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
}


void VolumeDisplay::draggerFinishCB( CallBacker* )
{
    boxdragger_->setBoxTransparency( mDefaultBoxTransparency );
    boxdragger_->showScaleTabs( true );
}


int VolumeDisplay::addSlice( int dim )
{
    visBase::OrthogonalSlice* slice = visBase::OrthogonalSlice::create();
    slice->ref();
    slice->setMaterial(0);
    slice->setDim( dim );
    mAttachCB( slice->motion, VolumeDisplay::sliceMoving );
    slices_ += slice;
    slice->setUiName( dim==cTimeSlice() ? uiStrings::sTime() :
		     (dim==cCrossLine() ? uiStrings::sCrossline()
					: uiStrings::sInline()) );

    addChild( slice->osgNode() );
    const TrcKeyZSampling cs = getTrcKeyZSampling( 0 );
    const Interval<float> defintv(-0.5,0.5);
    slice->setSpaceLimits( defintv, defintv, defintv );
    // TODO: adapt to multi-attrib
    if ( attribs_[0]->cache_ )
    {
	const Array3D<float>& arr = attribs_[0]->cache_->data();
	slice->setVolumeDataSize( arr.getSize(2), arr.getSize(1),
				  arr.getSize(0) );
    }

    return slice->id();
}


void VolumeDisplay::removeChild( int displayid )
{
/*
    if ( volren_ && displayid==volren_->id() )
    {
	VisualObjectImpl::removeChild( volren_->osgNode() );
	volren_->unRef();
	volren_ = 0;
	return;
    }
*/

    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild( slices_[idx]->osgNode() );
	    mDetachCB( slices_[idx]->motion, VolumeDisplay::sliceMoving );
	    slices_.removeSingle(idx,false)->unRef();
	    return;
	}
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( isosurfaces_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild(
		    isosurfaces_[idx]->osgNode() );

	    isosurfaces_.removeSingle(idx,false)->unRef();
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
	volren_->ref();
	volren_->setMaterial(0);
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
    return attribs_[0]->cache_ ? getColTabMapper(0).getRange().center()
			       : mUdf(float);
}


int VolumeDisplay::addIsoSurface( TaskRunner* tskr, bool updateisosurface )
{
    mVisMCSurf* isosurface = mVisMCSurf::create();
    isosurface->ref();
    isosurface->setRightHandSystem( righthandsystem_ );
    mDeclareAndTryAlloc( RefMan<MarchingCubesSurface>, surface,
			 MarchingCubesSurface() );
    isosurface->setSurface( *surface, tskr );
    isosurface->setUiName( tr("Iso Surface") );

    isosurfaces_ += isosurface;
    IsosurfaceSetting setting;
    setting.isovalue_ = defaultIsoValue();
    isosurfsettings_ += setting;

    if ( updateisosurface )
	updateIsoSurface( isosurfaces_.size()-1, tskr );

    //add before the volume transform
    addChild( isosurface->osgNode() );
    materialChange( 0 ); //updates new surface's material
    return isosurface->id();
}


int VolumeDisplay::volRenID() const
{
    return -1;
//    return volren_ ? volren_->id() : -1;
}


#define mSetVolumeTransform( name, center, width, cs, extrasteps ) \
\
    const Coord3 center( (cs.hsamp_.start_.inl() + cs.hsamp_.stop_.inl())/2.0, \
			 (cs.hsamp_.start_.crl() + cs.hsamp_.stop_.crl())/2.0, \
			 (cs.zsamp_.start + cs.zsamp_.stop)/2.0 ); \
\
    const Coord3 width( cs.hsamp_.stop_.inl() - cs.hsamp_.start_.inl(), \
			cs.hsamp_.stop_.crl() - cs.hsamp_.start_.crl(), \
			cs.zsamp_.stop - cs.zsamp_.start ); \
{ \
    const Coord3 step( cs.hsamp_.step_.inl(), cs.hsamp_.step_.crl(), \
		       cs.zsamp_.step ); \
\
    Coord3 trans( center ); \
    mVisTrans::transform( displaytrans_, trans ); \
    Coord3 scale( width + extrasteps*step ); \
    mVisTrans::transformSize( displaytrans_, scale ); \
    trans += 0.5 * scale; \
    scale = Coord3( scale.z_, -scale.y_, -scale.x_ ); \
    scalarfield_->set##name##Transform( trans, Coord3(0,1,0), M_PI_2, scale ); \
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
    mSetVolumeTransform( ROIVolume, center, width, cs, 0 );

    if ( dragmode )
	return;

    TrcKeyZSampling texcs = scalarfield_->getMultiAttribTrcKeyZSampling();
    if ( !texcs.isDefined() || texcs.isEmpty() )
	texcs = cs;

    mSetVolumeTransform( TexVolume, texcenter, texwidth, texcs, 1 );

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

    const StepInterval<float> inlrg( mCast(float,limcs.hsamp_.start_.inl()),
				     mCast(float,limcs.hsamp_.stop_.inl()),
				     mCast(float,limcs.hsamp_.step_.inl()) );
    const StepInterval<float> crlrg( mCast(float,limcs.hsamp_.start_.crl()),
				     mCast(float,limcs.hsamp_.stop_.crl()),
				     mCast(float,limcs.hsamp_.step_.crl()) );

    boxdragger_->setSpaceLimits( inlrg, crlrg, limcs.zsamp_ );

    const int minvoxwidth = 1;
    boxdragger_->setWidthLimits(
	Interval<float>( mCast(float,minvoxwidth*limcs.hsamp_.step_.inl()),
			 mUdf(float) ),
	Interval<float>( mCast(float,minvoxwidth*limcs.hsamp_.step_.crl()),
			 mUdf(float) ),
	Interval<float>( minvoxwidth*limcs.zsamp_.step, mUdf(float) ) );

    boxdragger_->setDragCtrlSpacing( inlrg, crlrg, limcs.zsamp_ );
}


float VolumeDisplay::getValue( int attrib, const Coord3& pos ) const
{
    if ( !attribs_.validIdx(attrib) || !attribs_[attrib]->cache_ )
	return mUdf(float);

    const BinID bid( SI().transform(pos.getXY()) );
    const TrcKeyZSampling samp( attribs_[attrib]->cache_->subSel() );
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
				 TaskRunner* tskr )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 )
	return;

    isosurfsettings_[idx].isovalue_ = nv;
    updateIsoSurface( idx, tskr );
}


mVisMCSurf* VolumeDisplay::getIsoSurface( int idx )
{ return isosurfaces_.validIdx(idx) ? isosurfaces_[idx] : 0; }


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


DBKey  VolumeDisplay::getSeedsID( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return DBKey();

    return isosurfsettings_[idx].seedsid_;
}


void VolumeDisplay::setSeedsID( const mVisMCSurf* mcd, DBKey mid )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return;

    isosurfsettings_[idx].seedsid_ = mid;
}


bool VolumeDisplay::updateSeedBasedSurface( int idx, TaskRunner* tskr )
{
    // TODO: adapt to multi-attrib
    if ( idx<0 || idx>=isosurfaces_.size() || !attribs_[0]->cache_ ||
	 mIsUdf(isosurfsettings_[idx].isovalue_) ||
	 isosurfsettings_[idx].seedsid_.isInvalid() )
	return false;

    ConstRefMan<Pick::Set> seeds
		= Pick::SetMGR().fetch( isosurfsettings_[idx].seedsid_ );
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
    cs.normalise();
    Pick::SetIter psiter( *seeds );
    while ( psiter.next() )
    {
	const Pick::Location& seedloc = psiter.get();
	const BinID bid = seedloc.binID();
	const int i = cs.inlIdx( bid.inl() );
	const int j = cs.crlIdx( bid.crl() );
	const int k = cs.zIdx( seedloc.z() );
	ff.addSeed( i, j, k );
    }
    psiter.retire();

    if ( !ff.execute() )
	return false;

    if ( !isosurfsettings_[idx].seedsaboveisoval_ )
    {
	const float outsideval = -1;
	const float threshold = isosurfsettings_[idx].isovalue_;
	float* newdata = newarr.getData();
	if ( newdata )
	{
            for ( od_int64 idy=newarr.totalSize()-1; idy>=0; idy-- )
		newdata[idy] = mIsUdf(newdata[idy]) ? outsideval
						    : threshold-newdata[idy];
	}
        else if ( newarr.getStorage() )
        {
            ValueSeries<float>* newstor = newarr.getStorage();
            for ( od_int64 idy=newarr.totalSize()-1; idy>=0; idy-- )
            {
		newstor->setValue(idy, mIsUdf(newstor->value(idy))
                        ? outsideval : threshold-newstor->value(idy) );
            }

        }
	else
	{
	    for ( int id0=0; id0<newarr.getSize(0); id0++ )
            {
		for ( int idy=0; idy<newarr.getSize(1); idy++ )
                {
		    for ( int idz=0; idz<newarr.getSize(2); idz++ )
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
						    threshold, tskr );
    return true;
}


int VolumeDisplay::getIsoSurfaceIdx( const mVisMCSurf* mcd ) const
{
    return isosurfaces_.indexOf( mcd );
}


void VolumeDisplay::updateIsoSurface( int idx, TaskRunner* tskr )
{
    // TODO:: adapt to multi-attrib
    const RegularSeisDataPack* cache = attribs_[0]->cache_;
    if ( !cache || cache->isEmpty() ||
	 mIsUdf(isosurfsettings_[idx].isovalue_) )
	isosurfaces_[idx]->getSurface()->removeAll();
    else
    {
	const TrcKeyZSampling samp( cache->subSel() );
	isosurfaces_[idx]->getSurface()->removeAll();
	isosurfaces_[idx]->setBoxBoundary(
		mCast(float,samp.hsamp_.inlRange().stop),
		mCast(float,samp.hsamp_.crlRange().stop),
		samp.zsamp_.stop );

	const SamplingData<float> inlsampling(
		mCast(float,samp.hsamp_.inlRange().start),
		mCast(float,samp.hsamp_.inlRange().step) );

	const SamplingData<float> crlsampling(
		mCast(float,samp.hsamp_.crlRange().start),
		mCast(float,samp.hsamp_.crlRange().step) );

	SamplingData<float> zsampling ( samp.zsamp_.start, samp.zsamp_.step );
	isosurfaces_[idx]->setScales( inlsampling, crlsampling, zsampling );

	if ( isosurfsettings_[idx].mode_ )
	{
	    const Array3D<float>& arr = cache->data();
	    if ( !arr.isOK() )  return;

	    const od_int64 size = arr.totalSize();
	    PtrMan< Array3D<float> > newarr =
		new Array3DImpl<float>(arr.info());
	    if ( !newarr->isOK() )
		return;

	    const float threshold = isosurfsettings_[idx].isovalue_;
	    const float* data = arr.getData();
	    if ( data && newarr->getData() )
	    {
		float* newdata = newarr->getData();
		for ( od_int64 idy=0; idy<size; idy++ )
		    newdata[idy] = threshold - data[idy];
	    }
	    else if ( arr.getStorage() && newarr->getStorage() )
	    {
		ValueSeries<float>* newstor = newarr->getStorage();
		const ValueSeries<float>* arrstor = arr.getStorage();

		for ( od_int64 idy=0; idy<size; idy++ )
		    newstor->setValue( idy, threshold - arrstor->value(idy) );
	    }
	    else
	    {
		for ( int id0=0; id0<arr.getSize(0); id0++ )
		{
		    for ( int idy=0; idy<arr.getSize(1); idy++ )
		    {
			for ( int idz=0; idz<arr.getSize(2); idz++ )
			{
			    newarr->set( id0, idy, idz,
				    threshold - arr.get(id0,idy,idz) );
			}
		    }
		}
	    }

	    isosurfaces_[idx]->getSurface()->setVolumeData(0,0,0,*newarr,0,
									tskr);
	}
	else
	{
	    if ( !updateSeedBasedSurface( idx, tskr ) )
		return;
	}

    }

    updateIsoSurfColor();
    isosurfaces_[idx]->touch( false, tskr );
}


BufferString VolumeDisplay::getManipulationString() const
{
    BufferString str;
    getObjectInfo( str );
    return str;
}


void VolumeDisplay::getObjectInfoText( uiString& info, bool compact ) const
{
    BufferString formatstr = "%1-%2 / %3-%4 / %5-%6";
    if ( !compact )
    {
	formatstr = "Inl: %1-%2, Crl: %3-%4, %7: %5-%6";
    }

    TrcKeyZSampling cs = getTrcKeyZSampling( true, true, 0 );

    const int userfactor = scene_
	? scene_->zDomainInfo().userFactor()
	: 1;

    info = toUiString( formatstr.buf() )
	.arg( cs.hsamp_.start_.inl() )
	.arg( cs.hsamp_.stop_.inl() )
	.arg( cs.hsamp_.start_.crl() )
	.arg( cs.hsamp_.stop_.crl() )
	.arg( mNINT32(cs.zsamp_.start*userfactor) )
	.arg( mNINT32(cs.zsamp_.stop*userfactor) );

    if ( !compact )
	info.arg( scene_->zDomainInfo().userName() );
}


void VolumeDisplay::getObjectInfo( BufferString& info ) const
{
    uiString uistring;
    getObjectInfoText( uistring, false );
    info = toString( uistring );
}


void VolumeDisplay::getTreeObjectInfo( uiString& info ) const
{
    getObjectInfoText( info, true );
}


void VolumeDisplay::sliceMoving( CallBacker* cb )
{
    mDynamicCastGet( visBase::OrthogonalSlice*, slice, cb );
    if ( !slice )
	return;

    slicename_ = slice->name();
    sliceposition_ = slicePosition( slice );
}


float VolumeDisplay::slicePosition( visBase::OrthogonalSlice* slice ) const
{
    if ( !slice ) return 0;
    const int dim = slice->getDim();
    float slicepos = slice->getPosition();
//    slicepos *= (float) -voltrans_->getScale()[dim];

    float pos;
    if ( dim == 2 )
    {
//	slicepos += (float) voltrans_->getTranslation()[0];
	pos = mCast( float, SI().inlRange(OD::UsrWork).snap(slicepos) );
    }
    else if ( dim == 1 )
    {
//	slicepos += (float) voltrans_->getTranslation()[1];
	pos = mCast( float, SI().crlRange(OD::UsrWork).snap(slicepos) );
    }
    else
    {
//	slicepos += (float) voltrans_->getTranslation()[2];
	pos = slicepos;
    }

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
	pos = (float)cs.hsamp_.inlRange().start;
    else if ( dim == 1 )
	pos = (float)cs.hsamp_.crlRange().start;
    else
	pos = (float)cs.zsamp_.start;

//    pos -= (float) voltrans_->getTranslation()[2-dim];
//    pos /= (float) -voltrans_->getScale()[dim];

    float slicenr =  nrslices ? (pos-rg.start)*nrslices/rg.width() : 0;
    float draggerpos = slicenr /(nrslices-1) *rg.width() + rg.start;
    Coord3 center(0,0,0);
    center[dim] = draggerpos;
    slice->setCenter( center, false );
    slice->motion.trigger();
}


SurveyObject::AttribFormat VolumeDisplay::getAttributeFormat( int ) const
{ return visSurvey::SurveyObject::Cube; }


const Attrib::SelSpecList* VolumeDisplay::getSelSpecs( int attrib ) const
{
    return attribs_.validIdx(attrib) ? attribs_[attrib]->as_ : 0;
}


const Attrib::SelSpec* VolumeDisplay::getSelSpec( int attrib, int version )const
{
    return attribs_.validIdx(attrib) ? &(*attribs_[attrib]->as_)[version] : 0;
}


void VolumeDisplay::setSelSpecs( int attrib, const Attrib::SelSpecList& as)
{
    SurveyObject::setSelSpecs( attrib, as );

    if ( !attribs_.validIdx(attrib) || *attribs_[attrib]->as_==as )
	return;

    *attribs_[attrib]->as_ = as;
    attribs_[attrib]->cache_ = 0;

    TrcKeyZSampling emptytkzs( false );
    emptytkzs.hsamp_.setIs3D();
    scalarfield_->setScalarField( attrib, 0, true, emptytkzs, 0 );

    updateAttribEnabling();

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );
}


TrcKeyZSampling VolumeDisplay::getTrcKeyZSampling( int attrib ) const
{ return getTrcKeyZSampling(true,false,attrib); }


bool VolumeDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				   TaskRunner* tskr )
{
    if ( !attribs_.validIdx(attrib) )
	return false;

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    auto regsdp = dpm.get<RegularSeisDataPack>(dpid);

    const bool res = setDataVolume( attrib, regsdp, tskr );
    if ( !res )
    {
	return false;
    }

    attribs_[attrib]->cache_ = regsdp;
    return true;
}


bool VolumeDisplay::setDataVolume( int attrib,
				   const RegularSeisDataPack* attribdata,
				   TaskRunner* tskr )
{
    if ( !attribs_.validIdx(attrib) || !attribdata || attribdata->isEmpty() )
	return false;

    TrcKeyZSampling tkzs( attribdata->subSel() );

    const Array3D<float>* usedarray = 0;
    bool arrayismine = true;
    if ( alreadyTransformed(attrib) || !datatransform_ )
	usedarray = &attribdata->data();
    else
    {
	if ( !datatransformer_ )
	    mTryAlloc( datatransformer_,ZAxisTransformer(*datatransform_,true));

//	datatransformer_->setInterpolate( !isClassification(attrib) );
	datatransformer_->setInterpolate( true );
	datatransformer_->setInput( attribdata->data(), tkzs );
	tkzs.zsamp_ = getTrcKeyZSampling(true,true,0).zsamp_;
	datatransformer_->setOutputRange( tkzs );

	if ( !TaskRunner::execute( tskr, *datatransformer_ ) )
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

    tkzs.hsamp_.setIs3D();
    scalarfield_->setScalarField( attrib, usedarray, !arrayismine, tkzs, tskr );

    setTrcKeyZSampling( getTrcKeyZSampling(true,true,0) );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setVolumeDataSize( usedarray->getSize(2),
					 usedarray->getSize(1),
					 usedarray->getSize(0) );

    if ( attribs_[attrib]->cache_ != attribdata )
    {
	attribs_[attrib]->cache_ = attribdata;
    }

    isinited_ = true;
    updateAttribEnabling();

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );

    return true;
}


const RegularSeisDataPack* VolumeDisplay::getCacheVolume( int attrib ) const
{
    return attribs_.validIdx(attrib) ? attribs_[attrib]->cache_ : 0;
}


DataPack::ID VolumeDisplay::getDataPackID( int attrib ) const
{
    DataPack::ID dpid = DataPack::cNoID();
    if ( attribs_.validIdx(attrib) && attribs_[attrib]->cache_ )
	dpid = attribs_[attrib]->cache_->id();

    return dpid;
}


void VolumeDisplay::getMousePosInfo( const visBase::EventInfo&,
				     Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "";
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

	res.zsamp_.start = (float) ( center.z_ - width.z_/2 );
	res.zsamp_.stop = (float) ( center.z_ + width.z_/2 );
	res.zsamp_.step = SI().zStep();

	SI().snap( res.hsamp_.start_ );
	SI().snap( res.hsamp_.stop_ );

	if ( !datatransform_ )
	{
	    SI().snapZ( res.zsamp_.start );
	    SI().snapZ( res.zsamp_.stop );
	}
	else
	{
	    StepInterval<float> zrg = datatransform_->getZInterval(false);
	    if ( scene_ )
		zrg.step = scene_->getTrcKeyZSampling().zsamp_.step;
	    else
		zrg.step = datatransform_->getGoodZStep();

	    zrg.snap( res.zsamp_.start );
	    zrg.snap( res.zsamp_.stop );
	}
    }

    const bool alreadytf = alreadyTransformed( attrib );
    if ( alreadytf )
    {
	if ( scene_ )
	    res.zsamp_.step = scene_->getTrcKeyZSampling().zsamp_.step;
	else if ( datatransform_ )
	    res.zsamp_.step = datatransform_->getGoodZStep();
	return res;
    }

    if ( datatransform_ )
    {
	if ( !displayspace )
	{
	    res.zsamp_.setFrom( datatransform_->getZInterval(true) );
	    res.zsamp_.step = SI().zStep(OD::UsrWork);
	}
	else
	{
	    if ( scene_ )
		res.zsamp_.step = scene_->getTrcKeyZSampling().zsamp_.step;
	    else
		res.zsamp_.step = datatransform_->getGoodZStep();
	}
    }
    else
	res.zsamp_.step = SI().zStep(OD::UsrWork);

    return res;
}


bool VolumeDisplay::allowsPicks() const
{
    return !isVolRenShown();
}


visSurvey::SurveyObject* VolumeDisplay::duplicate( TaskRunner* tskr ) const
{
    VolumeDisplay* vd = new VolumeDisplay;

    TypeSet<int> children;
    vd->getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	vd->removeChild( children[idx] );

    vd->setZAxisTransform( const_cast<ZAxisTransform*>(datatransform_), tskr );
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	const int sliceid = vd->addSlice( slices_[idx]->getDim() );
	mDynamicCastGet(visBase::OrthogonalSlice*,slice,
			visBase::DM().getObject(sliceid));
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

	vd->setSelSpecs( attrib, *attribs_[attrib]->as_ );
	vd->setDataVolume( attrib, attribs_[attrib]->cache_, tskr );
	vd->setColTabMapper( attrib, scalarfield_->getColTabMapper(attrib),
			     tskr );
	vd->setColTabSequence( attrib,
			       getChannels2RGBA()->getSequence(attrib), tskr );
    }

    return vd;
}


void VolumeDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	mDetachCB( eventcatcher_->eventhappened,
		   VolumeDisplay::updateMouseCursorCB );
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
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
    seedsid_ = DBKey();
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
		     rgba2rgba, getChannels2RGBA() );
    return !rgba2rgba;
}


void VolumeDisplay::setColTabSequence( int attrib, const ColTab::Sequence& seq,
					TaskRunner* tskr )
{
    if ( getChannels2RGBA() )
    {
	getChannels2RGBA()->setSequence( attrib, seq );
	scalarfield_->makeColorTables( attrib );
	updateIsoSurfColor();
    }
}


const ColTab::Sequence& VolumeDisplay::getColTabSequence( int attrib ) const
{
    return getChannels2RGBA() ? getChannels2RGBA()->getSequence(attrib)
	 : SurveyObject::getColTabSequence(attrib);
}


void VolumeDisplay::setColTabMapper( int attrib, const ColTab::Mapper& mpr,
				      TaskRunner* tskr )
{
    scalarfield_->setColTabMapper( attrib, mpr, tskr );
    updateIsoSurfColor();
}


const ColTab::Mapper& VolumeDisplay::getColTabMapper( int attrib ) const
{
    return scalarfield_->getColTabMapper(attrib);
}


bool VolumeDisplay::turnOn( bool yn )
{
    onoffstatus_ = yn;
    updateAttribEnabling();

    return VisualObjectImpl::turnOn( isAnyAttribEnabled() && yn );
}


bool VolumeDisplay::isOn() const
{
    return onoffstatus_;
}


void VolumeDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );
    const TrcKeyZSampling cs = getTrcKeyZSampling(false,true,0);
    cs.fillPar( par );
}


bool VolumeDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	return false;

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
	if ( !dataobj )
	    return 0;
	mDynamicCastGet(visBase::OrthogonalSlice*,os,dataobj.ptr())
	if ( !os )
	    return -1;

	os->ref();
	mAttachCB( os->motion, VolumeDisplay::sliceMoving );
	slices_ += os;
	addChild( os->osgNode() );
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
	    isosurfsettings_[idx].mode_ = mCast( char, smode );

	    str = sKeySeedsAboveIsov(); str += idx;
	    int aboveisov;
	    par.get( str, aboveisov );
	    isosurfsettings_[idx].seedsaboveisoval_ = mCast( char, aboveisov );

	    str = sKeySeedsMid(); str += idx;
	    DBKey mid;
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
	errmsg_ = uiStrings::phrCannotOpenForWrite( filename );
	return false;
    }

    errmsg_ = scalarfield_->writeVolumeFile( attrib, strm );
    return !errmsg_.isEmpty();
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


TrcKeyZSampling
	VolumeDisplay::sliceSampling( visBase::OrthogonalSlice* slice ) const
{
    TrcKeyZSampling cs(false);
    if ( !slice ) return cs;
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
		     getChannels2RGBA() );
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
		     coltab2rgba, getChannels2RGBA() );
    if ( coltab2rgba )
	coltab2rgba->setTransparency( attrib, trans );

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() );
    if ( rgba2rgba )
	rgba2rgba->setTransparency( trans );

    scalarfield_->setAttribTransparency( attrib, trans );
}


unsigned char VolumeDisplay::getAttribTransparency( int attrib ) const
{
    mDynamicCastGet( const visBase::ColTabTextureChannel2RGBA*,
		     coltab2rgba, getChannels2RGBA() );
    if ( coltab2rgba )
	return coltab2rgba->getTransparency( attrib );

    mDynamicCastGet( const visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() );
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
    return scalarfield_ ? scalarfield_->getChannels2RGBA() : 0;
}


const visBase::TextureChannel2RGBA* VolumeDisplay::getChannels2RGBA() const
{
    return scalarfield_ ? scalarfield_->getChannels2RGBA() : 0;
}


} // namespace visSurvey
