/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visplanedatadisplay.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "array2dresample.h"
#include "color.h"
#include "datapointset.h"
#include "seisdatapackzaxistransformer.h"
#include "settings.h"

#include "visevent.h"
#include "visgridlines.h"
#include "vismaterial.h"
#include "vistexturechannels.h"
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

    const char* getStandardDesc() const override
    { return "Move plane data"; }


    bool unDo() override
    { return pdd_->updatePlanePos( starttkz_ ); }


    bool reDo() override
    { return pdd_->updatePlanePos( endtkz_ ); }

private:
    PlaneDataDisplay*	  pdd_;
    const TrcKeyZSampling starttkz_;
    const TrcKeyZSampling endtkz_;
};


mDefineEnumUtils(PlaneDataDisplay,SliceType,"Orientation")
{ "Inline", "Crossline", "Z-slice", nullptr };


//PlaneDataDisplay
PlaneDataDisplay::PlaneDataDisplay()
    : MultiTextureSurveyObject()
    , dragger_(visBase::DepthTabPlaneDragger::create())
    , gridlines_(visBase::GridLines::create())
    , curicstep_(s3dgeom_->inlStep(),s3dgeom_->crlStep())
    , csfromsession_(false)
    , undo_(*new Undo())
    , moving_(this)
    , movefinished_(this)
    , datachanged_( this )
{
    datapacks_.setNullAllowed();
    transfdatapacks_.setNullAllowed();

    texturerect_ = visBase::TextureRectangle::create();
    addChild( texturerect_->osgNode() );

    texturerect_->setTextureChannels( channels_ );

    addChild( dragger_->osgNode() );

    rposcache_.setNullAllowed();

    dragger_->ref();
    mAttachCB( dragger_->started, PlaneDataDisplay::draggerStart );
    mAttachCB( dragger_->motion, PlaneDataDisplay::draggerMotion );
    mAttachCB( dragger_->finished, PlaneDataDisplay::draggerFinish );
    mAttachCB( dragger_->rightClicked(), PlaneDataDisplay::draggerRightClick );

    dragger_->setDim( (int) 0 );

    if ( (int) orientation_ )
	dragger_->setDim( (int) orientation_ );

    material_->setColor( OD::Color::White() );

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
    if ( !scene_ )
	return mUdf(float);

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pos, xytpos );
    const BinID binid = s3dgeom_->transform( Coord(xytpos.x,xytpos.y) );

    const TrcKeyZSampling cs = getTrcKeyZSampling(false,true);

    BinID inlcrldist( 0, 0 );

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
    float zdiff = 0.f;
    if ( !cs.zsamp_.includes(xytpos.z,false) )
    {
	zdiff = (float)(mMIN(fabs(xytpos.z - cs.zsamp_.start),
			     fabs(xytpos.z - cs.zsamp_.stop) ));
	zdiff *= getZScale() * scene_->getFixedZStretch();
    }

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
    const float zfactor = getZScale();
    float maxzdist = zfactor * s3dgeom_->zStep() / 2;
    if ( scene_ )
	maxzdist *= scene_->getFixedZStretch();

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
	if ( rposcache_[idx] )
	    setRandomPosDataNoCache( idx, rposcache_[idx], 0 );
	else
	    createTransformedDataPack( idx, 0 );

	updateChannels( idx, 0 );
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
    const TrcKeyZSampling curtkzs = getTrcKeyZSampling( false, true );
    const TrcKeyZSampling maniptkzs = getTrcKeyZSampling( true, true );

    if ( !curtkzs.isEqual(maniptkzs) )
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
    rposcache_ += nullptr;
    datapacks_ += nullptr;
    transfdatapacks_ += nullptr;
}


void PlaneDataDisplay::removeCache( int attrib )
{
    delete rposcache_.removeSingle( attrib );
    datapacks_.removeSingle( attrib );
    transfdatapacks_.removeSingle( attrib );

    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, 0 );
}


void PlaneDataDisplay::swapCache( int a0, int a1 )
{
    rposcache_.swap( a0, a1 );
    datapacks_.swap( a0, a1 );
    transfdatapacks_.swap( a0, a1 );
}


