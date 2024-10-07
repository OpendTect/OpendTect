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
#include "flatview.h"
#include "seisdatapackzaxistransformer.h"
#include "settings.h"
#include "uistrings.h"

#include "vismaterial.h"
#include "vistexturechannels.h"
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
    , curicstep_(s3dgeom_->inlStep(),s3dgeom_->crlStep())
    , csfromsession_(false)
    , undo_(*new Undo())
    , moving_(this)
    , movefinished_(this)
    , datachanged_(this)
{
    ref();
    datapacks_.setNullAllowed();
    transformedpacks_.setNullAllowed();

    texturerect_ = visBase::TextureRectangle::create();
    addChild( texturerect_->osgNode() );

    texturerect_->setTextureChannels( channels_.ptr() );

    dragger_ = visBase::DepthTabPlaneDragger::create();
    addChild( dragger_->osgNode() );

    rposcache_.setNullAllowed();

    mAttachCB( dragger_->started, PlaneDataDisplay::draggerStart );
    mAttachCB( dragger_->motion, PlaneDataDisplay::draggerMotion );
    mAttachCB( dragger_->finished, PlaneDataDisplay::draggerFinish );
    mAttachCB( dragger_->rightClicked(), PlaneDataDisplay::draggerRightClick );

    dragger_->setDim( (int) 0 );

    if ( (int) orientation_ )
	dragger_->setDim( (int) orientation_ );

    material_->setColor( OD::Color::White() );

    gridlines_ = visBase::GridLines::create();
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
    unRefNoDelete();
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( nullptr );
    deepErase( rposcache_ );
    setZAxisTransform( nullptr, nullptr );

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
                Interval<float>( 4*survey.zsamp_.step_, mUdf(float) ) );

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
	if ( orientation_==OD::SliceType::Z && datatransform_ && resetz )
	{
	    const float center = survey.zsamp_.snappedCenter();
	    if ( !mIsUdf(center) )
                newpos.zsamp_.start_ = newpos.zsamp_.stop_ = center;
	}
    }

    newpos = snapPosition( newpos );

    if ( newpos!=getTrcKeyZSampling(false,true) )
	setTrcKeyZSampling( newpos );
}


TrcKeyZSampling PlaneDataDisplay::snapPosition( const TrcKeyZSampling& cs,
						bool onlyic ) const
{
    TrcKeyZSampling res( cs );
    const Interval<float> inlrg( mCast(float,res.hsamp_.start_.inl()),
				 mCast(float,res.hsamp_.stop_.inl()) );
    const Interval<float> crlrg( mCast(float,res.hsamp_.start_.crl()),
				 mCast(float,res.hsamp_.stop_.crl()) );

    res.hsamp_.snapToSurvey();
    if ( orientation_==OD::SliceType::Inline )
    {
	res.hsamp_.start_.inl() = s3dgeom_->inlRange().snap( inlrg.center() );
	res.hsamp_.stop_.inl() = res.hsamp_.start_.inl();
    }
    else if ( orientation_==OD::SliceType::Crossline )
    {
	res.hsamp_.start_.crl() = s3dgeom_->crlRange().snap( crlrg.center() );
	res.hsamp_.stop_.crl() = res.hsamp_.start_.crl();
    }

    if ( scene_ && !onlyic )
    {
	const Interval<float> zrg( res.zsamp_ );
	const ZSampling& scenezrg = scene_->getTrcKeyZSampling().zsamp_;
	res.zsamp_.limitTo( scenezrg );
        res.zsamp_.start_ = scenezrg.snap( res.zsamp_.start_ );
        res.zsamp_.stop_ = scenezrg.snap( res.zsamp_.stop_ );

	if ( orientation_!=OD::SliceType::Inline &&
			    orientation_!=OD::SliceType::Crossline )
            res.zsamp_.start_ = res.zsamp_.stop_ = scenezrg.snap(zrg.center());
    }

    return res;
}


