#pragma once

/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
Date:          September 2007
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "sets.h"
#include "thread.h"
#include "callback.h"
#include "ptrman.h"
#include "refcount.h"
#include "geometry.h"
#include "enums.h"

class Coord3List;
class TaskRunner;

namespace Geometry
{

mExpClass(Geometry) PrimitiveSet : public ReferencedObject
{
public:
    enum	PrimitiveType{Points,Lines,Triangles,
			      LineStrips,TriangleStrip,TriangleFan,Other};
		mDeclareEnumUtils(PrimitiveType);

    virtual int			size() const				= 0;
    virtual int			get(int) const				= 0;
    virtual int			indexOf(const int)			= 0;
    virtual void		append( int )				= 0;
    virtual void		append(const int*,int num)		= 0;
    virtual void		setEmpty()				= 0;
    virtual void		getAll(TypeSet<int>&,bool) const;

    virtual PrimitiveType	getPrimitiveType() const;
    virtual void		setPrimitiveType(PrimitiveType tp);

protected:
				PrimitiveSet();
    PrimitiveType		primitivetype_;
};


mExpClass(Geometry) IndexedPrimitiveSet : public PrimitiveSet
{
public:
    static IndexedPrimitiveSet*	create(bool large);
				/*!<Set large if you will have larger indices
				    than 65535 */
    virtual int			pop()				= 0;
    virtual int			set(int,int)			= 0;
    virtual void		set(const int*,int num)		= 0;
};


mExpClass(Geometry) IndexedPrimitiveSetImpl : public IndexedPrimitiveSet
{
public:

    virtual int			size() const;
    virtual int			get(int) const;
    virtual int			indexOf(const int);
    virtual void		append( int );
    virtual void		append(const int*,int num);
    virtual void		setEmpty();
    virtual void		getAll(TypeSet<int>&,bool) const;

    virtual int			pop();
    virtual int			set(int,int);
    virtual void		set(const int*,int num);

protected:

    TypeSet<int>		indexset_;
};


mExpClass(Geometry) RangePrimitiveSet : public PrimitiveSet
{
public:
    static RangePrimitiveSet*	create();
    virtual void		setRange(const Interval<int>&)	= 0;
    virtual Interval<int>	getRange() const		= 0;
    virtual int			indexOf(const int)		= 0;
    virtual void		getAll(TypeSet<int>&,bool) const;
};


mExpClass(Geometry) PrimitiveSetCreator
{
public:
    virtual			~PrimitiveSetCreator() {}

    static PrimitiveSet*	create(bool indexed,bool large = false);
				/*!<Set large if you will have larger indices
				 than 65535 */
    static void			setCreator(PrimitiveSetCreator*);

protected:
    virtual PrimitiveSet*	doCreate(bool indexed,bool large)	= 0;

    static PtrMan<PrimitiveSetCreator>	creator_;
};


mExpClass(Geometry) PrimitiveSetCreatorDefImpl : public PrimitiveSetCreator
{
protected:
    virtual PrimitiveSet*	doCreate(bool indexed,bool large);
};


/*!A geometry that is defined by a number of coordinates (defined outside
   the class), by specifying connections between the coordinates. */

mExpClass(Geometry) IndexedGeometry
{
public:
    enum	Type { Points, Lines, Triangles, TriangleStrip, TriangleFan };
    enum	SetType { IndexSet = 0, RangeSet };

		IndexedGeometry(Type,Coord3List* coords=0,Coord3List* normals=0,
				Coord3List* texturecoords=0,
				SetType settype =IndexSet, bool large = false);
		/*!<If coords or normals are given, used indices will be
		    removed when object deleted or removeAll is called. If
		    multiple geometries are sharing the coords/normals,
		    this is probably not what you want. The bool "large:true"
		    indicates creating a geometry set by uint primitive
		    set type, otherwise set by ushort type*/
    virtual	~IndexedGeometry();

    void	removeAll(bool deep);
		/*!<deep will remove all things from lists (coords,normals++).
		    Non-deep will just leave them there */
    bool	isEmpty() const;

    bool	isHidden() const			{ return ishidden_; }
    void	hide(bool yn)				{ ishidden_ = yn; }


    void	  appendCoordIndices(const TypeSet<int>&,bool reverse=true);
    void	  setCoordIndices(const TypeSet<int>&);
    PrimitiveSet* getCoordsPrimitiveSet()	{ return primitiveset_; }
    Type	  getPrimitiveType() const	{ return primitivetype_; }
    SetType	  getPrimitiveSetType() const	{ return primitivesettype_; }

    mutable Threads::Lock		lock_;
    mutable bool			ischanged_;

    Type				primitivetype_;
    SetType				primitivesettype_;


protected:
    void	appendCoordIndicesAsTriangles(const TypeSet<int>&,bool);
    void	appendCoordIndicesAsTriangleStrips(const TypeSet<int>&);
    void	appendCoordIndicesAsTriangleFan(const TypeSet<int>&);

    bool				ishidden_;
    Coord3List*				coordlist_;
    Coord3List*				texturecoordlist_;
    Coord3List*				normallist_;
    Geometry::PrimitiveSet*		primitiveset_;
};


/*!Defines a shape with coordinates and connections between them. The shape
   is defined in an ObjectSet of IndexedGeometry. All IndexedGeometry share
   one common coordinate and normal list. */

mExpClass(Geometry) IndexedShape
{
public:
    virtual		~IndexedShape();

    virtual void	setCoordList(Coord3List* cl,Coord3List* nl=0,
				     Coord3List* texturecoords=0,
				     bool createnew = true);
    virtual bool	needsUpdate() const			{ return true; }
    virtual bool	update(bool forceall,TaskRunner* =0)	{ return true; }

    virtual void	setRightHandedNormals(bool) {}
    virtual void	removeAll(bool deep);
			/*!<deep will remove all things from lists
			    (coords,normals++). Non-deep will just leave them
			    there */


    virtual bool	createsNormals() const		{ return false; }
    virtual bool	createsTextureCoords() const	{ return false; }

    const ObjectSet<IndexedGeometry>&	getGeometry() const;
    ObjectSet<IndexedGeometry>&	getGeometry();
    const Coord3List*	coordList() const	{ return coordlist_; }
    Coord3List*		coordList()		{ return coordlist_; }

    const Coord3List*	normalCoordList() const { return normallist_; }
    Coord3List*		normalCoordList()	{ return normallist_; }

    const Coord3List*	textureCoordList() const{ return texturecoordlist_; }
    Coord3List*		textureCoordList()	{ return texturecoordlist_; }

    int			getVersion() const	{ return version_; }

protected:
			IndexedShape();

    Threads::Lock		geometrieslock_;
    ObjectSet<IndexedGeometry>	geometries_;

    Coord3List*			coordlist_;
    Coord3List*			normallist_;
    Coord3List*			texturecoordlist_;
    bool			righthandednormals_;

    void			addVersion();
				/*!<Should be called every time object is
				    changed. */
private:

    int				version_;
};

#define mGetIndexedShapeWriteLocker4Geometries() \
    Threads::Locker lckr( geometrieslock_, Threads::Locker::WriteLock )


mExpClass(Geometry) ExplicitIndexedShape : public IndexedShape
					 , public CallBacker
{
public:
			ExplicitIndexedShape()	{}
			~ExplicitIndexedShape()	{}

    const Coord3List*	normalCoordList() const	{ return normallist_; }
    Coord3List*		normalCoordList()	{ return normallist_; }

    const Coord3List*	textureCoordList() const{ return texturecoordlist_; }
    Coord3List*		textureCoordList()	{ return texturecoordlist_; }

    int			addGeometry(IndexedGeometry* ig);
    void		removeFromGeometries(const IndexedGeometry* ig);
    void		removeFromGeometries(int geoidx);
};


}; //namespace

