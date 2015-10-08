/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visplanedatadisplay.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "array2dresample.h"
#include "attribsel.h"
#include "binidvalue.h"
#include "datapointset.h"
#include "flatposdata.h"
#include "seisdatapack.h"
#include "seisdatapackzaxistransformer.h"
#include "settings.h"
#include "survinfo.h"
#include "zdomain.h"

#include "visdepthtabplanedragger.h"
#include "visevent.h"
#include "visgridlines.h"
#include "vismaterial.h"
#include "vistexturechannels.h"
#include "vistexturerect.h"
#include "zaxistransform.h"
#include "zaxistransformutils.h"


namespace visSurvey {

class PlaneDataMoveUndoEvent: public UndoEvent
{
public:
		    PlaneDataMoveUndoEvent( PlaneDataDisplay* pdd,
			const TrcKeyZSampling starttkz,
			const TrcKeyZSampling endtkz )
			: pdd_( pdd )
			, starttkz_( starttkz )
			, endtkz_( endtkz )
		    {}

    const char* getStandardDesc() const
    { return "Move plane data"; }


    bool unDo()
    { return pdd_->updatePlanePos( starttkz_ ); }


    bool reDo()
    { return pdd_->updatePlanePos( endtkz_ ); }

private:
    RefMan<PlaneDataDisplay> pdd_;
    const TrcKeyZSampling starttkz_;
    const TrcKeyZSampling endtkz_;
};


DefineEnumNames(PlaneDataDisplay,SliceType,1,"Orientation")
{ "Inline", "Crossline", "Z-slice", 0 };


PlaneDataDisplay::PlaneDataDisplay()
    : MultiTextureSurveyObject()
    , dragger_( visBase::DepthTabPlaneDragger::create() )
    , gridlines_( visBase::GridLines::create() )
    , curicstep_(s3dgeom_->inlStep(),s3dgeom_->crlStep())
    , datatransform_( 0 )
    , voiidx_(-1)
    , moving_(this)
    , movefinished_(this)
    , orientation_( OD::InlineSlice )
    , csfromsession_( false )
    , eventcatcher_( 0 )
    , texturerect_( 0 )
    , forcemanipupdate_( false )
    , interactivetexturedisplay_( false )
    , originalresolution_( -1 )
    , undo_( *new Undo() )
{
    texturerect_ = visBase::TextureRectangle::create();
    addChild( texturerect_->osgNode() );

    texturerect_->setTextureChannels( channels_ );

    addChild( dragger_->osgNode() );

    rposcache_.allowNull( true );

    dragger_->ref();
    mAttachCB( dragger_->started, PlaneDataDisplay::draggerStart );
    mAttachCB( dragger_->motion, PlaneDataDisplay::draggerMotion );
    mAttachCB( dragger_->finished, PlaneDataDisplay::draggerFinish );
    mAttachCB( dragger_->rightClicked(), PlaneDataDisplay::draggerRightClick );

    dragger_->setDim( (int) 0 );

    if ( (int) orientation_ )
	dragger_->setDim( (int) orientation_ );

    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.2 );

    gridlines_->ref();
    addChild( gridlines_->osgNode() );

    updateRanges( true, true );

    int buttonkey = OD::NoButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyDepthKey(), buttonkey );
    dragger_->setTransDragKeys( true, buttonkey );
    buttonkey = OD::ShiftButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPlaneKey(), buttonkey );
    dragger_->setTransDragKeys( false, buttonkey );

    startmovepos_.setEmpty();

    init();
    showManipulator( dragger_->isOn() );
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );
    deepErase( rposcache_ );
    setZAxisTransform( 0,0 );

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpm.release( datapackids_[idx] );

    for ( int idx=0; idx<transfdatapackids_.size(); idx++ )
	dpm.release( transfdatapackids_[idx] );

    dragger_->unRef();
    gridlines_->unRef();

    undo_.removeAll();
    delete &undo_;
}


const Undo& PlaneDataDisplay::undo() const	{ return undo_; }
Undo& PlaneDataDisplay::undo()			{ return undo_; }


void PlaneDataDisplay::setOrientation( SliceType nt )
{
    if ( orientation_==nt )
	return;

    orientation_ = nt;

    dragger_->setDim( (int) nt );
    updateRanges( true, true );
}


