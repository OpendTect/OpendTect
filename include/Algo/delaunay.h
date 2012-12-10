#ifndef delaunay_h
#define delaunay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y.C. Liu
 Date:          January 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "algomod.h"
#include "position.h"
#include "odmemory.h"
#include "sets.h"
#include "task.h"
#include "thread.h"
#include "trigonometry.h"

/*!Reference: "Parallel Incremental Delaunay Triangulation", by Kohout J.2005.
   For the triangulation, it will skip undefined or duplicated points, all the 
   points should be in random order. We use Kohout's pessimistic method to
   triangulate. The problem is that the pessimistic method only give a 10% speed
   increase, while the locks slows it down. The parallel code is thus
   disabled with a macro.
*/

#define mDAGTriangleForceSingleThread
mClass(Algo) DAGTriangleTree
{
public:
    			DAGTriangleTree();
    			DAGTriangleTree(const DAGTriangleTree&);
    virtual		~DAGTriangleTree();
    DAGTriangleTree&	operator=(const DAGTriangleTree&);

    static bool		computeCoordRanges(const TypeSet<Coord>&,
	    		    Interval<double>&,Interval<double>&);

    bool		setCoordList(const TypeSet<Coord>* OD);
    bool		setCoordList(TypeSet<Coord>*,OD::PtrPolicy);
    const TypeSet<Coord>& coordList() const { return *coordlist_; }

    bool		setBBox(const Interval<double>& xrg,
	    			const Interval<double>& yrg);

    bool		isOK() const { return triangles_.size(); }

    bool		init();

    bool		insertPoint(int pointidx, int& dupid);
    int			insertPoint(const Coord&, int& dupid);
    
    const Coord		getInitCoord(int vetexidx) const;
    bool		getTriangle(const Coord&,int& dupid,
	    			    TypeSet<int>& vertexindices) const;
    			/*!<search triangle contains the point.return crds. */
    bool		getCoordIndices(TypeSet<int>&) const;
    			/*!<Coord indices are sorted in threes, i.e
			    ci[0], ci[1], ci[2] is the first triangle
			    ci[3], ci[4], ci[5] is the second triangle. */
    bool		getSurroundingIndices(TypeSet<int>&) const;
    			/*!Points on the edge of the geometry shape. */

    bool		getConnections(int pointidx,TypeSet<int>&) const;
    bool		getWeights(int pointidx,const TypeSet<int>& conns,
				   TypeSet<double>& weights,
				   bool normailze=true) const;
    			/*!Calculate inverse distance weight for each conns.*/
    bool		getConnectionAndWeights(int ptidx,TypeSet<int>& conns,
	    				     TypeSet<double>& weights,
					     bool normailze=true) const;
    void		setEpsilon(double err)	{ epsilon_ = err; }

    void		dumpTo(std::ostream&) const;
    			//!<Dumps all triangles to stream;
    void		dumpTriangulationToIV(std::ostream&) const;


    static int		cNoVertex()	{ return -1; }

protected:

    static char		cIsOutside()	{ return 0; }
    static char		cIsInside()	{ return 1; }
    static char		cIsDuplicate()	{ return 2; }
    static char		cError()	{ return -1; }

    static int		cNoTriangle()	{ return -1; }
    static int		cInitVertex0()	{ return -2; }
    static int		cInitVertex1()	{ return -3; }
    static int		cInitVertex2()	{ return -4; }

    char	searchTriangle(const Coord& pt,int start,int& t0,
	    		       int& dupid) const;
    char	searchFurther(const Coord& pt,int& nti0,int& dupid) const;

    void	splitTriangleInside(int ci,int ti);
    		/*!ci is assumed to be inside the triangle ti. */
    void	legalizeTriangles(TypeSet<char>& v0s,TypeSet<char>& v1s,
			TypeSet<int>& tis);
    		/*!Check neighbor triangle of the edge v0-v1 in ti, 
		   where v0, v1 are local vetex indices 0, 1, 2. */

    int		getNeighbor(int v0,int v1,int ti) const;
    int		searchChild(int v0,int v1,int ti) const;
    char	isInside(const Coord& pt,int ti,int& dupid) const;

    struct DAGTriangle
    {
			DAGTriangle();
	bool		operator==(const DAGTriangle&) const;
	DAGTriangle&	operator=(const DAGTriangle&);

	int		coordindices_[3];
	int		childindices_[3];
	int		neighbors_[3];

	bool		hasChildren() const;
    };

    bool				multithreadsupport_;

    mutable Threads::ReadWriteLock	trianglelock_;

    double				epsilon_;
    TypeSet<DAGTriangle>		triangles_;

    TypeSet<Coord>*			coordlist_;
    bool				ownscoordlist_;

    mutable Threads::ReadWriteLock	coordlock_;

    Coord				initialcoords_[3]; 
    					/*!<-2,-3,-4 are their indices.*/
};


