#ifndef indexedshape_h
#define indexedshape_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
Date:          September 2007
RCS:           $Id: indexedshape.h,v 1.17 2012-07-10 14:08:30 cvskris Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "thread.h"
#include "callback.h"

class Coord3List;
class TaskRunner;

namespace Geometry
{

/*!A geomtetry that is defined by a number of coordinates (defined outside
   the class), by specifying connections between the coordiates. */

mClass IndexedGeometry
{
public:
    enum	Type { Points, Lines, Triangles, TriangleStrip, TriangleFan };
    enum	NormalBinding { PerVertex, PerFace };

    		IndexedGeometry(Type,NormalBinding=PerFace,
				Coord3List* coords=0, Coord3List* normals=0,
				Coord3List* texturecoords=0);
		/*!<If coords or normals are given, used indices will be
		    removed when object deleted or removeAll is called. If
		    multiple geometries are sharing the coords/normals, 
		    this is probably not what you want. */
    virtual 	~IndexedGeometry();

    void	removeAll(bool deep);
		/*!<deep will remove all things from lists (coords,normals++).
		    Non-deep will just leave them there */
    bool	isEmpty() const;

    bool	isHidden() const			{ return ishidden_; }
    void	hide(bool yn)				{ ishidden_ = yn; }


    mutable Threads::Mutex	lock_;

    Type			type_;
    NormalBinding		normalbinding_;

    TypeSet<int>		coordindices_;
    TypeSet<int>		texturecoordindices_;
    TypeSet<int>		normalindices_;

    mutable bool		ischanged_;

protected:
    bool			ishidden_;

    Coord3List*			coordlist_;
    Coord3List*			texturecoordlist_;
    Coord3List*			normallist_;
};


/*!Defines a shape with coodinates and connections between them. The shape
   is defined in an ObjectSet of IndexedGeometry. All IndexedGeometry share
   one common coordinate and normal list. */

mClass IndexedShape
{
public:
    virtual 		~IndexedShape();

    virtual void	setCoordList(Coord3List* cl,Coord3List* nl=0,
	    			     Coord3List* texturecoords=0);
    virtual bool	needsUpdate() const			{ return true; }
    virtual bool	update(bool forceall,TaskRunner* =0)	{ return true; }

    virtual void	setRightHandedNormals(bool);
    virtual void	removeAll(bool deep);
    			/*!<deep will remove all things from lists
			    (coords,normals++). Non-deep will just leave them
			    there */


    virtual bool	createsNormals() const 		{ return false; }
    virtual bool	createsTextureCoords() const 	{ return false; }

    const ObjectSet<IndexedGeometry>&	getGeometry() const;

    const Coord3List*		coordList() const 	{ return coordlist_; }
    Coord3List*			coordList()	 	{ return coordlist_; }

    int				getVersion() const	{ return version_; }

protected:
    				IndexedShape();

    Threads::ReadWriteLock	geometrieslock_;
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


mClass ExplicitIndexedShape : public IndexedShape, public CallBacker
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

#endif
