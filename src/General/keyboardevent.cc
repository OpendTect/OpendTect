/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2007
 RCS:		$Id: keyboardevent.cc,v 1.1 2007-10-26 05:33:13 cvsnanne Exp $
________________________________________________________________________

-*/

#include "keyboardevent.h"


KeyboardEvent::KeyboardEvent()
    : pos_(0,0)
{}


bool KeyboardEvent::operator ==( const KeyboardEvent& ev ) const
{ return pos_==ev.pos_; } 



KeyboardEventHandler::KeyboardEventHandler()
    : keyPressed(this)
    , keyReleased(this)
    , event_(0)
{
}


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
