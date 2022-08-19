/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "keyboardevent.h"
#include "odver.h"
#include "odplatform.h"

KeyboardEvent::KeyboardEvent()
    : key_(OD::KB_NoKey)
    , modifier_(OD::NoButton)
    , isrepeat_(false)
{
}


bool KeyboardEvent::operator ==( const KeyboardEvent& ev ) const
{
    return key_==ev.key_ && modifier_==ev.modifier_ && isrepeat_==ev.isrepeat_;
}


bool KeyboardEvent::operator !=( const KeyboardEvent& ev ) const
{ return !(ev==*this); }


bool KeyboardEvent::isUnDo( const KeyboardEvent& kbe )
{
    const OD::ButtonState bs =
	OD::ButtonState( kbe.modifier_ & OD::KeyButtonMask );
    return bs==OD::ControlButton && kbe.key_==OD::KB_Z && !kbe.isrepeat_;
}


bool KeyboardEvent::isReDo( const KeyboardEvent& kbe )
{
    const OD::ButtonState bs =
	OD::ButtonState(kbe.modifier_ & OD::KeyButtonMask);

    const OD::Platform platform = OD::Platform::local();

    if ( platform.isWindows() || platform.isLinux() )
	return bs==OD::ControlButton && kbe.key_==OD::KB_Y && !kbe.isrepeat_;
    else if ( platform.isMac() )
	return OD::ctrlKeyboardButton(bs) && OD::shiftKeyboardButton(bs) &&
	    kbe.key_==OD::KB_Z && !kbe.isrepeat_;

    return false;
}


bool KeyboardEvent::isSave( const KeyboardEvent& kbe )
{
    const OD::ButtonState bs =
	OD::ButtonState( kbe.modifier_ & OD::KeyButtonMask );
    return bs == OD::ControlButton && 
	!OD::shiftKeyboardButton(bs) && 
	kbe.key_==OD::KB_S && !kbe.isrepeat_;
}


bool KeyboardEvent::isSaveAs( const KeyboardEvent& kbe )
{
    const OD::ButtonState bs =
	OD::ButtonState( kbe.modifier_ & OD::KeyButtonMask );
    const bool ctrlplusshift = 
	OD::ctrlKeyboardButton(bs) && OD::shiftKeyboardButton(bs) ;
    return ctrlplusshift && kbe.key_==OD::KB_S && !kbe.isrepeat_;
}


bool KeyboardEvent::isSelectAll( const KeyboardEvent& kbe )
{
    const OD::ButtonState bs =
	OD::ButtonState( kbe.modifier_ & OD::KeyButtonMask );
    return bs==OD::ControlButton && kbe.key_==OD::KB_A && !kbe.isrepeat_;
}


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
