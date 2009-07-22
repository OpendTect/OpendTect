#ifndef mouseevent_h
#define mouseevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2005
 RCS:           $Id: mouseevent.h,v 1.8 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "keyenum.h"
#include "gendefs.h"
#include "geometry.h"

mClass MouseEvent
{
public:

 				MouseEvent( OD::ButtonState st=OD::NoButton,
					    int xx=0, int yy=0, float aa=0 )
				    : butstate_(st), pos_(xx,yy), angle_(aa)
				    			{}

    OD::ButtonState		buttonState() const	{ return butstate_; }
    const Geom::Point2D<int>&	pos() const		{ return pos_; }
    int				x() const		{ return pos_.x; }
    int				y() const		{ return pos_.y; }
    float			angle() const		{ return angle_; }
    				//!< used for wheel events

    bool			leftButton() const;
    bool			rightButton() const;
    bool			middleButton() const;
    bool			ctrlStatus() const;
    bool			altStatus() const;
    bool			shiftStatus() const;

    bool			operator ==( const MouseEvent& ev ) const;
    bool			operator !=( const MouseEvent& ev ) const
							{ return !(*this==ev); }

protected:

    OD::ButtonState		butstate_;
    Geom::Point2D<int>		pos_;
    float			angle_;

};

/*!
Handles mouse events. An instance of the MouseEventHandler is provided by
the object that detects the mouse-click, e.g. a gui or visualization object.

Once the event callback is recieved, it MUST check if someone else have
handled the event and taken necessary actions. If it is not already handled,
and your class handles it, it should set the isHandled flag to prevent
other objects in the callback chain to handle the event. It is often a good
idea to be very specific what events your function should handle to avoid
interference with other objects that are in the callback chain. As an example,
see the code below: The if-statement will only let right-clicks when no other
mouse or keyboard button are pressed through.

\code

void MyClass::handleMouseClick( CallBacker* cb )
{
    if ( eventhandler_->isHandled() )
    	return;

    const MouseEvent& event = eventhandler_->event();
    if ( event.rightButton() && !event.leftButton() && !event.middleButton() &&
    	 !event.ctrlStatus() && !event.altStatus() && !event.shiftStatus() )
    {
        eventhandler_->setHandled( true );
    	//show and handle menu
    }
}

/endcode

*/

mClass MouseEventHandler : public CallBacker
{
public:
    				MouseEventHandler();
    void			triggerMovement(const MouseEvent&);
    void			triggerButtonPressed(const MouseEvent&);
    void			triggerButtonReleased(const MouseEvent&);
    void			triggerDoubleClick(const MouseEvent&);
    void			triggerWheel(const MouseEvent&);

    Notifier<MouseEventHandler>	buttonPressed;
    Notifier<MouseEventHandler>	buttonReleased;
    Notifier<MouseEventHandler>	movement;
    Notifier<MouseEventHandler>	doubleClick;
    Notifier<MouseEventHandler>	wheelMove;

    bool			hasEvent() const	{ return event_; }
    const MouseEvent&		event() const		{ return *event_; }
    				/*!<\note only call in function triggered
				     by an event from this class. */

    bool			isHandled() const	{ return ishandled_; }
    void			setHandled(bool yn)	{ ishandled_ = yn; }

protected:

    const MouseEvent*		event_;
    bool			ishandled_;

};


#endif
