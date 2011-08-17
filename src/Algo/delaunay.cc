/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: delaunay.cc,v 1.47 2011-08-17 11:37:01 cvskris Exp $";

#include "delaunay.h"
#include "sorting.h"
#include "trigonometry.h"
#include "varlenarray.h"
#include <iostream>


ParallelDTriangulator::ParallelDTriangulator( DAGTriangleTree& dagt )
    : tree_( dagt )  
    , israndom_( true )
    , calcscope_( 0, dagt.coordList().size()-1 )
    , permutation_( 0 )
{}


ParallelDTriangulator::~ParallelDTriangulator()
{
    delete [] permutation_;
}


void ParallelDTriangulator::setCalcScope(const Interval<int>& rg)
{
    calcscope_.start =  mMAX( 0, rg.start );
    calcscope_.stop =  mMIN( tree_.coordList().size()-1, rg.stop );
}


od_int64 ParallelDTriangulator::nrIterations() const                
{ return calcscope_.width()+1; }


bool ParallelDTriangulator::doPrepare( int nrthreads )
{
    const od_int64 nrcoords = nrIterations();
    delete [] permutation_;
    permutation_ = 0;
    
    if ( !israndom_ )
    {
	mTryAlloc( permutation_, od_int64[nrcoords] );
	if ( !permutation_ ) //If failed to allocate memory, we ignore random
	{
	    delete [] permutation_;
	    permutation_ = 0;
	    return true;
	}
	
	for ( od_int64 idx=0; idx<nrcoords; idx++ )
	    permutation_[idx] = idx;
	
	std::random_shuffle( permutation_, permutation_+nrcoords );
    }
    
    return true;
}


bool ParallelDTriangulator::doWork( od_int64 start, od_int64 stop,int threadid )
{
    for ( od_int64 idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const od_int64 scopeidx = permutation_ ? permutation_[idx] : idx;
	const od_int64 coordid = calcscope_.atIndex( scopeidx, 1 );
	int dupid;
       	if ( !tree_.insertPoint( coordid, dupid ) )
	    return false;

	addToNrDone(1);
    }

    return true;
}


DAGTriangleTree::DAGTriangleTree()
    : coordlist_( 0 )
    , epsilon_( 1e-5 )
    , planedirection_( true )		       
    , ownscoordlist_( true )
{}


DAGTriangleTree::DAGTriangleTree( const DAGTriangleTree& b )
    : coordlist_( 0 )
    , epsilon_( 1e-5 )
    , planedirection_( true )		       
    , ownscoordlist_( true )
{
    *this = b;
}


DAGTriangleTree& DAGTriangleTree::operator=( const DAGTriangleTree& b )
{
    epsilon_ = b.epsilon_;
    planedirection_ = b.planedirection_;		       
    if ( ownscoordlist_ )
	delete coordlist_;

    if ( b.ownscoordlist_ )
	coordlist_ = b.coordlist_ ? new TypeSet<Coord>( *b.coordlist_ ) : 0;
    else
	coordlist_ = b.coordlist_;

    ownscoordlist_ = b.ownscoordlist_;

    triangles_ = b.triangles_;

    initialcoords_[0] = b.initialcoords_[0];
    initialcoords_[1] = b.initialcoords_[1];
    initialcoords_[2] = b.initialcoords_[2];
    return *this;
}


DAGTriangleTree::~DAGTriangleTree()
{
    if ( ownscoordlist_ )
	delete coordlist_;
}


bool DAGTriangleTree::computeCoordRanges( const TypeSet<Coord>& coordlist, 
					  Interval<double>& xrg,
					  Interval<double>& yrg )
{
    if ( !coordlist.size() )
	return false;

    xrg.start = xrg.stop = coordlist[0].x;
    yrg.start = yrg.stop = coordlist[0].y;

    for ( int idx=1; idx<coordlist.size(); idx++ )
    {
	xrg.include( coordlist[idx].x );
	yrg.include( coordlist[idx].y );
    }

    return true;
}


bool DAGTriangleTree::setCoordList( TypeSet<Coord>* coordlist,
				    OD::PtrPolicy policy )
{
#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeLock();
#endif
    if ( coordlist_ && ownscoordlist_ )
	delete coordlist_;

    coordlist_ = 0;
#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeUnLock();
#endif

    if ( !coordlist || coordlist->size()<3 )
	return false;

    Interval<double> xrg, yrg;
    computeCoordRanges( *coordlist, xrg, yrg );

#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeLock();
#endif

    if ( policy==OD::CopyPtr )
    {
	coordlist_ = new TypeSet<Coord>( *coordlist );
	ownscoordlist_ = true;
    }
    else
    {
	coordlist_ = coordlist;
	ownscoordlist_ = policy==OD::TakeOverPtr;
    }

#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeUnLock();
#endif
    setBBox( xrg, yrg );
    return true;
}


bool DAGTriangleTree::setBBox(const Interval<double>& xrg,
			  const Interval<double>& yrg)
{
    triangles_.erase();
    const double xlength = xrg.width();
    const double ylength = yrg.width();
    if ( mIsZero(xlength,1e-4) || mIsZero(ylength,1e-4) )
	return false;

    const Coord center( xrg.center(), yrg.center() );
    const double radius = sqrt( xlength*xlength+ylength*ylength )/2;
    initialcoords_[0] = Coord( center.x-radius*sqrt(3.0), center.y-radius );
    initialcoords_[1] = Coord( center.x+radius*sqrt(3.0), center.y-radius );
    initialcoords_[2] = Coord( center.x, center.y+2*radius );

    DAGTriangle initnode;
    initnode.coordindices_[0] = cInitVertex0();
    initnode.coordindices_[1] = cInitVertex1();
    initnode.coordindices_[2] = cInitVertex2();
    triangles_ +=initnode;

    epsilon_ = Math::Sqrt( xlength*xlength+ylength*ylength) * 1e-5;
    planedirection_ = ( (initialcoords_[1]-initialcoords_[0]).x*
			(initialcoords_[2]-initialcoords_[1]).y-
   			(initialcoords_[1]-initialcoords_[0]).y*
   			(initialcoords_[2]-initialcoords_[1]).x ) > 0;

    return true;
}

