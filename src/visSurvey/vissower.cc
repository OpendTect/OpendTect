/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : December 2010
-*/



#include "vissower.h"

#include "bendpointfinder.h"
#include "color.h"
#include "trckeyzsampling.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "settings.h"
#include "survinfo.h"
#include "timefun.h"
#include "visevent.h"
#include "vislocationdisplay.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "visplanedatadisplay.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"

namespace visSurvey
{

Sower::Sower( const visBase::VisualObjectImpl* editobj )
    : visBase::VisualObjectImpl(false)
    , editobject_(editobj)
    , eventcatcher_(0)
    , transformation_(0)
    , mode_(Idle)
    , sowingline_(visBase::PolyLine::create())
    , linelost_(false)
    , singleseeded_(true)
    , curpid_(EM::PosID::udf())
    , curpidstamp_(mUdf(int))
    , workrange_(0)
    , underlyingobjid_(-1)
    , sowingend(this)
    , sowing(this)
{
    sowingline_->ref();
    sowingline_->setMaterial( new visBase::Material );
    sowingline_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,1) );
    sowingline_->setPickable( false, false );
    addChild( sowingline_->osgNode() );
    reInitSettings();
}


Sower::~Sower()
{
    removeChild( sowingline_->osgNode() );
    sowingline_->unRef();
    deepErase( eventlist_ );

    if ( workrange_ )
    {
	delete workrange_;
	workrange_ = 0;
    }
}


void Sower::reInitSettings()
{
    reversesowingorder_ = false;
    alternatesowingorder_ = false;
    intersow_ = false;

    setIfDragInvertMask( false );
    setSequentSowMask();
    setLaserMask();
    setEraserMask();
}


void Sower::reverseSowingOrder( bool yn )
{ reversesowingorder_ = yn; }


void Sower::alternateSowingOrder( bool yn )
{ alternatesowingorder_ = yn; }


void Sower::intersow( bool yn )
{ intersow_ = yn; }


void Sower::setDisplayTransformation( const mVisTrans* transformation )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = transformation;
    if ( transformation_ )
	transformation_->ref();

    sowingline_->setDisplayTransformation( transformation );
}


void Sower::setEventCatcher( visBase::EventCatcher* eventcatcher )
{ eventcatcher_ = eventcatcher; }


#define mReturnHandled( yn ) \
{ \
    if ( yn && eventcatcher_ ) eventcatcher_->setHandled(); \
    return yn; \
}


bool Sower::activate( const OD::Color& color,
		    const visBase::EventInfo& eventinfo, int underlyingobjid,
		    const TrcKeySampling* workrg )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    Scene* scene = STM().currentScene();
    if ( scene && scene->getPolySelection()->getSelectionType() !=
	    					visBase::PolygonSelection::Off )
	mReturnHandled( false );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	mReturnHandled( false );

    if ( eventinfo.buttonstate_ & OD::RightButton )
	mReturnHandled( false );

    mode_ = Furrowing;
    furrowstamp_ = Time::getMilliSeconds();

    underlyingobjid_ = underlyingobjid;
    if ( workrange_ ) delete workrange_;
    workrange_ = 0;

    visBase::DataObject* dataobj = visBase::DM().getObject( underlyingobjid_ );
    mDynamicCastGet( PlaneDataDisplay*, pdd, dataobj );
    if ( pdd )
	workrange_ = new TrcKeySampling( pdd->getTrcKeyZSampling().hsamp_ );

    if ( workrg && workrg->isDefined() && !workrg->isEmpty() )
    {
	if ( workrange_ )
	    workrange_->limitTo( *workrg );
	else
	    workrange_ = new TrcKeySampling( *workrg );
    }

    if ( !accept(eventinfo) )
    {
	mode_ = Idle;
	mReturnHandled( false );
    }
    sowingline_->getMaterial()->setColor( color );
    sowingline_->turnOn( true );

    mReturnHandled( true );
}


Coord3 Sower::pivotPos() const
{
    if ( mode_<FirstSowing || eventlist_.isEmpty() )
	return Coord3::udf();

    Coord3 sum = eventlist_[0]->displaypickedpos;
    sum += eventlist_[eventlist_.size()-1]->displaypickedpos;
    return 0.5*sum;
}


bool Sower::moreToSow() const
{ return mode_>=FirstSowing && bendpoints_.size()>1; }


void Sower::stopSowing()
{ bendpoints_.erase(); }


