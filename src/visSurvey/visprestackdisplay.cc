/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Yuancheng Liu
 DAT:		May 2007
_______________________________________________________________________________

 -*/

#include "visprestackdisplay.h"

#include "filepath.h"
#include "flatposdata.h"
#include "ioman.h"
#include "iopar.h"
#include "oddirs.h"
#include "oscommand.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "prestackevents.h"
#include "prestackgather.h"
#include "prestackprocessor.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisread.h"
#include "seispsioprov.h"
#include "sorting.h"
#include "survinfo.h"

#include "viscoord.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "visevent.h"
#include "visflatviewer.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "visseis2ddisplay.h"

#include <math.h>

#define mDefaultWidth ((SI().inlDistance() + SI().crlDistance() ) * 100)

static const char* sKeyMultiID()	{ return "Data ID"; }
static const char* sKeyTraceNr()	{ return "TraceNr"; }

namespace visSurvey
{

PreStackDisplay::PreStackDisplay()
    : VisualObjectImpl( true )
    , planedragger_( visBase::DepthTabPlaneDragger::create() )
    , flatviewer_( visBase::FlatViewer::create() )
    , draggermoving( this )
    , draggerpos_( -1, -1 )
    , bid_( -1, -1 )
    , section_( 0 )
    , seis2d_( 0 )
    , factor_( 1 )
    , trcnr_( -1 )
    , basedirection_( mUdf(float), mUdf(float) )
    , seis2dpos_( mUdf(float), mUdf(float) )
    , width_( mDefaultWidth )
    , offsetrange_( 0, mDefaultWidth )
    , zrg_( SI().zRange(true) )
    , posside_( true )
    , autowidth_( true )
    , preprocmgr_( *new PreStack::ProcessManager )
    , reader_( 0 )
    , ioobj_( 0 )
    , movefinished_(this)
    , eventcatcher_( 0 )
{
    setMaterial( 0 );

    flatviewer_->ref();
    flatviewer_->enableTraversal( visBase::cDraggerIntersecTraversalMask(),
				  false);
    flatviewer_->setSelectable( false );
    flatviewer_->appearance().setGeoDefaults( true );
    flatviewer_->getMaterial()->setDiffIntensity( 0.2 );
    flatviewer_->getMaterial()->setAmbience( 0.8 );
    flatviewer_->appearance().ddpars_.vd_.mappersetup_.symmidval_ = 0;
    mAttachCB( flatviewer_->dataChanged, PreStackDisplay::dataChangedCB );
    addChild( flatviewer_->osgNode() );

    planedragger_->ref();
    planedragger_->removeScaleTabs();
    mAttachCB( planedragger_->motion, PreStackDisplay::draggerMotion );
    mAttachCB( planedragger_->finished, PreStackDisplay::finishedCB );
    addChild( planedragger_->osgNode() );
}


PreStackDisplay::~PreStackDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );

    flatviewer_->unRef();

    if ( planedragger_ )
	planedragger_->unRef();

    if ( section_ )
	section_->unRef();

    if ( seis2d_ )
	seis2d_->unRef();

    delete reader_;
    delete ioobj_;
    delete &preprocmgr_;
}


void PreStackDisplay::setSceneEventCatcher( visBase::EventCatcher* evcatcher )
{
    if ( eventcatcher_ )
    {
	mDetachCB( eventcatcher_->eventhappened,
		   PreStackDisplay::updateMouseCursorCB );
	eventcatcher_->unRef();
    }

    eventcatcher_ = evcatcher;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	mAttachCB( eventcatcher_->eventhappened,
		   PreStackDisplay::updateMouseCursorCB );
    }
}


void PreStackDisplay::updateMouseCursorCB( CallBacker* cb )
{
    if ( !planedragger_ || !isOn() || isLocked() )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
	initAdaptiveMouseCursor( cb, id(), mUdf(int), mousecursor_ );
}


void PreStackDisplay::allowShading( bool yn )
{ flatviewer_->allowShading( yn ); }


BufferString PreStackDisplay::getObjectName() const
{ return ioobj_->name(); }


