#ifndef delaunay_h
#define delaunay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Y.C. Liu
 Date:          January 2008
 RCS:           $Id: delaunay.h,v 1.5 2008-04-18 16:48:22 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"
#include "task.h"
#include "thread.h"

class Coord2List;
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
    			DelaunayTriangulation(const Coord2List&);
			~DelaunayTriangulation();

    int			triangulate();
    			/*!<Return 1 if success;
			     0 if there are less than 3 points;
			    -1 if all the points(>2) are on the same line;
			    -2	fail for some other reasons.	 */

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
    
    void		createPermutation();
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
    const Coord2List&	coordlist_;
    int			totalsz_;
};


/*!For the triangulation, it will skip undefined or duplicated pointsi, all the 
   points should be in random order. We use Optimistic method to triangulate.*/
class DAGTriangleTree
{
public:
    			DAGTriangleTree(const TypeSet<Coord>&);
    virtual		~DAGTriangleTree()	{}	

    bool		insertPoint(int pointidx);
    bool		getCoordIndices(TypeSet<int>&) const;
    			/*!<Coord indices are sorted in threes, i.e
			    ci[0], ci[1], ci[2] is the first triangle
			    ci[3], ci[4], ci[5] is the second triangle. */
    void		setEpsilon(double err)	{ epsilon_ = err; }

protected:

    char		searchTriangle(int ci,int start, int& t0,int& t1) const;
    			/*!<return t0, t1 as the index of two triangles which
			    share on one edge of ci. if ci is in only one 
			    triangle, then t1=-1. */
    char		searchFurther( int ci,int ti0,int ti1,
				       int& nti0, int& nti1 ) const; 
    bool		searchTriangleOnEdge(int ci,int ti,int& resti) const;
   			/*!<assume ci is on the edge of ti.*/

    void		splitTriangleInside(int ci,int ti);
    			/*!ci is assumed to be inside the triangle ti. */
    void		splitTriangleOnEdge(int ci,int ti0,int ti1);
    			/*!ci is on the shared edge of triangles ti0, ti1. */
    void		legalizeTriangles(TypeSet<char>& v0s,TypeSet<char>& v1s,
					  TypeSet<int>& tis);
    			/*!Check neighbor triangle of the edge v0-v1 in ti, 
			   where v0, v1 are local vetex indices 0, 1, 2. */
    
    void		setNeighbors(const int& vetexid, const int& crd,
				     const int& ti,int& nb0,int& nb1);
    int			searchChild(int v0,int v1,int ti) const;
    char		isInside(int ci,int ti) const;
    			/*!<\assume ci is not a vertex of ti;
			     return -1 outside; 0 on one edge; 1 inside; */
    struct DAGTriangle
    {
			DAGTriangle();
	bool		operator==(const DAGTriangle&) const;

	int		coordindices_[3];
	int		childindices_[3];
	int		neighbors_[3];
    };

    Threads::ConditionVar	condvar_; 
    double			epsilon_;
    TypeSet<DAGTriangle>	triangles_;
    const TypeSet<Coord>*	coordlist_;
    Coord			initialcoords_[3]; 
    				/*!<-2,-3,-4 are their indices.*/
};


class ParallelDelaunayTriangulator : public ParallelTask
{
public:
			ParallelDelaunayTriangulator(const TypeSet<Coord>&);
	
    bool		getCoordIndices(TypeSet<int>&) const;
    bool		isDataRandom()		{ return israndom_; }
    void		dataIsRandom(bool yn)	{ israndom_ = yn; }

protected:

    int			totalNr() const 	{ return nrcoords_; }
    bool		doWork( int, int, int );
    bool		doPrepare(int);

    DAGTriangleTree	dagtritree_;
    TypeSet<int>	permutation_;
    int			nrcoords_;
    bool		israndom_;
};



#endif