const Coord DAGTriangleTree::getInitCoord( int vetexidx ) const
{
    if ( vetexidx==cInitVertex0() )
	return initialcoords_[0];
    else if ( vetexidx==cInitVertex1() )
	return initialcoords_[1];
    else if ( vetexidx==cInitVertex2() )
	return initialcoords_[2];

    return Coord( mUdf(float), mUdf(float) );
}

bool DAGTriangleTree::init()
{
    if ( !triangles_.size() ) return false;

#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeLock();
#endif
    if ( !coordlist_ )
    {
	coordlist_ = new TypeSet<Coord>;
	ownscoordlist_ = true;
    }
#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeUnLock();
#endif

    return true;
}


int DAGTriangleTree::insertPoint( const Coord& coord, int& dupid )
{
#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeLock();
#endif
    const int ci = coordlist_->size();
    (*coordlist_) += coord;
#ifndef mDAGTriangleForceSingleThread
    coordlock_.writeUnLock();
#endif

    if ( !insertPoint( ci, dupid ) )
    {
#ifndef mDAGTriangleForceSingleThread
	coordlock_.writeLock();
#endif
	if ( coordlist_->size()==ci+1 )
	    coordlist_->remove( ci );
#ifndef mDAGTriangleForceSingleThread
	coordlock_.writeUnLock();
#endif

	return cNoVertex();
    }

    return ci;
}


bool DAGTriangleTree::getTriangle( const Coord& pt, int& dupid,
				   TypeSet<int>& vertices ) const
{
    dupid = cNoVertex();
    vertices.erase();
    int ti0, ti1;
    const char res = searchTriangle( pt, 0, ti0, ti1, dupid );
    if ( dupid!=cNoVertex() )
	return true;

    if ( ti0==cNoVertex() && ti1==cNoVertex() )
	return false;

    if ( ti0==cNoVertex() )
    {
	ti0 = ti1;
	ti1 = cNoVertex();
    }

    vertices += triangles_[ti0].coordindices_[0];
    vertices += triangles_[ti0].coordindices_[1];
    vertices += triangles_[ti0].coordindices_[2];
    if ( ti1!=cNoVertex() && (vertices[0]<0 || vertices[1]<0 || vertices[2]<0))
    {
  	vertices[0] = triangles_[ti1].coordindices_[0];
  	vertices[1] = triangles_[ti1].coordindices_[1];
 	vertices[2] = triangles_[ti1].coordindices_[2];
    }

    return true;
}


#define mCrd( idx ) \
	(idx>=0 ? (*coordlist_)[idx] : initialcoords_[-idx-2])


bool DAGTriangleTree::insertPoint( int ci, int& dupid )
{
    dupid = cNoVertex();
#ifndef mDAGTriangleForceSingleThread
    coordlock_.readLock();
#endif
    if ( mIsUdf((*coordlist_)[ci].x) || mIsUdf((*coordlist_)[ci].y) ) 
    {
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	BufferString msg = "The point ";
	msg += ci;
	msg +=" is not defined!";
	pErrMsg( msg );
	return true; //For undefined point, skip.
    }

#ifndef mDAGTriangleForceSingleThread
    coordlock_.readUnLock();
#endif

    int ti0, ti1;
    const char res = searchTriangle( mCrd(ci), 0, ti0, ti1, dupid );
    
    if ( res==cIsInside() || res==cIsOnEdge() )
    {
#ifndef mDAGTriangleForceSingleThread
	trianglelock_.permissiveWriteLock();
#endif
	int nti0 = ti0, nti1 = ti1;
	const char nres = searchFurther( mCrd(ci), nti0, nti1, dupid );

	if ( nres==cIsInside() || nres==cIsOnEdge() )
	{
	    if ( nti1==cNoTriangle() )
		splitTriangleInside( ci, nti0 );
	    else
		splitTriangleOnEdge( ci, nti0, nti1 );
	    
#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.permissiveWriteUnLock();
#endif
	    return true;
	}
	else
	{
#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.permissiveWriteUnLock();
#endif

	    if ( nres==cIsDuplicate() )
		return true;
	    else
	    {
		BufferString msg = "\n Insert point ";
		msg += ci;
		msg += "failed!";
		pErrMsg( msg );
		return false;
	    }
	}
    }
    else if ( res==cIsDuplicate() )
	return true;
    else  
    {
	BufferString msg = "\n Insert point ";
	msg += ci;
	msg += "failed!";
	pErrMsg( msg );
	return false;
    }

    return true;
}


int DAGTriangleTree::searchNeighbor( int ti, char edge ) const
{
    const int* crd = triangles_[ti].coordindices_;
    const int* nbor = triangles_[ti].neighbors_;

    int neighbor = cNoTriangle();

    if ( edge==0 )
	neighbor = searchChild( crd[0], crd[1], nbor[0] );
    else if ( edge==1 )
	neighbor = searchChild( crd[1], crd[2], nbor[1] );
    else if ( edge==2 )
	neighbor = searchChild( crd[2], crd[0], nbor[2] );
    else
    {
	pErrMsg("Should not happen");
    }

    return neighbor;
}


char DAGTriangleTree::searchTriangle( const Coord& pt, int startti, int& ti0, 
	int& ti1, int& dupid ) const
{
    if ( startti<0 )
	startti = 0;

    ti0 = startti;
    ti1 = cNoTriangle();

    return searchFurther( pt,  ti0, ti1, dupid );
}