void PreStackDisplay::setMultiID( const MultiID& mid )
{
    mid_ = mid;
    delete ioobj_; ioobj_ = IOM().get( mid_ );
    delete reader_; reader_ = 0;
    if ( !ioobj_ )
	return;

    if ( section_ )
	reader_ = SPSIOPF().get3DReader( *ioobj_ );
    else if ( seis2d_ )
	reader_ = SPSIOPF().get2DReader( *ioobj_, seis2d_->getGeomID() );

    if ( !reader_ )
	return;

    if ( seis2d_ )
    {
	mAttachCB( seis2d_->getMovementNotifier(),
		   PreStackDisplay::seis2DMovedCB );
    }

    if ( section_ )
    {
	mAttachCB( section_->getMovementNotifier(),
		   PreStackDisplay::sectionMovedCB );
    }
}


DataPack::ID PreStackDisplay::preProcess()
{
    if ( !ioobj_ || !reader_ )
	return -1;

    if ( !preprocmgr_.nrProcessors() || !preprocmgr_.reset() )
	return -1;

    if ( !preprocmgr_.prepareWork() )
	return -1;

    const BinID stepout = preprocmgr_.getInputStepout();

    BinID relbid;
    for ( relbid.inl()=-stepout.inl(); relbid.inl()<=stepout.inl();
				       relbid.inl()++ )
    {
	for ( relbid.crl()=-stepout.crl(); relbid.crl()<=stepout.crl();
					   relbid.crl()++ )
	{
	    if ( !preprocmgr_.wantsInput(relbid) )
		continue;

	    TrcKey tk;
	    if ( is3DSeis() )
	    {
		const BinID inputbid = bid_ +
				relbid * BinID(SI().inlStep(),SI().crlStep());
		tk.setPosition( inputbid );
	    }
	    else
	    {
		const int trcnr = trcnr_ + relbid.trcNr();
		tk.setGeomID( seis2d_->getGeomID() ).setTrcNr( trcnr );
	    }

	    auto* gather = new PreStack::Gather;
	    if ( !gather->readFrom(*ioobj_,*reader_,tk) )
	    {
		delete gather;
		continue;
	    }

	    DPM( DataPackMgr::FlatID() ).addAndObtain( gather );
	    preprocmgr_.setInput( relbid, gather->id() );
	    DPM( DataPackMgr::FlatID() ).release( gather );
	}
    }

    if ( !preprocmgr_.process() )
	return -1;

    return preprocmgr_.getOutput();
}


bool PreStackDisplay::setPosition( const BinID& nb )
{
    const TrcKey tk( nb );
    return setPosition( tk );
}


bool PreStackDisplay::setPosition( const TrcKey& tk )
{
    if ( !tk.is3D() )
	{ pErrMsg("Incorrect TrcKey type"); }

    if ( bid_ == tk.position() )
	return true;

    bid_ = tk.position();

    PtrMan<PreStack::Gather> gather = new PreStack::Gather;
    if ( !ioobj_ || !reader_ || !gather->readFrom(*ioobj_,*reader_,tk) )
    {
	mDefineStaticLocalObject( bool, shown3d, = false );
	mDefineStaticLocalObject( bool, resetpos, = true );
	if ( !shown3d )
	{
	    resetpos = true;
	    shown3d = true;
	}

	bool hasdata = false;
	if ( resetpos )
	{
	    BinID nearbid = getNearBinID( bid_ );
	    if ( nearbid.inl()==-1 || nearbid.crl()==-1 )
	    {
		const StepInterval<int> rg = getTraceRange( bid_, false );
		BufferString msg( "No gather data at the whole section.\n" );
		msg.add( "Data available at: ").add( rg.start ).add( " - " )
		    .add( rg.stop ).add( " - " ).add( rg.step );
		OD::DisplayErrorMessage( msg );
	    }
	    else
	    {
		bid_ = nearbid;
		hasdata = true;
	    }
	}
	else
	    OD::DisplayErrorMessage( "No gather data at the picked location." );

	if ( !hasdata )
	{
	    flatviewer_->appearance().annot_.x1_.showgridlines_ = false;
	    flatviewer_->appearance().annot_.x2_.showgridlines_ = false;
	    flatviewer_->turnOnGridLines( false, false );
	}
    }

    draggerpos_ = bid_;
    draggermoving.trigger();
    dataChangedCB( nullptr );
    return updateData();
}