void Sower::calibrateEventInfo( visBase::EventInfo& eventinfo )
{
    if ( mode_==Idle || !transformation_ )
	return;

    visBase::DataObject* dataobj = visBase::DM().getObject( underlyingobjid_ );
    mDynamicCastGet( PlaneDataDisplay*, pdd, dataobj );
    Scene* scene = STM().currentScene();
    if ( !pdd || pdd->getOrientation()==OD::ZSlice || !scene )
	return;

    TrcKeyZSampling cs = pdd->getTrcKeyZSampling( false, false );
    Coord3 p0( SI().transform(cs.hsamp_.start_), cs.zsamp_.start );
    transformation_->transform( p0 );
    scene->getTempZStretchTransform()->transform( p0 );
    Coord3 p1( SI().transform( cs.hsamp_.stop_), cs.zsamp_.start );
    transformation_->transform( p1 );
    scene->getTempZStretchTransform()->transform( p1 );
    Coord3 p2( SI().transform(cs.hsamp_.start_), cs.zsamp_.stop );
    transformation_->transform( p2 );
    scene->getTempZStretchTransform()->transform( p2 );

    double t;
    const Plane3 plane( p0, p1, p2 );
    if ( !eventinfo.mouseline.intersectWith(plane,t) || t<0.0 || t>1.0 )
	return;

    Coord3 pos = eventinfo.mouseline.getPoint( t );

    eventinfo.displaypickedpos = pos;
    eventinfo.pickdepth = t;
    scene->getTempZStretchTransform()->transformBack( pos );
    transformation_->transformBack( pos, eventinfo.worldpickedpos );
}


bool Sower::accept( const visBase::EventInfo& ei )
{
    PtrMan<visBase::EventInfo> eventinfo = new visBase::EventInfo( ei );
    calibrateEventInfo( *eventinfo );

    if ( eventinfo->tabletinfo )
	return acceptTablet( *eventinfo );

    return acceptMouse( *eventinfo );
}


bool Sower::isInWorkRange( const visBase::EventInfo& eventinfo ) const
{
    if ( !workrange_ || !workrange_->isDefined() )
	return false;

    const BinID eventbid = SI().transform( eventinfo.worldpickedpos );
    return workrange_->includes(eventbid);
}


void Sower::tieToWorkRange( const visBase::EventInfo& eventinfo )
{
    if ( !workrange_ || !workrange_->isDefined() || !eventlist_.size() )
	return;

    if ( workrange_->nrInl()!=1 && workrange_->nrCrl()!=1 )
	return;

    if ( isInWorkRange(eventinfo) )
    {
	if ( eventinfo.type!=visBase::MouseMovement || eventlist_.size()!=1 )
	    return;
    }
    else if ( eventinfo.type!=visBase::MouseClick || eventinfo.pressed )
	return;

    Coord3& lastworldpos = eventlist_[eventlist_.size()-1]->worldpickedpos;
    const BinID lastbid = SI().transform( lastworldpos );
    const BinID start = workrange_->start_;
    const BinID stop = workrange_->stop_;

    const int min2 = (int)mMIN( lastbid.sqDistTo(start),
				lastbid.sqDistTo(stop) );
    const int step = workrange_->nrCrl()==1 ? workrange_->step_.inl()
					    : workrange_->step_.crl();
    if ( min2 > 100*step*step )
	return;

    lastworldpos.coord() = lastbid.sqDistTo(start) < lastbid.sqDistTo(stop) ?
			   SI().transform(start) : SI().transform(stop);

    Scene* scene = STM().currentScene();
    if ( transformation_ && scene )
    {
	Coord3& displaypos = eventlist_[eventlist_.size()-1]->displaypickedpos;
	transformation_->transform( lastworldpos, displaypos );
	scene->getTempZStretchTransform()->transform( displaypos );
    }
}


