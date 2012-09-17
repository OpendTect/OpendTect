/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: keyboardevent.cc,v 1.4 2009/07/22 16:01:32 cvsbert Exp $";

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
