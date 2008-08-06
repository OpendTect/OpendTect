#ifndef delaunay3d_h
#define delaunay3d_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Y.C. Liu
 Date:          June 2008
 RCS:           $Id: delaunay3d.h,v 1.4 2008-08-06 22:03:30 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"
#include "task.h"
#include "thread.h"


/*<Delaunay triangulation for 3D points. Should make sure all the points are 
   defined. */

class DAGTetrahedraTree
{
public:
    			DAGTetrahedraTree();
    			DAGTetrahedraTree(const DAGTetrahedraTree&);
    virtual		~DAGTetrahedraTree();
    DAGTetrahedraTree&	operator=(const DAGTetrahedraTree&);

    bool		setCoordList(const TypeSet<Coord3>&,bool copy);
    const TypeSet<Coord3>& coordList() const { return *coordlist_; }
    
    bool		init();
    bool		isOK() const	{ return tetrahedras_.size(); }
    static bool		computeCoordRanges(const TypeSet<Coord3>&,
	    				   Interval<double>& xrg,
					   Interval<double>& yrg,
					   Interval<double>& zrg);
    bool		setBBox(const Interval<double>& xrg,
	    			const Interval<double>& yrg,
				const Interval<double>& zrg);

    bool		insertPoint(int pointidx, int& dupid);
    int			insertPoint(const Coord3&, int& dupid);
   			
    bool		getConnections(int pointidx,TypeSet<int>&) const;
    bool		getTetrahedras(TypeSet<int>&) const;
    			/*<Coord indices are sorted in fours, i.e.
			   ci[0], ci[1], ci[2], ci[3] is the first tetrahedra
			   ci[4], ci[5], ci[6], ci[7] is the second one. */
    bool		getSurfaceTriangles(TypeSet<int>&) const;
    			/*<Coord indices are sorted in threes, i.e.
			   ci[0], ci[1], ci[2] is the first triangle
			   ci[3], ci[4], ci[5] is the second triangle. */

    bool		getSurfaceNoTriangle(const int& v0,const int& v1,
	    			const int& v2, TypeSet<int>& result) const;
    bool		getSurfaceNoTriangle(const Coord3& v0,const Coord3& v1,
	    			const Coord3& v2, TypeSet<int>& result) const;
    void		setEpsilon(double err)	{ epsilon_ = err; }
    static int		cNoVertex()	{ return -1; }

protected:

    static char		cIsInside()	{ return 0; }
    static char		cIsOnFace()	{ return 1; }
    static char		cIsOnEdge() 	{ return 2; }
    
    static char		cEdge01()	{ return 0; }
    static char		cEdge12()	{ return 1; }
    static char		cEdge20() 	{ return 2; }

    static char		cIsDuplicate()	{ return 3; }
    static char		cIsOutside()	{ return 4; }
    static char		cNotOnEdge() 	{ return 5; }
    static char		cError()	{ return -1; }

    static int		cNoTetrahedra()	{ return -1; }
    static int		cNoFace()	{ return -1; }
    static int		cInitVertex0()	{ return -2; }
    static int		cInitVertex1()	{ return -3; }
    static int		cInitVertex2()	{ return -4; }
    static int		cInitVertex3()	{ return -5; }

    void	splitInitialBox(int ci);
    void	splitTetrahedraInside(int ci,int ti);
    void	splitTetrahedraOnFace(int ci,int ti0,int ti1,char face);
    void	splitTetrahedraOnEdge(int ci,const TypeSet<int>& tis,
	    			      int& sharedv0,int& sharedv1);
    void	legalizeTetrahedras(TypeSet<int>& v0s,TypeSet<int>& v1s,
				    TypeSet<int>& v2s,TypeSet<int>& tis);
    char	searchTetrahedra(int ci,int start,TypeSet<int>& tis,char& face,
	    			 int& sharedv0,int& sharedv1,int& dupid) const;
    int		searchFaceOnNeighbor(int a,int b,int c,int ti) const;
    int		searchFaceOnChild(int a,int b,int c,int ti) const;
    int		searchFaceOnList(int ci,int v0,int v1,int rep,
	    			 const TypeSet<int>& tis) const;
    char	location(int ci,int ti,char& face,int& dupid,
	    		 int&edgeend0,int&edgeend1) const;
    char	isIntersect(const Coord3& p,const Coord3& q,const Coord3& a,
	   		const Coord3& b,const Coord3& c,char& edge) const;
    		/*!<ret inside, outside, on edge, or duplicate. edge will be
		  defined if pq intersects ABC on edge 0, 1, 2 in order. */

    struct DAGTetrahedra
    {
			DAGTetrahedra();
	bool		operator==(const DAGTetrahedra&) const;
	DAGTetrahedra&	operator=(const DAGTetrahedra&);

	int	coordindices_[4];
	int	childindices_[4];
	int	neighbors_[4];
    };

    TypeSet<DAGTetrahedra>		tetrahedras_;
    TypeSet<Coord3>*			coordlist_;
    Coord3				center_;
    Coord3				initialcoords_[4]; 
    					/*!<-2,-3,-4, -5 are their indices.*/
    bool				ownscoordlist_;
    double				epsilon_;
};


class ParallelDTetrahedralator : public ParallelTask
{
public:
			ParallelDTetrahedralator(DAGTetrahedraTree&);
    bool		isDataRandom()          { return israndom_; }
    void		dataIsRandom(bool yn)   { israndom_ = yn; }

protected:

    int			maxNrThreads() const 	{ return 1; }
    int			totalNr() const;
    bool		doWork(int,int,int);
    bool		doPrepare(int);

    TypeSet<int>	permutation_;
    bool		israndom_;
    DAGTetrahedraTree&	tree_;
};



#endif