void PlaneDataDisplay::updateRanges( bool resetic, bool resetz )
{
    if ( !scene_ )
	return;

    TrcKeyZSampling survey = scene_->getTrcKeyZSampling();
    const Interval<float> inlrg( mCast(float,survey.hsamp_.start_.inl()),
				    mCast(float,survey.hsamp_.stop_.inl()) );
    const Interval<float> crlrg( mCast(float,survey.hsamp_.start_.crl()),
				    mCast(float,survey.hsamp_.stop_.crl()) );

    dragger_->setSpaceLimits( inlrg, crlrg, survey.zsamp_ );
    dragger_->setWidthLimits(
      Interval<float>( mCast(float,4*survey.hsamp_.step_.inl()), mUdf(float) ),
      Interval<float>( mCast(float,4*survey.hsamp_.step_.crl()), mUdf(float) ),
      Interval<float>( 4*survey.zsamp_.step, mUdf(float) ) );

    TrcKeyZSampling newpos = getTrcKeyZSampling(false,true);
    if ( !newpos.isEmpty() )
    {
	if ( !survey.includes( newpos ) )
	    newpos.limitTo( survey );
    }

    if ( !newpos.hsamp_.isEmpty() && !resetic && resetz )
	survey.hsamp_ = newpos.hsamp_;

    if ( resetic || resetz || newpos.isEmpty() )
    {
	newpos = survey;
	if ( orientation_==OD::ZSlice && datatransform_ && resetz )
	{
	    const float center = survey.zsamp_.snappedCenter();
	    if ( !mIsUdf(center) )
		newpos.zsamp_.start = newpos.zsamp_.stop = center;
	}
    }

    newpos = snapPosition( newpos );

    if ( newpos!=getTrcKeyZSampling(false,true) )
	setTrcKeyZSampling( newpos );
}


TrcKeyZSampling PlaneDataDisplay::snapPosition(const TrcKeyZSampling& cs) const
{
    TrcKeyZSampling res( cs );
    const Interval<float> inlrg( mCast(float,res.hsamp_.start_.inl()),
				    mCast(float,res.hsamp_.stop_.inl()) );
    const Interval<float> crlrg( mCast(float,res.hsamp_.start_.crl()),
				    mCast(float,res.hsamp_.stop_.crl()) );
    const Interval<float> zrg( res.zsamp_ );

    res.hsamp_.snapToSurvey();
    if ( scene_ )
    {
	const StepInterval<float>& scenezrg =
					scene_->getTrcKeyZSampling().zsamp_;
	res.zsamp_.limitTo( scenezrg );
	res.zsamp_.start = scenezrg.snap( res.zsamp_.start );
	res.zsamp_.stop = scenezrg.snap( res.zsamp_.stop );

	if ( orientation_!=OD::InlineSlice && orientation_!=OD::CrosslineSlice )
	    res.zsamp_.start = res.zsamp_.stop = scenezrg.snap(zrg.center());
    }

    if ( orientation_==OD::InlineSlice )
	res.hsamp_.start_.inl() = res.hsamp_.stop_.inl() =
	    s3dgeom_->inlRange().snap( inlrg.center() );
    else if ( orientation_==OD::CrosslineSlice )
	res.hsamp_.start_.crl() = res.hsamp_.stop_.crl() =
	    s3dgeom_->crlRange().snap( crlrg.center() );

    return res;
}


Coord3 PlaneDataDisplay::getNormal( const Coord3& pos ) const
{
    if ( orientation_==OD::ZSlice )
	return Coord3(0,0,1);

    return Coord3( orientation_==OD::InlineSlice
		  ? s3dgeom_->binID2Coord().rowDir()
		  : s3dgeom_->binID2Coord().colDir(), 0 );
}


float PlaneDataDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pos, xytpos );
    const BinID binid = s3dgeom_->transform( Coord(xytpos.x,xytpos.y) );

    const TrcKeyZSampling cs = getTrcKeyZSampling(false,true);

    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    inlcrldist.inl() =
	binid.inl()>=cs.hsamp_.start_.inl() &&
	binid.inl()<=cs.hsamp_.stop_.inl()
	     ? 0
	     : mMIN( abs(binid.inl()-cs.hsamp_.start_.inl()),
		     abs( binid.inl()-cs.hsamp_.stop_.inl()) );
    inlcrldist.crl() =
	binid.crl()>=cs.hsamp_.start_.crl() &&
	binid.crl()<=cs.hsamp_.stop_.crl()
	     ? 0
	     : mMIN( abs(binid.crl()-cs.hsamp_.start_.crl()),
		     abs( binid.crl()-cs.hsamp_.stop_.crl()) );
    const float zfactor = scene_ ? scene_->getZScale() : s3dgeom_->zScale();
    zdiff = cs.zsamp_.includes(xytpos.z,false)
	? 0
	: (float)(mMIN(fabs(xytpos.z-cs.zsamp_.start),
	   fabs(xytpos.z-cs.zsamp_.stop))
	   * zfactor  * scene_->getFixedZStretch() );

    const float inldist = s3dgeom_->inlDistance();
    const float crldist = s3dgeom_->crlDistance();
    float inldiff = inlcrldist.inl() * inldist;
    float crldiff = inlcrldist.crl() * crldist;

    return Math::Sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


float PlaneDataDisplay::getZScale() const
{
    return scene_ ? scene_->getZScale() : s3dgeom_->zScale();
}



