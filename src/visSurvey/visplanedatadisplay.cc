/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/


#include "visplanedatadisplay.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "array2dresample.h"
#include "datapointset.h"
#include "probe.h"
#include "probeimpl.h"
#include "seisdatapack.h"
#include "volumedatapackzaxistransformer.h"
#include "settings.h"
#include "mousecursor.h"
#include "uistrings.h"
#include "survinfo.h"

#include "visdepthtabplanedragger.h"
#include "visevent.h"
#include "visgridlines.h"
#include "vismaterial.h"
#include "vistexturechannels.h"
#include "visrgbatexturechannel2rgba.h"
#include "vistexturerect.h"
#include "zaxistransform.h"
#include "zaxistransformutils.h"


mDefineEnumUtils(visSurvey::PlaneDataDisplay,SliceType,"Orientation")
{ "Inline", "Crossline", "Z-slice", 0 };

template<>
void EnumDefImpl<visSurvey::PlaneDataDisplay::SliceType>::init()
{
    uistrings_ += uiStrings::sInline();
    uistrings_ += uiStrings::sCrossline();
    uistrings_ += uiStrings::sZSlice();
}


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
    PlaneDataDisplay*	  pdd_;
    const TrcKeyZSampling starttkz_;
    const TrcKeyZSampling endtkz_;
};


PlaneDataDisplay::PlaneDataDisplay()
    : MultiTextureSurveyObject()
    , dragger_( visBase::DepthTabPlaneDragger::create() )
    , gridlines_( visBase::GridLines::create() )
    , curicstep_(s3dgeom_->inlRange().step,s3dgeom_->crlRange().step)
    , datatransform_( 0 )
    , voiidx_(-1)
    , moving_(this)
    , movefinished_(this)
    , datachanged_(this)
    , poschanged_(this)
    , orientation_( OD::InlineSlice )
    , csfromsession_( false )
    , eventcatcher_( 0 )
    , texturerect_( 0 )
    , forcemanipupdate_( false )
    , interactivetexturedisplay_( false )
    , originalresolution_( -1 )
    , probe_(0)
    , undo_( *new Undo() )
    , mousecursor_( *new MouseCursor )
{
    texturerect_ = visBase::TextureRectangle::create();
    addChild( texturerect_->osgNode() );

    texturerect_->setTextureChannels( channels_ );

    addChild( dragger_->osgNode() );

    rposcache_.setNullAllowed( true );

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

    init();
    showManipulator( dragger_->isOn() );
    startmovepos_.setEmpty();
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );
    deepErase( rposcache_ );
    setZAxisTransform( 0,0 );

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpm.unRef( datapackids_[idx] );

    for ( int idx=0; idx<transfdatapackids_.size(); idx++ )
	dpm.unRef( transfdatapackids_[idx] );

    dragger_->unRef();
    gridlines_->unRef();
    undo_.removeAll();
    delete &undo_;
    delete &mousecursor_;
}


const Undo& PlaneDataDisplay::undo() const	{ return undo_; }
Undo& PlaneDataDisplay::undo()			{ return undo_; }


void PlaneDataDisplay::setProbe( Probe* probe )
{
    if ( !probe || probe_.ptr()==probe )
	return;

    SliceType st;
    BufferString probetype = probe->type();
    if ( probetype==InlineProbe::sFactoryKey() )
	st = OD::InlineSlice;
    else if ( probetype==CrosslineProbe::sFactoryKey() )
	st = OD::CrosslineSlice;
    else if ( probetype==ZSliceProbe::sFactoryKey() )
	st = OD::ZSlice;
    else
    {
	pErrMsg( "Wrong probe type set" );
	return;
    }

    probe_ = probe;
    setOrientation( st );
    setTrcKeyZSampling( probe_->position() );
}


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
    const StepInterval<float> inlrg( mCast(float,survey.hsamp_.start_.inl()),
				     mCast(float,survey.hsamp_.stop_.inl()),
				     mCast(float,survey.hsamp_.step_.inl()) );
    const StepInterval<float> crlrg( mCast(float,survey.hsamp_.start_.crl()),
				     mCast(float,survey.hsamp_.stop_.crl()),
				     mCast(float,survey.hsamp_.step_.crl()) );

    dragger_->setSpaceLimits( inlrg, crlrg, survey.zsamp_ );
    dragger_->setWidthLimits(
      Interval<float>( mCast(float,4*survey.hsamp_.step_.inl()), mUdf(float) ),
      Interval<float>( mCast(float,4*survey.hsamp_.step_.crl()), mUdf(float) ),
      Interval<float>( 4*survey.zsamp_.step, mUdf(float) ) );

    dragger_->setDragCtrlSpacing( inlrg, crlrg, survey.zsamp_ );

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


