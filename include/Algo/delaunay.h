#ifndef delaunay_h
#define delaunay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Y.C. Liu
 Date:          January 2008
 RCS:           $Id: delaunay.h,v 1.12 2008-06-23 20:17:32 cvskris Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"
#include "task.h"
#include "thread.h"

/*!Reference: "Parallel Incremental Delaunay Triangulation", by Kohout J.2005.
   For the triangulation, it will skip undefined or duplicated points, all the 
   points should be in random order. We use Optimistic method to triangulate.*/
class DAGTriangleTree
{
public:
    			DAGTriangleTree();
    			DAGTriangleTree(const DAGTriangleTree&);
    virtual		~DAGTriangleTree();
    DAGTriangleTree&	operator=(const DAGTriangleTree&);

    static bool		computeCoordRanges(const TypeSet<Coord>&,
	    		    Interval<double>&,Interval<double>&);

    bool		setCoordList(const TypeSet<Coord>&,bool copy);
    const TypeSet<Coord>& coordList() const { return *coordlist_; }

    bool		setBBox(const Interval<double>& xrg,
	    			const Interval<double>& yrg);

    bool		isOK() const { return triangles_.size(); }

    bool		init();

    bool		insertPoint(int pointidx, int& dupid,
	    			    unsigned char lockid=0);
    int			insertPoint(const Coord&, int& dupid,
	    			    unsigned char lockid=0);
    bool		getTriangle(const Coord&,int& dupid,
	    			    TypeSet<int>& vertexindices,
				    unsigned char lockid=0);
    			/*!<search triangle contains the point.return crds. */
    bool		getCoordIndices(TypeSet<int>&) const;
    			/*!<Coord indices are sorted in threes, i.e
			    ci[0], ci[1], ci[2] is the first triangle
			    ci[3], ci[4], ci[5] is the second triangle. */
    bool		getConnections(int pointidx,TypeSet<int>&) const;
    void		setEpsilon(double err)	{ epsilon_ = err; }

    void		dumpTo(std::ostream&) const;
    			//!<Dumps all triangles to stream;

    static int		cNoVertex()	{ return -1; }
protected:
    static char		cIsOnEdge() 	{ return 0; }
    static char		cNotOnEdge() 	{ return 1; }
    static char		cIsInside()	{ return 2; }
    static char		cIsDuplicate()	{ return 3; }
    static char		cIsOutside()	{ return 4; }
    static char		cError()	{ return -1; }

    static int		cNoTriangle()	{ return -1; }
    static int		cInitVertex0()	{ return -2; }
    static int		cInitVertex1()	{ return -3; }
    static int		cInitVertex2()	{ return -4; }

    char	getCommonEdge(int ti0,int ti1,int lockid) const;
    		/*!return the common edge in ti0. */
    char	searchTriangle(int ci,int start, int& t0,int& t1,
	    		       int& dupid, unsigned char lockid);
    char	searchFurther( int ci,int& nti0,int& nti1, int& dupid,
			       unsigned char lockid); 
    char	searchTriangleOnEdge(int ci,int ti,int& resti,
	    			     char& edge, int& did,
	    			     unsigned char lockid);
   		/*!<assume ci is on the edge of ti.*/
    int		searchNeighbor(int ti,char edge,
	    		       unsigned char lockid) const;

    void	splitTriangleInside(int ci,int ti,unsigned char lockid);
    		/*!ci is assumed to be inside the triangle ti. */
    void	splitTriangleOnEdge(int ci,int ti0,int ti1,
	    				    unsigned char lockid);
    		/*!ci is on the shared edge of triangles ti0, ti1. */
    void	legalizeTriangles(TypeSet<char>& v0s,TypeSet<char>& v1s,
			TypeSet<int>& tis,unsigned char lockid);
    		/*!Check neighbor triangle of the edge v0-v1 in ti, 
		   where v0, v1 are local vetex indices 0, 1, 2. */

    const int	getNeighbor(const int v0,const int v1,const int ti,
	    		    unsigned char lockid); 
    int		searchChild(int v0,int v1,int ti,unsigned char lockid) const;
    char	isOnEdge(const Coord& p,const Coord& a,
	    		 const Coord& b, bool& duponfirst ) const;
    char	isInside(int ci,int ti,char& edge,int& dupid) const;

    struct DAGTriangle
    {
			DAGTriangle();
	bool		operator==(const DAGTriangle&) const;
	DAGTriangle&	operator=(const DAGTriangle&);

	int	coordindices_[3];
	int	childindices_[3];
	int	neighbors_[3];

	bool	hasChildren() const;
	bool	readLock(Threads::ConditionVar&,unsigned char lockid) const;
	void	readUnLock(Threads::ConditionVar&) const;
	bool	writeLock(Threads::ConditionVar&, unsigned char lockid) const;
	void	writeUnLock(Threads::ConditionVar&) const;

     protected:	
	mutable char	lockcounts_; /*<writelock -1, otherwise, nrlocks. */
    };

    double				epsilon_;
    TypeSet<DAGTriangle>		triangles_;

    TypeSet<Coord>*			coordlist_;
    bool				ownscoordlist_;
    mutable Threads::ConditionVar	condvar_;
    mutable Threads::ReadWriteLock	coordlock_;

    Coord				initialcoords_[3]; 
    					/*!<-2,-3,-4 are their indices.*/
};

/*!<The parallel DTriangulation works for only one processor now.*/
class ParallelDTriangulator : public ParallelTask
{
public:
			ParallelDTriangulator(DAGTriangleTree&);
	
    bool		isDataRandom()		{ return israndom_; }
    void		dataIsRandom(bool yn)	{ israndom_ = yn; }

protected:

    int			totalNr() const;
    bool		doWork( int, int, int );
    bool		doPrepare(int);

    TypeSet<int>	permutation_;
    bool		israndom_;
    DAGTriangleTree&	tree_;
};


#endif