/*!<The parallel triangulation works for only one processor now.*/
mClass(Algo) DelaunayTriangulator : public ParallelTask
{
public:
			DelaunayTriangulator(DAGTriangleTree&);
			~DelaunayTriangulator();
	
    bool		isDataRandom()		{ return israndom_; }
    void		dataIsRandom(bool yn)	{ israndom_ = yn; }
    void		setCalcScope(const Interval<int>& rg);

protected:

#ifdef mDAGTriangleForceSingleThread
    int			maxNrThreads() const { return 1; }
#endif
    od_int64		nrIterations() const;
    bool		doWork(od_int64,od_int64, int );
    bool		doPrepare(int);

    const char*		nrDoneText() const
			{ return "Points triagulated"; }
    const char*		message() const { return "Triangulating"; }

    od_int64*		permutation_;
    bool		israndom_;
    DAGTriangleTree&	tree_;
    Interval<od_int64>	calcscope_;
};


/*For a given triangulated geometry(set of points), interpolating any point 
  located in or nearby the goemetry. If the point is located outside of the 
  boundary of the geometry, we compare azimuth to find related points and then
  apply inverse distance to calculate weights. */
mClass(Algo) Triangle2DInterpolator
{
public:
    			Triangle2DInterpolator(const DAGTriangleTree&);

			/*The vertices are indices from the DAGTriangleTree 
			  coordlist, corresponding to the weights.*/
    bool		computeWeights(const Coord&,TypeSet<int>& vertices,
	    			       TypeSet<float>& weights,
				       double maxdist=mUdf(double),
				       bool dointerpolate=true);
    			/*If don't do interpolate, nearest node will be taken.*/

protected:

    bool			setFromAzimuth(const TypeSet<int>& tmpvertices,
	    				       const Coord&,
					       TypeSet<int>& vertices, 
					       TypeSet<float>& weights);
    const DAGTriangleTree&	triangles_;
    TypeSet<int>		corner0_;
    TypeSet<double>		cornerweights0_;
    TypeSet<int>		corner1_;
    TypeSet<double>		cornerweights1_;
    TypeSet<int>		corner2_;
    TypeSet<double>		cornerweights2_;
    Coord			initcenter_;

    TypeSet<int>		perimeter_;
    TypeSet<double>		perimeterazimuth_;
};


/*Simple polyon triangulation, does not work if you have holes inside it. 
  return each triangle with three indicies in order. */
inline  bool PolygonTriangulate( const TypeSet<Coord>& knots,TypeSet<int>& res )
{
    const int nrknots = knots.size();
    if ( nrknots < 3 ) 
      return false;

    /* Make sure it is a counter-clockwise polygon in ci */
    double area=0;
    for( int idx=nrknots-1, idy=0; idy<nrknots; idx=idy++ )
      area += (knots[idx].x*knots[idy].y - knots[idy].x*knots[idx].y);
    area *= 0.5;

    TypeSet<int> ci;
    if ( 0<area )
    {
    	for ( int idx=0; idx<nrknots; idx++ ) 
	    ci += idx;
    }
    else
    {
    	for( int idx=0; idx<nrknots; idx++ ) 
	    ci += (nrknots-1-idx);
    }

    /*Triangulate: three consecutive vertices (idx0,idx,idx1) in polygon,
      remove cursize-2 Vertices, creating 1 triangle every time */
    int cursize = nrknots;
    int errcheck = 2*cursize; 

    for( int idx=cursize-1; cursize>2; )
    {
	if ( 0 >= (errcheck--) )
  	    return false;

	const int idx0 = cursize<=idx ? 0 : idx; 
	idx = cursize<=idx0+1 ? 0 : idx0+1;
	const int idx1 = cursize<=idx+1 ? 0 : idx+1;

	const Coord& pos0 = knots[ci[idx0]];
    	const Coord& pos = knots[ci[idx]];
    	const Coord& pos1 = knots[ci[idx1]];
    	if ( (((pos.x-pos0.x)*(pos1.y-pos0.y)) - 
	      ((pos.y-pos0.y)*(pos1.x-pos0.x)))<0 ) 
      	    continue;

	bool isvalid = true;
    	for ( int idy=0; idy<cursize; idy++ )
    	{
	    if( (idy==idx0) || (idy==idx) || (idy==idx1) ) 
		continue;
	    
	    if ( pointInTriangle2D(knots[ci[idy]],pos0,pos,pos1,0.0) ) 
	    {
		isvalid = false;
		break;
	    }
    	}

	if ( isvalid )
	{
	  res += ci[idx0];
	  res += ci[idx];
	  res += ci[idx1];

	  /* remove idx from remaining polygon */
	  for( int i=idx, j=idx+1; j<cursize; i++, j++ ) 
	      ci[i] = ci[j]; 
	  
	  cursize--;
	  errcheck = 2*cursize;
	}
    }

    return true;
}

#endif