Coord3 PlaneDataDisplay::getNormal( const Coord3& pos  ) const
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
    const BinID binid = s3dgeom_->transform( xytpos.getXY() );

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
    zdiff = cs.zsamp_.includes(xytpos.z_,false)
	? 0
	: (float)(mMIN(fabs(xytpos.z_-cs.zsamp_.start),
	   fabs(xytpos.z_-cs.zsamp_.stop))
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
		     * s3dgeom_->zRange().step / 2;
    return orientation_==OD::ZSlice ? maxzdist : SurveyObject::sDefMaxDist();
}


bool PlaneDataDisplay::setZAxisTransform( ZAxisTransform* zat,
					  TaskRunner* tskr )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,PlaneDataDisplay,dataTransformCB) );
	if ( voiidx_ != -1 )
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
	SilentTaskRunnerProvider trprov;
	if ( rposcache_[idx] )
	    setRandomPosDataNoCache( idx, rposcache_[idx], trprov );
	else
	    createTransformedDataPack( idx, 0 );

	updateChannels( idx, trprov );
    }
}


void PlaneDataDisplay::draggerStart( CallBacker* )
{ startmovepos_ = getTrcKeyZSampling( true,true ); }


void PlaneDataDisplay::draggerMotion( CallBacker* )
{
    moving_.trigger();

    const TrcKeyZSampling dragcs = getTrcKeyZSampling(true,true);
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
	poschanged_.trigger();
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

    if ( interactivetexturedisplay_ )
	forcemanipupdate_ = true;

    interactivetexturedisplay_ = false;
    updateSel();
    poschanged_.trigger();
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
    moving_.trigger();
    poschanged_.trigger();
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
    if ( width.x_ < 1 ) width.x_ = 1; \
    if ( width.y_ < 1 ) width.y_ = 1; \
    if ( width.z_ < thecs.zsamp_.step * 0.5 ) width.z_ = 1; \
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


void PlaneDataDisplay::setResolution( int res, TaskRunner* tskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;

    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, ExistingTaskRunnerProvider(tskr) );
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

    DPM(DataPackMgr::SeisID()).unRef( datapackids_[attrib] );
    datapackids_.removeSingle( attrib );

    DPM(DataPackMgr::SeisID()).unRef( transfdatapackids_[attrib] );
    transfdatapackids_.removeSingle( attrib );

    const SilentTaskRunnerProvider trprov;
    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, trprov );
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

    DPM(DataPackMgr::SeisID()).unRef( datapackids_[attrib] );
    datapackids_[attrib] = DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).unRef( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = DataPack::cNoID();

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr,
				SilentTaskRunnerProvider() );
}


bool PlaneDataDisplay::hasCache( int attrib ) const
{
    return (datapackids_[attrib] != DataPack::cNoID()) || rposcache_[attrib];
}


TrcKeyZSampling PlaneDataDisplay::getTrcKeyZSampling( int attrib ) const
{
    return getTrcKeyZSampling( true, false, attrib );
}


void PlaneDataDisplay::getTraceKeyPath( TrcKeyPath& path,TypeSet<Coord>* ) const
{
    path.erase();
    if ( orientation_==OD::ZSlice )
	return;

    const TrcKeyZSampling trczs = getTrcKeyZSampling( true, true, 0 );

    TrcKeySamplingIterator iter( trczs.hsamp_ );
    do
    {
	path += iter.curTrcKey();
    } while ( iter.next() );
}


Interval<float> PlaneDataDisplay::getDataTraceRange() const
{
    const TrcKeyZSampling tkzs = getTrcKeyZSampling( false, false, -1 );
    Interval<float> res;
    res.setFrom( tkzs.zsamp_ );
    return res;
}


TrcKeyZSampling PlaneDataDisplay::getDataPackSampling( int attrib ) const
{
    const DataPackMgr& dpm = DPM( DataPackMgr::SeisID() );
    const DataPack::ID dpid = getDataPackID( attrib );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    return regsdp ? TrcKeyZSampling(regsdp->subSel())
		  : getTrcKeyZSampling( attrib );
}


