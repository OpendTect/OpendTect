/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2006
-*/

static const char* rcsID = "$Id: mouseevent.cc,v 1.3 2006-09-20 15:59:41 cvskris Exp $";

#include "mouseevent.h"

MouseEvent::MouseEvent(OD::ButtonState st, int xx, int yy )
    : butstate_( st )
    , pos_( xx, yy )
{}


OD::ButtonState MouseEvent::buttonState() const
{ return butstate_; }


bool MouseEvent::ctrlStatus() const
{ return butstate_ & OD::ControlButton; }


bool MouseEvent::altStatus() const
{ return butstate_ & OD::AltButton; }


bool MouseEvent::shiftStatus() const
{ return butstate_ & OD::ShiftButton; }


const Geom::Point2D<int>& MouseEvent::pos() const
{ return pos_; }


int MouseEvent::x() const
{ return pos_.x; }


int MouseEvent::y() const
{ return pos_.y; }


bool MouseEvent::operator ==( const MouseEvent& ev ) const
{ return butstate_ == ev.butstate_ && pos_==ev.pos_; } 


bool MouseEvent::operator !=( const MouseEvent& ev ) const
{ return !(*this == ev); }

MouseEventHandler::MouseEventHandler()
    : buttonPressed( this )
    , buttonReleased( this )
    , movement( this )
    , doubleClick( this )
{}


void MouseEventHandler::triggerMovement( const MouseEvent& ev )
{
    ishandled_ = false;
    event_ = &ev;
    movement.trigger();
}


void MouseEventHandler::triggerButtonPressed( const MouseEvent& ev )
{
    ishandled_ = false;
    event_ = &ev;
    buttonPressed.trigger();
}


void MouseEventHandler::triggerButtonReleased( const MouseEvent& ev )
{
    ishandled_ = false;
    event_ = &ev;
    buttonReleased.trigger();
}


void MouseEventHandler::triggerDoubleClick( const MouseEvent& ev )
{
    ishandled_ = false;
    event_ = &ev;
    doubleClick.trigger();
}
