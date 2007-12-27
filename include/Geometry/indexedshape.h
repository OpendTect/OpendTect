#ifndef indexedshape_h
#define indexedshape_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl
Date:          September 2007
RCS:           $Id: indexedshape.h,v 1.2 2007-12-27 16:13:04 cvskris Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "thread.h"

class CoordList;
class TaskRunner;

namespace Geometry
{

/*!A geomtetry that is defined by a number of coordinates (defined outside
   the class), by specifying connections between the coordiates. */

class IndexedGeometry
{
public:
    enum	Type { Lines, TriangleStrip, TriangleFan };
    enum	NormalBinding { PerVertex, PerFace };

    		IndexedGeometry(Type,NormalBinding=PerFace,
				CoordList* coords=0, CoordList* normals=0);
		/*!<If coords or normals are given, used indices will be
		    removed when object deleted or removeAll is called. If
		    multiple geometries are sharing the coords/normals, 
		    this is probably not what you want. */
    virtual 	~IndexedGeometry();

    void	removeAll();


    Threads::Mutex	lock_;

    Type		type_;
    NormalBinding	normalbinding_;

    TypeSet<int>	coordindices_;
    TypeSet<int>	normalindices_;

    mutable bool	ischanged_;

protected:
    CoordList*		coordlist_;
    CoordList*		normallist_;
};


/*!Defines a shape with coodinates and connections between them. The shape
   is defined in an ObjectSet of IndexedGeometry. All IndexedGeometry share
   one common coordinate and normal list. */

class IndexedShape
{
public:
    virtual 		~IndexedShape();

    virtual void	setCoordList(CoordList* cl,CoordList* nl=0);
    virtual bool	needsUpdate() const			{ return true; }
    virtual bool	update(bool forceall,TaskRunner* =0)	{ return true; }

    virtual void	setRightHandedNormals(bool);
    virtual void	removeAll();

    const ObjectSet<IndexedGeometry>&	getGeometry() const;

protected:
    				IndexedShape();

    Threads::ReadWriteLock	geometrieslock_;
    ObjectSet<IndexedGeometry>	geometries_;

    CoordList*			coordlist_;
    CoordList*			normallist_;
    bool			righthandednormals_;
};

}; //namespace

#endif