void PlaneDataDisplay::getRandomPos( DataPointSet& pos, TaskRunner* tskr ) const
{
    if ( !datatransform_ )
	return;

    const TrcKeyZSampling cs = getTrcKeyZSampling( true, true, 0 ); //attrib?
    ZAxisTransformPointGenerator generator( *datatransform_ );
    ExistingTaskRunnerProvider trprov( tskr );
    generator.setInput( cs, trprov );
    generator.setOutputDPS( pos );
    generator.execute();
}


void PlaneDataDisplay::setRandomPosData( int attrib, const DataPointSet* data,
					 const TaskRunnerProvider& trprov )
{
    if ( attrib>=nrAttribs() )
	return;

    setRandomPosDataNoCache( attrib, &data->bivSet(), trprov );

    if ( rposcache_[attrib] )
	delete rposcache_[attrib];

    rposcache_.replace( attrib, data ? new BinnedValueSet(data->bivSet()) : 0 );
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

    updateTexShiftAndGrowth();

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

    res.hsamp_.setIs3D();
    res.hsamp_.start_ = res.hsamp_.stop_ = BinID(mNINT32(c0.x_),mNINT32(c0.y_));
    res.zsamp_.start = res.zsamp_.stop = (float) c0.z_;
    res.hsamp_.include( BinID(mNINT32(c1.x_),mNINT32(c1.y_)) );
    res.zsamp_.include( (float) c1.z_ );
    res.hsamp_.step_ = BinID( s3dgeom_->inlRange().step,
			      s3dgeom_->crlRange().step );
    res.zsamp_.step = s3dgeom_->zRange().step;

    if ( manippos )
	res = snapPosition( res );

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
	    res.zsamp_.step = SI().zStep( OD::UsrWork );
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
				      TaskRunner* tskr )
{
    auto& dpm = DPM( DataPackMgr::SeisID() );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );

    if ( !regsdp || regsdp->isEmpty() )
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr,
				    SilentTaskRunnerProvider() );
	return false;
    }

    dpm.unRef( datapackids_[attrib] );
    datapackids_[attrib] = dpid;
    dpm.ref( dpid );

    createTransformedDataPack( attrib, tskr );
    updateChannels( attrib, ExistingTaskRunnerProvider(tskr) );
    datachanged_.trigger();
    return true;
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
		const BinnedValueSet* bivset, const TaskRunnerProvider& trprov )
{
    if ( !bivset || !datatransform_ )
	return;

    const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true, 0 );
    const DataPack::ID dpid = RegularSeisDataPack::createDataPackForZSlice(
	bivset, tkzs, datatransform_->toZDomainInfo(), userrefs_[attrib] );

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    dpm.unRef( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = dpid;
    dpm.ref( dpid );

    updateChannels( attrib, trprov );
}


void PlaneDataDisplay::updateChannels( int attrib,
					const TaskRunnerProvider& trprov )
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp ) return;

    updateTexOriginAndScale( attrib, TrcKeyZSampling(regsdp->subSel()) );

    const int nrversions = regsdp->nrComponents();
    channels_->setNrVersions( attrib, nrversions );

    typedef Array3D<float>::dim_idx_type dim_idx_type;
    const dim_idx_type dim0 = orientation_==OD::InlineSlice ? 1 : 0;
    const dim_idx_type dim1 = orientation_==OD::ZSlice ? 1 : 2;

    for ( int idx=0; idx<nrversions; idx++ )
    {
	const Array3D<float>& array = regsdp->data( idx );
	const int sz0 = 1 + (array.getSize(dim0)-1) * (resolution_+1);
	const int sz1 = 1 + (array.getSize(dim1)-1) * (resolution_+1);

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
		short orientation = mCast(short,orientation_);
							 //SliceTypeToShortHack
		slice2d.setPos( orientation, 0 );
		slice2d.init();

		UserShowWait usw( this, uiStrings::sCollectingData() );
		Array2DReSampler<float,float> resampler(
					    slice2d, tmparr, sz0, sz1, true );
		resampler.setInterpolate( true );
		TaskRunner::execute( 0, resampler );
	    }

	    arr = tmparr;
	    cp = OD::TakeOverPtr;
	}

	channels_->setSize( attrib, 1, sz0, sz1 );
	channels_->setUnMappedData( attrib, idx, arr, cp,
				    SilentTaskRunnerProvider() );
    }

    if ( !getUpdateStageNr() )
	updateTexShiftAndGrowth();

    channels_->turnOn( true );
}