float PlaneDataDisplay::maxDist() const
{
    const float zfactor = scene_ ? scene_->getZScale():s3dgeom_->zScale();
    float maxzdist = zfactor * scene_->getFixedZStretch()
		     * s3dgeom_->zStep() / 2;
    return orientation_==OD::ZSlice ? maxzdist : SurveyObject::sDefMaxDist();
}


bool PlaneDataDisplay::setZAxisTransform( ZAxisTransform* zat,
					  TaskRunner* taskr )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,PlaneDataDisplay,dataTransformCB) );
	if ( voiidx_>0 )
	{
	    datatransform_->removeVolumeOfInterest( voiidx_ );
	    voiidx_ = -1;
	}

	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,PlaneDataDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* PlaneDataDisplay::getZAxisTransform() const
{ return datatransform_; }


void PlaneDataDisplay::setTranslationDragKeys( bool depth, int ns )
{ dragger_->setTransDragKeys( depth, ns ); }


int PlaneDataDisplay::getTranslationDragKeys(bool depth) const
{ return dragger_->getTransDragKeys( depth ); }


void PlaneDataDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( rposcache_[idx] )
	    setRandomPosDataNoCache( idx, rposcache_[idx], 0 );
	else
	    createTransformedDataPack( idx );

	updateChannels( idx, 0 );
    }
}


void PlaneDataDisplay::draggerStart( CallBacker* )
{ startmovepos_ = getTrcKeyZSampling( true,true ); }


void PlaneDataDisplay::draggerMotion( CallBacker* )
{
    moving_.trigger();

    const TrcKeyZSampling dragcs = getTrcKeyZSampling(true,true);
    const TrcKeyZSampling snappedcs = snapPosition( dragcs );
    const TrcKeyZSampling oldcs = getTrcKeyZSampling(false,true);

    bool showplane = false;
    if ( orientation_==OD::InlineSlice
	    && dragcs.hsamp_.start_.inl()!=oldcs.hsamp_.start_.inl() )
	showplane = true;
    else if ( orientation_==OD::CrosslineSlice &&
	      dragcs.hsamp_.start_.crl()!=oldcs.hsamp_.start_.crl() )
	showplane = true;
    else if ( orientation_==OD::ZSlice &&
	      dragcs.zsamp_.start!=oldcs.zsamp_.start )
	showplane = true;

    dragger_->showPlane( showplane );
    dragger_->showDraggerBorder( !showplane );

    if ( canDisplayInteractively() )
    {
	if ( originalresolution_ < 0 )
	    originalresolution_ = resolution_;

	resolution_ = 0;
	interactivetexturedisplay_ = true;
	updateSel();
    }
}


void PlaneDataDisplay::draggerFinish( CallBacker* )
{
    const TrcKeyZSampling cs = getTrcKeyZSampling(true,true);
    const TrcKeyZSampling snappedcs = snapPosition( cs );

    if ( cs!=snappedcs )
	setDraggerPos( snappedcs );

    if ( originalresolution_ >= 0 )
    {
	setResolution( originalresolution_, 0 );
	originalresolution_ = -1;
    }

    interactivetexturedisplay_ = false;

    forcemanipupdate_ = true;
    updateSel();
    forcemanipupdate_ = false;

    PlaneDataMoveUndoEvent* undoevent =
	new PlaneDataMoveUndoEvent( this, startmovepos_, snappedcs );

    undo_.addEvent( undoevent, 0 );
    undo_.setUserInteractionEnd( undo_.currentEventID() );
}


bool PlaneDataDisplay::updatePlanePos( const TrcKeyZSampling& tkz )
{
    if ( !tkz.isDefined() || tkz.isEmpty() )
	return false;

    setDraggerPos( tkz );
    updateSel();
    return true;
}


void PlaneDataDisplay::draggerRightClick( CallBacker* cb )
{
    triggerRightClick( dragger_->rightClickedEventInfo() );
}

#define mDefineCenterAndWidth( thecs ) \
    const Coord3 center( \
		(thecs.hsamp_.start_.inl()+thecs.hsamp_.stop_.inl())/2.0, \
		(thecs.hsamp_.start_.crl()+thecs.hsamp_.stop_.crl())/2.0, \
		 thecs.zsamp_.center() ); \
    Coord3 width( thecs.hsamp_.stop_.inl()-thecs.hsamp_.start_.inl(), \
		  thecs.hsamp_.stop_.crl()-thecs.hsamp_.start_.crl(), \
		  thecs.zsamp_.width() ); \
    if ( width.x < 1 ) width.x = 1; \
    if ( width.y < 1 ) width.y = 1; \
    if ( width.z < thecs.zsamp_.step * 0.5 ) width.z = 1; \
 \
    const Coord3 oldwidth = dragger_->size(); \
    width[(int)orientation_] = oldwidth[(int)orientation_]

void PlaneDataDisplay::setDraggerPos( const TrcKeyZSampling& cs )
{
    mDefineCenterAndWidth( cs );
    dragger_->setCenter( center );
    dragger_->setSize( width );
}


