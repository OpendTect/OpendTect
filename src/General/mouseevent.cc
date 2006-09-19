/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : September 2006
-*/

static const char* rcsID = "$Id: mouseevent.cc,v 1.2 2006-09-19 19:13:40 cvskris Exp $";

#include "mouseevent.h"

MouseEvent::MouseEvent(OD::ButtonState st, int xx, int yy )
    : butstate_( st )
    , pos_( xx, yy )
{}


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