char DAGTriangleTree::searchFurther( const Coord& pt, int& ti0, int& ti1,
				     int& dupid ) const
{
    while ( true )
    {
	const int curtriangle = ti0;

	if ( ti1==cNoTriangle() ) //I.e I must be inside my ti0
	{
#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.readLock();
#endif
	    if (  !triangles_[curtriangle].hasChildren() )
	    {
#ifndef mDAGTriangleForceSingleThread
		trianglelock_.readUnLock();
#endif
		return cIsInside();
	    }

	    const int* cptr = triangles_[curtriangle].childindices_;
	    const int children[] = { cptr[0], cptr[1], cptr[2] };
#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.readUnLock();
#endif
	  
	    int candidates[] = { cNoTriangle(), cNoTriangle() }; 
	    bool found = false; 
	    for ( int childidx = 0; childidx<3; childidx++ )
	    {
		const int curchild = children[childidx];
		if ( curchild==cNoTriangle() ) continue;

		char edge;
		double disttoedge;
		const char mode = isInside( pt, curchild, edge, disttoedge,
					    dupid );
		if ( mode==cIsOutside() ) 
		    continue;

		if ( mode==cIsDuplicate() )
		    return mode;

		if ( mode==cIsOnEdge() )
		{
		    const int sharedtriangle =
			triangles_[curchild].neighbors_[edge];
		    
		    if ( disttoedge<0 &&
			    (children[0]==sharedtriangle ||
			     children[1]==sharedtriangle ||
			     children[2]==sharedtriangle ) )
		    {
			if ( candidates[0]==cNoTriangle() ||
			     candidates[1]!=curchild )
	  		{
   			    candidates[0] = curchild;
 			    candidates[1] = sharedtriangle;
      			}

			continue;
		    }

		    ti0 = curchild;
		    ti1 = sharedtriangle;
		    found = true;
		    break;
		}
		else if ( mode==cIsInside() )
		{
		    ti0 = curchild;
		    ti1 = cNoTriangle();

#ifndef mDAGTriangleForceSingleThread
		    trianglelock_.readLock();
#endif
		    const bool haschildren = triangles_[curchild].hasChildren();
#ifndef mDAGTriangleForceSingleThread
		    trianglelock_.readUnLock();
#endif
		    found = true;

		    if ( !haschildren )
			return cIsInside();

		    break;
		}
		else
		{
		    pErrMsg("Hmm");
		}
	    }

	    if ( !found )
	    {
		if ( candidates[0]==cNoTriangle() || 
		     candidates[1]==cNoTriangle() )
		{
    		    pErrMsg("No child found");
    		    return cError();
		}

		ti0 = candidates[0];
		ti1 = candidates[1];
	    }
	}
	else
	{
#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.readLock();
#endif
	    if ( !triangles_[curtriangle].hasChildren() )
	    {
		if ( triangles_[ti1].hasChildren() )
		{
#ifndef mDAGTriangleForceSingleThread
		    trianglelock_.readUnLock();
#endif
		    const char edge = getCommonEdge( ti0, ti1);
		    if ( edge<0 )
		    {
			pErrMsg("Hmm");
			return cError();
		    }

		    ti1 = searchNeighbor( ti0, edge);
		    if ( ti1==cNoTriangle() )
		    {
			pErrMsg("Hmm");
			return cError();
		    }
		}
		else
		{
#ifndef mDAGTriangleForceSingleThread
		    trianglelock_.readUnLock();
#endif
		}

		return cIsOnEdge();
	    }

#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.readUnLock();
#endif

	    char edge;
	    const char res = 
		searchTriangleOnEdge( pt, curtriangle, ti0, edge, dupid );

	    if ( res==cIsInside() )
	    {
		if ( ti0==cNoTriangle() )
		    return cError();

		ti1 = cNoTriangle();
	    }
	    else if ( res==cIsDuplicate() )
		return res;
	    else if ( res==cIsOnEdge() )
	    {
		ti1 = searchNeighbor( ti0, edge );
		if ( ti1==cNoTriangle() )
		    return cError();

		return cIsOnEdge();
	    }
	    else
	    {
		pErrMsg( "Should not happen" );
		return cError();
	    }
	}
    }

    return cError();
}


char DAGTriangleTree::searchTriangleOnEdge( const Coord& pt, int ti, 
	int& resti, char& edge, int& dupid ) const
{
#ifndef mDAGTriangleForceSingleThread
    trianglelock_.readLock();
#endif

    if ( !triangles_[ti].hasChildren() )
    {
#ifndef mDAGTriangleForceSingleThread
	trianglelock_.readUnLock();
#endif
	return cError();
    }

    const int* cptr = triangles_[ti].childindices_;
    const int children[] = { cptr[0], cptr[1], cptr[2] };

#ifndef mDAGTriangleForceSingleThread
    trianglelock_.readUnLock();
#endif

    for ( int idx=0; idx<3; idx++ )
    {
	const int curchild = children[idx];
	if ( curchild==cNoTriangle() )
	    continue;

	double disttoedge;
    	const char inchild = isInside( pt, curchild, edge, disttoedge, dupid );
	if ( inchild==cIsDuplicate() )
	    return cIsDuplicate();
	else if ( inchild==cIsOnEdge() )
	{
#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.readLock();
#endif
	    const bool haschildren = triangles_[curchild].hasChildren();
#ifndef mDAGTriangleForceSingleThread
	    trianglelock_.readUnLock();
#endif

	    resti = curchild;
	    if ( haschildren )
		return searchTriangleOnEdge( pt, curchild, resti, edge, dupid );
	    return cIsOnEdge();
	}
	else if ( inchild==cIsInside() )
	{
	    resti = curchild;

	    return cIsInside();
	}
    }

    pErrMsg( "This should never happen." );
    resti = cNoTriangle();

    return cError();
}


char DAGTriangleTree::isOnEdge( const Coord& p, const Coord& a,	const Coord& b,
       				bool& duponfirst, double& signedsqdist ) const
{
    const Coord linevec = b - a;
    const Coord newvec = p - a;
    const double sqlen = linevec.sqAbs();
    if ( mIsZero(newvec.sqAbs(),mDefEps) )
    {
	duponfirst = true;
	return cIsDuplicate();
    }
    else if ( mIsZero((p-b).sqAbs(),mDefEps) )
    {
	duponfirst = true;
	return cIsDuplicate();
    }

    const double t = linevec.dot(newvec) / sqlen;
    const Coord closestpt = a + linevec * t;
    const Coord vec = closestpt - p;
    const double sqdist = vec.sqAbs();
    const bool sign = Coord(linevec.y, -linevec.x).dot( vec ) > 0;
    signedsqdist = sign==planedirection_ ? sqdist : -sqdist;
    if ( t<0 || t>1 || sqdist>epsilon_*epsilon_ )
	return cNotOnEdge();

    if ( mIsZero(t,1e-3) )
    {
	duponfirst = true;
	return cIsDuplicate();
    }
    else if ( mIsEqual(t,1,1e-3) )
    {
	duponfirst = false;
	return cIsDuplicate();
    }

    return cIsOnEdge();
}


