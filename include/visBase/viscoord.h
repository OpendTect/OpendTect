#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "positionlist.h"
#include "thread.h"
#include "visdata.h"
#include "visosg.h"
#include "vistransform.h"


class UTMPosition;

namespace Geometry { class PosIdHolder; }

namespace osg { class Array; }


namespace visBase
{

class CoordinatesOsgImpl;

/*!\brief
A set of coordinates. The coordinates will be transformed by the
transformation before given to Coin, and transformed back when doing a
getPos.
\note The object will not react to changes in the transformation when it is
set
*/

mExpClass(visBase) Coordinates : public DataObject
{
public:

    static RefMan<Coordinates> create();
			mCreateDataObj(Coordinates);

    friend		class CoordinatesBuilder;
    friend		class CoordListAdapter;

    void		setDisplayTransformation(const mVisTrans*) override;
			/*!<\note All existing
			     coords will be recalculated back from the old
			     transformation and transformed by the new one. */

    const mVisTrans*	getDisplayTransformation() const override;

    void		copyFrom(const Coordinates&);

    int			nextID(int previd) const;
			//!<If previd == -1, first id is returned.
			//!<If -1 is returned, no more id's are available.
    int			size(bool includedelete=false) const;
    int			addPos(const Coord3&);
    Coord3		getPos(int,bool scenespace=false) const;
    bool		isDefined(int) const;
    void		setPos(int,const Coord3&);
    void		setPositions(const TypeSet<Coord3>&);
    void		setPositions(const Coord3*,int sz,int start,
				     bool scenespace=false);
    void		insertPos(int,const Coord3&);
    void		removePos(int, bool keepidxafter=true );
    void		removeAfter(int);
    void		setAllPositions(const Coord3& pos,int sz,int start);

    void		setAllZ(const float*,int sz,bool dotransf);

    osg::Array*		osgArray() { return osgcoords_; }
    const osg::Array*	osgArray() const { return osgcoords_; }

    void		setEmpty();
    bool		isEmpty() const { return size()==0; }
    void		dirty() const;

    Notifier<Coordinates>change;

protected:
			~Coordinates();

    void		getPositions(TypeSet<Coord3>&) const;

    void		setPosWithoutLock(int, const Coord3&,bool scenespace);
			/*!< Object should be locked when calling */

    int			getFreeIdx();
			/*!< Object should be locked when calling */
    int			arraySize() const;

    TypeSet<int>		unusedcoords_;
    mutable Threads::Mutex	mutex_;
    ConstRefMan<mVisTrans>	transformation_;
    osg::Array*			osgcoords_;

    friend class SetOrGetCoordinates;
};


/*!Adapter between a CoordList and Coordinates. */


mExpClass(visBase) CoordListAdapter : public Coord3List
{
public:
			CoordListAdapter(Coordinates&);

    int			nextID(int) const override;
    int			add(const Coord3&) override;
    Coord3		get(int) const override;
    void		set(int,const Coord3&) override;
    void		remove(int) override;
    bool		isDefined(int) const override;
    void		addValue(int,const Coord3&) override;
    int			size() const override	{ return coords_->size(); }
    void		remove(const TypeSet<int>&) override;

    Coordinates*	getCoordinates() { return coords_.ptr(); }

protected:
			~CoordListAdapter();

    RefMan<Coordinates> coords_;

};

} // namespace visBase
