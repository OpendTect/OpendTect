#ifndef viscoord_h
#define viscoord_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscoord.h,v 1.16 2007-11-27 18:56:54 cvsyuancheng Exp $
________________________________________________________________________


-*/
#include "callback.h"
#include "positionlist.h"
#include "thread.h"
#include "visdata.h"

class SoCoordinate3;
class SoGroup;
class UTMPosition;
class Executor;

namespace Geometry { class PosIdHolder; }


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

    void		setDisplayTransformation(Transformation*);
    			/*!<\note All existing
			     coords will be recalculated back from the old
			     transformation and transformed by the new one.
			*/

    Transformation*	getDisplayTransformation();

    void		setLocalTranslation(const Coord&);
    Coord		getLocalTranslation() const;

    int			size(bool includedelete=false) const;
    int			addPos(const Coord3&);
    Coord3		getPos(int,bool scenespace=false) const;
    void		setPos(int,const Coord3&);
    void		insertPos(int,const Coord3&);
    void		removePos(int, bool keepidxafter=true );
    void		removeAfter(int);
    void		setAutoUpdate(bool);
    bool		autoUpdate();
    void		update();

    SoNode*		getInventorNode();
protected:
    void		getPositions(TypeSet<Coord3>&) const;
    void		setPositions(const TypeSet<Coord3>&);

    void		setPosWithoutLock(int, const Coord3&);
    			/*!< Object should be locked when calling */
    void		setLocalTranslationWithoutLock(const Coord&);
    			/*!< Object should be locked when calling */

    int			getFreeIdx();
    			/*!< Object should be locked when calling */

    			~Coordinates();
    SoCoordinate3*	coords;
    SoGroup*		root;
    UTMPosition*	utmposition;
    TypeSet<int>	unusedcoords;
    Threads::Mutex	mutex;
    Transformation*	transformation;
};


/*!Adapter between a CoordList and Coordinates. */


class CoordListAdapter : public CoordList
{
public:
    		CoordListAdapter(Coordinates&);

    int		add(const Coord3&);
    Coord3	get(int) const;
    void	set(int,const Coord3&);
    void	remove(int);

protected:
    virtual		~CoordListAdapter();
    Coordinates&	coords_;

};

};

#endif