bool PreStackDisplay::updateData()
{
    if ( (is3DSeis() && (bid_.inl()==-1 || bid_.crl()==-1)) ||
	 (!is3DSeis() && !seis2d_) || !ioobj_ || !reader_ )
    {
	turnOn(false);
	return true;
    }

    const bool haddata = flatviewer_->hasPack( false );
    auto* gather = new PreStack::Gather;

	DataPack::ID displayid = DataPack::cNoID();
	if ( preprocmgr_.nrProcessors() )
	{
	    displayid = preProcess();
	    delete gather;
	}
	else
	{
	    TrcKey tk;
	    if ( is3DSeis() )
		tk.setPosition( bid_ );
	    else
		tk.setGeomID( seis2d_->getGeomID() ).setTrcNr( trcnr_ );

	    if ( gather->readFrom(*ioobj_,*reader_,tk) )
	    {
		DPM(DataPackMgr::FlatID()).add( gather );
		displayid = gather->id();
	    }
	    else
		delete gather;
	}

	if ( displayid==DataPack::cNoID() )
	{
	    if ( haddata )
	    {
		flatviewer_->setVisible( false, false );
		flatviewer_->setPack( false, DataPack::cNoID() );
	    }
	    else
		dataChangedCB( 0 );

	    return false;
	}
	else
	{
	    flatviewer_->setVisible( false, true );
	    flatviewer_->setPack( false, displayid, !haddata );
	}

    turnOn( true );
    return true;
}


StepInterval<int> PreStackDisplay::getTraceRange( const BinID& bid,
						  bool oncurrentline ) const
{
    if ( is3DSeis() )
    {
	mDynamicCastGet( SeisPS3DReader*, rdr3d, reader_ );
	if ( !rdr3d )
	    return StepInterval<int>(mUdf(int),mUdf(int),1);

	const PosInfo::CubeData& posinfo = rdr3d->posData();
	const bool docrlrg = (isOrientationInline() && oncurrentline) ||
			     (!isOrientationInline() && !oncurrentline);
	if ( docrlrg )
	{
	    const int inlidx = posinfo.indexOf( bid.inl() );
	    if ( inlidx==-1 )
		return StepInterval<int>(mUdf(int),mUdf(int),1);

	    const int seg = posinfo[inlidx]->nearestSegment( bid.crl() );
	    return posinfo[inlidx]->segments_[seg];
	}
	else
	{
	    StepInterval<int> res;
	    posinfo.getInlRange( res );
	    return res;
	}
    }
    else
    {
	mDynamicCastGet( SeisPS2DReader*, rdr2d, reader_ );
	if ( !seis2d_ || !rdr2d )
	    return StepInterval<int>(mUdf(int),mUdf(int),1);

	const TypeSet<PosInfo::Line2DPos>& posnrs
	    = rdr2d->posData().positions();
	const int nrtraces = posnrs.size();
	if ( !nrtraces )
	     return StepInterval<int>(mUdf(int),mUdf(int),1);

	mAllocVarLenArr( int, trcnrs, nrtraces );

	for ( int idx=0; idx<nrtraces; idx++ )
	    trcnrs[idx] = posnrs[idx].nr_;

	quickSort( mVarLenArr(trcnrs), nrtraces );
	const int trstep = nrtraces>1 ? trcnrs[1]-trcnrs[0] : 0;
	return StepInterval<int>( trcnrs[0], trcnrs[nrtraces-1], trstep );
    }
}


BinID PreStackDisplay::getNearBinID( const BinID& bid ) const
{
    const StepInterval<int> tracerg = getTraceRange( bid );
    if ( tracerg.isUdf() )
	return BinID(-1,-1);

    BinID res = bid;
    if ( isOrientationInline() )
    {
	res.crl() = bid.crl()<tracerg.start ? tracerg.start :
	    ( bid.crl()>tracerg.stop ? tracerg.stop : tracerg.snap(bid.crl()) );
    }
    else
    {
	res.inl() = bid.inl()<tracerg.start ? tracerg.start :
	    ( bid.inl()>tracerg.stop ? tracerg.stop : tracerg.snap(bid.inl()) );
    }

    return res;
}