bool Sower::acceptMouse( const visBase::EventInfo& eventinfo )
{
    if ( mode_==Idle &&
	 eventinfo.type==visBase::MouseClick && !eventinfo.pressed )
    {
	const EM::PosID pid = getMarkerID( eventinfo );
	if ( pid.isUdf() )
	    mReturnHandled( false );
    }

    if ( mode_ != Furrowing )
	mReturnHandled( false );

    if ( eventinfo.type == visBase::Keyboard )
	mReturnHandled( true );

    if ( eventinfo.type==visBase::MouseClick && eventinfo.pressed )
	pressedbutstate_ = eventinfo.buttonstate_;

    const int sz = eventlist_.size();
    if ( eventinfo.type==visBase::MouseMovement || eventinfo.pressed )
    {
	if ( sz && eventinfo.mousepos==eventlist_[sz-1]->mousepos )
	    mReturnHandled( true );

	Coord3 furrowpos = eventinfo.worldpickedpos;
	Scene* scene = STM().currentScene();
	if ( scene && transformation_ && eventinfo.displaypickedpos.isDefined())
	{
	    furrowpos = eventinfo.mouseline.getPoint(eventinfo.pickdepth-0.01);
	    scene->getTempZStretchTransform()->transformBack( furrowpos );
	    transformation_->transformBack( furrowpos );
	}

	bool isvalidpos = !sz ||
			  eventinfo.pickedobjids==eventlist_[0]->pickedobjids ||
			  eventinfo.pickedobjids.isPresent(underlyingobjid_);
	if ( workrange_ )
	    isvalidpos = isInWorkRange( eventinfo );

	if ( !isvalidpos )
	{
	    if ( eventinfo.worldpickedpos.isDefined() && !linelost_ )
		sowingline_->addPoint( furrowpos );
	    else
		linelost_ = true;

	    mReturnHandled( true );
	}

	linelost_ = false;
	sowingline_->addPoint( furrowpos );

	if ( !sz )
	    singleseeded_ = true;
	else if ( eventinfo.mousepos.distTo(eventlist_[0]->mousepos) > 5 )
	{
	    singleseeded_ = false;
	    sowingline_->dirtyCoordinates();
	}

	eventlist_ += new visBase::EventInfo( eventinfo );
	mousecoords_ += eventinfo.mousepos;

	tieToWorkRange( eventinfo );
	mReturnHandled( true );
    }
    else
	tieToWorkRange( eventinfo );

    if ( !eventlist_.size() || !sowingline_->isOn() )
    {
	reset();
	mReturnHandled( true );
    }

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( Time::passedSince(furrowstamp_) < 200 )
	singleseeded_ = true;

    int butstate = pressedbutstate_;
    if ( !singleseeded_ )
	butstate ^= ifdraginvertmask_;

    eventlist_[0]->type = visBase::MouseClick;
    eventlist_[0]->buttonstate_ = (OD::ButtonState) butstate;
    butstate &= sequentsowmask_;

    for ( int idx=eventlist_.size()-1; idx>0; idx--)
    {
	if ( singleseeded_ )
	{
	    delete eventlist_.removeSingle( idx );
	    mousecoords_.removeSingle( idx );
	}
	else
	{
	    eventlist_[idx]->type = visBase::MouseClick;
	    eventlist_[idx]->buttonstate_ = (OD::ButtonState) butstate;
	}
    }

    const bool intersowing = intersow_ && eventlist_.size()>1;

    float bendthreshold = 2.0;
    mSettUse( get, "dTect.Seed dragging", "Bend threshold", bendthreshold );
    if ( intersowing )
	bendthreshold *= 2;
    if ( bendthreshold < 0.1 )
	bendthreshold = 0.1;

    BendPointFinder2D bpfinder( mousecoords_, bendthreshold );
    bpfinder.execute();

    bendpoints_.erase();

    const int last = intersowing ? eventlist_.size()-1
				 : bpfinder.bendPoints().size()-1;

    for ( int idx=0; idx<=last; idx++ )
    {
	int eventidx = idx;
	if ( alternatesowingorder_ )
	    eventidx = idx%2 ? last-idx/2 : idx/2;

	bendpoints_ += intersowing ? eventidx : bpfinder.bendPoints()[eventidx];
	if ( intersowing && bpfinder.bendPoints().isPresent(eventidx) )
	    bendpoints_ += eventidx;
    }

    if ( reversesowingorder_ )
	bendpoints_.reverse();

    if ( intersowing )
	bendpoints_.insert( 1,  bendpoints_[bendpoints_.size()-1] );

    mode_ = FirstSowing;
    int count = 0;
    const bool sow = bendpoints_.size()>1;
    if ( bendpoints_.size()>2 )
	sowing.trigger();

    while ( bendpoints_.size() )
    {
	int eventidx = bendpoints_[0];
	for ( int yn=1; yn>=0; yn-- )
	{
	    eventlist_[eventidx]->pressed = yn;
	    if ( eventcatcher_ )
		eventcatcher_->reHandle( *eventlist_[eventidx] );
	}

	bendpoints_.removeSingle( 0 );

	count++;
	if ( !intersowing || count>2 )
	    mode_ = SequentSowing;
    }

    if ( sow )
	sowingend.trigger();

    reset();
    mReturnHandled( true );
}