Coord3 PlaneDataDisplay::getNormal( const Coord3& pos  ) const
{
    if ( orientation_==OD::SliceType::Z )
	return Coord3(0,0,1);

    return Coord3( orientation_==OD::SliceType::Inline
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
    const BinID binid = s3dgeom_->transform( Coord(xytpos.x_,xytpos.y_) );

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
    if ( !cs.zsamp_.includes(xytpos.z_,false) )
    {
        zdiff = (float)(mMIN(fabs(xytpos.z_ - cs.zsamp_.start_),
                             fabs(xytpos.z_ - cs.zsamp_.stop_) ));
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

    return orientation_==OD::SliceType::Z ? maxzdist
					  : SurveyObject::sDefMaxDist();
}


bool PlaneDataDisplay::setZAxisTransform( ZAxisTransform* zat,
					  TaskRunner* taskr )
{
    const bool haddatatransform = datatransform_ || displaytrans_;
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
    }

    datatransform_ = zat;
    if ( datatransform_ )
    {
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,PlaneDataDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* PlaneDataDisplay::getZAxisTransform() const
{
    return datatransform_.ptr();
}


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
	    setRandomPosDataNoCache( idx, rposcache_[idx], nullptr );
	else
	    createTransformedDataPack( idx, nullptr );

	updateChannels( idx, nullptr );
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
    if ( orientation_==OD::SliceType::Inline
	    && dragcs.hsamp_.start_.inl()!=oldcs.hsamp_.start_.inl() )
	showplane = true;
    else if ( orientation_==OD::SliceType::Crossline &&
	      dragcs.hsamp_.start_.crl()!=oldcs.hsamp_.start_.crl() )
	showplane = true;
    else if ( orientation_==OD::SliceType::Z &&
              dragcs.zsamp_.start_!=oldcs.zsamp_.start_ )
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


void PlaneDataDisplay::draggerRightClick( CallBacker* )
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
    if ( width.z_ < thecs.zsamp_.step_ * 0.5 ) width.z_ = 1; \
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
    if ( inEditMode() )
	yn = false;

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
    if ( inEditMode() )
	return;

    TrcKeyZSampling cs = getTrcKeyZSampling( true, true );
    setTrcKeyZSampling( cs );

    if ( !getUpdateStageNr() )
    {
	dragger_->showPlane( false );
	dragger_->showDraggerBorder( true );
    }
}


uiString PlaneDataDisplay::getManipulationString() const
{
    uiString str;
    getObjectInfo( str );
    return str;
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

    return datatransform_ && orientation_==OD::SliceType::Z
	? SurveyObject::RandomPos : SurveyObject::Cube;
}


void PlaneDataDisplay::addCache()
{
    rposcache_ += nullptr;
    datapacks_ += nullptr;
    transformedpacks_ += nullptr;
}


void PlaneDataDisplay::removeCache( int attrib )
{
    delete rposcache_.removeSingle( attrib );
    datapacks_.removeSingle( attrib );
    transformedpacks_.removeSingle( attrib );

    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateChannels( idx, nullptr );
}


void PlaneDataDisplay::swapCache( int a0, int a1 )
{
    rposcache_.swap( a0, a1 );
    datapacks_.swap( a0, a1 );
    transformedpacks_.swap( a0, a1 );
}


void PlaneDataDisplay::emptyCache( int attrib )
{
    delete rposcache_.replace( attrib, nullptr );
    datapacks_.replace( attrib, nullptr );
    transformedpacks_.replace( attrib, nullptr );

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, nullptr );
}


bool PlaneDataDisplay::hasCache( int attrib ) const
{
    return datapacks_[attrib] || rposcache_[attrib];
}


TrcKeyZSampling PlaneDataDisplay::getTrcKeyZSampling( bool displayspace,
							int attrib ) const
{
    return getTrcKeyZSampling( true, displayspace, attrib );
}


void PlaneDataDisplay::getTraceKeyPath( TrcKeySet& path,TypeSet<Coord>* ) const
{
    path.erase();
    if ( orientation_==OD::SliceType::Z )
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
    ConstRefMan<RegularSeisDataPack> regsdp = getVolumeDataPack( attrib );
    const TrcKeyZSampling tkzs =
		    regsdp ? regsdp->sampling() : getTrcKeyZSampling( attrib );
    return tkzs;
}


bool PlaneDataDisplay::getRandomPos( DataPointSet& pos, TaskRunner* taskr )const
{
    if ( !datatransform_ )
	return false;

    const TrcKeyZSampling cs = getTrcKeyZSampling( true, true, 0 ); //attrib?
    ZAxisTransformPointGenerator generator( *datatransform_.getNonConstPtr() );
    generator.setInput( cs, taskr );
    generator.setOutputDPS( pos );
    return generator.execute();
}


bool PlaneDataDisplay::setRandomPosData( int attrib, const DataPointSet* data,
					 TaskRunner* taskr )
{
    if ( attrib < 0 || attrib>=nrAttribs() )
	return false;

    if ( !setRandomPosDataNoCache(attrib,&data->bivSet(),taskr) )
	return false;

    if ( rposcache_[attrib] )
	delete rposcache_[attrib];

    rposcache_.replace( attrib, data ? new BinIDValueSet(data->bivSet())
				     : nullptr );
    return true;
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
	halfsize[sCast(int,orientation_)] = 0;

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

	halfsize[sCast(int,orientation_)] = 0;

	c0 = center + halfsize;
	c1 = center - halfsize;
    }

    res.hsamp_.init( s3dgeom_->getID() );
    res.hsamp_.start_ = res.hsamp_.stop_ = BinID(mNINT32(c0.x_),mNINT32(c0.y_));
    res.hsamp_.include( BinID(mNINT32(c1.x_),mNINT32(c1.y_)) );
    res.zsamp_.start_ = res.zsamp_.stop_ = (float) c0.z_;
    res.zsamp_.include( (float) c1.z_ );

    if ( manippos )
	res = snapPosition( res, true );

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
	if ( displayspace )
	{
	    if ( scene_ )
                res.zsamp_.step_ = scene_->getTrcKeyZSampling().zsamp_.step_;
	    else
                res.zsamp_.step_ = datatransform_->getZInterval( false ).step_;
	}
	else
	    res.zsamp_ = datatransform_->getZInterval( true );
    }

    return res;
}


