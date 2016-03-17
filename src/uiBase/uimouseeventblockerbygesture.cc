/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Sept 2014
________________________________________________________________________

-*/



#include "uimouseeventblockerbygesture.h"

#include <QGesture>
#include <QTouchEvent>
#include <QDateTime>

uiMouseEventBlockerByGestures::uiMouseEventBlockerByGestures( int delay_ms )
    : delaytime_(delay_ms)
    , gesturestatus_(Inactive)
    , gestureendtime_(*new QDateTime)
{
    eventfilter_.addEventType( uiEventFilter::Gesture );
    eventfilter_.addEventType( uiEventFilter::MouseButtonPress );
    eventfilter_.addEventType( uiEventFilter::MouseButtonRelease );
    eventfilter_.addEventType( uiEventFilter::MouseTrackingChange );
    eventfilter_.addEventType( uiEventFilter::MouseMove );

    mAttachCB( eventfilter_.eventhappened,
		uiMouseEventBlockerByGestures::qtEventCB );
}


uiMouseEventBlockerByGestures::~uiMouseEventBlockerByGestures()
{
    detachAllNotifiers();
    delete &gestureendtime_;
}


void uiMouseEventBlockerByGestures::attachToQObj( QObject* qobj )
{
    eventfilter_.attachToQObj( qobj );
}


void uiMouseEventBlockerByGestures::attach( uiBaseObject* uiobj )
{
    eventfilter_.attach( uiobj );
}


bool uiMouseEventBlockerByGestures::isGestureHappening()
{
    if ( gesturestatus_ == Active )
	return true;

     if ( gesturestatus_ == Inactive )
	return false;

    od_int64 passedtime = gestureendtime_.msecsTo(QDateTime::currentDateTime());

    if ( passedtime < delaytime_ )
	return true;

    gesturestatus_ = Inactive;
    return false;
}


void uiMouseEventBlockerByGestures::qtEventCB( CallBacker* )
{
    if ( updateFromEvent( eventfilter_.getCurrentEvent() ) )
    {
	eventfilter_.getCurrentEvent()->accept();
	eventfilter_.setBlockEvent( true );
    }
}


bool uiMouseEventBlockerByGestures::updateFromEvent( const QEvent* qev )
{
    const QEvent::Type eventtype = qev->type();
    if ( eventtype == QEvent::Gesture )
    {
	const QGestureEvent* gestureevent =
	    static_cast<const QGestureEvent*>(qev);

	QGesture* gesture = gestureevent->gesture(Qt::TapGesture);
	if ( !gesture ) gesture  = gestureevent->gesture(Qt::TapAndHoldGesture);
	if ( !gesture ) gesture  = gestureevent->gesture(Qt::PanGesture);
	if ( !gesture ) gesture  = gestureevent->gesture(Qt::PinchGesture);
	if ( !gesture ) gesture  = gestureevent->gesture(Qt::SwipeGesture);

	if ( gesture )
	{
	    const Qt::GestureState state = gesture->state();
	    if ( state==Qt::GestureStarted )
	    {
		gesturestatus_ = Active;
	    }
	    else if ( state==Qt::GestureFinished || state==Qt::GestureCanceled )
	    {
		gesturestatus_ = Finished;
		gestureendtime_ = QDateTime::currentDateTime();
	    }
	}

	return false;
    }

    if ( eventtype==QEvent::MouseButtonPress ||
	 eventtype==QEvent::MouseButtonRelease ||
	 eventtype==QEvent::MouseMove ||
	 eventtype==QEvent::MouseTrackingChange )
    {
	return isGestureHappening();
    }

    return false;
}