void PlaneDataDisplay::createTransformedDataPack( int attrib, TaskRunner* tskr )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDataPackID( attrib );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp || regsdp->isEmpty() )
	return;

    RefMan<VolumeDataPack> transformed = 0;
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true );
	if ( datatransform_->needsVolumeOfInterest() )
	{
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );
	    ExistingTaskRunnerProvider trprov( tskr );
	    datatransform_->loadDataIfMissing( voiidx_, trprov );
	}

	VolumeDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( regsdp.ptr() );
	transformer.setInterpolate( textureInterpolationEnabled() );
	transformer.setOutputZRange( tkzs.zsamp_ );
	transformer.execute();

        transformed = transformer.getOutput();
    }

    dpm.unRef( transfdatapackids_[attrib] );
    transfdatapackids_[attrib] = transformed
	? transformed->id()
	: DataPack::cNoID();

    refPtr( transformed );
}


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
	info = "Z-slice: ";
	info += mNINT32(val * zdinf.userFactor());
    }
}


bool PlaneDataDisplay::getCacheValue( int attrib, int version,
				      const Coord3& pos, float& val ) const
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPack::ID dpid = getDisplayedDataPackID( attrib );
    auto regsdp = dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp || regsdp->isEmpty() )
	return false;

    const TrcKeyZSampling tkzs( regsdp->subSel() );
    const BinID bid = s3dgeom_->transform( pos.getXY() );
    const int inlidx = tkzs.hsamp_.inlRange().nearestIndex( bid.inl() );
    const int crlidx = tkzs.hsamp_.crlRange().nearestIndex( bid.crl() );
    const int zidx = tkzs.zsamp_.nearestIndex( pos.z_ );
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


void PlaneDataDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    dragger_->setRightHandSystem( yn );
}


void PlaneDataDisplay::updateMouseCursorCB( CallBacker* cb )
{
    if ( !isManipulatorShown() || !isOn() || isLocked() )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
	initAdaptiveMouseCursor( cb, id(), dragger_->getTransDragKeys(false),
				 mousecursor_ );
}


SurveyObject* PlaneDataDisplay::duplicate( TaskRunner* tskr ) const
{
    PlaneDataDisplay* pdd = new PlaneDataDisplay();
    pdd->setOrientation( orientation_ );
    pdd->setTrcKeyZSampling( getTrcKeyZSampling(false,true,0) );
    pdd->setZAxisTransform( datatransform_, tskr );

    while ( nrAttribs() > pdd->nrAttribs() )
	pdd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	const Attrib::SelSpecList* selspecs = getSelSpecs( idx );
	if ( selspecs )
	    pdd->setSelSpecs( idx, *selspecs );

	pdd->setDataPackID( idx, getDataPackID(idx), tskr );
	pdd->setColTabMapper( idx, getColTabMapper(idx), tskr );
	pdd->setColTabSequence( idx, getColTabSequence( idx ), tskr );
    }

    return pdd;
}


void PlaneDataDisplay::fillPar( IOPar& par ) const
{
    MultiTextureSurveyObject::fillPar( par );

    par.set( sKeyOrientation(), toString( orientation_) );
    getTrcKeyZSampling( false, true ).fillPar( par );
}


bool PlaneDataDisplay::usePar( const IOPar& par )
{
    if ( !MultiTextureSurveyObject::usePar( par ) )
	return false;

    SliceType orientation = OD::InlineSlice;
    FixedString orstr = par.find( sKeyOrientation() );
    if ( !SliceTypeDef().parse(orstr,orientation) && orstr == "Timeslice" )
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
	updateTexShiftAndGrowth();
	dragger_->showPlane( false );
	dragger_->showDraggerBorder( true );
    }
    else if ( !getUpdateStageNr() )
    {
	updatestageinfo_.oldtkzs_ = getTrcKeyZSampling( false, true );
	updatestageinfo_.oldorientation_ = orientation_;
	updatestageinfo_.refreeze_ = true;
    }
    else if ( updatestageinfo_.refreeze_ )
	texturerect_->freezeDisplay( false );	// thaw to refreeze

    texturerect_->freezeDisplay( yn );
    SurveyObject::annotateNextUpdateStage( yn );
}


