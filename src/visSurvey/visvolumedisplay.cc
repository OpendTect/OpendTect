/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visvolumedisplay.cc,v 1.85 2008-06-16 19:46:46 cvskris Exp $
________________________________________________________________________

-*/


#include "visvolumedisplay.h"

#include "visboxdragger.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarchingcubessurface.h"
#include "vismaterial.h"
#include "visselman.h"
#include "vistransform.h"
#include "visvolorthoslice.h"
#include "visvolrenscalarfield.h"
#include "visvolren.h"

#include "arrayndimpl.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "cubesampling.h"
#include "iopar.h"
#include "marchingcubes.h"
#include "sorting.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "zaxistransformer.h"

mCreateFactoryEntry( visSurvey::VolumeDisplay );

namespace visSurvey {

const char* VolumeDisplay::volumestr = "Cube ID";
const char* VolumeDisplay::volrenstr = "Volren";
const char* VolumeDisplay::inlinestr = "Inline";
const char* VolumeDisplay::crosslinestr = "Crossline";
const char* VolumeDisplay::timestr = "Time";

const char* VolumeDisplay::nrslicesstr = "Nr of slices";
const char* VolumeDisplay::slicestr = "SliceID ";
const char* VolumeDisplay::texturestr = "TextureID";


VolumeDisplay::VolumeDisplay()
    : VisualObjectImpl(true)
    , boxdragger_(visBase::BoxDragger::create())
    , scalarfield_(0)
    , volren_(0)
    , as_(*new Attrib::SelSpec)
    , cache_(0)
    , cacheid_(DataPack::cNoID)
    , slicemoving(this)
    , voltrans_(visBase::Transformation::create())
    , allowshading_(false)
    , datatransform_(0)
    , datatransformer_(0)
    , csfromsession_(true)
    , eventcatcher_( 0 )
{
    boxdragger_->ref();
    addChild( boxdragger_->getInventorNode() );
    boxdragger_->finished.notify( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    getMaterial()->setColor( Color::White );
    getMaterial()->setAmbience( 0.3 );
    getMaterial()->setDiffIntensity( 0.8 );
    getMaterial()->change.notify(mCB(this,VolumeDisplay,materialChange) );
    voltrans_->ref();
    addChild( voltrans_->getInventorNode() );
    voltrans_->setRotation( Coord3(0,1,0), M_PI_2 );
}


VolumeDisplay::~VolumeDisplay()
{
    setSceneEventCatcher( 0 );

    if ( scalarfield_ )
    {
	scalarfield_->getColorTab().rangechange.remove(
		mCB(this,VolumeDisplay,colTabChange) );
	scalarfield_->getColorTab().sequencechange.remove(
		mCB(this,VolumeDisplay,colTabChange) );
	scalarfield_->getColorTab().autoscalechange.remove(
		mCB(this,VolumeDisplay,colTabChange) );
    }

    if ( getMaterial() )
	getMaterial()->change.remove( mCB(this,VolumeDisplay,materialChange) );

    delete &as_;
    DPM( DataPackMgr::CubeID ).release( cacheid_ );
    if ( cache_ ) cache_->unRef();

    TypeSet<int> children;
    getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	removeChild( children[idx] );

    boxdragger_->finished.remove( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    boxdragger_->unRef();
    voltrans_->unRef();
    if ( scalarfield_ ) scalarfield_->unRef();

    setDataTransform( 0 );
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


void VolumeDisplay::colTabChange( CallBacker* )
{
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( mIsUdf(isovalues_[idx]) )
	    continue;

	isosurfaces_[idx]->getMaterial()->setColor(
	    scalarfield_->getColorTab().color(isovalues_[idx]));
    }
}


bool VolumeDisplay::setDataTransform( ZAxisTransform* zat )
{
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


const ZAxisTransform* VolumeDisplay::getDataTransform() const
{ return datatransform_; }


void VolumeDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    if ( cache_ ) setDataVolume( 0, cache_ );
}


void VolumeDisplay::updateRanges( bool updateic, bool updatez )
{
    if ( !datatransform_ ) return;

    if ( csfromsession_ != SI().sampling(true) )
	setCubeSampling( csfromsession_ );
    else
    {
	Interval<float> zrg = datatransform_->getZInterval( false );
	CubeSampling cs = getCubeSampling( 0 );
	assign( cs.zrg, zrg );
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

    slice->setName( dim==cTimeSlice() ? timestr : 
	    	   (dim==cCrossLine() ? crosslinestr : inlinestr) );

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
	    slices_.removeFast(idx);
	    return;
	}
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( isosurfaces_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild(isosurfaces_[idx]->getInventorNode());
	    isosurfaces_[idx]->unRef();
	    isosurfaces_.removeFast(idx);
	    isovalues_.removeFast(idx);
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
	volren_->setName( volrenstr );
    }

    if ( volren_ ) volren_->turnOn( yn );
}


bool VolumeDisplay::isVolRenShown() const
{ return volren_ && volren_->isOn(); }


int VolumeDisplay::addIsoSurface( TaskRunner* tr )
{
    visBase::MarchingCubesSurface* isosurface =
				    visBase::MarchingCubesSurface::create();
    isosurface->ref();
    isosurface->setRightHandSystem( righthandsystem_ );
    mDeclareAndTryAlloc( RefMan<MarchingCubesSurface>, surface,
	    		 MarchingCubesSurface() );
    isosurface->setSurface( *surface );
    isosurface->setName( "Iso surface" );

    isosurfaces_ += isosurface;
    isovalues_ += cache_ ? scalarfield_->getColorTab().getInterval().center()
			 : mUdf(float);

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

    scalarfield_->turnOn( false );

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


float VolumeDisplay::isoValue( const visBase::MarchingCubesSurface* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    return idx<0 ? mUdf(float) : isovalues_[idx];
}


void VolumeDisplay::setIsoValue( const visBase::MarchingCubesSurface* mcd,
				 float nv, TaskRunner* tr )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 )
	return;

    isovalues_[idx] = nv;
    updateIsoSurface( idx, tr );
}


void VolumeDisplay::updateIsoSurface( int idx, TaskRunner* tr )
{
    if ( !cache_ || !cache_->getCube(0).isOK() || mIsUdf(isovalues_[idx]) )
	isosurfaces_[idx]->getSurface()->removeAll();
    else
    {
	isosurfaces_[idx]->getSurface()->removeAll();
	isosurfaces_[idx]->setScales(
		cache_->inlsampling, cache_->crlsampling,
		SamplingData<float>( cache_->z0*cache_->zstep, cache_->zstep ));
	isosurfaces_[idx]->getSurface()->setVolumeData( 0, 0, 0,
		cache_->getCube(0), isovalues_[idx], tr );
	isosurfaces_[idx]->getMaterial()->setColor(
	    scalarfield_->getColorTab().color(isovalues_[idx]));
    }

    isosurfaces_[idx]->touch( false, tr );
}


void VolumeDisplay::manipMotionFinishCB( CallBacker* )
{
    if ( scene_ && scene_->getDataTransform() )
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
    const Coord3 newcenter( (cs.hrg.stop.inl + cs.hrg.start.inl) / 2,
			    (cs.hrg.stop.crl + cs.hrg.start.crl) / 2,
			    (cs.zrg.stop + cs.zrg.start) / 2 );
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
	pos = mNINT(slicepos*1000);
    }

    return pos;
}


void VolumeDisplay::setColorTab( visBase::VisColorTab& ctab )
{
    scalarfield_->getColorTab().rangechange.remove(
	    mCB(this,VolumeDisplay,colTabChange) );
    scalarfield_->getColorTab().sequencechange.remove(
	    mCB(this,VolumeDisplay,colTabChange) );
    scalarfield_->getColorTab().autoscalechange.remove(
	    mCB(this,VolumeDisplay,colTabChange) );

    scalarfield_->setColorTab( ctab );

    scalarfield_->getColorTab().rangechange.notify(
	    mCB(this,VolumeDisplay,colTabChange) );
    scalarfield_->getColorTab().sequencechange.notify(
	    mCB(this,VolumeDisplay,colTabChange) );
    scalarfield_->getColorTab().autoscalechange.notify(
	    mCB(this,VolumeDisplay,colTabChange) );

}


int VolumeDisplay::getColTabID( int attrib ) const
{
    return attrib ? -1 : getColorTab().id();
}


const visBase::VisColorTab& VolumeDisplay::getColorTab() const
{ return scalarfield_->getColorTab(); }


visBase::VisColorTab& VolumeDisplay::getColorTab()
{ return scalarfield_->getColorTab(); }


const TypeSet<float>* VolumeDisplay::getHistogram( int attrib ) const
{ return attrib ? 0 : &scalarfield_->getHistogram(); }


visSurvey::SurveyObject::AttribFormat VolumeDisplay::getAttributeFormat() const
{ return visSurvey::SurveyObject::Cube; }


const Attrib::SelSpec* VolumeDisplay::getSelSpec( int attrib ) const
{ return attrib ? 0 : &as_; }


void VolumeDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    if ( attrib || as_==as ) return;
    as_ = as;
    if ( cache_ ) cache_->unRef();
    cache_ = 0;

