/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2006
-*/

static const char* rcsID = "$Id: mouseevent.cc,v 1.10 2010-10-06 13:41:25 cvsjaap Exp $";

#include "mouseevent.h"



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
    , tabletispressed_( false )
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

/*
#define mImplMouseEventHandlerFn(fn,trig) \
void MouseEventHandler::trigger##fn( const MouseEvent& ev ) \
{ \
    ishandled_ = false; \
    setEvent( &ev ); \
    trig.trigger(); \
    setEvent( 0 ); \
}

mImplMouseEventHandlerFn(Movement,movement)
mImplMouseEventHandlerFn(ButtonPressed,buttonPressed)
mImplMouseEventHandlerFn(ButtonReleased,buttonReleased)
mImplMouseEventHandlerFn(DoubleClick,doubleClick)
mImplMouseEventHandlerFn(Wheel,wheelMove)
*/

// Macro used by hack to repair Qt-Linux tablet bug
#define mTabletPressCheck( insync ) \
    if ( event_->tabletInfo()->eventtype_ == TabletInfo::Press ) \
    { \
	tabletispressed_ = true; \
	tabletinsyncwithmouse_ = insync; \
    }

void MouseEventHandler::triggerMovement( const MouseEvent& ev )
{
    ishandled_ = false;
    setEvent( &ev );

    // Hack to repair missing mouse release events from tablet pen on Linux
    if ( event_ && event_->tabletInfo() )
    {
	mTabletPressCheck( false );
	if ( tabletispressed_ )
	{
	    if ( event_->tabletInfo()->eventtype_==TabletInfo::Release ||
		 (!event_->tabletInfo()->pressure_ && !tabletinsyncwithmouse_) )
	    {
		event_->setButtonState( curtabletbutstate_ );
		tabletispressed_ = false;
		buttonReleased.trigger();
		setEvent( 0 );
		return;
	    }
	}
    }
    // End of hack

    movement.trigger();
    setEvent( 0 );
}


void MouseEventHandler::triggerButtonPressed( const MouseEvent& ev )
{
    ishandled_ = false;
    setEvent( &ev );

    // Hack to repair missing mouse release events from tablet pen on Linux
    if ( event_ && event_->tabletInfo() )
    {
	mTabletPressCheck( true );
	curtabletbutstate_ = event_->buttonState();
    }
    // End of hack

    buttonPressed.trigger();
    setEvent( 0 );
}


void MouseEventHandler::triggerButtonReleased( const MouseEvent& ev )
{
    ishandled_ = false;
    setEvent( &ev );

    // Hack to repair missing mouse release events from tablet pen on Linux
    if ( event_ && event_->tabletInfo() )
    {
	mTabletPressCheck( false );
	if ( !tabletispressed_ )
	{
	    movement.trigger();
	    setEvent( 0 );
	    return;
	}
	tabletispressed_ = false;
    }
    // End of hack

    buttonReleased.trigger();
    setEvent( 0 );
}


void MouseEventHandler::triggerDoubleClick( const MouseEvent& ev )
{
    ishandled_ = false;
    setEvent( &ev );

    // Hack to repair missing mouse release events from tablet pen on Linux
    if ( event_ && event_->tabletInfo() )
    {
	mTabletPressCheck( true );
	curtabletbutstate_ = event_->buttonState();
    }
    // End of hack

    doubleClick.trigger();
    setEvent( 0 );
}


void MouseEventHandler::triggerWheel( const MouseEvent& ev )
{
    ishandled_ = false;
    setEvent( &ev );
    wheelMove.trigger();
    setEvent( 0 );
}


MouseCursorExchange::MouseCursorExchange()
    : notifier( this )
{}


MouseCursorExchange::Info::Info( const Coord3& pos, float offset )
    : surveypos_( pos )
    , offset_( offset ) 
{}