void PlaneDataDisplay::coltabChanged( CallBacker* )
{
    // Hack for correct transparency display
    bool manipshown = isManipulatorShown();
    if ( manipshown ) return;
    showManipulator( true );
    showManipulator( false );
}


void PlaneDataDisplay::showManipulator( bool yn )
{
    dragger_->turnOn( yn );
    texturerect_->enableTraversal(visBase::cDraggerIntersecTraversalMask(),!yn);
}


bool PlaneDataDisplay::isManipulatorShown() const
{
    return dragger_->isOn();
}


bool PlaneDataDisplay::isManipulated() const
{
    if ( getTrcKeyZSampling(true,true) != getTrcKeyZSampling(false,true) )
	return true;

    return forcemanipupdate_;
}


void PlaneDataDisplay::resetManipulation()
{
    TrcKeyZSampling cs = getTrcKeyZSampling( false, true );
    setDraggerPos( cs );

    dragger_->showPlane( false );
    dragger_->showDraggerBorder( true );
}


void PlaneDataDisplay::acceptManipulation()
{
    TrcKeyZSampling cs = getTrcKeyZSampling( true, true );
    setTrcKeyZSampling( cs );

    if ( !getUpdateStageNr() )
    {
	dragger_->showPlane( false );
	dragger_->showDraggerBorder( true );
    }
}


BufferString PlaneDataDisplay::getManipulationString() const
{
    BufferString res;
    getObjectInfo( res );
    return res;
}


NotifierAccess* PlaneDataDisplay::getManipulationNotifier()
{ return &moving_; }


int PlaneDataDisplay::nrResolutions() const
{
    return 3;
}


void PlaneDataDisplay::setResolution( int res, TaskRunner* taskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;

    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, taskr );
}


SurveyObject::AttribFormat
    PlaneDataDisplay::getAttributeFormat( int attrib ) const
{
    if ( alreadyTransformed(attrib) )
	return SurveyObject::Cube;

    return datatransform_ && orientation_==OD::ZSlice
	? SurveyObject::RandomPos : SurveyObject::Cube;
}


void PlaneDataDisplay::addCache()
{
    rposcache_ += 0;
    datapackids_ += DataPack::cNoID();
    transfdatapackids_ += DataPack::cNoID();
}


void PlaneDataDisplay::removeCache( int attrib )
{
    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.removeSingle( attrib );

    DPM(DataPackMgr::SeisID()).release( datapackids_[attrib] );
    datapackids_.removeSingle( attrib );

    DPM(DataPackMgr::SeisID()).release( transfdatapackids_[attrib] );
    transfdatapackids_.removeSingle( attrib );

    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, 0 );
}


void PlaneDataDisplay::swapCache( int a0, int a1 )
{
    rposcache_.swap( a0, a1 );

    datapackids_.swap( a0, a1 );
    transfdatapackids_.swap( a0, a1 );
}


void PlaneDataDisplay::emptyCache( int attrib )
{
    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.replace( attrib, 0 );

    DPM(DataPackMgr::SeisID()).release( datapackids_[attrib] );
    datapackids_[attrib] = DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).release( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = DataPack::cNoID();

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, 0 );
}


bool PlaneDataDisplay::hasCache( int attrib ) const
{
    return (datapackids_[attrib] != DataPack::cNoID()) || rposcache_[attrib];
}


TrcKeyZSampling PlaneDataDisplay::getTrcKeyZSampling( int attrib ) const
{
    return getTrcKeyZSampling( true, false, attrib );
}


void PlaneDataDisplay::getRandomPos( DataPointSet& pos, TaskRunner* ) const
{
    if ( !datatransform_ ) return;

    const TrcKeyZSampling cs = getTrcKeyZSampling( true, true, 0 ); //attrib?
    ZAxisTransformPointGenerator generator( *datatransform_ );
    generator.setInput( cs );
    generator.setOutputDPS( pos );
    generator.execute();
}


void PlaneDataDisplay::setRandomPosData( int attrib, const DataPointSet* data,
					 TaskRunner* taskr )
{
    if ( attrib>=nrAttribs() )
	return;

    setRandomPosDataNoCache( attrib, &data->bivSet(), taskr );

    if ( rposcache_[attrib] )
	delete rposcache_[attrib];

    rposcache_.replace( attrib, data ? new BinIDValueSet(data->bivSet()) : 0 );
}


void PlaneDataDisplay::setTrcKeyZSampling( const TrcKeyZSampling& wantedcs )
{
    TrcKeyZSampling cs = snapPosition( wantedcs );

    mDefineCenterAndWidth( cs );
    width[(int)orientation_] = 0;
    texturerect_->setCenter( center );
    texturerect_->setWidth( width );
    texturerect_->swapTextureAxes();

    setDraggerPos( cs );
    if ( gridlines_ ) gridlines_->setPlaneTrcKeyZSampling( cs );

    curicstep_ = cs.hsamp_.step_;

    setUpdateStageTextureTransform();

    //channels_->clearAll();
    movefinished_.trigger();
}