int PreStackDisplay::getNearTraceNr( int trcnr ) const
{
    mDynamicCastGet(SeisPS2DReader*, rdr2d, reader_ );
    if ( !rdr2d )
	return -1;

    const TypeSet<PosInfo::Line2DPos>& posnrs = rdr2d->posData().positions();
    if ( posnrs.isEmpty() )
	return -1;

    int mindist=-1, residx=0;
    for ( int idx=0; idx<posnrs.size(); idx++ )
    {
	const int dist = abs( posnrs[idx].nr_ - trcnr );
	if ( mindist==-1 || mindist>dist )
	{
	    mindist = dist;
	    residx = idx;
	}
    }

   return posnrs[residx].nr_;
}


void PreStackDisplay::displaysAutoWidth( bool yn )
{
    if ( autowidth_ == yn )
	return;

    autowidth_ = yn;
    dataChangedCB( 0 );
}


void PreStackDisplay::displaysOnPositiveSide( bool yn )
{
    if ( posside_ == yn )
	return;

    posside_ = yn;
    dataChangedCB( 0 );
}


void PreStackDisplay::setFactor( float scale )
{
    if (  factor_ == scale )
	return;

    factor_ = scale;
    dataChangedCB( 0 );
}

void PreStackDisplay::setWidth( float width )
{
    if ( width_ == width )
	return;

    width_ = width;
    dataChangedCB( 0 );
}


void PreStackDisplay::dataChangedCB( CallBacker* )
{
    if ( (!section_ && !seis2d_) || factor_<0 || width_<0 )
	return;

    if ( section_ && ( bid_.inl()<0 || bid_.crl()<0 ) )
	return;

    const Coord direction = posside_ ? basedirection_ : -basedirection_;
    const double offsetscale = Coord( basedirection_.x*SI().inlDistance(),
				     basedirection_.y*SI().crlDistance()).abs();

    ConstDataPackRef<FlatDataPack> fdp = flatviewer_->obtainPack( false );
    int nrtrcs = 0;
    if ( fdp )
    {
	offsetrange_.setFrom( fdp->posData().range( true ) );
	zrg_.setFrom( fdp->posData().range( false ) );
	nrtrcs = fdp->size( true );
    }

    if ( nrtrcs < 2 )
	offsetrange_.set( 0.f, mDefaultWidth );
    else
    {
	const float inltrcdist = SI().inlDistance() * SI().inlStep();
	const float crltrcdist = SI().crlDistance() * SI().crlStep();
	const float minwidth = (nrtrcs - 1) * (inltrcdist + crltrcdist);
	if ( offsetrange_.width() < minwidth )
	    offsetrange_.set( 0.f, minwidth );
    }

    Coord startpos( bid_.inl(), bid_.crl() );
    if ( seis2d_ )
	startpos = seis2dpos_;

    const Coord stoppos = autowidth_
	? startpos + direction*offsetrange_.width()*factor_ / offsetscale
	: startpos + direction*width_ / offsetscale;

    if ( seis2d_ )
	seis2dstoppos_ = stoppos;

    if ( autowidth_ )
	width_ = offsetrange_.width()*factor_;
    else
	factor_ = width_/offsetrange_.width();

    const Coord3 c00( startpos, zrg_.start );
    const Coord3 c01( startpos, zrg_.stop );
    const Coord3 c11( stoppos, zrg_.stop );
    const Coord3 c10( stoppos, zrg_.start );

    flatviewer_->setPosition( c00, c01, c10, c11 );

    Interval<float> xlim( mCast(float, SI().inlRange(true).start),
			  mCast(float, SI().inlRange(true).stop) );
    Interval<float> ylim( mCast(float, SI().crlRange(true).start),
			  mCast(float, SI().crlRange(true).stop) );

    bool isinline = true;
    if ( section_ )
    {
	isinline = section_->getOrientation()==OD::InlineSlice;
	if ( isinline )
	{
	    xlim.set( mCast(float,startpos.x), mCast(float,stoppos.x) );
	    xlim.sort();
	}
	else
	{
	    ylim.set( mCast(float,startpos.y), mCast(float,stoppos.y) );
	    ylim.sort();
	}
    }
    else if ( seis2d_ && !seis2d_->getGeometry().positions().isEmpty() )
    {
	const Coord startpt = seis2d_->getGeometry().positions().first().coord_;
	const Coord stoppt = seis2d_->getGeometry().positions().last().coord_;
	const BinID startbid = SI().transform( startpt );
	const BinID stopbid = SI().transform( stoppt );
	const BinID diff = stopbid - startbid;
	isinline = Math::Abs(diff.inl()) < Math::Abs(diff.crl());

	xlim.start = mCast(float, mMIN(startbid.inl(),stopbid.inl()));
	xlim.stop = mCast(float, mMAX(startbid.inl(),stopbid.inl()));
	ylim.start = mCast(float, mMIN(startbid.crl(),stopbid.crl()));
	ylim.stop = mCast(float, mMAX(startbid.crl(),stopbid.crl()));
    }
    else
	return;

    planedragger_->setDim( isinline ? 1 : 0 );

    const float xwidth =
	isinline ? (float) fabs(stoppos.x-startpos.x) : SI().inlDistance();
    const float ywidth =
	isinline ?  SI().crlDistance() : (float) fabs(stoppos.y-startpos.y);

    planedragger_->setSize( Coord3(xwidth,ywidth,zrg_.width(true)) );
    planedragger_->setCenter( (c01+c10)/2 );
    planedragger_->setSpaceLimits( xlim, ylim, SI().zRange(true) );
}