bool PlaneDataDisplay::setVolumeDataPack( int attrib, VolumeDataPack* voldp,
					TaskRunner* taskr )
{
    mDynamicCastGet(RegularSeisDataPack*,regseisdp,voldp);
    datapacks_.replace( attrib, regseisdp );
    if ( !regseisdp || regseisdp->isEmpty() )
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, nullptr );
	return false;
    }

    createTransformedDataPack( attrib, taskr );
    updateChannels( attrib, taskr );
    datachanged_.trigger();
    return true;
}


ConstRefMan<DataPack> PlaneDataDisplay::getDataPack( int attrib ) const
{
    return getVolumeDataPack( attrib );
}


ConstRefMan<VolumeDataPack>
			PlaneDataDisplay::getVolumeDataPack( int attrib ) const
{
    return getNonConst(*this).getVolumeDataPack( attrib );
}


RefMan<VolumeDataPack> PlaneDataDisplay::getVolumeDataPack( int attrib )
{
    if ( !datapacks_.validIdx(attrib) || !datapacks_[attrib] )
	return nullptr;

    return datapacks_[attrib];
}


ConstRefMan<VolumeDataPack> PlaneDataDisplay::getDisplayedVolumeDataPack(
							int attrib ) const
{
    return getNonConst(*this).getDisplayedVolumeDataPack( attrib );
}


RefMan<VolumeDataPack>
		PlaneDataDisplay::getDisplayedVolumeDataPack( int attrib )
{
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	if ( !transformedpacks_.validIdx(attrib) || !transformedpacks_[attrib] )
	    return nullptr;

	return transformedpacks_[attrib];
    }

    return getVolumeDataPack( attrib );
}


bool PlaneDataDisplay::setRandomPosDataNoCache( int attrib,
			const BinIDValueSet* bivset, TaskRunner* taskr )
{
    if ( !bivset || !datatransform_ )
	return false;

    const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true, 0 );
    RefMan<RegularSeisDataPack> regsdp =
	RegularSeisDataPack::createDataPackForZSliceRM( bivset, tkzs,
			datatransform_->toZDomainInfo(), userrefs_[attrib] );

    transformedpacks_.replace( attrib, regsdp.ptr() );

    updateChannels( attrib, taskr );
    return true;
}


