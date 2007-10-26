#ifndef keyboardevent_h
#define keyboardevent_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2007
 RCS:           $Id: keyboardevent.h,v 1.1 2007-10-26 05:33:13 cvsnanne Exp $
________________________________________________________________________

-*/

#include "keyenum.h"
#include "geometry.h"

class KeyboardEvent
{
public:
 				KeyboardEvent();

    const Geom::Point2D<int>&	pos() const	{ return pos_; }
    int				x() const	{ return pos_.x; }
    int				y() const	{ return pos_.y; }

    bool			operator ==(const KeyboardEvent&) const;
    bool			operator !=( const KeyboardEvent& ev ) const
				{ return !(*this==ev); }

protected:

    Geom::Point2D<int>		pos_;
};


class KeyboardEventHandler : public CallBacker
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