char DAGTriangleTree::isInside( const Coord& pt, int ti, char& edge, 
				double& disttoedge, int& dupid ) const
{
    if ( ti==cNoTriangle() )
	return cIsOutside();

    const int* crds = triangles_[ti].coordindices_;
#ifndef mDAGTriangleForceSingleThread
    coordlock_.readLock();
#endif
    const Coord& tricoord0 = mCrd(crds[0]);
    const Coord& tricoord1 = mCrd(crds[1]);
    const Coord& tricoord2 = mCrd(crds[2]);

    Interval<double> rg( tricoord0.x, tricoord0.x );
    rg.include( tricoord1.x, false ); rg.include( tricoord2.x, false );
    if ( !rg.includes(pt.x, false) )
    {
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	return cIsOutside();
    }

    rg.start = rg.stop = tricoord0.y;
    rg.include( tricoord1.y, false ); rg.include( tricoord2.y, false );
    if ( !rg.includes(pt.y, false) )
    {
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	return cIsOutside();
    }

    char bestedge = -1;
    bool duponfirst;
    double nearestedgedist;
    double signedsqdist[3];
    const char res0 = isOnEdge( pt, tricoord0, tricoord1, duponfirst, 
	    			signedsqdist[0] );
    if ( res0==cIsDuplicate() )
    {
	dupid = duponfirst ? crds[0] : crds[1];
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	return res0;
    }
    else if ( res0==cIsOnEdge() )
    {
	bestedge = 0;
	nearestedgedist = Math::Sqrt(fabs(signedsqdist[0]));
    }

    const char res1 = isOnEdge( pt, tricoord1, tricoord2, duponfirst, 
	    			signedsqdist[1] );
    if ( res1==cIsDuplicate() )
    {
	dupid = duponfirst ? crds[1] : crds[2];
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	return res1;
    }
    else if ( res1==cIsOnEdge() )
    {
	const double dist = Math::Sqrt(fabs(signedsqdist[1]));
	if ( bestedge==-1 || dist<nearestedgedist )
	{
	    bestedge = 1;
	    nearestedgedist = dist;
	}
    }

    const char res2 = isOnEdge( pt, tricoord2, tricoord0, duponfirst, 
	    			signedsqdist[2] );
    if ( res2==cIsDuplicate() )
    {
	dupid = duponfirst ? crds[2] : crds[0];
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	return res2;
    }
    else if ( res2==cIsOnEdge() )
    {
	const double dist = Math::Sqrt(fabs(signedsqdist[2]));
	if ( bestedge==-1 || dist<nearestedgedist )
	{
	    bestedge = 2;
	    nearestedgedist = dist;
	}
    }

    if ( signedsqdist[0]>0 && signedsqdist[1]>0 && signedsqdist[2]>0 )
    {
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	return cIsInside();
    }
   
    if ( bestedge!=-1 )
    {
	edge = bestedge;
	disttoedge = signedsqdist[edge];
#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	return cIsOnEdge();
    }

#ifndef mDAGTriangleForceSingleThread
    coordlock_.readUnLock();
#endif
    return cIsOutside();
}


void DAGTriangleTree::splitTriangleInside( int ci, int ti )
{
    if ( ti<0 || ti>=triangles_.size() )
	return;

    const int crd0 = triangles_[ti].coordindices_[0];
    const int crd1 = triangles_[ti].coordindices_[1];
    const int crd2 = triangles_[ti].coordindices_[2];
    const int* nbti = triangles_[ti].neighbors_;

    const int ti0 = triangles_.size();
    const int ti1 = triangles_.size()+1;
    const int ti2 = triangles_.size()+2;

    DAGTriangle child0;
    child0.coordindices_[0] = crd0;
    child0.coordindices_[1] = crd1;
    child0.coordindices_[2] = ci;
    child0.neighbors_[0] = searchChild( crd0, crd1,nbti[0] );
    child0.neighbors_[1] = ti2;
    child0.neighbors_[2] = ti1;

    DAGTriangle child1;
    child1.coordindices_[0] = crd0;
    child1.coordindices_[1] = ci; 
    child1.coordindices_[2] = crd2;
    child1.neighbors_[0] = ti0;
    child1.neighbors_[1] = ti2;
    child1.neighbors_[2] = searchChild( crd2, crd0, nbti[2] );
    
    DAGTriangle child2;
    child2.coordindices_[0] = ci; 
    child2.coordindices_[1] = crd1;
    child2.coordindices_[2] = crd2;
    child2.neighbors_[0] = ti0;
    child2.neighbors_[1] = searchChild( crd1, crd2, nbti[1] );
    child2.neighbors_[2] = ti1;

#ifndef mDAGTriangleForceSingleThread
    trianglelock_.convPermissiveToWriteLock();
#endif
    triangles_ += child0;
    triangles_ += child1;
    triangles_ += child2;

    triangles_[ti].childindices_[0] = ti0;
    triangles_[ti].childindices_[1] = ti1;
    triangles_[ti].childindices_[2] = ti2;
#ifndef mDAGTriangleForceSingleThread
    trianglelock_.convWriteToPermissive();
#endif

    TypeSet<char> v0s; 
    TypeSet<char> v1s; 
    TypeSet<int> tis; 

    v0s += 0; v1s += 1; tis += ti0;
    v0s += 0; v1s += 2; tis += ti1;
    v0s += 1; v1s += 2; tis += ti2;

    legalizeTriangles( v0s, v1s, tis );
}


int DAGTriangleTree::getNeighbor( int v0, int v1, int ti ) const
{
    if ( ti==cNoTriangle() )
	return cNoVertex();

    const int* crds = triangles_[ti].coordindices_;
    int id0=-1;
    int id1=-1;
    for ( int idx=0; idx<3; idx++ )
    {
	if ( crds[idx]==v0 )
	    id0 = idx;
	
	if ( crds[idx]==v1 )
	    id1 = idx;
    }

    if ( id0==-1 || id1==-1 )
    {
	pErrMsg( "vertex is not on the triangle" );
	return cNoVertex();
    }

    int res;
    
    if ( (id0==0 && id1==1) || (id0==1 && id1==0) )
	res = searchChild( v0, v1, triangles_[ti].neighbors_[0] );
    if ( (id0==0 && id1==2) || (id0==2 && id1==0) )
	res = searchChild( v0, v1, triangles_[ti].neighbors_[2] );
    else if ( (id0==1 && id1==2) || (id0==2 && id1==1) )
	res = searchChild( v0, v1, triangles_[ti].neighbors_[1] );

    return res;
}


