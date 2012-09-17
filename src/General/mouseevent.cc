/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2006
-*/

static const char* rcsID = "$Id: mouseevent.cc,v 1.12 2011/09/29 15:59:37 cvsjaap Exp $";

#include "mouseevent.h"
#include "timefun.h"



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
    static TabletInfo lateststate_;
    return lateststate_;
}


const TabletInfo* TabletInfo::currentState()
{
    return (latestState().eventtype_==None ||
	    latestState().eventtype_==LeaveProximity) ? 0 : &latestState();
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
    return globalpresspos_.isDefined() ? globalpresspos_.distTo(globalpos_)
				       : mUdf(float);
}


float TabletInfo::maxPostPressDist() const
{ return maxpostpressdist_; }


MouseEvent::~MouseEvent()
{
    setTabletInfo( 0 );
}


MouseEvent& MouseEvent::operator=( const MouseEvent& mouseevent )
{
    if ( &mouseevent == this )
	return *this;

     butstate_ = mouseevent.butstate_;
     pos_ = mouseevent.pos_;
     angle_ = mouseevent.angle_;
     setTabletInfo( mouseevent.tabletinfo_ );

     return *this;
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
	delete tabletinfo_;
	tabletinfo_ = 0;
    }
}


MouseEventHandler::MouseEventHandler()
    : buttonPressed(this)
    , buttonReleased(this)
    , movement(this)
    , doubleClick(this)
    , wheelMove(this)
    , event_(0)
{}


MouseEventHandler::~MouseEventHandler()
{
    setEvent( 0 );
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
	event_ = 0;
    }
}


#define mImplMouseEventHandlerFn(fn,trig) \
void MouseEventHandler::trigger##fn( const MouseEvent& ev ) \
{ \
    ishandled_ = false; \
    MouseEvent* parentevent = event_ ? new MouseEvent(*event_) : 0; \
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


MouseCursorExchange::Info::Info( const Coord3& pos, float offset )
    : surveypos_( pos )
    , offset_( offset ) 
{}
