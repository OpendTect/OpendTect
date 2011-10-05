/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : December 2010
-*/

static const char* rcsID = "$Id: vissower.cc,v 1.5 2011-10-05 14:59:30 cvsjaap Exp $";


#include "vissower.h"

#include "bendpointfinder.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "timefun.h"
#include "visevent.h"
#include "vislocationdisplay.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"


namespace visSurvey
{


Sower::Sower( const visBase::VisualObjectImpl* editobj )
    : visBase::VisualObjectImpl( false )
    , editobject_( editobj )
    , eventcatcher_( 0 )
    , mode_( Idle )
    , sowingline_( visBase::PolyLine::create() )
    , linelost_( false )
    , singleseeded_( true )
    , curpid_( EM::PosID::udf() )
    , curpidstamp_( mUdf(int) )
{
    sowingline_->ref();
    addChild( sowingline_->getInventorNode() );
    sowingline_->setMaterial( visBase::Material::create() );
    reInitSettings();
}


Sower::~Sower()
{
    removeChild( sowingline_->getInventorNode() );
    sowingline_->unRef();
    deepErase( eventlist_ );
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


void Sower::setDisplayTransformation( visBase::Transformation* transformation )
{ sowingline_->setDisplayTransformation( transformation ); }


void Sower::setEventCatcher( visBase::EventCatcher* eventcatcher )
{ eventcatcher_ = eventcatcher; }


#define mReturnHandled( yn ) \
{ \
    if ( yn && eventcatcher_ ) eventcatcher_->setHandled(); \
    return yn; \
}

bool Sower::activate( const Color& color, const visBase::EventInfo& eventinfo )
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


bool Sower::accept( const visBase::EventInfo& eventinfo )
{
    if ( eventinfo.tabletinfo )
	return acceptTablet( eventinfo );

    return acceptMouse( eventinfo );
}


bool Sower::acceptMouse( const visBase::EventInfo& eventinfo )
{
    if ( mode_==Idle &&
	 eventinfo.type==visBase::MouseClick && !eventinfo.pressed )
    {

	const EM::PosID pid = getMarkerID( eventinfo );
	if ( pid.isUdf() )
	    mReturnHandled( true );
    }

    if ( mode_ != Furrowing )
	mReturnHandled( false );

    if ( eventinfo.type == visBase::Keyboard )
	mReturnHandled( true );

    const int sz = eventlist_.size();
    if ( eventinfo.type==visBase::MouseMovement || eventinfo.pressed )
    {
	if ( sz && eventinfo.mousepos==eventlist_[sz-1]->mousepos )
	    mReturnHandled( true );

	if ( sz && eventinfo.pickedobjids!=eventlist_[0]->pickedobjids )
	{
	    if ( eventinfo.worldpickedpos.isDefined() && !linelost_ )
		sowingline_->addPoint( eventinfo.worldpickedpos );
	    else
		linelost_ = true;

	    mReturnHandled( true );
	}

	linelost_ = false;
	sowingline_->addPoint( eventinfo.worldpickedpos );

	if ( !sz )
	    singleseeded_ = true;
	else if ( eventinfo.mousepos.distTo(eventlist_[0]->mousepos) > 5 )
	    singleseeded_ = false;

	eventlist_ += new visBase::EventInfo( eventinfo );
	mousecoords_ += eventinfo.mousepos;
	mReturnHandled( true );
    }

    if ( !sz || !sowingline_->isOn() )
    {
	reset();
	mReturnHandled( true );
    }

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( Time::passedSince(furrowstamp_) < 200 )
	singleseeded_ = true;

    int butstate = eventlist_[0]->buttonstate_;
    if ( !singleseeded_ )
	butstate ^= ifdraginvertmask_;

    eventlist_[0]->buttonstate_ = (OD::ButtonState) butstate;
    butstate &= sequentsowmask_;

    for ( int idx=sz-1; idx>0; idx--)
    {
	if ( singleseeded_ )
	{
	    eventlist_.remove( idx );
	    mousecoords_.remove( idx );
	}
	else
	{
	    eventlist_[idx]->type = visBase::MouseClick;
	    eventlist_[idx]->buttonstate_ = (OD::ButtonState) butstate;
	}
    }

    BendPointFinder2D bpfinder ( mousecoords_, 2 );
    bpfinder.execute( true );

    bendpoints_.erase();

    const bool intersowing = intersow_ && eventlist_.size()>1;

    const int last = intersowing ? eventlist_.size()-1
				 : bpfinder.bendPoints().size()-1;

    for ( int idx=0; idx<=last; idx++ )
    {
	int eventidx = idx;
	if ( alternatesowingorder_ )
	    eventidx = idx%2 ? last-idx/2 : idx/2;

	bendpoints_ += intersowing ? eventidx : bpfinder.bendPoints()[eventidx];
	if ( intersowing && bpfinder.bendPoints().indexOf(eventidx)>=0 )
	    bendpoints_ += eventidx;
    }

    if ( reversesowingorder_ )
	bendpoints_.reverse();

    if ( intersowing )
	bendpoints_[0] = bendpoints_[bendpoints_.size()-1];

    mode_ = FirstSowing;
    int count = 0;
    while ( bendpoints_.size() )
    {
	int eventidx = bendpoints_[0];
	for ( int yn=1; yn>=0; yn-- )
	{
	    eventlist_[eventidx]->pressed = yn;
	    if ( eventcatcher_ )
		eventcatcher_->reHandle( *eventlist_[eventidx] );
	}

	bendpoints_.remove( 0 );

	if ( !intersowing || count++ )
	    mode_ = SequentSowing;
    }

    reset();
    mReturnHandled( true );
}


void Sower::reset()
{
    sowingline_->turnOn( false );
    for ( int idx=sowingline_->size()-1; idx>=0; idx-- )
	sowingline_->removePoint( idx );

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
	    mDynamicCastGet( const visBase::Marker*, marker, dataobj );
	    if ( marker )
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
	sowingline_->turnOn( false );

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
	const int knotid = locdisp->isMarkerClick( eventinfo.pickedobjids );
	return knotid<0 ? EM::PosID::udf() : EM::PosID( 0, 0, knotid );
    }

    return EM::PosID::udf();
}


}; //namespace
