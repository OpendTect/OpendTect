#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2005
________________________________________________________________________

-*/

#include "generalmod.h"
#include "keyenum.h"
#include "gendefs.h"
#include "geometry.h"
#include "position.h"
#include "trckeyvalue.h"


mExpClass(General) TabletInfo
{
    friend class	QtTabletEventFilter;

public:
			TabletInfo();

    enum		EventType { None=0, Move=87, Press=92, Release=93,
				    EnterProximity=171, LeaveProximity=172 };
    EventType		eventtype_;
			// Use mouse event-type if possible. The two might
			// mismatch because of a QTabletEvent bug for Linux.

    enum		PointerType { UnknownPointer, Pen, Cursor, Eraser };
    PointerType		pointertype_;

    enum		TabletDevice { NoDevice, Puck, Stylus, Airbrush,
				       FourDMouse, RotationStylus };
    TabletDevice	device_;

    Geom::Point2D<int>	globalpos_;
    Geom::Point2D<int>	pos_;

    double		pressure_;
    double		rotation_;
    double		tangentialpressure_;

    od_int64		uniqueid_;

    int			xtilt_;
    int			ytilt_;
    int			z_;

    int			postPressTime() const;
    float 		postPressDist() const;
    float		maxPostPressDist() const;

    static const TabletInfo*	currentState();

protected:

    int			presstimestamp_;
    float		maxpostpressdist_;
    Geom::Point2D<int>	globalpresspos_;

    static TabletInfo&	latestState();
    void		updatePressData();
};


mExpClass(General) MouseEvent
{
public:

				MouseEvent( OD::ButtonState st=OD::NoButton,
					    int xx=0, int yy=0, float aa=0 )
				    : butstate_(st), pressed_(false)
				    , angle_(aa), tabletinfo_(nullptr)
				{
				    pos_.setXY( xx, yy );
				    dpos_.setXY( xx, yy );
				}

				MouseEvent( OD::ButtonState st,
					    float xx, float yy, float aa=0 )
				    : butstate_(st), pressed_(false)
				    , angle_(aa), tabletinfo_(nullptr)
				{
				    Geom::Point2D<double> dpos;
				    dpos.setXY( xx, yy );
				    setPos( dpos );
				}

				MouseEvent( OD::ButtonState st,
					    double xx, double yy, float aa=0 )
				    : butstate_(st), pressed_(false)
				    , angle_(aa), tabletinfo_(nullptr)
				{
				    Geom::Point2D<double> dpos;
				    dpos.setXY( xx, yy );
				    setPos( dpos );
				}

				MouseEvent(const MouseEvent& me)
				    : tabletinfo_(nullptr)
				{ *this = me; }

				~MouseEvent();
    MouseEvent&			operator=(const MouseEvent&);

    OD::ButtonState		buttonState() const	{ return butstate_; }
    void			setButtonState(const OD::ButtonState&);

    bool			isPressed() const	{ return pressed_; }
    void			setPressed(bool yn)	{ pressed_ = yn; }

    void			setPos(const Geom::Point2D<double>&);
    const Geom::Point2D<int>&	pos() const		{ return pos_; }
    Geom::Point2D<float>	getFPos() const;
    const Geom::Point2D<double>& getDPos() const;
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

    TabletInfo*			tabletInfo();
    const TabletInfo*		tabletInfo() const;
    void			setTabletInfo(const TabletInfo*);

    static float		getDefaultTrackpadZoomFactor();
    static float		getDefaultMouseWheelZoomFactor();

protected:

    OD::ButtonState		butstate_;
    bool			pressed_;
    Geom::Point2D<int>		pos_;
    Geom::Point2D<double>	dpos_;
    float			angle_;
    TabletInfo*			tabletinfo_;
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

\endcode

*/

mExpClass(General) MouseEventHandler : public CallBacker
{
public:
				MouseEventHandler();
				~MouseEventHandler();

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

    void			setEvent(const MouseEvent*);
    MouseEvent*			event_;

    bool			ishandled_;
};


/*!Synchronizes cursor information between scenes. A window that catches a
   mouse movement may trigger the notifier. All windows interested in
   displaying a marker (or similar) at the current positions may subscribe to
   the notifier. */

mExpClass(General) MouseCursorExchange : public CallBacker
{
public:
				MouseCursorExchange();
    mExpClass(General) Info
    {
    public:
				Info(const TrcKeyValue&,
				     float offset=mUdf(float));

	TrcKeyValue		trkv_;
	float			offset_;
    };

    CNotifier<MouseCursorExchange,const Info&>	notifier;
};


/*!\brief Stores event information from gesture event */
mExpClass(General) GestureEvent
{
public:
			    GestureEvent( int xx, int yy, float sc, float angl )
			    : pos_(xx,yy),scale_(sc),angle_(angl)
			    {}


    const Geom::Point2D<int>&	pos() const	    { return pos_; }
    int				x() const	    { return pos_.x; }
    int				y() const	    { return pos_.y; }
    float			scale() const	    { return scale_; }
    float			angle() const	    { return angle_; }

    enum			State{ Started, Moving, Finished };
    State			getState() const { return state_; }
    void		        setState(State st) { state_ = st; }

protected:

    Geom::Point2D<int>		pos_;
    float			scale_;
    float			angle_;
    State			state_ = Started;
};


/*!\brief Handles gesture event and triggers notifier with GestureEventInfo

The callback function should look like this. It also has isHandled()
and setHandled() functions similar to the mouse events, to explicitly handle the
callback which prevents other objects in the chain to use it.

\code
void MyClass::handlePinchEventCB( CallBacker* cb )
{
    mDynamicCastGet(const GestureEventHandler*,evh,cb);

    if ( !evh || evh->isHandled() )
	return;

    const GestureEventInfo* gevinfo = evh->getPinchEventInfo();
    if ( !gevinfo )
	return;

    Geom::Point2D<int> pos = gevinfo->pos();
    // do some work

    evh->setHandled( true );
}

\endcode
*/

mExpClass(General) GestureEventHandler : public CallBacker
{
public:
				GestureEventHandler();
				~GestureEventHandler();


    void			triggerPinchEvent(
					    const GestureEvent& pinchevnt);
    //!<Only available during events
    const GestureEvent*		getPinchEventInfo() const;
    bool			isHandled() const	{ return ishandled_; }
    void			setHandled(bool yn)	{ ishandled_ = yn; }

    Notifier<GestureEventHandler> pinchnotifier;

private:

    const GestureEvent*		    currentevent_;
    bool			    ishandled_;
};

