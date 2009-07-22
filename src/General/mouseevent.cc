/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2006
-*/

static const char* rcsID = "$Id: mouseevent.cc,v 1.7 2009-07-22 16:01:32 cvsbert Exp $";

#include "mouseevent.h"

bool MouseEvent::leftButton() const	{ return butstate_ & OD::LeftButton; }
bool MouseEvent::rightButton() const	{ return butstate_ & OD::RightButton; }
bool MouseEvent::middleButton() const	{ return butstate_ & OD::MidButton; }
bool MouseEvent::ctrlStatus() const	{ return butstate_ & OD::ControlButton;}
bool MouseEvent::altStatus() const	{ return butstate_ & OD::AltButton; }
bool MouseEvent::shiftStatus() const	{ return butstate_ & OD::ShiftButton; }


bool MouseEvent::operator ==( const MouseEvent& ev ) const
{ return butstate_ == ev.butstate_ && pos_==ev.pos_ && angle_==ev.angle_; } 


MouseEventHandler::MouseEventHandler()
    : buttonPressed(this)
    , buttonReleased(this)
    , movement(this)
    , doubleClick(this)
    , wheelMove(this)
    , event_(0)
{
}


#define mImplMouseEventHandlerFn(fn,trig) \
void MouseEventHandler::trigger##fn( const MouseEvent& ev ) \
{ \
    ishandled_ = false; \
    event_ = &ev; \
    trig.trigger(); \
    event_ = 0; \
}

mImplMouseEventHandlerFn(Movement,movement)
mImplMouseEventHandlerFn(ButtonPressed,buttonPressed)
mImplMouseEventHandlerFn(ButtonReleased,buttonReleased)
mImplMouseEventHandlerFn(DoubleClick,doubleClick)
mImplMouseEventHandlerFn(Wheel,wheelMove)
