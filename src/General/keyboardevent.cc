/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: keyboardevent.cc,v 1.6 2012-05-02 15:11:34 cvskris Exp $";

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