const BinID& PreStackDisplay::getPosition() const
{ return bid_; }


bool PreStackDisplay::isOrientationInline() const
{
    if ( !section_ )
	return false;

    return section_->getOrientation() == OD::InlineSlice;
}


const visSurvey::PlaneDataDisplay* PreStackDisplay::getSectionDisplay() const
{ return section_;}


visSurvey::PlaneDataDisplay* PreStackDisplay::getSectionDisplay()
{ return section_;}


void PreStackDisplay::setDisplayTransformation(
					const visBase::Transformation* nt )
{
    flatviewer_->setDisplayTransformation( nt );
    if ( planedragger_ )
	planedragger_->setDisplayTransformation( nt );
}


void PreStackDisplay::setSectionDisplay( PlaneDataDisplay* pdd )
{
    if ( section_ )
    {
	mDetachCB( section_->getMovementNotifier(),
		   PreStackDisplay::sectionMovedCB );
	section_->unRef();
    }

    section_ = pdd;
    if ( !section_ ) return;
    section_->ref();

    if ( section_->id() > id() )
    {
	pErrMsg("The display restore order is wrong. The section id has to be \
		  lower than PreStack id so that it can be restored earlier \
		  than PreStack display in the sessions." );
	return;
    }


    if ( ioobj_ && !reader_ )
	reader_ = SPSIOPF().get3DReader( *ioobj_ );

    const bool offsetalonginl =
	section_->getOrientation()==OD::CrosslineSlice;
    basedirection_ = offsetalonginl ? Coord( 0, 1  ) : Coord( 1, 0 );

    if ( section_->getOrientation() == OD::ZSlice )
	return;

    mAttachCB(section_->getMovementNotifier(), PreStackDisplay::sectionMovedCB);
}


void PreStackDisplay::sectionMovedCB( CallBacker* )
{
    BinID newpos = bid_;

    if ( !section_ )
	return;
    else
    {
	if ( section_->getOrientation() == OD::InlineSlice )
	{
	    newpos.inl() =
		section_->getTrcKeyZSampling( -1 ).hsamp_.start_.inl();
	}
	else if ( section_->getOrientation() == OD::CrosslineSlice )
	{
	    newpos.crl() =
		section_->getTrcKeyZSampling( -1 ).hsamp_.start_.crl();
	}
	else
	    return;
    }

    if ( !setPosition(TrcKey(newpos)) )
	return;
}


const visSurvey::Seis2DDisplay* PreStackDisplay::getSeis2DDisplay() const
{ return seis2d_; }


DataPack::ID PreStackDisplay::getDataPackID(int) const
{
    return flatviewer_->packID( false );
}


bool PreStackDisplay::is3DSeis() const
{
    return section_;
}


