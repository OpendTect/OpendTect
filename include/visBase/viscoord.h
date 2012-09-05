#ifndef viscoord_h
#define viscoord_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscoord.h,v 1.33 2012-09-05 09:35:09 cvskris Exp $
________________________________________________________________________


-*/
#include "visbasemod.h"
#include "callback.h"
#include "positionlist.h"
#include "thread.h"
#include "visdata.h"


class SoCoordinate3;
class SoGroup;
class UTMPosition;
class Executor;

namespace Geometry { class PosIdHolder; }

#define mOsgVecArrPtr	void*
#define mGetOsgVecArr(ptr) ((osg::Vec3Array*) ptr )

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

mClass(visBase) Coordinates : public DataObject
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

    mOsgVecArrPtr	osgArray() { return osgcoords_; }
    const mOsgVecArrPtr	osgArray() const { return osgcoords_; }

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
    
    mOsgVecArrPtr		osgcoords_;

    virtual SoNode*		gtInvntrNode();

};

    
mClass(visBase) CoinFloatVertexAttribList : public FloatVertexAttribList
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


mClass(visBase) CoordListAdapter : public Coord3List
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

