#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "keyenum.h"
#include "visdata.h"
#include "visosg.h"
#include "position.h"
#include "trigonometry.h"

class TabletInfo;
class Timer;

namespace osg { class Node; }


namespace visBase
{

/*!\brief


*/

class EventCatchHandler;
class Detail;


enum EventType		{ Any=7, MouseClick=1, Keyboard=2, MouseMovement=4,
			  MouseDoubleClick=5};

mExpClass(visBase) EventInfo
{
public:
    				EventInfo();
				EventInfo(const EventInfo&);

				~EventInfo();
    EventInfo&			operator=(const EventInfo&);

    EventType			type;

    OD::ButtonState		buttonstate_;

    Coord			mousepos;

    Line3			mouseline;
    				/*!< The line projected from the mouse-position
				     into the scene. Line is in display coords.
				*/
    double			pickdepth;
				//!< Mouseline parameter value of picked pos

    bool			pressed;
				/*!< Only set if type == MouseClick or Keyboard
				     If false, the button has been released.
				*/
    bool			dragging;
    				//!< Only set if type == MouseMovement

    OD::KeyboardKey		key_;
    				//!< Only set if type == Keyboard

    TypeSet<int>		pickedobjids;

    Coord3			displaypickedpos;	// display space
    Coord3			localpickedpos; 	// object space
    Coord3			worldpickedpos; 	// world space

    TabletInfo*			tabletinfo;
    void			setTabletInfo(const TabletInfo*);
};


mExpClass(visBase) EventCatcher : public DataObject
{
    friend class EventCatchHandler;

public:

    static EventCatcher*	create()
				mCreateDataObj(EventCatcher);

    void			setEventType( int type );
    int				eventType() const { return type_; }

    void			releaseEventsPostOsg( bool yn );
				/*!True by default to enable scene update from
				   right-click menu dialogs, but only sound
				   when being last in event handling chain. */

    CNotifier<EventCatcher, const EventInfo&>		eventhappened;
    CNotifier<EventCatcher, const EventInfo&>		nothandled;

    bool			isHandled() const;
    void			setHandled();
    void			reHandle(const EventInfo&);

    void			setUtm2Display(ObjectSet<Transformation>&);

protected:
    				~EventCatcher();

    void			releaseEventsCB(CallBacker*);

    int				type_;
    ObjectSet<Transformation>	utm2display_;

    static const char*		eventtypestr();

    bool			ishandled_;
    bool			rehandling_;
    bool			rehandled_;

    osg::Node*			osgnode_;
    EventCatchHandler*		eventcatchhandler_;

    ObjectSet<EventInfo>	eventqueue_;
    Threads::Lock		eventqueuelock_;
    bool			eventreleasepostosg_;
    Timer*			eventreleasetimer_;
};

}; // Namespace