void Sower::reset()
{
    sowingline_->turnOn( false );
    for ( int idx=sowingline_->size()-1; idx>=0; idx-- )
	sowingline_->removePoint( idx );

    sowingline_->dirtyCoordinates();
    deepErase( eventlist_ );
    mousecoords_.erase();

    mode_ = Idle;
}


bool Sower::acceptTablet( const visBase::EventInfo& eventinfo )
{
    if ( !eventinfo.tabletinfo )
	mReturnHandled( false );

    const EM::PosID pid = getMarkerID( eventinfo );
    if ( pid != curpid_ )
    {
	curpidstamp_ = Time::getMilliSeconds();
	curpid_ = pid;
    }

    if ( eventinfo.tabletinfo->pointertype_ == TabletInfo::Eraser )
    {
	if ( !pid.isUdf() )
	    return acceptEraser( eventinfo );

	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    mDynamicCastGet( const visBase::MarkerSet*, markerset, dataobj );
	    if ( markerset )
		return acceptEraser( eventinfo );
	}

	mReturnHandled( true );
    }

    if ( mode_==Idle && eventinfo.type==visBase::MouseMovement &&
	 !pid.isUdf() && !mIsUdf(curpidstamp_) &&
	 Time::passedSince(curpidstamp_) > 300 )
    {
	curpidstamp_ = mUdf(int);
	return acceptLaser( eventinfo );
    }

    if ( !pid.isUdf() && mode_==Furrowing && singleseeded_ )
    {
	sowingline_->turnOn( false );
    }

    return acceptMouse( eventinfo );
}


bool Sower::acceptLaser( const visBase::EventInfo& eventinfo )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    mode_ = Lasering;

    visBase::EventInfo newevent( eventinfo );
    newevent.type = visBase::MouseClick;

    int butstate = newevent.buttonstate_ | lasermask_;
    newevent.buttonstate_ = (OD::ButtonState) butstate;

    for ( int yn=1; yn>=0; yn-- )
    {
	newevent.pressed = yn;
	if ( eventcatcher_ )
	    eventcatcher_->reHandle( newevent );
    }

    mode_ = Idle;
    mReturnHandled( true );
}


bool Sower::acceptEraser( const visBase::EventInfo& eventinfo )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    if ( eventinfo.type==visBase::MouseMovement &&
	 !eventinfo.tabletinfo->pressure_ )
	mReturnHandled( false );

    mode_ = Erasing;

    visBase::EventInfo newevent( eventinfo );
    newevent.type = visBase::MouseClick;

    int butstate = newevent.buttonstate_ | erasermask_;
    newevent.buttonstate_ = (OD::ButtonState) butstate;

    for ( int yn=1; yn>=0; yn-- )
    {
	newevent.pressed = yn;
	if ( eventcatcher_ )
	    eventcatcher_->reHandle( newevent );
    }

    mode_ = Idle;
    mReturnHandled( true );
}


void Sower::setSequentSowMask( bool yn, OD::ButtonState mask )
{ sequentsowmask_ = yn ? mask : OD::ButtonState(~OD::NoButton); }


void Sower::setIfDragInvertMask( bool yn, OD::ButtonState mask )
{ ifdraginvertmask_ = yn ? mask : OD::NoButton; }


void Sower::setLaserMask( bool yn, OD::ButtonState mask )
{ lasermask_ = yn ? mask : OD::NoButton; }


void Sower::setEraserMask( bool yn, OD::ButtonState mask )
{ erasermask_ = yn ? mask : OD::NoButton; }


EM::PosID Sower::getMarkerID( const visBase::EventInfo& eventinfo ) const
{
    if ( !editobject_ ) return EM::PosID::udf();

    mDynamicCastGet( const MPEEditor*, mpeeditor, editobject_ );
    if ( mpeeditor )
	return mpeeditor->mouseClickDragger( eventinfo.pickedobjids );

    mDynamicCastGet( const LocationDisplay*, locdisp, editobject_ );
    if ( locdisp )
    {
	const int knotid = locdisp->clickedMarkerIndex( eventinfo );
	EM::PosID posid;
	if ( knotid >= 0 )
	    posid.setSubID( knotid ); // knotid as a subid???

	return posid;
    }

    return EM::PosID::udf();
}

} // namespace visSurvey