void DAGTriangleTree::splitTriangleOnEdge( int ci, int ti0, int ti1 )
{
    if ( ti0==cNoTriangle() || ti1==cNoTriangle() )
	return; 

    const int* crds0 = triangles_[ti0].coordindices_;
    const int* crds1 = triangles_[ti1].coordindices_;
    int shared0 = cNoVertex();
    int shared1 = cNoVertex();
    int vti0 = cNoVertex();
    int vti1 = cNoVertex();

    for ( int idx=0; idx<3; idx++ )
    {
	if ( crds0[idx]!=crds1[0] && crds0[idx]!=crds1[1] && 
		crds0[idx]!=crds1[2] )
	{
	    vti0 = crds0[idx];
	    if ( idx==0 )
	    {
		shared0 = crds0[1];
		shared1 = crds0[2];
	    }
	    else if ( idx==1 )
	    {           
		shared0 = crds0[2];
		shared1 = crds0[0];
	    }
	    else
	    {
		shared0 = crds0[0];
		shared1 = crds0[1]; 
	    }
	}

	if ( crds1[idx]!=crds0[0] && crds1[idx]!=crds0[1] &&
		crds1[idx]!=crds0[2] )
	    vti1 = crds1[idx];
    }

    if ( shared0==cNoVertex() || shared1==cNoVertex() ||
	 vti0==cNoVertex() || vti1==cNoVertex() )
    {
	pErrMsg( "Two triangles don't share edge" );
	return;
    }

    const int nti0 = triangles_.size();
    const int nti1 = triangles_.size()+1;
    const int nti2 = triangles_.size()+2;
    const int nti3 = triangles_.size()+3;

    DAGTriangle child0;
    child0.coordindices_[0] = shared0;
    child0.coordindices_[1] = ci;
    child0.coordindices_[2] = vti0;
    child0.neighbors_[0] = nti2;
    child0.neighbors_[1] = nti1;
    child0.neighbors_[2] = getNeighbor(vti0,shared0,ti0 );

    DAGTriangle child1;
    child1.coordindices_[0] = shared1;
    child1.coordindices_[1] = vti0;
    child1.coordindices_[2] = ci; 
    child1.neighbors_[0] = getNeighbor(shared1,vti0,ti0 );
    child1.neighbors_[1] = nti0;
    child1.neighbors_[2] = nti3;

    DAGTriangle child2;
    child2.coordindices_[0] = shared0;
    child2.coordindices_[1] = vti1;
    child2.coordindices_[2] = ci; 
    child2.neighbors_[0] = getNeighbor(shared0,vti1,ti1 );
    child2.neighbors_[1] = nti3;
    child2.neighbors_[2] = nti0;

    DAGTriangle child3;
    child3.coordindices_[0] = shared1;
    child3.coordindices_[1] = ci;;
    child3.coordindices_[2] = vti1; 
    child3.neighbors_[0] = nti1;
    child3.neighbors_[1] = nti2;
    child3.neighbors_[2] = getNeighbor(vti1,shared1,ti1 );

#ifndef mDAGTriangleForceSingleThread
    trianglelock_.convPermissiveToWriteLock();
#endif
    triangles_ += child0;
    triangles_ += child1;
    triangles_ += child2;
    triangles_ += child3;
    triangles_[ti0].childindices_[0] = nti0;
    triangles_[ti0].childindices_[1] = nti1;
    triangles_[ti1].childindices_[0] = nti2;
    triangles_[ti1].childindices_[1] = nti3;
#ifndef mDAGTriangleForceSingleThread
    trianglelock_.convWriteToPermissive();
#endif

    TypeSet<char> v0s; 
    TypeSet<char> v1s; 
    TypeSet<int> tis; 

    v0s += 0; v1s += 2; tis += nti0;
    v0s += 0; v1s += 1; tis += nti1;
    v0s += 0; v1s += 1; tis += nti2;
    v0s += 0; v1s += 2; tis += nti3;

    legalizeTriangles( v0s, v1s, tis );
}


