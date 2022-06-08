#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/
#include "visbasemod.h"
#include "callback.h"
#include "positionlist.h"
#include "thread.h"
#include "visdata.h"
#include "visosg.h"


class UTMPosition;

namespace Geometry { class PosIdHolder; }

namespace osg { class Array; }


namespace visBase
{
class Transformation;
class Normals;

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

    Notifier<Coordinates>change;
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

protected:

    void		getPositions(TypeSet<Coord3>&) const;

    void		setPosWithoutLock(int, const Coord3&,bool scenespace);
			/*!< Object should be locked when calling */

    int			getFreeIdx();
			/*!< Object should be locked when calling */
    int			arraySize() const;

			~Coordinates();

    TypeSet<int>		unusedcoords_;
    mutable Threads::Mutex	mutex_;
    const mVisTrans*		transformation_;

    osg::Array*			osgcoords_;
    friend class	 SetOrGetCoordinates;
};


mExpClass(visBase) CoinFloatVertexAttribList : public FloatVertexAttribList
{
public:
			CoinFloatVertexAttribList(Coordinates&,Normals*);

    virtual int		size() const;
    virtual bool	setSize(int,bool cpdata);

    virtual void	setCoord(int,const float*);
    virtual void	getCoord(int,float*) const;

    virtual void	setNormal(int,const float*);
    virtual void	getNormal(int,float*) const;

    virtual void	setTCoord(int,const float*);
    virtual void	getTCoord(int,float*) const;

protected:
			~CoinFloatVertexAttribList();

    Normals*		normals_;
    Coordinates&	coords_;
};


/*!Adapter between a CoordList and Coordinates. */


mExpClass(visBase) CoordListAdapter : public Coord3List
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
    int		size() const			{ return coords_.size(); }
    void	remove(const TypeSet<int>&);

    Coordinates*    getCoordinates() { return &coords_; }

protected:
    virtual		~CoordListAdapter();
    Coordinates&	coords_;

};

};