void PreStackDisplay::setTraceNr( int trcnr )
{
    if ( seis2d_ )
    {
	PtrMan<PreStack::Gather> gather = new PreStack::Gather;
	const TrcKey tk( seis2d_->getGeomID(), trcnr );
	if ( !ioobj_ || !reader_ || !gather->readFrom(*ioobj_,*reader_,tk) )
	{
	    mDefineStaticLocalObject( bool, show2d, = false );
	    mDefineStaticLocalObject( bool, resettrace, = true );
	    if ( !show2d )
	    {
//		resettrace = uiMSG().askContinue(
//		"There is no data at the selected location."
//		"\n\nDo you want to find a nearby location to continue?" );
		show2d = true;
	    }

	    if ( resettrace )
	    {
		trcnr_ = getNearTraceNr( trcnr );
		if ( trcnr_==-1 )
		{
//		    uiMSG().warning("Can not read or no data at the section.");
		    trcnr_ = trcnr; //If no data, we still display the panel.
		}
	    }
	}
	else
	    trcnr_ = trcnr;
    }
    else
	trcnr_ = trcnr;

    draggermoving.trigger();
    seis2DMovedCB( nullptr );
    updateData();
    turnOn( true );
}


bool PreStackDisplay::setSeis2DDisplay( Seis2DDisplay* s2d, int trcnr )
{
    if ( !s2d )
	return false;

    if ( seis2d_ )
    {
	mDetachCB( seis2d_->getMovementNotifier(),
		   PreStackDisplay::seis2DMovedCB );
	seis2d_->unRef();
    }

    seis2d_ = s2d;
    seis2d_->ref();

    if ( seis2d_->id() > id() )
    {
	pErrMsg("The display restore order is wrong. The Seis2d display id \
		has to be lower than PreStack id so that it can be restored \
		earlier than PreStack display in the sessions." );
	return false;
    }

     if ( ioobj_ && !reader_ )
	 reader_ = SPSIOPF().get2DReader( *ioobj_, seis2d_->getGeomID() );

    setTraceNr( trcnr );
    if ( trcnr_ < 0 )
	return false;

    const Coord orig = SI().binID2Coord().transformBackNoSnap( Coord(0,0) );
    basedirection_ = SI().binID2Coord().transformBackNoSnap(
			seis2d_->getNormal(trcnr_) ) - orig;
    seis2dpos_ = SI().binID2Coord().transformBackNoSnap(
			seis2d_->getCoord(trcnr_) );

    mAttachCB( seis2d_->getMovementNotifier(), PreStackDisplay::seis2DMovedCB );
    planedragger_->showDraggerBorder( false );

    return updateData();
}


void PreStackDisplay::seis2DMovedCB( CallBacker* )
{
    if ( !seis2d_ || trcnr_<0 )
	return;

    const Coord orig = SI().binID2Coord().transformBackNoSnap( Coord(0,0) );
    basedirection_ = SI().binID2Coord().transformBackNoSnap(
	    seis2d_->getNormal(trcnr_) ) - orig;
    seis2dpos_ = SI().binID2Coord().transformBackNoSnap(
	    seis2d_->getCoord(trcnr_) );
    dataChangedCB( nullptr );
}


const Coord PreStackDisplay::getBaseDirection() const
{ return basedirection_; }


BufferString PreStackDisplay::lineName() const
{
    if ( !seis2d_ )
	return 0;

    return seis2d_->name();
}


void PreStackDisplay::otherObjectsMoved( const ObjectSet<const SurveyObject>&
					 , int whichobj )
{
    if ( !section_ && ! seis2d_ )
	return;

    if ( whichobj == -1 )
    {
	if ( section_ )
	    turnOn( section_->isShown() );

	if ( seis2d_ )
	    turnOn( seis2d_->isShown() );
	return;
    }

    if ( (section_ && section_->id() != whichobj) ||
	 (seis2d_ && seis2d_->id() != whichobj) )
	return;

    if ( section_ )
	turnOn( section_->isShown() );

    if ( seis2d_ )
	turnOn( seis2d_->isShown() );
}