void DAGTriangleTree::legalizeTriangles( TypeSet<char>& v0s, TypeSet<char>& v1s, 				TypeSet<int>& tis )
{
    int start = 0;
    while ( v0s.size()>start )
    {
	const char v0 = v0s[start]; 
	const char v1 = v1s[start];
	const int ti = tis[start];
	if ( ti<0 )
	    continue;
	
	if ( start>10000 )
	{
	    v0s.remove( 0, start );
	    v1s.remove( 0, start );
	    tis.remove( 0, start );
	    start = 0;
	}
	else
  	    start++;


	int shared0, shared1, crdci;
	int checkti = cNoTriangle();
	if ( (v0==0 && v1==1) || (v0==1 && v1==0) )
	{
	    checkti = triangles_[ti].neighbors_[0];
	    crdci = triangles_[ti].coordindices_[2];
	    if ( v0==0 && v1==1 )
	    {
		shared0 =  triangles_[ti].coordindices_[0];
		shared1 =  triangles_[ti].coordindices_[1];  
	    }
	    else
	    {
		shared0 = triangles_[ti].coordindices_[1];
		shared1 = triangles_[ti].coordindices_[0];   
	    }
	}
	else if ( (v0==0 && v1==2) || (v0==2 && v1==0) )
	{
	    checkti = triangles_[ti].neighbors_[2];
	    crdci = triangles_[ti].coordindices_[1];
	    if ( v0==0 && v1==2 )
	    {
		shared0 = triangles_[ti].coordindices_[2];
		shared1 = triangles_[ti].coordindices_[0];   
	    }
	    else
	    {
		shared0 = triangles_[ti].coordindices_[0];
		shared1 = triangles_[ti].coordindices_[2];   
	    }
	}
	else if ( (v0==1 && v1==2) || (v0==2 && v1==1) )
	{
	    checkti = triangles_[ti].neighbors_[1];
	    crdci = triangles_[ti].coordindices_[0];
	    if ( v0==1 && v1==2 )
	    {
		shared0 = triangles_[ti].coordindices_[1];
		shared1 = triangles_[ti].coordindices_[2];   
	    }
	    else
	    {
		shared0 = triangles_[ti].coordindices_[2];
		shared1 = triangles_[ti].coordindices_[1];   
	    }
	}
	
	if ( checkti==cNoTriangle() )
	    continue;

	const int* checkcrds = triangles_[checkti].coordindices_;
	int checkpt =cNoVertex();
	for ( int idx=0; idx<3; idx++ )
	{
	    if ( checkcrds[idx] != shared0 && checkcrds[idx] != shared1 )
	    {
		checkpt = checkcrds[idx];
		break;
	    }
	}

	if ( checkpt==cNoVertex() )
	{
	    pErrMsg("Impossible case.");
	    continue;
	}

#ifndef mDAGTriangleForceSingleThread
	coordlock_.readLock();
#endif
	if ( !isInsideCircle(mCrd(checkpt), mCrd(crdci), mCrd(shared0),
		    mCrd(shared1)) || checkpt==crdci )
	{
#ifndef mDAGTriangleForceSingleThread
	    coordlock_.readUnLock();
#endif
	    continue;
	}

#ifndef mDAGTriangleForceSingleThread
	coordlock_.readUnLock();
#endif
	
	const int nti0 = triangles_.size();
	const int nti1 = triangles_.size()+1;
   
	DAGTriangle child0;
	child0.coordindices_[0] = crdci;
	child0.coordindices_[1] = shared0; 
	child0.coordindices_[2] = checkpt;
	child0.neighbors_[0] = getNeighbor(shared0,crdci,ti );
	child0.neighbors_[1] = getNeighbor(checkpt,shared0, checkti );
	child0.neighbors_[2] = nti1;

	DAGTriangle child1;
	child1.coordindices_[0] = shared1;
	child1.coordindices_[1] = crdci; 
	child1.coordindices_[2] = checkpt; 
	child1.neighbors_[0] = getNeighbor(crdci,shared1,ti );
	child1.neighbors_[1] = nti0;
	child1.neighbors_[2] = getNeighbor(shared1,checkpt, checkti );

#ifndef mDAGTriangleForceSingleThread
	trianglelock_.convPermissiveToWriteLock();
#endif

	triangles_ += child0;
	triangles_ += child1;

	triangles_[ti].childindices_[0] = nti0;
	triangles_[ti].childindices_[1] = nti1;
	triangles_[checkti].childindices_[0] = nti0;
	triangles_[checkti].childindices_[1] = nti1;
#ifndef mDAGTriangleForceSingleThread
	trianglelock_.convWriteToPermissive();
#endif

	v0s += 1; v1s += 2; tis += nti0;
	v0s += 0; v1s += 2; tis += nti1;
    }
}


bool DAGTriangleTree::getCoordIndices( TypeSet<int>& result ) const
{
    for ( int idx=triangles_.size()-1; idx>=0; idx-- )
    {
	const int* child = triangles_[idx].childindices_;
	if ( child[0]>=0 || child[1]>=0 || child[2]>=0 )
	    continue;

	const int* c = triangles_[idx].coordindices_;
	if ( c[0]<0 || c[1]<0 || c[2]<0 )
	    continue;
	
	result += c[0];
	result += c[1];
	result += c[2];
    }

    return result.size();
}


bool DAGTriangleTree::getSurroundingIndices( TypeSet<int>& result ) const 
{
    result.erase();
    for ( int idx=triangles_.size()-1; idx>=0; idx-- )
    {
	const int* child = triangles_[idx].childindices_;
	if ( child[0]>=0 || child[1]>=0 || child[2]>=0 )
	    continue;

	const int* c = triangles_[idx].coordindices_;
	const char nrinits = (c[0]<0) + (c[1]<0) + (c[2]<0);
	if ( !nrinits )
	    continue;

	for ( int idy=0; idy<3; idy++ )
	    if ( c[idy]>=0 && result.indexOf(c[idy])==-1 ) result += c[idy];
    }

    return result.size();
}

bool DAGTriangleTree::getConnections( int vertex, TypeSet<int>& result ) const
{
    result.erase();
    for ( int idx=triangles_.size()-1; idx>=0; idx-- )
    {
	const int* child = triangles_[idx].childindices_;
	if ( child[0]>=0 || child[1]>=0 || child[2]>=0 )
	    continue;

	const int* c = triangles_[idx].coordindices_;
	if ( c[0]!=vertex && c[1]!=vertex && c[2]!=vertex )
	    continue;

	for ( int idy=0; idy<3; idy++ )
	{
	    if ( c[idy]<0 || c[idy]==vertex || result.indexOf(c[idy])!=-1 )
		continue;

	    result += c[idy];
	}
    }

    return result.size();
}


bool DAGTriangleTree::getConnectionAndWeights( int vertex, TypeSet<int>& conns,
			       TypeSet<double>& weights, bool normalize ) const
{
    if ( !getConnections( vertex, conns ) )
	return false;

    return getWeights( vertex, conns, weights, normalize );
}


bool DAGTriangleTree::getWeights( int vertexidx, const TypeSet<int>& conns,
			      TypeSet<double>& weights, bool normalize) const
{
    weights.erase();

    double sum = 0;
    const Coord& vertex = mCrd(vertexidx);
    for ( int knot=0; knot<conns.size(); knot++ )
    {
	const double weight = 1/vertex.distTo(mCrd(conns[knot]));
	weights += weight;
	if ( normalize ) sum += weight;
    }

    if ( !normalize )
	return true;

    for ( int knot=0; knot<weights.size(); knot++ )
	weights[knot] /= sum;

    return true;
}


void DAGTriangleTree::dumpTo(std::ostream& stream) const
{
    for ( int idx=0; idx<triangles_.size(); idx++ )
    {
	const DAGTriangle& triangle = triangles_[idx];
	stream << (int) triangle.coordindices_[0] << '\t'
	       << (int) triangle.coordindices_[1] << '\t'
	       << (int) triangle.coordindices_[2] << '\n';
    }
}


