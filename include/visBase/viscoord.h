#ifndef viscoord_h
#define viscoord_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscoord.h,v 1.2 2003-01-03 08:15:15 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "position.h"
#include "vissceneobj.h"

class SoCoordinate3;
class Executor;

namespace Geometry { class PosIdHolder; }

namespace visBase
{
class CoordinatesBuilder;

class CoordinateMessage
{
public:
    enum Event		{ ChangedPos, NrChanged } event;
    unsigned long	coordnr;
    unsigned long	newnr; //Only set when NrChanged
};


/*!\brief
A set of coordinates along with some notification service. It can quicly be
build with a Geometry::PosIdHolder&, but that is not nessesary.
*/

class Coordinates : public SceneObject
{
public:

    static Coordinates*	create()
			mCreateDataObj(Coordinates);
    friend		class CoordinatesBuilder;

    Executor*		setPositions( Geometry::PosIdHolder&, bool connect );
    Coord3		getPos( int );

    void		notify( const CallBack& );
    void		removeNotification( const CallBack& cb );

    SoNode*		getData();
protected:
    				~Coordinates();
     void			hanldePosIdHolderCh( CallBacker* );

     Geometry::PosIdHolder*	posidholder;
     TypeSet<unsigned int>	posids;

     SoCoordinate3*		coords;
     CNotifier<visBase::Coordinates,
     	      const visBase::CoordinateMessage&> notifier; 
};

};

#endif
