#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "keyenum.h"
#include "geometry.h"

/*!
\brief Keyboard event.
*/

mExpClass(General) KeyboardEvent
{
public:
 				KeyboardEvent();

    OD::KeyboardKey		key_;
    OD::ButtonState		modifier_;	//Alt/Ctrl/Shift++
    bool			isrepeat_;

    bool			operator ==(const KeyboardEvent&) const;
    bool			operator !=( const KeyboardEvent& ev ) const;
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