#define mSearch( child ) \
if ( child!=cNoTriangle() ) \
{ \
    const int* gc = triangles_[child].coordindices_; \
    if ( (gc[0]==v0 && gc[1]==v1) || (gc[1]==v0 && gc[0]==v1) || \
	 (gc[0]==v0 && gc[2]==v1) || (gc[2]==v0 && gc[0]==v1) || \
	 (gc[1]==v0 && gc[2]==v1) || (gc[2]==v0 && gc[1]==v1) )  \
    { \
	const int res = searchChild( v0, v1, child ); \
	return res; \
    } \
}


int DAGTriangleTree::searchChild( int v0, int v1, int ti ) const
{
    if ( ti==cNoTriangle() )
	return cError();

#ifndef mDAGTriangleForceSingleThread
    trianglelock_.readLock();
#endif
    const int* cptr = triangles_[ti].childindices_;
    const int children[] = { cptr[0], cptr[1], cptr[2] };
#ifndef mDAGTriangleForceSingleThread
    trianglelock_.readUnLock();
#endif

    if ( children[0]==cNoTriangle() && children[1]==cNoTriangle() &&
	 children[2]==cNoTriangle() )
	return ti;

    for ( int idx=0; idx<3; idx++ )
	mSearch( children[idx] );

    return cError();
}


DAGTriangleTree::DAGTriangle::DAGTriangle()
{
    coordindices_[0] = DAGTriangleTree::cNoVertex();
    coordindices_[1] = DAGTriangleTree::cNoVertex();
    coordindices_[2] = DAGTriangleTree::cNoVertex();
    childindices_[0] = DAGTriangleTree::cNoTriangle();
    childindices_[1] = DAGTriangleTree::cNoTriangle();
    childindices_[2] = DAGTriangleTree::cNoTriangle();
    neighbors_[0] = DAGTriangleTree::cNoTriangle();
    neighbors_[1] = DAGTriangleTree::cNoTriangle();
    neighbors_[2] = DAGTriangleTree::cNoTriangle();
}


char DAGTriangleTree::getCommonEdge( int ti0, int ti1 ) const
{
    const int* crds0 = triangles_[ti0].coordindices_;
    const int* crds1 = triangles_[ti1].coordindices_;
    for ( int idx=0; idx<3; idx++ )
    {
	if ( crds0[idx]!=crds1[0] && crds0[idx]!=crds1[1] && 
	     crds0[idx]!=crds1[2] )
	{
	    if ( idx==0 )
		return 1;
	    else if ( idx==1)
		return 2;
	    else
		return 0;
	}
    }

    return cNoTriangle();
}


bool DAGTriangleTree::DAGTriangle::hasChildren() const
{
    return childindices_[0]!=-1 || childindices_[1]!=-1 ||childindices_[2]!=-1;
}


bool DAGTriangleTree::DAGTriangle::operator==( 
	const DAGTriangle& dag ) const
{
    const int d0 = dag.coordindices_[0];
    const int d1 = dag.coordindices_[1];
    const int d2 = dag.coordindices_[2];
    return
       (d0==coordindices_[0] && d1==coordindices_[1] && d2==coordindices_[2]) ||
       (d0==coordindices_[0] && d1==coordindices_[2] && d2==coordindices_[1]) ||
       (d0==coordindices_[1] && d1==coordindices_[0] && d2==coordindices_[2]) ||
       (d0==coordindices_[1] && d1==coordindices_[2] && d2==coordindices_[0]) ||
       (d0==coordindices_[2] && d1==coordindices_[1] && d2==coordindices_[0]) ||
       (d0==coordindices_[2] && d1==coordindices_[0] && d2==coordindices_[1]);
}


DAGTriangleTree::DAGTriangle&
DAGTriangleTree::DAGTriangle::operator=( const DAGTriangleTree::DAGTriangle& b )
{
    for ( int idx=0; idx<3; idx++ )
    {
	coordindices_[idx] = b.coordindices_[idx];
	childindices_[idx] = b.childindices_[idx];
	neighbors_[idx] = b.neighbors_[idx];
    }

    return *this;
}


#define mInitCorner0 -2
#define mInitCorner1 -3
#define mInitCorner2 -4

Triangle2DInterpolator::Triangle2DInterpolator( const DAGTriangleTree& tri )
    : triangles_( tri )
{
    triangles_.getSurroundingIndices( perimeter_ );
    triangles_.getConnectionAndWeights(mInitCorner0, corner0_, cornerweights0_);
    triangles_.getConnectionAndWeights(mInitCorner1, corner1_, cornerweights1_);
    triangles_.getConnectionAndWeights(mInitCorner2, corner2_, cornerweights2_);
     
    initcenter_ = ( triangles_.getInitCoord(mInitCorner0) + 
	    	    triangles_.getInitCoord(mInitCorner1) +
	    	    triangles_.getInitCoord(mInitCorner2) )/3;
}


bool Triangle2DInterpolator::computeWeights( const Coord& pt, 
	TypeSet<int>& vertices, TypeSet<float>& weights, 
	double maxdist, bool dointerpolate )
{
    int dupid = -1;
    TypeSet<int> tmpvertices;
    if ( !triangles_.getTriangle(pt,dupid,tmpvertices) )
	return false;

    if ( dupid!=-1 )
    {
	vertices += dupid;
	weights += 1;
	return true;
    }

    const int nrvertices = tmpvertices.size();
    if ( !nrvertices )
    {
	pErrMsg("Hmm");
	return false;
    }

    if ( !dointerpolate ) //Get the nearest node only
    {
	double minsqdist = 0;
	for ( int ptidx=0; ptidx<nrvertices; ptidx++ )
	{
	    if ( tmpvertices[ptidx]<0 )
		continue;

	    const Coord diff = triangles_.coordList()[tmpvertices[ptidx]] - pt;
	    const double sqdist = diff.sqAbs();

	    if ( !vertices.size() )
	    {
		vertices += tmpvertices[ptidx];
		minsqdist = sqdist;
	    }
	    else if ( minsqdist>sqdist )
	    {
		vertices[0] = tmpvertices[ptidx];
		minsqdist = sqdist;
	    }
	}

	if ( !vertices.size() )
	    return false;
	
	weights += 1;
	return true;
    }
	
    bool usedinit = false;
    bool result = true;
    TypeSet<float> tmpw;
    TypeSet<int> tmpv;
    for ( int ptidx=0; ptidx<nrvertices; ptidx++ )
    {
	if ( tmpvertices[ptidx]<0 ) 
	{
	    usedinit = true;
	    result = setFromAzimuth( tmpvertices, pt, tmpv, tmpw );
	    break;
	}
    }
	    
    if ( !usedinit )
    {
    	float weight[3];
    	interpolateOnTriangle2D( pt,
    		triangles_.coordList()[tmpvertices[0]], 
    		triangles_.coordList()[tmpvertices[1]],	
    		triangles_.coordList()[tmpvertices[2]],
    		weight[0], weight[1], weight[2] );
    	
    	tmpv += tmpvertices[0]; tmpw += weight[0];
    	tmpv += tmpvertices[1]; tmpw += weight[1];
    	tmpv += tmpvertices[2]; tmpw += weight[2];
    }
    
    if ( mIsUdf(maxdist) )
    {
	vertices = tmpv;
	weights = tmpw;	
    }
    else
    {
	float weightsum = 0, remwsum = 0;
	for ( int idx=0; idx<tmpv.size(); idx++ )
	{
	    const int vertice = tmpv[idx];
	    const float weight = tmpw[idx];

	    const Coord df = triangles_.coordList()[vertice] - pt;
	    if ( weight>mDefEps && df.sqAbs()<maxdist*maxdist )
	    {
		vertices += vertice;
		weights += weight;
		weightsum += weight;
	    }
	    else
		remwsum += weight;
	}	

	for ( int idx=0; idx<vertices.size(); idx++ )
	    weights[idx] += remwsum*weights[idx]/weightsum;
    }
    
    return result;
}