void PlaneDataDisplay::emptyCache( int attrib )
{
    delete rposcache_.replace( attrib, nullptr );
    datapacks_.replace( attrib, nullptr );
    transfdatapacks_.replace( attrib, nullptr );

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, nullptr );
}


bool PlaneDataDisplay::hasCache( int attrib ) const
{
    return datapacks_[attrib] || rposcache_[attrib];
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
    TrcKey curkey = TrcKey::udf();
    while ( iter.next(curkey) )
    {
	path += curkey;
    }
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
    const DataPackID dpid = getDataPackID( attrib );
    ConstRefMan<RegularSeisDataPack> regsdp =
				     dpm.get<RegularSeisDataPack>( dpid );
    const TrcKeyZSampling tkzs =
	regsdp ? regsdp->sampling() : getTrcKeyZSampling( attrib );
    return tkzs;
}


void PlaneDataDisplay::getRandomPos( DataPointSet& pos, TaskRunner* taskr )const
{
    if ( !datatransform_ ) return;

    const TrcKeyZSampling cs = getTrcKeyZSampling( true, true, 0 ); //attrib?
    ZAxisTransformPointGenerator generator( *datatransform_ );
    generator.setInput( cs, taskr );
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
	if ( halfsize.isNull() )
	{
	    res.setEmpty();
	    return res;
	}

	halfsize[orientation_] = 0;

	c0 = center + halfsize;
	c1 = center - halfsize;
    }

    res.hsamp_.init( s3dgeom_->getID() );
    res.hsamp_.start_ = res.hsamp_.stop_ = BinID(mNINT32(c0.x),mNINT32(c0.y) );
    res.hsamp_.include( BinID(mNINT32(c1.x),mNINT32(c1.y)) );
    res.zsamp_.start = res.zsamp_.stop = (float) c0.z;
    res.zsamp_.include( (float) c1.z );

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


bool PlaneDataDisplay::setDataPackID( int attrib, DataPackID dpid,
				      TaskRunner* taskr )
{
    DataPackMgr& dpm = DPM( DataPackMgr::SeisID() );
    RefMan<RegularSeisDataPack> regsdp =
				     dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp || regsdp->isEmpty() )
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, nullptr );
	return false;
    }

    datapacks_.replace( attrib, regsdp );

    createTransformedDataPack( attrib, taskr );
    updateChannels( attrib, taskr );
    datachanged_.trigger();
    return true;
}


DataPackID PlaneDataDisplay::getDataPackID( int attrib ) const
{
    return datapacks_.validIdx(attrib) && datapacks_[attrib]
		? datapacks_[attrib]->id()
		: DataPack::cNoID();
}


DataPackID PlaneDataDisplay::getDisplayedDataPackID( int attrib ) const
{
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	return transfdatapacks_.validIdx(attrib) && transfdatapacks_[attrib]
	    ? transfdatapacks_[attrib]->id()
	    : DataPack::cNoID();
    }

    return getDataPackID( attrib );
}


void PlaneDataDisplay::setRandomPosDataNoCache( int attrib,
			const BinIDValueSet* bivset, TaskRunner* taskr )
{
    if ( !bivset || !datatransform_ )
	return;

    const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true, 0 );
    const DataPackID dpid = RegularSeisDataPack::createDataPackForZSlice(
	bivset, tkzs, datatransform_->toZDomainInfo(), userrefs_[attrib] );

    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    RefMan<RegularSeisDataPack> regsdp = dpm.get<RegularSeisDataPack>( dpid );

    transfdatapacks_.replace( attrib, regsdp );

    updateChannels( attrib, taskr );
}


void PlaneDataDisplay::updateChannels( int attrib, TaskRunner* taskr )
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPackID dpid = getDisplayedDataPackID( attrib );
    ConstRefMan<RegularSeisDataPack> regsdp =
				     dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp )
	return;

    updateTexOriginAndScale( attrib, regsdp->sampling() );

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

		MouseCursorChanger mousecursorchanger( MouseCursor::Wait );
		Array2DReSampler<float,float> resampler(
					    slice2d, tmparr, sz0, sz1, true );
		resampler.setInterpolate( true );
		TaskRunner::execute( 0, resampler );
	    }

	    arr = tmparr;
	    cp = OD::TakeOverPtr;
	}

	channels_->setSize( attrib, 1, sz0, sz1 );
	channels_->setUnMappedData( attrib, idx, arr, cp, nullptr,
				    interactivetexturedisplay_ );
    }

    if ( !getUpdateStageNr() )
	updateTexShiftAndGrowth();

    channels_->turnOn( true );
}