    DPM( DataPackMgr::CubeID ).release( cacheid_ );
    cacheid_ = DataPack::cNoID;

    scalarfield_->setScalarField( 0 );

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );
}


CubeSampling VolumeDisplay::getCubeSampling( int attrib ) const
{ return getCubeSampling(true,false,attrib); }


bool VolumeDisplay::setDataPackID( int attrib, DataPack::ID dpid )
{
    if ( attrib>0 ) return false;

    DataPackMgr& dpman = DPM( DataPackMgr::CubeID );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    const bool res = setDataVolume( attrib, cdp ? &cdp->cube() : 0 );
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
				   const Attrib::DataCubes* attribdata )
{
    if ( attrib || !attribdata )
	return false;

    PtrMan<Array3D<float> > tmparray = 0;
    const Array3D<float>* usedarray = 0;
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

	if ( !datatransformer_->execute() )
	{
	    pErrMsg( "Transform failed" );
	    return false;
	}

	tmparray = datatransformer_->getOutput( true );
	if ( !tmparray )
	{
	    pErrMsg( "No output from transform" );
	    return false;
	}
	usedarray = tmparray;
    }

    scalarfield_->setScalarField( usedarray );

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
{ return attrib==0 ? cacheid_ : DataPack::cNoID; }


void VolumeDisplay::getMousePosInfo( const visBase::EventInfo&,
				     const Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "";
    val = "undef";
    if ( !isManipulatorShown() )
	val = getValue( pos );
}