TrcKeyZSampling PlaneDataDisplay::getTrcKeyZSampling( bool manippos,
						bool displayspace,
						int attrib ) const
{
    TrcKeyZSampling res;
    Coord3 c0, c1;

    if ( manippos )
    {
	const Coord3 center = dragger_->center();
	Coord3 halfsize = dragger_->size()/2;
	halfsize[orientation_] = 0;

	c0 = center + halfsize;
	c1 = center - halfsize;
    }
    else
    {
	const Coord3 center = texturerect_->getCenter();
	Coord3 halfsize = texturerect_->getWidth()/2;
	halfsize[orientation_] = 0;

	c0 = center + halfsize;
	c1 = center - halfsize;
    }

    res.hsamp_.start_ = res.hsamp_.stop_ = BinID(mNINT32(c0.x),mNINT32(c0.y) );
    res.zsamp_.start = res.zsamp_.stop = (float) c0.z;
    res.hsamp_.include( BinID(mNINT32(c1.x),mNINT32(c1.y)) );
    res.zsamp_.include( (float) c1.z );
    res.hsamp_.step_ = BinID( s3dgeom_->inlStep(), s3dgeom_->crlStep() );
    res.zsamp_.step = s3dgeom_->zRange().step;
    res.hsamp_.survid_ = Survey::GM().default3DSurvID();

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
	    res.zsamp_.step = SI().zRange(true).step;
	}
	else
	{
	    if ( scene_ )
		res.zsamp_.step = scene_->getTrcKeyZSampling().zsamp_.step;
	    else
		res.zsamp_.step = datatransform_->getGoodZStep();
	}
    }

    return res;
}


bool PlaneDataDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				      TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM( DataPackMgr::SeisID() );
    const DataPack* datapack = dpm.obtain( dpid );
    mDynamicCastGet( const RegularSeisDataPack*, regsdp, datapack );
    if ( !regsdp || regsdp->isEmpty() )
    {
	dpm.release( dpid );
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, 0 );
	return false;
    }

    dpm.release( datapackids_[attrib] );
    datapackids_[attrib] = dpid;

    createTransformedDataPack( attrib );
    updateChannels( attrib, taskr );
    return true;
}


void PlaneDataDisplay::setVolumeDataPackNoCache( int attrib,
			const RegularSeisDataPack* regsdp )
{
    if ( !regsdp ) return;

    //set display datapack.
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );

    TypeSet<DataPack::ID> attridpids;
    ObjectSet<const FlatDataPack> displaypacks;
    ObjectSet<const FlatDataPack> tfpacks;
    //mLoadFDPs( regsdp, attridpids, displaypacks );

    //transform data if necessary.
    const bool usetf = tfpacks.size();
    if ( nrAttribs()>1 )
    {
	const int oldchannelsz0 =
		  (channels_->getSize(0,1)+resolution_) / (resolution_+1);
	const int oldchannelsz1 =
		  (channels_->getSize(0,2)+resolution_) / (resolution_+1);

	//check current attribe sizes
	int newsz0 = 0, newsz1 = 0;
	bool hassamesz = true;
	if ( oldchannelsz0 && oldchannelsz1 )
	{
	    for ( int idx=0; idx<attridpids.size(); idx++ )
	    {
		const int sz0 = usetf ? tfpacks[idx]->data().info().getSize(0)
		    : displaypacks[idx]->data().info().getSize(0);
		const int sz1 = usetf ? tfpacks[idx]->data().info().getSize(1)
		    : displaypacks[idx]->data().info().getSize(1);

		if ( idx && (sz0!=newsz0 || sz1!=newsz1) )
		    hassamesz = false;

		if ( newsz0<sz0 ) newsz0 = sz0;
		if ( newsz1<sz1 ) newsz1 = sz1;
	    }
	}

	const bool onlycurrent = newsz0<=oldchannelsz0 && newsz1<=oldchannelsz1;
	const int attribsz = (newsz0<2 || newsz1<2) || (newsz0==oldchannelsz0
		&& newsz1==oldchannelsz1 && hassamesz) ? 0 : nrAttribs();
	if ( newsz0<oldchannelsz0 ) newsz0 = oldchannelsz0;
	if ( newsz1<oldchannelsz1 ) newsz1 = oldchannelsz1;
	for ( int idx=0; idx<attribsz; idx++ )
	{
	    if ( onlycurrent && idx!=attrib )
		continue;

	    TypeSet<DataPack::ID> pids;
	    ObjectSet<const FlatDataPack> packs;
	    /*if ( idx!=attrib &&  volumecache_[idx] )
	    {
		mLoadFDPs( volumecache_[idx], pids, packs );
	    }*/

	    bool needsupdate = false;
	    const int idsz = idx==attrib ? attridpids.size() : pids.size();
	    for ( int idy=0; idy<idsz; idy++ )
	    {
		const FlatDataPack* dp = idx!=attrib ? packs[idy] :
		    ( usetf ? tfpacks[idy] : displaypacks[idy] );
		StepInterval<double> rg0 = dp->posData().range(true);
		StepInterval<double> rg1 = dp->posData().range(false);
		const int sz0 = dp->data().info().getSize(0);
		const int sz1 = dp->data().info().getSize(1);
		if ( sz0==newsz0 && sz1==newsz1 )
		    continue;

		needsupdate = true;
		mDeclareAndTryAlloc( Array2DImpl<float>*, arr,
			Array2DImpl<float> (newsz0,newsz1) );
		interpolArray(idx,arr->getData(),newsz0,newsz1,dp->data(),0);
		mDeclareAndTryAlloc( FlatDataPack*, fdp,
			FlatDataPack( dp->category(), arr ) );

		rg0.step = newsz0!=1 ? rg0.width()/(newsz0-1) : rg0.width();
		rg1.step = newsz1!=1 ? rg1.width()/(newsz1-1) : rg1.width();

		fdp->posData().setRange( true, rg0 );
		fdp->posData().setRange( false, rg1 );

		dpman.addAndObtain( fdp );
		if ( idx==attrib )
		    attridpids[idy] = fdp->id();
		else
		    pids[idy] = fdp->id();

		dpman.release( dp );
	    }

	    if ( idx!=attrib )
	    {
		if ( needsupdate )
		    setDisplayDataPackIDs( idx, pids );

		for ( int idy=0; idy<pids.size(); idy++ )
		    dpman.release( pids[idy] );
	    }
	}
    }

    setDisplayDataPackIDs( attrib, attridpids );

    for ( int idx=0; idx<attridpids.size(); idx++ )
	dpman.release( attridpids[idx] );
}


