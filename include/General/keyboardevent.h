#ifndef keyboardevent_h
#define keyboardevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2007
 RCS:           $Id: keyboardevent.h,v 1.5 2012-08-03 13:00:24 cvskris Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
#include "keyenum.h"
#include "geometry.h"

mClass(General) KeyboardEvent
{
public:
 				KeyboardEvent();

    OD::KeyboardKey		key_;		
    OD::ButtonState		modifier_;	//Alt/Ctrl/Shift++
    bool			isrepeat_;

    bool			operator ==(const KeyboardEvent&) const;
    bool			operator !=( const KeyboardEvent& ev ) const;
};


mClass(General) KeyboardEventHandler : public CallBacker
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


#endif

