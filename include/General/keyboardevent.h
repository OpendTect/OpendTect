#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2007
________________________________________________________________________

-*/

#include "generalmod.h"
#include "keyenum.h"
#include "geometry.h"
#include "notify.h"

/*!
\brief Keyboard event.
*/

mExpClass(General) KeyboardEvent
{
public:

				KeyboardEvent();
				mImplSimpleEqOpers3Memb( KeyboardEvent,
				    key_, modifier_, isrepeat_ )


    OD::KeyboardKey		key_;
    OD::ButtonState		modifier_;	//Alt/Ctrl/Shift++
    bool			isrepeat_;

    static bool			isUnDo(const KeyboardEvent&);
    static bool			isReDo(const KeyboardEvent&);
    static bool			isSave(const KeyboardEvent&);
    static bool			isSaveAs(const KeyboardEvent&);
    static bool			isSelectAll(const KeyboardEvent&);

};


/*!
\brief Handles KeyboardEvent.
*/

mExpClass(General) KeyboardEventHandler : public CallBacker
{
public:
				KeyboardEventHandler();

    void			triggerKeyPressed(const KeyboardEvent&);
    void			triggerKeyReleased(const KeyboardEvent&);

    Notifier<KeyboardEventHandler>	keyPressed;
    Notifier<KeyboardEventHandler>	keyReleased;

    bool			hasEvent() const	{ return event_; }
    const KeyboardEvent&	event() const		{ return *event_; }
				/*!<\note only ok to call in function triggered
				     by an event from this class. */
    bool			isHandled() const	{ return ishandled_; }
    void			setHandled( bool yn )	{ ishandled_ = yn; }

protected:
    const KeyboardEvent*	event_;
    bool			ishandled_;
};