DataPack::ID PlaneDataDisplay::getDataPackID( int attrib ) const
{
    return datapackids_.validIdx(attrib) ? datapackids_[attrib]
					 : DataPack::cNoID();
}


DataPack::ID PlaneDataDisplay::getDisplayedDataPackID( int attrib ) const
{
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	const TypeSet<DataPack::ID>& dpids = transfdatapackids_;
	return dpids.validIdx(attrib) ? dpids[attrib] : DataPack::cNoID();
    }

    return getDataPackID( attrib );
}


void PlaneDataDisplay::setRandomPosDataNoCache( int attrib,
			const BinIDValueSet* bivset, TaskRunner* taskr )
{
    if ( !bivset || !datatransform_ )
	return;

    const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true, 0 );
    const DataPack::ID dpid = RegularSeisDataPack::createDataPackForZSlice(
	bivset, tkzs, datatransform_->toZDomainInfo(), *userrefs_[attrib] );

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    dpm.release( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = dpid;
    dpm.obtain( dpid );

    updateChannels( attrib, taskr );
}


void PlaneDataDisplay::setDisplayDataPackIDs( int attrib,
			const TypeSet<DataPack::ID>& newdpids )
{
    TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idx] );

    dpids = newdpids;
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).obtain( dpids[idx] );

    updateChannels( attrib, 0 );
}


void PlaneDataDisplay::updateChannels( int attrib, TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    ConstDataPackRef<RegularSeisDataPack> regsdp = dpm.obtain( dpid );
    if ( !regsdp ) return;

    const int nrversions = regsdp->nrComponents();
    channels_->setNrVersions( attrib, nrversions );

    const int dim0 = orientation_==OD::InlineSlice ? 1 : 0;
    const int dim1 = orientation_==OD::ZSlice ? 1 : 2;
    for ( int idx=0; idx<nrversions; idx++ )
    {
	const Array3D<float>& array = regsdp->data( idx );
	const int sz0 = 1 + (array.info().getSize(dim0)-1) * (resolution_+1);
	const int sz1 = 1 + (array.info().getSize(dim1)-1) * (resolution_+1);

	const float* arr = array.getData();
	OD::PtrPolicy cp = OD::UsePtr;

	if ( !arr || resolution_>0 )
	{
	    mDeclareAndTryAlloc( float*, tmparr, float[sz0*sz1] );
	    if ( !tmparr ) continue;

	    if ( resolution_ == 0 )
		array.getAll( tmparr );
	    else
	    {
		Array2DSlice<float> slice2d( array );
		slice2d.setDimMap( 0, dim0 );
		slice2d.setDimMap( 1, dim1 );
		slice2d.setPos( orientation_, 0 );
		slice2d.init();
		interpolArray( attrib, tmparr, sz0, sz1, slice2d, taskr );
	    }

	    arr = tmparr;
	    cp = OD::TakeOverPtr;
	}

	channels_->setSize( attrib, 1, sz0, sz1 );
	channels_->setUnMappedData( attrib, idx, arr, cp, 0,
				    interactivetexturedisplay_ );
    }

    channels_->turnOn( true );
}