bool Triangle2DInterpolator::setFromAzimuth( const TypeSet<int>& tmpvertices, 
	const Coord& pt, TypeSet<int>& vertices, TypeSet<float>& weights )
{
    const Coord startpt = initcenter_ + Coord(1,0);
    const int perimetersize = perimeter_.size();
    
    if ( !perimeterazimuth_.size() )
    {
	initazimuth_[0] = initcenter_.angle( startpt,
		triangles_.getInitCoord(mInitCorner0) );
    	initazimuth_[1] = initcenter_.angle( startpt, 
		triangles_.getInitCoord(mInitCorner1) );
    	initazimuth_[2] = initcenter_.angle( startpt,
		triangles_.getInitCoord(mInitCorner2) );
	
	for ( int idx=0; idx<perimetersize; idx++ )
	    perimeterazimuth_ += initcenter_.angle( startpt,
		    triangles_.coordList()[perimeter_[idx]] );
    
	sort_coupled(perimeterazimuth_.arr(), perimeter_.arr(), perimetersize);
    }
    
    const double ptazim = initcenter_.angle( startpt, pt );

    int preidx = -1, aftidx = -1;
    for ( int idx=0; idx<perimetersize; idx++ )
    {
	if ( perimeterazimuth_[idx]<ptazim )
	    preidx = perimeter_[idx];
	else
	    aftidx = perimeter_[idx];
	
	if ( preidx!=-1 && aftidx!=-1 )
	    break;
    }

    if ( preidx==-1 && aftidx==-1 )
    {
	pErrMsg("Hmm");
	return false;
    }
    
    if ( preidx==-1 )
	preidx = perimeter_[0];
    else if ( aftidx==-1 )
	aftidx = perimeter_[0];
    
    Line2 center_ptline( initcenter_, pt );
    Line2 edgeline( triangles_.coordList()[preidx], 
	    	    triangles_.coordList()[aftidx] );
   
    //Get the init edge the point close to. 
    char usedinit[2] = {-1, -1};
    for ( int idx=0; idx<tmpvertices.size(); idx++ )
    {
	if ( tmpvertices[idx]<0 )
	{
	    if ( usedinit[0]==-1 )
		usedinit[0] = tmpvertices[idx];
	    else
		usedinit[1] = tmpvertices[idx];
	}
    }

    const Coord inita = triangles_.getInitCoord( usedinit[0] );
    Coord initb;
    if ( usedinit[1]==-1 )
    {
	for ( int initidx=-4; initidx<-1; initidx++ )
	{
	    if ( initidx==usedinit[0] ) continue;

	    usedinit[1] = initidx;
	    initb = triangles_.getInitCoord( usedinit[1] );
	    if ( pointInTriangle2D(pt,initcenter_,inita,initb,0) )
		break;
	}
    }
    else
	initb = triangles_.getInitCoord(usedinit[1]);

    Line2 initline( inita, initb );
    const Coord intersect0 = center_ptline.intersection( edgeline, false );
    const Coord intersect1 = center_ptline.intersection( initline, false );
    //May want to check if pt is located between intersect0 && intersect1.

    /*Basice Geometry
     *			      x				  y
     *		*(inita)----------------*(intersect1)------------*(initb)
     *					 \ 
     *					  \f	
     *					   \	
     *					    *(pt)
     *					     \	
     *					      \e
     *					       \	
     *			*(edgepre)--------------*(intersect0)-------*(edgeaft)
     * 					c	 \		d
     *						  \
     *						   *(center)	
     */

    const double x = intersect1.distTo( inita );
    const double y = intersect1.distTo( initb );
    const double f = intersect1.distTo( pt );
    const double e = intersect0.distTo( pt );
    const double c = intersect0.distTo( triangles_.coordList()[preidx] );
    const double d = intersect0.distTo( triangles_.coordList()[aftidx] );

    //Inverse distance weighting.
    vertices += preidx;
    weights += (f/(e+f))*(d/(c+d));
    vertices += aftidx;
    weights += (f/(e+f))*(c/(c+d));

    for ( int idx=0; idx<2; idx++ )
    {
	TypeSet<int>* conns;
	TypeSet<double>* ws;
	if ( usedinit[idx]==mInitCorner0 )
	{ conns = &corner0_; ws = &cornerweights0_; }
	else if ( usedinit[idx]==mInitCorner1 )
	{ conns = &corner1_; ws = &cornerweights1_; }
	else
	{ conns = &corner2_; ws = &cornerweights2_; }
	
	const double useddist = idx ? x : y;
	const float factor = (e/(e+f))*(useddist/(x+y));
	for ( int idz=0; idz<conns->size(); idz++ )
	{
	    vertices += (*conns)[idz];
	    weights += (*ws)[idz] * factor;
	}
    }
    
    return true;
}
