CubeSampling VolumeDisplay::getCubeSampling( bool manippos, bool displayspace,
					     int attrib ) const
{
    CubeSampling res;
    if ( manippos )
    {
	Coord3 center_ = boxdragger_->center();
	Coord3 width_ = boxdragger_->width();

	res.hrg.start = BinID( mNINT( center_.x - width_.x / 2 ),
			      mNINT( center_.y - width_.y / 2 ) );

	res.hrg.stop = BinID( mNINT( center_.x + width_.x / 2 ),
			     mNINT( center_.y + width_.y / 2 ) );

	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = center_.z - width_.z / 2;
	res.zrg.stop = center_.z + width_.z / 2;
    }
    else
    {
	const Coord3 transl = voltrans_->getTranslation();
	Coord3 scale = voltrans_->getScale();
	double dummy = scale.x; scale.x=scale.z; scale.z = dummy;

	res.hrg.start = BinID( mNINT(transl.x+scale.x/2),
			       mNINT(transl.y+scale.y/2) );
	res.hrg.stop = BinID( mNINT(transl.x-scale.x/2),
			       mNINT(transl.y-scale.y/2) );
	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = transl.z+scale.z/2;
	res.zrg.stop = transl.z-scale.z/2;
    }

    if ( alreadyTransformed(attrib) ) return res;

    if ( datatransform_ && !displayspace )
    {
	res.zrg.setFrom( datatransform_->getZInterval(true) );
	res.zrg.step = SI().zRange( true ).step;
    }

    return res;
}


bool VolumeDisplay::allowPicks() const
{
    return !isVolRenShown();
}


visSurvey::SurveyObject* VolumeDisplay::duplicate() const
{
    VolumeDisplay* vd = create();

    SoNode* node = vd->getInventorNode();

    TypeSet<int> children;
    vd->getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	vd->removeChild( children[idx] );

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
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visBase::DM().getObject(isosurfid));
	vd->isovalues_[idx] = isovalues_[idx];
    }

    vd->showVolRen( isVolRenShown() );

    vd->setCubeSampling( getCubeSampling(false,true,0) );

    vd->setSelSpec( 0, as_ );
    vd->setDataVolume( 0, cache_ );
    return vd;
}