void PreStackDisplay::draggerMotion( CallBacker* )
{
    Coord draggerbidf = planedragger_->center();

    bool showplane = false;
    if ( section_ )
    {
	const OD::SliceType orientation = section_->getOrientation();
	const int newinl = SI().inlRange( true ).snap( draggerbidf.x );
	const int newcrl = SI().crlRange( true ).snap( draggerbidf.y );
	if ( orientation==OD::InlineSlice && newcrl!=bid_.crl() )
	    showplane = true;
	else if ( orientation==OD::CrosslineSlice && newinl!=bid_.inl() )
	    showplane = true;

	draggerpos_ = BinID( newinl, newcrl );
    }
    else if ( seis2d_ )
    {
	const int dimtoadjust = planedragger_->getDim() ? 0 : 1;
	draggerbidf[dimtoadjust] = seis2dpos_[dimtoadjust];
	const Coord draggercrd = SI().binID2Coord().transform( draggerbidf );
	const int nearesttrcnr =
		seis2d_->getNearestTraceNr( Coord3(draggercrd,0.) );
	if ( nearesttrcnr != trcnr_ )
	    showplane = true;

	const Coord trcpos = seis2d_->getCoord( nearesttrcnr );
	const Coord newdraggerbidf =
			SI().binID2Coord().transformBackNoSnap( trcpos );

	const Coord direction = posside_ ? basedirection_ : -basedirection_;
	const float offsetscale = mCast(float,
		Coord(basedirection_.x*SI().inlDistance(),
		      basedirection_.y*SI().crlDistance()).abs());

	seis2dpos_ = newdraggerbidf;
	seis2dstoppos_ = autowidth_
	    ? seis2dpos_ + direction*offsetrange_.width()*factor_ / offsetscale
	    : seis2dpos_ + direction*width_ / offsetscale;

	const Coord3 c01( seis2dpos_, zrg_.stop );
	const Coord3 c10( seis2dstoppos_, zrg_.start );

	planedragger_->setCenter( (c01+c10)/2 );
	trcnr_ = nearesttrcnr;
    }

    planedragger_->showPlane( showplane );
    planedragger_->showDraggerBorder( !showplane && section_ );

    draggermoving.trigger();
}


void PreStackDisplay::finishedCB( CallBacker* )
{
    Coord draggerbidf = planedragger_->center();
    if ( section_ )
    {
	int newinl = SI().inlRange( true ).snap( draggerbidf.x );
	int newcrl = SI().crlRange( true ).snap( draggerbidf.y );
	if ( section_->getOrientation() == OD::InlineSlice )
	    newinl = section_->getTrcKeyZSampling( -1 ).hsamp_.start_.inl();
	else if ( section_->getOrientation() == OD::CrosslineSlice )
	    newcrl = section_->getTrcKeyZSampling( -1 ).hsamp_.start_.crl();

	setPosition( TrcKey(BinID(newinl,newcrl)) );
    }
    else if ( seis2d_ )
    {
	const int dimtoadjust = planedragger_->getDim() ? 0 : 1;
	draggerbidf[dimtoadjust] = seis2dpos_[dimtoadjust];
	const Coord draggercrd = SI().binID2Coord().transform( draggerbidf );
	const int nearesttrcnr =
		seis2d_->getNearestTraceNr( Coord3(draggercrd,0.) );
	setTraceNr( nearesttrcnr );
    }
    else
	return;

    planedragger_->showPlane( false );
    planedragger_->showDraggerBorder( section_ );
}


