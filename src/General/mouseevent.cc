/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mouseevent.h"
#include "timefun.h"


float MouseEvent::getDefaultTrackpadZoomFactor() { return 0.02f; }
float MouseEvent::getDefaultMouseWheelZoomFactor() { return 0.1f; }


TabletInfo::TabletInfo()
    : eventtype_( None )
    , pointertype_ ( UnknownPointer )
    , device_( NoDevice )
    , globalpos_( 0, 0 )
    , pos_( 0, 0 )
    , pressure_( 0.0 )
    , rotation_( 0.0 )
    , tangentialpressure_( 0.0 )
    , uniqueid_( mUdf(od_int64) )
    , xtilt_( 0 )
    , ytilt_( 0 )
    , z_( 0 )
    , presstimestamp_( mUdf(int) )
    , maxpostpressdist_( mUdf(float) )
    , globalpresspos_( Geom::Point2D<int>::udf() )
{}


TabletInfo& TabletInfo::latestState()
{
    mDefineStaticLocalObject( TabletInfo, lateststate_, );
    return lateststate_;
}


const TabletInfo* TabletInfo::currentState()
{
    return latestState().eventtype_==None ||
	   latestState().eventtype_==LeaveProximity ? nullptr : &latestState();
}


void TabletInfo::updatePressData()
{
    if ( eventtype_ == Press )
    {
	presstimestamp_ = Time::getMilliSeconds();
	globalpresspos_ = TabletInfo::currentState()->globalpos_;
	maxpostpressdist_ = 0;
    }
    else if ( !mIsUdf(maxpostpressdist_) && maxpostpressdist_<postPressDist() )
	maxpostpressdist_ = postPressDist();
}


int TabletInfo::postPressTime() const
{
    return mIsUdf(presstimestamp_) ? mUdf(int)
				   : Time::passedSince(presstimestamp_);
}


float TabletInfo::postPressDist() const
{
    return globalpresspos_.isDefined()
	? sCast(float,globalpresspos_.distTo(globalpos_)) : mUdf(float);
}


float TabletInfo::maxPostPressDist() const
{ return maxpostpressdist_; }


MouseEvent::~MouseEvent()
{
    setTabletInfo( nullptr );
}


MouseEvent& MouseEvent::operator=( const MouseEvent& mouseevent )
{
    if ( &mouseevent == this )
	return *this;

     butstate_ = mouseevent.butstate_;
     pressed_ = mouseevent.pressed_;
     pos_ = mouseevent.pos_;
     dpos_ = mouseevent.dpos_;
     angle_ = mouseevent.angle_;
     setTabletInfo( mouseevent.tabletinfo_ );

     return *this;
}


void MouseEvent::setPos( const Geom::Point2D<double>& dpos )
{
    dpos_ = dpos;
    pos_.x = mNINT32( dpos.x );
    pos_.y = mNINT32( dpos.y );
}


Geom::Point2D<float> MouseEvent::getFPos() const
{
    Geom::Point2D<float> fpos; fpos.setFrom( dpos_ );
    return fpos;
}


const Geom::Point2D<double>& MouseEvent::getDPos() const
{
    return dpos_;
}

void MouseEvent::setButtonState( const OD::ButtonState& bs )
{ butstate_ = bs; }


bool MouseEvent::leftButton() const	{ return butstate_ & OD::LeftButton; }
bool MouseEvent::rightButton() const	{ return butstate_ & OD::RightButton; }
bool MouseEvent::middleButton() const	{ return butstate_ & OD::MidButton; }
bool MouseEvent::ctrlStatus() const	{ return butstate_ & OD::ControlButton;}
bool MouseEvent::altStatus() const	{ return butstate_ & OD::AltButton; }
bool MouseEvent::shiftStatus() const	{ return butstate_ & OD::ShiftButton; }


bool MouseEvent::operator ==( const MouseEvent& ev ) const
{ return butstate_ == ev.butstate_ && pos_==ev.pos_ && angle_==ev.angle_; }


TabletInfo* MouseEvent::tabletInfo()
{ return tabletinfo_; }


const TabletInfo* MouseEvent::tabletInfo() const
{ return tabletinfo_; }


void MouseEvent::setTabletInfo( const TabletInfo* newtabinf )
{
    if ( newtabinf )
    {
	if ( !tabletinfo_ )
	    tabletinfo_ = new TabletInfo();

	*tabletinfo_ = *newtabinf;
    }
    else if ( tabletinfo_ )
    {
	deleteAndNullPtr( tabletinfo_ );
    }
}


MouseEventHandler::MouseEventHandler()
    : buttonPressed(this)
    , buttonReleased(this)
    , movement(this)
    , doubleClick(this)
    , wheelMove(this)
    , event_(nullptr)
{}


MouseEventHandler::~MouseEventHandler()
{
    setEvent( nullptr );
}


void MouseEventHandler::setEvent( const MouseEvent* ev )
{
    if ( ev )
    {
	if ( !event_ )
	    event_ = new MouseEvent();

	*event_ = *ev;
	if ( !event_->tabletInfo() )
	    event_->setTabletInfo( TabletInfo::currentState() );
    }
    else if ( event_ )
    {
	delete event_;
	event_ = nullptr;
    }
}


#define mImplMouseEventHandlerFn(fn,trig) \
void MouseEventHandler::trigger##fn( const MouseEvent& ev ) \
{ \
    ishandled_ = false; \
    MouseEvent* parentevent = event_ ? new MouseEvent(*event_) : nullptr; \
    setEvent( &ev ); \
    trig.trigger(); \
    setEvent( parentevent ); \
}

mImplMouseEventHandlerFn(Movement,movement)
mImplMouseEventHandlerFn(ButtonPressed,buttonPressed)
mImplMouseEventHandlerFn(ButtonReleased,buttonReleased)
mImplMouseEventHandlerFn(DoubleClick,doubleClick)
mImplMouseEventHandlerFn(Wheel,wheelMove)


MouseCursorExchange::MouseCursorExchange()
    : notifier( this )
{}


MouseCursorExchange::Info::Info( const TrcKeyValue& tkv, float offset )
    : trkv_( tkv )
    , offset_( offset )
{}


//Gesture Events
GestureEventHandler::GestureEventHandler()
    : pinchnotifier(this)
    , ishandled_(false)
{}


GestureEventHandler::~GestureEventHandler()
{}


void GestureEventHandler::triggerPinchEvent( const GestureEvent& info )
{
    ishandled_ = false;
    currentevent_ = &info;
    pinchnotifier.trigger();
    currentevent_ = nullptr;
}


const GestureEvent*	GestureEventHandler::getPinchEventInfo() const
{
    return currentevent_;
}
