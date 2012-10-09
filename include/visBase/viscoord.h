#ifndef viscoord_h
#define viscoord_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
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

mClass Coordinates : public DataObject
{
public:

    static Coordinates*	create()
			mCreateDataObj(Coordinates);
    friend		class CoordinatesBuilder;
    friend		class CoordListAdapter;

    void		setDisplayTransformation(const mVisTrans*);
    			/*!<\note All existing
			     coords will be recalculated back from the old
			     transformation and transformed by the new one.
			*/

    const mVisTrans*	getDisplayTransformation() const;

    void		copyFrom(const Coordinates&);

    void		setLocalTranslation(const Coord&);
    Coord		getLocalTranslation() const;

    int			nextID(int previd) const;
			//!<If previd == -1, first id is returned.
			//!<If -1 is returned, no more id's are available.
    int			size(bool includedelete=false) const;
    int			addPos(const Coord3&);
    Coord3		getPos(int,bool scenespace=false) const;
    bool		isDefined(int) const;
    void		setPos(int,const Coord3&);
    void		setPositions(const Coord3*,int sz,int start);
    void		insertPos(int,const Coord3&);
    void		removePos(int, bool keepidxafter=true );
    void		removeAfter(int);

    void		setAllZ(const float*,int sz,float zscale=1);

    void		setAutoUpdate(bool);
    bool		autoUpdate();
    void		update();

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

    SoCoordinate3*		coords_;
    SoGroup*			root_;
    UTMPosition*		utmposition_;
    TypeSet<int>		unusedcoords_;
    mutable Threads::Mutex	mutex_;
    const mVisTrans*		transformation_;

    virtual SoNode*		gtInvntrNode();

};


/*!Adapter between a CoordList and Coordinates. */


mClass CoordListAdapter : public Coord3List
{
public:
    		CoordListAdapter(Coordinates&);

    int		nextID(int) const;
    int		add(const Coord3&);
    Coord3	get(int) const;
    void	set(int,const Coord3&);
    void	remove(int);
    bool	isDefined(int) const;
    void	addValue(int,const Coord3&);
    int		getSize() const	{ return coords_.size(); }

protected:
    virtual		~CoordListAdapter();
    Coordinates&	coords_;

};

};

#endif