void PlaneDataDisplay::createTransformedDataPack( int attrib )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDataPackID( attrib );
    ConstDataPackRef<RegularSeisDataPack> regsdp = dpm.obtain( dpid );
    if ( !regsdp || regsdp->isEmpty() )
	return;

    DataPack::ID outputid = DataPack::cNoID();
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
/*	TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true );
	if ( voiidx_ < 0 )
	    voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	else
	    datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );
	datatransform_->loadDataIfMissing( voiidx_ );*/

	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( regsdp.ptr() );
	transformer.setOutput( outputid );
	transformer.setInterpolate( textureInterpolationEnabled() );
	transformer.execute();
    }

    dpm.release( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = outputid;
    dpm.obtain( outputid );
}


void PlaneDataDisplay::interpolArray( int attrib, float* res, int sz0, int sz1,
				      const Array2D<float>& inp,
				      TaskRunner* taskr ) const
{
    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );
    Array2DReSampler<float,float> resampler( inp, res, sz0, sz1, true );
    resampler.setInterpolate( true );
    TaskRunner::execute( taskr, resampler );
}


#define mIsValid(idx,sz) ( idx>=0 && idx<sz )

void PlaneDataDisplay::getMousePosInfo( const visBase::EventInfo&,
					Coord3& pos,
					BufferString& val,
					BufferString& info ) const
{
    info = getManipulationString();
    getValueString( pos, val );
}


void PlaneDataDisplay::getObjectInfo( BufferString& info ) const
{
    if ( orientation_==OD::InlineSlice )
    {
	info = "In-line: ";
	info += getTrcKeyZSampling(true,true).hsamp_.start_.inl();
    }
    else if ( orientation_==OD::CrosslineSlice )
    {
	info = "Cross-line: ";
	info += getTrcKeyZSampling(true,true).hsamp_.start_.crl();
    }
    else
    {
	float val = getTrcKeyZSampling(true,true).zsamp_.start;
	if ( !scene_ ) { info = val; return; }

	const ZDomain::Info& zdinf = scene_->zDomainInfo();
	info = mFromUiStringTodo(zdinf.userName()); info += ": ";
	info += mNINT32(val * zdinf.userFactor());
    }
}


bool PlaneDataDisplay::getCacheValue( int attrib, int version,
				      const Coord3& pos, float& val ) const
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    ConstDataPackRef<RegularSeisDataPack> regsdp = dpm.obtain( dpid );
    if ( !regsdp || regsdp->isEmpty() )
	return false;

    const TrcKeyZSampling& tkzs = regsdp->sampling();
    const BinID bid = SI().transform( pos );
    const int inlidx = tkzs.hsamp_.inlRange().nearestIndex( bid.inl() );
    const int crlidx = tkzs.hsamp_.crlRange().nearestIndex( bid.crl() );
    const int zidx = tkzs.zsamp_.nearestIndex( pos.z );
    const Array3DImpl<float>& array = regsdp->data( version );
    if ( !array.info().validPos(inlidx,crlidx,zidx) )
	return false;

    val = array.get( inlidx, crlidx, zidx );
    return true;
}


bool PlaneDataDisplay::isVerticalPlane() const
{
    return orientation_ != OD::ZSlice;
}


void PlaneDataDisplay::setScene( Scene* sc )
{
    SurveyObject::setScene( sc );
    if ( sc ) updateRanges( false, false );
}


void PlaneDataDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
		mCB(this,PlaneDataDisplay,updateMouseCursorCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
		mCB(this,PlaneDataDisplay,updateMouseCursorCB) );
    }
}


void PlaneDataDisplay::updateMouseCursorCB( CallBacker* cb )
{
    if ( !isManipulatorShown() || !isOn() || isLocked() )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
	initAdaptiveMouseCursor( cb, id(), dragger_->getTransDragKeys(false),
				 mousecursor_ );
}


SurveyObject* PlaneDataDisplay::duplicate( TaskRunner* taskr ) const
{
    PlaneDataDisplay* pdd = new PlaneDataDisplay();
    pdd->setOrientation( orientation_ );
    pdd->setTrcKeyZSampling( getTrcKeyZSampling(false,true,0) );
    pdd->setZAxisTransform( datatransform_, taskr );

    while ( nrAttribs() > pdd->nrAttribs() )
	pdd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( !getSelSpec(idx) ) continue;

	pdd->setSelSpec( idx, *getSelSpec(idx) );
	pdd->setDataPackID( idx, getDataPackID(idx), taskr );
	if ( getColTabMapperSetup( idx ) )
	    pdd->setColTabMapperSetup( idx, *getColTabMapperSetup( idx ),
				       taskr );
	if ( getColTabSequence( idx ) )
	    pdd->setColTabSequence( idx, *getColTabSequence( idx ), taskr );
    }

    return pdd;
}


