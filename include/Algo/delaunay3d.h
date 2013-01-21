#ifndef delaunay3d_h
#define delaunay3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y.C. Liu
 Date:          June 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "algomod.h"
#include "position.h"
#include "sets.h"
#include "task.h"

/*!
\ingroup Algo
\brief Delaunay triangulation for 3D points. Should make sure all the points
are defined.
*/

mExpClass(Algo) DAGTetrahedraTree
{
public:
    			DAGTetrahedraTree();
    			DAGTetrahedraTree(const DAGTetrahedraTree&);
    virtual		~DAGTetrahedraTree();
    DAGTetrahedraTree&	operator=(const DAGTetrahedraTree&);

    bool		setCoordList(const TypeSet<Coord3>&,bool copy);
    const TypeSet<Coord3>& coordList() const { return *coordlist_; }
    
    bool		init();
    void		setInitSizeFactor(float);
    			/*<If the triangulation is not good enough because of 
			   the init tetrahedra, we may abjust the size of it.*/
    float		getInitSizeFactor() const { return initsizefactor_; }

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
    char		locationToTetrahedra(const Coord3& checkpt,
	    				const Coord3* v,char& face,
					int& dupididx,int& edgeend0idx,
					int& edgeend1idx,double& dist) const;
    			/*<Find the relative position of checkpt to tetrahedra
			   with verticies v[0], v[1],v[2],v[3]. */
    char		searchTetrahedra(const Coord3&);
    			/*<Check pt is outside, on, or inside the body surface,
			   return -1, 0, 1 respectively. */
    
    bool		getConnections(int pointidx,TypeSet<int>&) const;
    bool		getTetrahedras(TypeSet<int>&) const;
    			/*<Coord indices are sorted in fours, i.e.
			   ci[0], ci[1], ci[2], ci[3] is the first tetrahedra
			   ci[4], ci[5], ci[6], ci[7] is the second one. */
    bool		getSurfaceTriangles(TypeSet<int>&) const;
    			/*<Coord indices are sorted in threes, i.e.
			   ci[0], ci[1], ci[2] is the first triangle
			   ci[3], ci[4], ci[5] is the second triangle. */
    void		setEpsilon(double err)	{ epsilon_ = err; }
    static char		cNoVertex()	{ return -1; }

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
    static char		cNotOnPlane() 	{ return 6; }
    static char		cError()	{ return -1; }

    static char		cNoTetrahedra()	{ return -1; }
    static char		cNoFace()	{ return -1; }
    static char		cInitVertex0()	{ return -2; }
    static char		cInitVertex1()	{ return -3; }
    static char		cInitVertex2()	{ return -4; }
    static char		cInitVertex3()	{ return -5; }

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
	    		 int& edgeend0,int& edgeend1,double& dist) const;
    char	isOnEdge(const Coord3& p,const Coord3& a,const Coord3& b, 
	    		 const Coord3 planenormal,bool& duponfirst, 
			 double& signedsqdist) const;
    char	locationToTriangle(const Coord3& pt,const Coord3& a,
	    		const Coord3& b,const Coord3& c,double& signedsqdist,
			double& closeedgedist,char& dupid,char& edgeidx) const;

    char	isIntersect(const Coord3& p,const Coord3& q,const Coord3& a,
	   		const Coord3& b,const Coord3& c,char& edge) const;
    		/*!<ret inside, outside, on edge, or duplicate. edge will be
		  defined if pq intersects ABC on edge 0, 1, 2 in order. */
    void	addTriangle(int v0,int v1,int v2,TypeSet<int>& triangles) const;
    		/*!<if triangles have {v0,v1,v2}, then return, else, add it.*/

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
    float				initsizefactor_;
};


/*!
\ingroup Algo
\brief Delaunay triangulation for 3D points.
*/

mExpClass(Algo) ParallelDTetrahedralator : public ParallelTask
{
public:
			ParallelDTetrahedralator(DAGTetrahedraTree&);
    bool		isDataRandom()          { return israndom_; }
    void		dataIsRandom(bool yn)   { israndom_ = yn; }

protected:

    int			maxNrThreads() const 	{ return 1; }
    od_int64		nrIterations() const;
    bool		doWork(od_int64,od_int64,int);
    bool		doPrepare(int);

    TypeSet<int>	permutation_;
    bool		israndom_;
    DAGTetrahedraTree&	tree_;
};


#endif


