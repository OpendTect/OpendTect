#ifndef visevent_h
#define visevent_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visevent.h,v 1.10 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"
#include "position.h"

class SoEventCallback;

namespace visBase
{

/*!\brief


*/

class Detail;

    
enum EventType		{ Any, MouseClick, Keyboard, MouseMovement };

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

    TypeSet<int>		pickedobjids;    
    Coord3			pickedpos;
    Coord3			localpickedpos;
    Detail*			detail;
    
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
    static EventCatcher*	create()
				mCreateDataObj(EventCatcher);

    void			setEventType( EventType type );
    EventType			eventType() const { return type; }

    CNotifier<EventCatcher, const EventInfo&>		eventhappened;

    SoNode*			getData();

    bool			isEventHandled() const;
    void			eventIsHandled();

    void			fillPar( IOPar&, TypeSet<int>& ) const;
    int				usePar( const IOPar& );

protected:
    void			removeCBs();
    void			setCBs();

				~EventCatcher();
    static void			internalCB( void*, SoEventCallback* );

    EventType			type;
    SoEventCallback*		node;

    static const char*		eventtypestr;
};

}; // Namespace


#endif