void PlaneDataDisplay::fillPar( IOPar& par ) const
{
    MultiTextureSurveyObject::fillPar( par );

    par.set( sKeyOrientation(), getSliceTypeString( orientation_) );
    getTrcKeyZSampling( false, true ).fillPar( par );
}


bool PlaneDataDisplay::usePar( const IOPar& par )
{
    if ( !MultiTextureSurveyObject::usePar( par ) )
	return false;

    SliceType orientation = OD::InlineSlice;
    FixedString orstr = par.find( sKeyOrientation() );
    if ( !parseEnumSliceType(orstr,orientation) && orstr == "Timeslice" )
	orientation = OD::ZSlice;	// Backward compatibilty with 4.0

    setOrientation( orientation );
    TrcKeyZSampling cs;
    if ( cs.usePar( par ) )
    {
	csfromsession_ = cs;
	setTrcKeyZSampling( cs );
    }

    return true;
}


void PlaneDataDisplay::setDisplayTransformation( const mVisTrans* t )
{
    displaytrans_ = t;
    texturerect_->setDisplayTransformation( t );
    dragger_->setDisplayTransformation( t );
    if ( gridlines_ )
	gridlines_->setDisplayTransformation( t );
}


void PlaneDataDisplay::annotateNextUpdateStage( bool yn )
{
    if ( !yn )
    {
	texturerect_->setTextureShift( Coord(0.0,0.0) );
	texturerect_->setTextureGrowth( Coord(0.0,0.0) );
	dragger_->showPlane( false );
	dragger_->showDraggerBorder( true );
    }
    else if ( !getUpdateStageNr() )
    {
	updatestageinfo_.oldcs_ = getTrcKeyZSampling( false, true );
	updatestageinfo_.oldorientation_ = orientation_;
	// Needs refinement when introducing variably sized texture layers
	updatestageinfo_.oldimagesize_.x = channels_->getSize(0,2);
	updatestageinfo_.oldimagesize_.y = channels_->getSize(0,1);
	updatestageinfo_.refreeze_ = true;
    }
    else if ( updatestageinfo_.refreeze_ )
	texturerect_->freezeDisplay( false );	// thaw to refreeze

    texturerect_->freezeDisplay( yn );
    SurveyObject::annotateNextUpdateStage( yn );
}


void PlaneDataDisplay::setUpdateStageTextureTransform()
{
    if ( !getUpdateStageNr() )
	return;

    const TrcKeyZSampling& oldcs = updatestageinfo_.oldcs_;
    const TrcKeyZSampling  newcs = getTrcKeyZSampling( false, true );

    Coord samplingratio( updatestageinfo_.oldimagesize_.x/oldcs.nrZ(),
			 updatestageinfo_.oldimagesize_.y/oldcs.nrInl() );
    Coord startdif( (newcs.zsamp_.start-oldcs.zsamp_.start) / newcs.zsamp_.step,
		    newcs.hsamp_.start_.inl()-oldcs.hsamp_.start_.inl() );
    Coord growth( newcs.nrZ()-oldcs.nrZ(), newcs.nrInl()-oldcs.nrInl() );
    updatestageinfo_.refreeze_ =
	newcs.hsamp_.start_.crl()==oldcs.hsamp_.start_.crl();

    if ( orientation_ == OD::InlineSlice )
    {
	samplingratio.y = updatestageinfo_.oldimagesize_.y / oldcs.nrCrl();
	startdif.y = newcs.hsamp_.start_.crl() - oldcs.hsamp_.start_.crl();
	growth.y = newcs.nrCrl() - oldcs.nrCrl();
	updatestageinfo_.refreeze_ =
	    newcs.hsamp_.start_.inl()==oldcs.hsamp_.start_.inl();
    }

    if ( orientation_ == OD::ZSlice )
    {
	samplingratio.x = updatestageinfo_.oldimagesize_.x / oldcs.nrCrl();
	startdif.x = newcs.hsamp_.start_.crl() - oldcs.hsamp_.start_.crl();
	growth.x = newcs.nrCrl() - oldcs.nrCrl();
	updatestageinfo_.refreeze_ = newcs.zsamp_.start==oldcs.zsamp_.start;
    }

    const int texturebordercoord = oldcs.nrInl() + oldcs.nrCrl() + oldcs.nrZ();
    if ( updatestageinfo_.oldorientation_!=orientation_ )
    {
	startdif = Coord( texturebordercoord, texturebordercoord );
	updatestageinfo_.refreeze_ = true;
    }

    startdif.x  *= samplingratio.x; startdif.y  *= samplingratio.y;
    growth.x *= samplingratio.x; growth.y *= samplingratio.y;

    texturerect_->setTextureGrowth( growth );
    texturerect_->setTextureShift( -startdif - growth*0.5 );
}


} // namespace visSurvey
