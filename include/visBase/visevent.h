#ifndef visevent_h
#define visevent_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visevent.h,v 1.1 2002-04-08 07:52:45 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"
#include "position.h"
#include "geompos.h"

class SoEventCallback;

namespace visBase
{

/*!\brief


*/

enum EventType		{ MouseClick, Keyboard, MouseMovement };

class EventInfo
{
public:
    EventType			type;

    char			mousebutton;
				//!< Only set if type==MouseClick
    bool			pressed;
				/*!< Only set if type==MouseClick
				     If it is false, the button has been
				     released.
				*/

    int				pickedobjid;    
    				/* Set to -1 if nothing was picked */
    Geometry::Pos		pickedpos;
    					
    int				key;
    				/*!< Only set if type==Keyboard */

    				// These are always set
    Coord			mousepos;
    bool			shift;
    bool			ctrl;
    bool			alt;
};


class EventCatcher : public SceneObject
{
public:
    				EventCatcher( EventType );
				~EventCatcher();

    EventType			eventType() const { return type; }

    CNotifier<EventCatcher, EventInfo>		eventhappened;

    const SoNode*		getData() const;
protected:
    static void			internalCB( void*, SoEventCallback* );

    EventType			type;
    SoEventCallback*		node;
};

}; // Namespace


#endif
