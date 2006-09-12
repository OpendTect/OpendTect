#ifndef mouseevent_h
#define mouseevent_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2005
 RCS:           $Id: mouseevent.h,v 1.1 2006-09-12 18:40:11 cvskris Exp $
________________________________________________________________________

-*/

#include "keyenum.h"
#include "gendefs.h"
#include "geometry.h"

class MouseEvent
{
public:

 				MouseEvent(OD::ButtonState st=OD::NoButton,
				      int xx=0, int yy=0 );

    OD::ButtonState		buttonState() const	{ return butstate_; }
    const Geom::Point2D<int>&	pos() const 		{ return pos_; }
    int				x() const		{ return pos_.x; }
    int				y() const		{ return pos_.y; }

    bool			operator ==( const MouseEvent& ev ) const;
    bool			operator !=( const MouseEvent& ev ) const;

protected:

    OD::ButtonState		butstate_;
    Geom::Point2D<int>		pos_;
};


class MouseEventHandler : public CallBacker
{
public:
    				MouseEventHandler();
    void			triggerMovement(const MouseEvent&);
    void			triggerButtonPressed(const MouseEvent&);
    void			triggerButtonReleased(const MouseEvent&);
    void			triggerDoubleClick(const MouseEvent&);

    Notifier<MouseEventHandler>	buttonPressed;
    Notifier<MouseEventHandler>	buttonReleased;
    Notifier<MouseEventHandler>	movement;
    Notifier<MouseEventHandler>	doubleClick;

    const MouseEvent&		event() const		{ return *event_; }
    bool			isHandled() const	{ return ishandled_; }
    void			setHandled(bool yn)	{ ishandled_ = yn; }

protected:
    const MouseEvent*		event_;
    bool			ishandled_;
};


#endif
