/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2007
 RCS:		$Id: keyboardevent.cc,v 1.2 2008-07-03 13:02:18 cvskris Exp $
________________________________________________________________________

-*/

#include "keyboardevent.h"


KeyboardEvent::KeyboardEvent()
    : key_( OD::NoKey )
    , modifier_( OD::NoButton )
    , isrepeat_( false )
{}


bool KeyboardEvent::operator ==( const KeyboardEvent& ev ) const
{ return key_==ev.key_ && modifier_==ev.modifier_ && isrepeat_==ev.isrepeat_; }


bool KeyboardEvent::operator !=( const KeyboardEvent& ev ) const
{ return !(ev==*this); }


KeyboardEventHandler::KeyboardEventHandler()
    : keyPressed(this)
    , keyReleased(this)
    , event_(0)
{}


#define mImplKeyboardEventHandlerFn(fn,trig) \
void KeyboardEventHandler::trigger##fn( const KeyboardEvent& ev ) \
{ \
    ishandled_ = false; \
    event_ = &ev; \
    trig.trigger(); \
    event_ = 0; \
}

mImplKeyboardEventHandlerFn(KeyPressed,keyPressed)
mImplKeyboardEventHandlerFn(KeyReleased,keyReleased)