void PreStackDisplay::getMousePosInfo( const visBase::EventInfo& ei,
				      Coord3& pos,
				      BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = "";
    if ( !flatviewer_  )
	return;

    ConstDataPackRef<FlatDataPack> fdp = flatviewer_->obtainPack( false );
    if ( !fdp ) return;

    const int nrtrcs = fdp->size( true );
    const FlatPosData& posdata = fdp->posData();

    int trcidx = -1;
    Coord disppos;
    if ( seis2d_ )
    {
	const double displaywidth = seis2dstoppos_.distTo(seis2dpos_);
	const double curdist =
	    SI().binID2Coord().transformBackNoSnap( pos ).distTo( seis2dpos_ );
	trcidx = mNINT32( (nrtrcs-1)*curdist/displaywidth );
	disppos = SI().binID2Coord().transform( seis2dpos_ );
    }
    else if ( section_ )
    {
	if ( mIsZero(width_,mDefEpsF) )
	    return;

	disppos = SI().transform( bid_ );
	const double distance = pos.coord().distTo( disppos );
	trcidx = mNINT32( (nrtrcs-1)*distance/width_ );
    }

    if ( trcidx<0 )
	trcidx = 0;
    else if ( trcidx>=nrtrcs )
	trcidx = nrtrcs-1;

    IOPar ipar; float offset = mUdf(float), azimuth = mUdf(float);
    fdp->getAuxInfo( trcidx, 0, ipar );
    ipar.get( sKey::Offset(), offset );
    ipar.get( sKey::Azimuth(), azimuth );

    pos = Coord3( disppos, pos.z );

    if ( seis2d_ )
	info.add( "TrcNr: " ).add( trcnr_ ).addSpace();
    info.add( "Offset: " ).add( offset ).addSpace();
    if ( !mIsUdf(azimuth) )
	info.add( "Azimuth: " ).add(mNINT32(azimuth*360/M_PI));

    const int zsample = posdata.range(false).nearestIndex( pos.z );
    val = fdp->data().get( trcidx, zsample );
}


void PreStackDisplay::fillPar( IOPar& par ) const
{
    if ( !section_ && !seis2d_ )
	return;

    VisualObjectImpl::fillPar( par );
    SurveyObject::fillPar( par );

    if ( section_ )
    {
	par.set( sKeyParent(), section_->id() );
	par.set( sKey::Position(), bid_ );
    }

    if  ( seis2d_ )
    {
	par.set( sKeyParent(), seis2d_->id() );
	par.set( sKeyTraceNr(), trcnr_ );
    }

    par.set( sKeyMultiID(), mid_ );
    par.setYN( sKeyAutoWidth(), autowidth_ );
    par.setYN( sKeySide(), posside_ );

    if ( flatviewer_ )
	flatviewer_->appearance().ddpars_.fillPar( par );

    if ( autowidth_ )
	par.set( sKeyFactor(), factor_ );
    else
	par.set( sKeyWidth(), width_ );
}


bool PreStackDisplay::usePar( const IOPar& par )
{
    if ( !VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	 return false;

    int parentid = -1;
    if ( !par.get( sKeyParent(), parentid ) )
    {
	if ( !par.get( "Seis2D ID", parentid ) )
	    if ( !par.get( "Section ID", parentid ) )
		return false;
    }

    visBase::DataObject* parent = visBase::DM().getObject( parentid );
    if ( !parent ) return false;

    MultiID mid;
    if ( !par.get( sKeyMultiID(), mid ) )
    {
	if ( !par.get("PreStack MultiID",mid) )
	{
	    return false;
	}
    }

    setMultiID( mid );

    mDynamicCastGet( PlaneDataDisplay*, pdd, parent );
    mDynamicCastGet( Seis2DDisplay*, s2d, parent );
    if ( !pdd && !s2d )
	return false;

    if ( pdd )
    {
	setSectionDisplay( pdd );
	BinID bid;
	if ( !par.get( sKey::Position(), bid ) )
	{
	    if ( !par.get("PreStack BinID",bid) )
		return false;
	}

	if ( !setPosition( TrcKey(bid) ) )
	    return false;
    }

    if ( s2d )
    {
	int tnr;
	if ( !par.get( sKeyTraceNr(), tnr ) )
	{
	    if ( !par.get( "Seis2D TraceNumber", tnr ) )
		return false;
	}

	setSeis2DDisplay( s2d, tnr );
    }

    float factor, width;
    if ( par.get(sKeyFactor(), factor) )
	setFactor( factor );

    if ( par.get(sKeyWidth(), width) )
	setWidth( width );

    bool autowidth, side;
    if ( par.getYN(sKeyAutoWidth(), autowidth) )
	 displaysAutoWidth( autowidth );

    if ( par.getYN(sKeySide(), side) )
	displaysOnPositiveSide( side );

    if ( flatviewer_ )
    {
	flatviewer_->appearance().ddpars_.usePar( par );
	flatviewer_->handleChange( FlatView::Viewer::DisplayPars );
    }

    return true;
}


} // namespace visSurvey
