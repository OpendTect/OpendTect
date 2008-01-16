#ifndef delaunay_h
#define delaunay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Y.C. Liu
 Date:          January 2008
 RCS:           $Id: delaunay.h,v 1.2 2008-01-16 22:06:26 cvskris Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "task.h"
#include "position.h"

namespace Geometry
{

/*! Constructs a Delaunay triangulation of a set of 2D vertices.
    References: 
    1: "Voronoi diagrams - a study of a fundamental geometric data structure"
        by Franz Aurenhammer, ACM Computing Surveys,Vol 23, No3, 1991.
    2: "GEOMPACK -a software package for the generation of meshes using 
        geometric algorithms", by Barry Joe, Advances in Engineering Software, 
	Vol 13, pages 325-331, 1991. */ 

class DelaunayTriangulation
{

public:
    			DelaunayTriangulation(const TypeSet<Coord>& coords);
			~DelaunayTriangulation();

    bool		triangulate();

    const TypeSet<int>&	getCoordIndices() const	{ return result_; }
    			/*!<Coord indices are sorted in threes, i.e
			    ci[0], ci[1], ci[2] is the first triangle
			    ci[3], ci[4], ci[5] is the second triangle. */
    const TypeSet<int>&	getNeighborIndices() const { return neighbours_; }
    			/*!<Neighbor indices are sorted in threes, i.e.
			    ni[0] to ni[2] are the triangle indices of the
			    first triangle.

			    ni[3] to ni[5] are the triangle indices of the
			    second triangle.

			    negative neighbor index means that the triangle
			    edge lies on the side of the convex hull. */

protected:
    
    bool		createPermutation();
    bool		ensureDistinctness();
    int			findHealthyTriangle(int cn0,int cn1,int cn,int& lr);
    			/*<Start from point cn, search next point index which
			   makes a good triangle with cn0, cn1. */ 

    bool		createInitialTriangles(int pid,int lr);
    bool		insertTriangle(int idx,int& ledg,int& ltri);
    
    void		visibleEdge(int pid,int& lt,int& le,int& rt,int& re);
    void		findRightMostEdge(int& rtri,int& redg,int pid);
    void		findLeftMostEdge(int& ltri,int& ledg,int pid);
    int			swapEdge(int id,int& ltri,int& ledg);
    int			getSideOfLine(int pio,int pi1,int pid);
    			/*<For permuted index, check the point pid which is to 
			   the right of, on, left of the line pi0-pi1.
			   return +1, 0, -1 respectively. */

    int 		selectDiagonal(int cn0,int cn1,int cn2,int cn3) const;
    			/*!<Corners are in counterclockwise order. return +1 if
			    the diagonal 02 is chosen; -1 if the diagonal 13 is
			    chosen, 0 if four corners are cocircular.*/

    double		mEpsilon() const;
    int			wrap(int val,int lowlimit,int highlimit);

    TypeSet<int>	result_;
    TypeSet<int>	neighbours_;
    TypeSet<int>	permutation_;
    TypeSet<int>	stack_;
    const TypeSet<Coord>& coordlist_;
};


class ParallelDelaunayTriangulation : public SequentialTask
{
    			ParallelDelaunayTriangulation(const TypeSet<Coord>&);
			~ParallelDelaunayTriangulation();

    const TypeSet<int>&	getCoordIndices() const;
    			/*!<Coord indices are sorted in threes, i.e
			    ci[0], ci[1], ci[2] is the first triangle
			    ci[3], ci[4], ci[5] is the second triangle. */

    int			nextStep()
    			{
			    const int sz = permutation_.size();
			    if ( !sz ) return 0;
			    if ( !insertPoint( permutation_[sz-1] ) )
				return -1;

			    permutation_.remove( sz-1 );
			    return 1;
			}

protected:

    bool		insertPoint(int idx);
    char		isInside(int ci,int ti) const;
    			/*!<\retval -1 outside
			    \retval 0 on edge
			    \revval 1 inside*/
    bool		searchTriangle(int ci,TypeSet<int>& path) const;
    			/*!<Returns the triangle that has ci inside.*/
    bool		divideTriangle(int ci,int ti);
    			/*!ci is assumed to be inside triangle*/
    bool		splitTriangle(int ci,int ti); //Wrong interface
    			/*!ci is assumed to be inside triangle*/

    struct DAGTriangle
    {
			DAGTriangle();

	bool		operator==(const DAGTriangle&);

	int		coordindices_[3];
	int		childindices_[3];
    };

    int				curpt_;
    Coord3			initialcoords_[3];
    TypeSet<DAGTriangle>	triangles_;
    const TypeSet<Coord>&	coordlist_;
    TypeSet<int>		permutation_;
};

};
#endif