void PlaneDataDisplay::updateChannels( int attrib, TaskRunner* taskr )
{
    ConstRefMan<VolumeDataPack> voldp = getDisplayedVolumeDataPack( attrib );
    mDynamicCastGet(const RegularSeisDataPack*,regsdp,voldp.ptr());
    if ( !regsdp )
	return;

    updateTexOriginAndScale( attrib, regsdp->sampling() );

    const int nrversions = regsdp->nrComponents();
    channels_->setNrVersions( attrib, nrversions );

    const int dim0 = orientation_==OD::SliceType::Inline ? 1 : 0;
    const int dim1 = orientation_==OD::SliceType::Z ? 1 : 2;

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
		slice2d.setPos( sCast(int,orientation_), 0 );
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
    ConstRefMan<RegularSeisDataPack> regsdp = getVolumeDataPack( attrib );
    if ( !regsdp || regsdp->isEmpty() )
	return;

    RefMan<RegularSeisDataPack> transformed;
    if ( datatransform_ && !alreadyTransformed(attrib) )
    {
	datatransform_->setDataFromZDomainInfo( regsdp->zDomain() );
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

    transformedpacks_.replace( attrib, transformed.ptr() );
}


visBase::GridLines* PlaneDataDisplay::gridlines()
{
    return gridlines_.ptr();
}


void PlaneDataDisplay::getMousePosInfo( const visBase::EventInfo&,
					Coord3& pos,
					BufferString& val,
					uiString& info ) const
{
    info = getManipulationString();
    getValueString( pos, val );
}


void PlaneDataDisplay::getObjectInfo( uiString& info ) const
{
    const TrcKeyZSampling tkzs = getTrcKeyZSampling( true, true );
    if ( orientation_==OD::SliceType::Inline )
    {
	info.set( uiStrings::sInline() )
	    .addMoreInfo( tkzs.hsamp_.start_.inl() );
    }
    else if ( orientation_==OD::SliceType::Crossline )
    {
	info.set( uiStrings::sCrossline() )
	    .addMoreInfo( tkzs.hsamp_.start_.crl() );
    }
    else if ( orientation_==OD::SliceType::Z )
    {
	const Scene* scene = getScene();
	const ZDomain::Info& datazdom = scene ? scene->zDomainInfo()
					      : SI().zDomainInfo();
	const ZDomain::Info& displayzdom = datazdom.isDepth()
			? ZDomain::DefaultDepth( true ) : datazdom;
	const float zval = tkzs.zsamp_.start_ *
			FlatView::Viewer::userFactor( datazdom, &displayzdom );
	const int nrzdec = displayzdom.nrDecimals( tkzs.zsamp_.step_ );
	info.set( displayzdom.getLabel() )
	    .addMoreInfo( ::toUiStringDec(zval,nrzdec) );
    }
}


bool PlaneDataDisplay::getCacheValue( int attrib, int version,
				      const Coord3& pos, float& val ) const
{
    ConstRefMan<VolumeDataPack> voldp = getDisplayedVolumeDataPack( attrib );
    mDynamicCastGet(const RegularSeisDataPack*,regsdp,voldp.ptr());
    if ( !regsdp || regsdp->isEmpty() )
	return false;

    const TrcKeyZSampling& tkzs = regsdp->sampling();
    const BinID bid = SI().transform( pos );
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
    return orientation_ != OD::SliceType::Z;
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
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
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


PlaneDataDisplay* PlaneDataDisplay::createTransverseSection( const Coord3& pos,
						OD::SliceType slicetype ) const
{
    if ( slicetype == orientation_ )
    {
	pErrMsg("Cannot create transverse section with the same orientation");
	return nullptr;
    }

    if ( !scene_ )
	return nullptr;

    TrcKey tk( SI().transform(pos) );
    float zpos = mCast( float, pos.z_ );
    const TrcKeyZSampling mytkzs( getTrcKeyZSampling() );
    tk = mytkzs.hsamp_.getNearest( tk );
    zpos = mytkzs.zsamp_.snap( zpos );

    TrcKeyZSampling newtkzs = scene_->getTrcKeyZSampling();
    if ( slicetype == OD::SliceType::Inline )
    {
	newtkzs.hsamp_.setLineRange( Interval<int>(tk.inl(),tk.inl()) );
	if ( orientation_ == OD::SliceType::Crossline )
	    newtkzs.zsamp_ = mytkzs.zsamp_;
	else
	    newtkzs.hsamp_.setTrcRange( mytkzs.hsamp_.trcRange() );
    }
    else if ( slicetype == OD::SliceType::Crossline )
    {
	newtkzs.hsamp_.setTrcRange( Interval<int>(tk.crl(),tk.crl()) );
	if ( orientation_ == OD::SliceType::Inline )
	    newtkzs.zsamp_ = mytkzs.zsamp_;
	else
	    newtkzs.hsamp_.setLineRange( mytkzs.hsamp_.lineRange() );
    }
    else // OD::SliceType::Z
    {
	newtkzs.zsamp_.start_ = newtkzs.zsamp_.stop_ = zpos;
	if ( orientation_ == OD::SliceType::Inline )
	    newtkzs.hsamp_.setTrcRange( mytkzs.hsamp_.trcRange() );
	else
	    newtkzs.hsamp_.setLineRange( mytkzs.hsamp_.lineRange() );
    }

    auto newpdd = new PlaneDataDisplay;
    newpdd->setOrientation( slicetype );
    newpdd->setTrcKeyZSampling( newtkzs );
    newpdd->setZAxisTransform( datatransform_.getNonConstPtr(), nullptr );

    while ( nrAttribs() > newpdd->nrAttribs() )
	newpdd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( !getSelSpec(idx) )
	    continue;

	const TypeSet<Attrib::SelSpec>* selspecs = getSelSpecs( idx );
	if ( selspecs )
	    newpdd->setSelSpecs( idx, *selspecs );

	if ( getColTabMapperSetup( idx ) )
	    newpdd->setColTabMapperSetup( idx, *getColTabMapperSetup(idx),
					  nullptr );
	if ( getColTabSequence( idx ) )
	    newpdd->setColTabSequence( idx, *getColTabSequence(idx), nullptr );
    }

    return newpdd;
}


SurveyObject* PlaneDataDisplay::duplicate( TaskRunner* taskr ) const
{
    auto* pdd = new PlaneDataDisplay();
    pdd->setOrientation( orientation_ );
    pdd->setTrcKeyZSampling( getTrcKeyZSampling(false,true,0) );
    pdd->setZAxisTransform( datatransform_.getNonConstPtr(), taskr );

    while ( nrAttribs() > pdd->nrAttribs() )
	pdd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	const TypeSet<Attrib::SelSpec>* selspecs = getSelSpecs( idx );
	if ( !selspecs )
	    continue;

	pdd->setSelSpecs( idx, *selspecs );
	ConstRefMan<VolumeDataPack> voldp = getVolumeDataPack( idx );
	pdd->setVolumeDataPack( idx, voldp.getNonConstPtr(), taskr );
	const ColTab::MapperSetup* mappersetup = getColTabMapperSetup( idx );
	if ( mappersetup )
	    pdd->setColTabMapperSetup( idx, *mappersetup, taskr );

	const ColTab::Sequence* colseq = getColTabSequence( idx );
	if ( colseq )
	    pdd->setColTabSequence( idx, *colseq, taskr );
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
    if ( !MultiTextureSurveyObject::usePar(par) )
	return false;

    SliceType orientation = OD::SliceType::Inline;
    const BufferString orstr = par.find( sKeyOrientation() );
    if ( !parseEnumSliceType(orstr,orientation) && orstr.isEqual("Timeslice") )
	orientation = OD::SliceType::Z;	// Backward compatibilty with 4.0

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


const visBase::TextureRectangle* PlaneDataDisplay::getTextureRectangle() const
{
    return texturerect_.ptr();
}


const mVisTrans* PlaneDataDisplay::getDisplayTransformation() const
{
    return displaytrans_.ptr();
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

    const float zdif = tkzs.zsamp_.start_ - si.zsamp_.start_;
    const float zfactor = resolutionfactor / si.zsamp_.step_;

    Coord startdif( zdif*zfactor - erg0.start_, inldif*inlfactor - erg1.start_);
    Coord growth( tkzs.zsamp_.width()*zfactor - erg0.width(),
		  tkzs.hsamp_.lineRange().width()*inlfactor - erg1.width() );

    bool refreeze = tkzs.hsamp_.start_.crl()==oldtkzs.hsamp_.start_.crl();

    if ( orientation_ == OD::SliceType::Inline )
    {
        startdif.y_ = crldif * crlfactor - erg1.start_;
        growth.y_ = tkzs.hsamp_.trcRange().width()*crlfactor - erg1.width();
	refreeze = tkzs.hsamp_.start_.inl()==oldtkzs.hsamp_.start_.inl();
    }

    if ( orientation_ == OD::SliceType::Z )
    {
        startdif.x_ = crldif * crlfactor - erg0.start_;
        growth.x_ = tkzs.hsamp_.trcRange().width()*crlfactor - erg0.width();
        refreeze = tkzs.zsamp_.start_==oldtkzs.zsamp_.start_;
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

    const float zdif = tkzs.zsamp_.start_ - si.zsamp_.start_;
    const float zfactor = resolutionfactor / si.zsamp_.step_;

    Coord origin( zdif * zfactor, inldif * inlfactor );

    Coord scale( tkzs.zsamp_.step_ / si.zsamp_.step_,
		 tkzs.hsamp_.step_.inl() / si.hsamp_.step_.inl() );

    if ( orientation_ == OD::SliceType::Inline )
    {
        origin.y_ = crldif * crlfactor;
        scale.y_ = (float)tkzs.hsamp_.step_.crl() / si.hsamp_.step_.crl();
    }

    if ( orientation_ == OD::SliceType::Z )
    {
        origin.x_ = crldif * crlfactor;
        scale.x_ = (float)tkzs.hsamp_.step_.crl() / si.hsamp_.step_.crl();
    }

    channels_->setOrigin( attrib, origin );
    channels_->setScale( attrib, scale );
}


} // namespace visSurvey
