#ifndef visevent_h
#define visevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "keyenum.h"
#include "visdata.h"
#include "position.h"
#include "trigonometry.h"

class SoEventCallback;
class TabletInfo;

namespace visBase
{

/*!\brief


*/

class Detail;


enum EventType		{ Any=7, MouseClick=1, Keyboard=2, MouseMovement=4 };

mClass EventInfo
{
public:
    				EventInfo();
				EventInfo(const EventInfo&);

				~EventInfo();
    EventInfo&			operator=(const EventInfo&);

    EventType			type;

    OD::ButtonState		buttonstate_;

    Line3			mouseline;
    				/*!< The line projected from the mouse-position
    						into the scene. The line is
						in world coordinates.
				*/
    bool			pressed;
				/*!< Only set if type==MouseClick or
				     type==Keyboard
				     If it is false, the button has been
				     released.
				*/

    TypeSet<int>		pickedobjids;
    Coord3			displaypickedpos;	//display space
    Coord3			localpickedpos; 	//object space
    Coord3			worldpickedpos; 	//world space

    Detail*			detail;
    void			setDetail(const Detail*);

    TabletInfo*			tabletinfo;
    void			setTabletInfo(const TabletInfo*);

    
    int				key;
    				/*!< Only set if type==Keyboard */

    				// These are always set
    Coord			mousepos;

};


mClass EventCatcher : public DataObject
{
public:

    static EventCatcher*	create()
				mCreateDataObj(EventCatcher);

    void			setEventType( int type );
    int				eventType() const { return type_; }

    CNotifier<EventCatcher, const EventInfo&>		eventhappened;

    CNotifier<EventCatcher, const EventInfo&>		nothandled;

    bool			isHandled() const;
    void			setHandled();
    void			reHandle(const EventInfo&);

    void			fillPar( IOPar&, TypeSet<int>& ) const;
    int				usePar( const IOPar& );

    void			setUtm2Display(ObjectSet<Transformation>&);

protected:

    bool			_init();
    void			removeCBs();
    void			setCBs();

				~EventCatcher();
    static void			internalCB( void*, SoEventCallback* );

    int				type_;
    SoEventCallback*		node_;
    ObjectSet<Transformation>	utm2display_;

    static const char*		eventtypestr();

    bool			rehandling_;
    bool			rehandled_;

    virtual SoNode*		gtInvntrNode();

};

}; // Namespace


#endif