void PlaneDataDisplay::createTransformedDataPack( int attrib, TaskRunner* taskr)
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPackID dpid = getDataPackID( attrib );
    ConstRefMan<RegularSeisDataPack> regsdp =
				     dpm.get<RegularSeisDataPack>( dpid );
    if ( !regsdp || regsdp->isEmpty() )
	return;

    RefMan<RegularSeisDataPack> transformed;
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true );
	if ( datatransform_->needsVolumeOfInterest() )
	{
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );
	    datatransform_->loadDataIfMissing( voiidx_, taskr );
	}

	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( regsdp.ptr() );
	transformer.setInterpolate( textureInterpolationEnabled() );
	transformer.setOutputZRange( tkzs.zsamp_ );
	transformer.execute();

	transformed = transformer.getOutput();
    }

    transfdatapacks_.replace( attrib, transformed );
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
    const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true );
    if ( orientation_==OD::InlineSlice )
    {
	info = "In-line: ";
	info += tkzs.hsamp_.start_.inl();
    }
    else if ( orientation_==OD::CrosslineSlice )
    {
	info = "Cross-line: ";
	info += tkzs.hsamp_.start_.crl();
    }
    else
    {
	const float val = tkzs.zsamp_.start;
	if ( !scene_ ) { info = val; return; }

	const ZDomain::Info& zdinf = scene_->zDomainInfo();
	info = mFromUiStringTodo(zdinf.userName()); info += ": ";

	const float userval = tkzs.zsamp_.step * zdinf.userFactor();
	const int nrdec = Math::NrSignificantDecimals( userval );
	info.add( val*zdinf.userFactor(), nrdec );
    }
}


bool PlaneDataDisplay::getCacheValue( int attrib, int version,
				      const Coord3& pos, float& val ) const
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    const DataPackID dpid = getDisplayedDataPackID( attrib );
    ConstRefMan<RegularSeisDataPack> regsdp =
				     dpm.get<RegularSeisDataPack>( dpid );
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

	const TypeSet<Attrib::SelSpec>* selspecs = getSelSpecs( idx );
	if ( selspecs ) pdd->setSelSpecs( idx, *selspecs );

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
    const BufferString orstr = par.find( sKeyOrientation() );
    if ( !parseEnumSliceType(orstr,orientation) && orstr.isEqual("Timeslice") )
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
	updatestageinfo_.oldcs_ = getTrcKeyZSampling( false, true );
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
    const TrcKeyZSampling& oldtkzs = updatestageinfo_.oldcs_;

    const TrcKeyZSampling& si = SI().sampling( true );
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
	startdif.y = crldif * crlfactor - erg1.start;
	growth.y = tkzs.hsamp_.trcRange().width()*crlfactor - erg1.width();
	refreeze = tkzs.hsamp_.start_.inl()==oldtkzs.hsamp_.start_.inl();
    }

    if ( orientation_ == OD::ZSlice )
    {
	startdif.x = crldif * crlfactor - erg0.start;
	growth.x = tkzs.hsamp_.trcRange().width()*crlfactor - erg0.width();
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

    const TrcKeyZSampling& si = SI().sampling( true );
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
	origin.y = crldif * crlfactor;
	scale.y = (float)tkzs.hsamp_.step_.crl() / si.hsamp_.step_.crl();
    }

    if ( orientation_ == OD::ZSlice )
    {
	origin.x = crldif * crlfactor;
	scale.x = (float)tkzs.hsamp_.step_.crl() / si.hsamp_.step_.crl();
    }

    channels_->setOrigin( attrib, origin );
    channels_->setScale( attrib, scale );
}


} // namespace visSurvey
