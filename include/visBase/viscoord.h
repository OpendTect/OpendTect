#ifndef viscoord_h
#define viscoord_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscoord.h,v 1.9 2004-01-05 09:43:47 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "position.h"
#include "visdata.h"

class SoCoordinate3;
class Executor;

namespace Geometry { class PosIdHolder; }
namespace Threads { class Mutex; };


namespace visBase
{
class Transformation;

/*!\brief
A set of coordinates. The coordinates will be transformed by the
transformation before given to Coin, and transformed back when doing a
getPos. 
\note The object will not react to changes in the transformation when it is
set
*/

class Coordinates : public DataObject
{
public:

    static Coordinates*	create()
			mCreateDataObj(Coordinates);
    friend		class CoordinatesBuilder;

    void		setTransformation( Transformation* );
    			/*!<\note All existing
			     coords will be recalculated back from the old
			     transformation and transformed by the new one.
			 */

    Transformation*	getTransformation();

    int			size(bool includedelete=false) const;
    int			addPos( const Coord3& );
    Coord3		getPos( int, bool scenespace=false ) const;
    void		setPos( int,  const Coord3& );
    void		removePos( int );
    void		setAutoUpdate( bool );
    bool		autoUpdate();
    void		update();

    SoNode*		getInventorNode();
protected:

    int			getFreeIdx();
    			/*!< Object should be locked when calling */

    			~Coordinates();
     SoCoordinate3*	coords;
     TypeSet<int>	unusedcoords;
     Threads::Mutex&	mutex;
     Transformation*	transformation;
};

class CoordinateMessage
{
public:
    enum Event		{ ChangedPos, NrChanged } event;
    unsigned long	coordnr;
    unsigned long	newnr; //Only set when NrChanged
};


};

#endif