void PlaneDataDisplay::updateTexShiftAndGrowth()
{
    const Interval<float> erg0 = channels_->getEnvelopeRange( 0 );
    const Interval<float> erg1 = channels_->getEnvelopeRange( 1 );

    if ( erg0.isUdf() || erg1.isUdf() )
	return;

    const TrcKeyZSampling tkzs = getTrcKeyZSampling( false, true );
    const TrcKeyZSampling& oldtkzs = updatestageinfo_.oldtkzs_;

    const TrcKeyZSampling si( OD::UsrWork );
    const float resolutionfactor = mCast( float, resolution_+1 );

    const int inldif = tkzs.hsamp_.start_.inl() - si.hsamp_.start_.inl();
    const float inlfactor = resolutionfactor / si.hsamp_.step_.inl();

    const int crldif = tkzs.hsamp_.start_.crl() - si.hsamp_.start_.crl();
    const float crlfactor = resolutionfactor / si.hsamp_.step_.crl();

    const float zdif = tkzs.zsamp_.start - si.zsamp_.start;
    const float zfactor = resolutionfactor / si.zsamp_.step;

    Coord startdif( zdif*zfactor - erg0.start, inldif*inlfactor - erg1.start );
    Coord growth( tkzs.zsamp_.width()*zfactor - erg0.width(),
		  tkzs.hsamp_.lineRange().width()*inlfactor - erg1.width() );

    bool refreeze = tkzs.hsamp_.start_.crl()==oldtkzs.hsamp_.start_.crl();

    if ( orientation_ == OD::InlineSlice )
    {
	startdif.y_ = crldif * crlfactor - erg1.start;
	growth.y_ = tkzs.hsamp_.trcRange().width()*crlfactor - erg1.width();
	refreeze = tkzs.hsamp_.start_.inl()==oldtkzs.hsamp_.start_.inl();
    }

    if ( orientation_ == OD::ZSlice )
    {
	startdif.x_ = crldif * crlfactor - erg0.start;
	growth.x_ = tkzs.hsamp_.trcRange().width()*crlfactor - erg0.width();
	refreeze = tkzs.zsamp_.start==oldtkzs.zsamp_.start;
    }

    if ( getUpdateStageNr() == 1 )
    {
	if ( updatestageinfo_.oldorientation_ != orientation_ )
	{
	    updatestageinfo_.refreeze_ = true;
	    const int texborderval = si.nrInl() + si.nrCrl() + si.nrZ();
	    startdif = Coord(texborderval,texborderval) * resolutionfactor;
	}
	else
	    updatestageinfo_.refreeze_ = refreeze;
    }

    texturerect_->setTextureGrowth( growth );
    texturerect_->setTextureShift( -startdif - growth*0.5 );
}


void PlaneDataDisplay::updateTexOriginAndScale( int attrib,
						const TrcKeyZSampling& tkzs )
{
    if ( !tkzs.isDefined() || tkzs.isEmpty() )
	return;

    const TrcKeyZSampling si( OD::UsrWork );
    const float resolutionfactor = mCast( float, resolution_+1 );

    const int inldif = tkzs.hsamp_.start_.inl() - si.hsamp_.start_.inl();
    const float inlfactor = resolutionfactor / si.hsamp_.step_.inl();

    const int crldif = tkzs.hsamp_.start_.crl() - si.hsamp_.start_.crl();
    const float crlfactor = resolutionfactor / si.hsamp_.step_.crl();

    const float zdif = tkzs.zsamp_.start - si.zsamp_.start;
    const float zfactor = resolutionfactor / si.zsamp_.step;

    Coord origin( zdif * zfactor, inldif * inlfactor );

    Coord scale( tkzs.zsamp_.step / si.zsamp_.step,
		 tkzs.hsamp_.step_.inl() / si.hsamp_.step_.inl() );

    if ( orientation_ == OD::InlineSlice )
    {
	origin.y_ = crldif * crlfactor;
	scale.y_ = tkzs.hsamp_.step_.crl() / si.hsamp_.step_.crl();
    }

    if ( orientation_ == OD::ZSlice )
    {
	origin.x_ = crldif * crlfactor;
	scale.x_ = tkzs.hsamp_.step_.crl() / si.hsamp_.step_.crl();
    }

    channels_->setOrigin( attrib, origin );
    channels_->setScale( attrib, scale );
}


} // namespace visSurvey
