#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "keyenum.h"
#include "position.h"
#include "trigonometry.h"
#include "visdata.h"
#include "visosg.h"
#include "vistransform.h"

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

    EventType			type		= Any;
    OD::ButtonState		buttonstate_	= OD::NoButton;
    Coord			mousepos	= Coord::udf();
    Line3			mouseline;
				/*!< The line projected from the mouse-position
				     into the scene. Line is in display coords.
				*/
    double			pickdepth	= mUdf(double);
				//!< Mouseline parameter value of picked pos

    bool			pressed		= false;
				/*!< Only set if type == MouseClick or Keyboard
				     If false, the button has been released.
				*/
    bool			dragging	= false;
				//!< Only set if type == MouseMovement

    OD::KeyboardKey		key_;
				//!< Only set if type == Keyboard

    TypeSet<VisID>		pickedobjids;

    Coord3			displaypickedpos = Coord3::udf();
								// display space
    Coord3			localpickedpos = Coord3::udf(); // object space
    Coord3			worldpickedpos = Coord3::udf(); // world space

    TabletInfo*			tabletinfo_	= nullptr;
    void			setTabletInfo(const TabletInfo*);
};


mExpClass(visBase) EventCatcher : public DataObject
{
    friend class EventCatchHandler;

public:

    static RefMan<EventCatcher> create();
				mCreateDataObj(EventCatcher);

    void			setEventType(EventType);
    EventType			eventType() const { return type_; }

    void			releaseEventsPostOsg(bool yn);
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

    EventType			type_ = visBase::Any;
    RefObjectSet<Transformation> utm2display_;

    static const char*		eventtypestr();

    bool			ishandled_	= true;
    bool			rehandling_	= false;
    bool			rehandled_	= false;

    osg::Node*			osgnode_	= nullptr;
    EventCatchHandler*		eventcatchhandler_ = nullptr;

    ObjectSet<EventInfo>	eventqueue_;
    Threads::Lock		eventqueuelock_;
    bool			eventreleasepostosg_ = true;
    Timer*			eventreleasetimer_;
};

} // namespace visBase
