#ifndef visevent_h
#define visevent_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visevent.h,v 1.14 2004-09-14 12:16:01 kristofer Exp $
________________________________________________________________________


-*/

#include "visdata.h"
#include "position.h"
#include "trigonometry.h"

class SoEventCallback;

namespace visBase
{

/*!\brief


*/

class Detail;

    
enum EventType		{ Any=7, MouseClick=1, Keyboard=2, MouseMovement=4 };

class EventInfo
{
public:
    				EventInfo();
				~EventInfo();
    EventType			type;

    char			mousebutton;
				//!< Only set if type==MouseClick
    Line3			mouseline;
    				/*!<\The line projected from the mouse-position
    						into the scene. The line is
						in world coordinates.
				*/
    bool			pressed;
				/*!< Only set if type==MouseClick or
				     type==Keyboard
				     If it is false, the button has been
				     released.
				*/
    Transformation*		objecttoworldtrans;
    				/*!<The transformation from the coordinates
				    given to OI to the world coordinates
				    (i.e. the transform that is accumulated
				          while traversing the scene-graph).
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


class EventCatcher : public DataObject
{
public:
    static EventCatcher*	create()
				mCreateDataObj(EventCatcher);

    void			setEventType( int type );
    int				eventType() const { return type; }

    CNotifier<EventCatcher, const EventInfo&>		eventhappened;

    CNotifier<EventCatcher, const EventInfo&>		nothandled;

    SoNode*			getInventorNode();

    bool			isEventHandled() const;
    void			eventIsHandled();

    void			fillPar( IOPar&, TypeSet<int>& ) const;
    int				usePar( const IOPar& );

protected:
    void			_init();
    void			removeCBs();
    void			setCBs();

				~EventCatcher();
    static void			internalCB( void*, SoEventCallback* );

    int				type;
    SoEventCallback*		node;

    static const char*		eventtypestr;
};

}; // Namespace


#endif