SoNode* VolumeDisplay::getInventorNode()
{
    if ( !scalarfield_ )
    {
	scalarfield_ = visBase::VolumeRenderScalarField::create();
	scalarfield_->ref();
	scalarfield_->useShading( allowshading_ );
	addChild( scalarfield_->getInventorNode() );

	CubeSampling cs(false); CubeSampling sics = SI().sampling(true);
	cs.hrg.start.inl = (5*sics.hrg.start.inl+3*sics.hrg.stop.inl)/8;
	cs.hrg.start.crl = (5*sics.hrg.start.crl+3*sics.hrg.stop.crl)/8;
	cs.hrg.stop.inl = (3*sics.hrg.start.inl+5*sics.hrg.stop.inl)/8;
	cs.hrg.stop.crl = (3*sics.hrg.start.crl+5*sics.hrg.stop.crl)/8;
	cs.zrg.start = ( 5*sics.zrg.start + 3*sics.zrg.stop ) / 8;
	cs.zrg.stop = ( 3*sics.zrg.start + 5*sics.zrg.stop ) / 8;
	SI().snap( cs.hrg.start, BinID(0,0) );
	SI().snap( cs.hrg.stop, BinID(0,0) );
	float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
	float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;
	
	setCubeSampling( cs );
	setColorTab( getColorTab() );
	showManipulator( true );
	scalarfield_->turnOn( true );

	addSlice(cInLine()); addSlice(cCrossLine()); addSlice(cTimeSlice());
	showVolRen( true ); showVolRen( false );

	scalarfield_->getColorTab().rangechange.notify(
		mCB( this, VolumeDisplay, colTabChange ));
	scalarfield_->getColorTab().sequencechange.notify(
		mCB( this, VolumeDisplay, colTabChange ));
	scalarfield_->getColorTab().autoscalechange.notify(
		mCB( this, VolumeDisplay, colTabChange ));
    }

    return VisualObjectImpl::getInventorNode();
}


void VolumeDisplay::fillPar( IOPar& par, TypeSet<int>& saveids) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    const CubeSampling cs = getCubeSampling(false,true,0);
    cs.fillPar( par );

    if ( volren_ )
    {
	int volid = volren_->id();
	par.set( volumestr, volid );
	if ( saveids.indexOf( volid )==-1 ) saveids += volid;
    }

    const int textureid = scalarfield_->id();
    par.set( texturestr, textureid );
    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;

    const int nrslices = slices_.size();
    par.set( nrslicesstr, nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( slicestr ); str += idx;
	const int sliceid = slices_[idx]->id();
	par.set( str, sliceid );
	if ( saveids.indexOf(sliceid) == -1 ) saveids += sliceid;
    }

    const int nrisosurfaces = isosurfaces_.size();
    par.set( sKeyNrIsoSurfaces(), nrisosurfaces );
    for ( int idx=0; idx<nrisosurfaces; idx++ )
    {
	BufferString str( sKeyIsoValueStart() ); str += idx;
	par.set( str, isovalues_[idx] );

	str = sKeyIsoOnStart(); str += idx;
	par.setYN( str, isosurfaces_[idx]->isOn() );
    }

    as_.fillPar( par );
    fillSOPar( par );
}


int VolumeDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    if ( !as_.usePar(par) ) return -1;

    int textureid;
    if ( !par.get(texturestr,textureid) ) return false;

    visBase::DataObject* dataobj = visBase::DM().getObject( textureid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(visBase::VolumeRenderScalarField*,vt,dataobj)
    if ( !vt ) return -1;
    if ( scalarfield_ )
    {
	if ( childIndex(scalarfield_->getInventorNode()) !=-1 )
	    VisualObjectImpl::removeChild(scalarfield_->getInventorNode());
	scalarfield_->unRef();
    }

    scalarfield_ = vt;
    scalarfield_->ref();
    insertChild( 0, scalarfield_->getInventorNode() );

    int volid;
    if ( par.get(volumestr,volid) )
    {
	dataobj = visBase::DM().getObject( volid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::VolrenDisplay*,vr,dataobj)
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

    int nrisosurfaces;
    if ( par.get( sKeyNrIsoSurfaces(), nrisosurfaces ) )
    {
	for ( int idx=0; idx<nrisosurfaces; idx++ )
	{
	    BufferString str( sKeyIsoValueStart() ); str += idx;
	    float isovalue;
	    if ( par.get( str, isovalue ) )
	    {
		addIsoSurface();
		isovalues_[idx] = isovalue;
	    }

	    str = sKeyIsoOnStart(); str += idx;
	    bool status = true;
	    par.getYN( str, status );
	    isosurfaces_[idx]->turnOn( status );
	}
    }

    int nrslices = 0;
    par.get( nrslicesstr, nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( slicestr ); str += idx;
	int sliceid;
	par.get( str, sliceid );
	dataobj = visBase::DM().getObject( sliceid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::OrthogonalSlice*,os,dataobj)
	if ( !os ) return -1;
	os->ref();
	os->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
	slices_ += os;
	addChild( os->getInventorNode() );
	// set correct dimensions ...
	if ( !strcmp(os->name(),inlinestr) )
	    os->setDim( cInLine() );
	else if ( !strcmp(os->name(),timestr) )
	    os->setDim( cTimeSlice() );
    }

    setColorTab( getColorTab() );

    CubeSampling cs;
    if ( cs.usePar(par) )
    {
	csfromsession_ = cs;
	setCubeSampling( cs );
    }

    useSOPar( par );
    return 1;
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


} // namespace visSurvey
