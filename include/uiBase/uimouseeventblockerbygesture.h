#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Sept 2014
________________________________________________________________________

-*/

#include "uieventfilter.h"

/*!\brief
    Blocks mouse events for specified time(ms) after a gesture event occured.
    It has own event filters to be atached to any QObject/uiBaseOjeets.

    It can even block special Qt events coming from some other sources via
    the updateFromEvent(QEvent*) function.
*/

class QDateTime;

mExpClass(uiBase) uiMouseEventBlockerByGestures : public CallBacker
{
public:
				uiMouseEventBlockerByGestures(int delay_ms=-1);
				~uiMouseEventBlockerByGestures();

    void			attachToQObj(QObject*);
    void			attach(uiBaseObject*);

    bool			isGestureHappening();

    bool			updateFromEvent(const QEvent*);
				//!< Blocks events coming from other source
				// Use if the filter fails to block these events
private:

    void			qtEventCB(CallBacker*);

    uiEventFilter		eventfilter_;
    QDateTime&			gestureendtime_;
    enum GestureStatus		{ Inactive, Active, Finished };
    GestureStatus		gesturestatus_;
    od_int64			delaytime_;
};
