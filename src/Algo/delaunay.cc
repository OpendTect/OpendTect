/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id$";

#include "delaunay.h"
#include "sorting.h"
#include "trigonometry.h"
#include "varlenarray.h"
#include <iostream>

DelaunayTriangulator::DelaunayTriangulator( DAGTriangleTree& dagt )
    : tree_( dagt )  
    , israndom_( true )
    , calcscope_( 0, dagt.coordList().size()-1 )
    , permutation_( 0 )
{}


DelaunayTriangulator::~DelaunayTriangulator()
{
    delete [] permutation_;
}


void DelaunayTriangulator::setCalcScope(const Interval<int>& rg)
{
    calcscope_.start =  mMAX( 0, rg.start );
    calcscope_.stop =  mMIN( tree_.coordList().size()-1, rg.stop );
}


od_int64 DelaunayTriangulator::nrIterations() const                
{ return calcscope_.width()+1; }


bool DelaunayTriangulator::doPrepare( int nrthreads )
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


bool DelaunayTriangulator::doWork( od_int64 start, od_int64 stop,int threadid )
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


#define mMultiThread( statements ) \
    if ( multithreadsupport_ ) { statements; };

DAGTriangleTree::DAGTriangleTree()
    : coordlist_( 0 )
    , epsilon_( 1e-5 )
    , ownscoordlist_( true )
{
#ifdef mDAGTriangleForceSingleThread
    multithreadsupport_ = false;
#else
    multithreadsupport_ = true;
#endif
}


DAGTriangleTree::DAGTriangleTree( const DAGTriangleTree& b )
    : coordlist_( 0 )
    , epsilon_( 1e-5 )
    , ownscoordlist_( true )
{
    *this = b;

#ifdef mDAGTriangleForceSingleThread
    multithreadsupport_ = false;
#else
    multithreadsupport_ = true;
#endif
}


DAGTriangleTree& DAGTriangleTree::operator=( const DAGTriangleTree& b )
{
    epsilon_ = b.epsilon_;
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

    bool rangesaredefined = false;

    for ( int idx=0; idx<coordlist.size(); idx++ )
    {
	if ( mIsUdf(coordlist[idx].x) || mIsUdf(coordlist[idx].y) )
	    continue;

	if ( !rangesaredefined ) 
	{
	    xrg.start = xrg.stop = coordlist[idx].x;
	    yrg.start = yrg.stop = coordlist[idx].y;
	    rangesaredefined = true;
	}

	xrg.include( coordlist[idx].x );
	yrg.include( coordlist[idx].y );
    }

    return rangesaredefined;
}


bool DAGTriangleTree::setCoordList( TypeSet<Coord>* coordlist,
				    OD::PtrPolicy policy )
{
    mMultiThread( coordlock_.writeLock() );

    if ( coordlist_ && ownscoordlist_ )
	delete coordlist_;

    coordlist_ = 0;
    mMultiThread( coordlock_.writeUnLock() );

    if ( !coordlist || coordlist->size()<3 )
	return false;

    Interval<double> xrg, yrg;
    if ( !computeCoordRanges(*coordlist, xrg, yrg) )
	return false;

    mMultiThread( coordlock_.writeLock() );

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

    mMultiThread( coordlock_.writeUnLock() );
    return setBBox( xrg, yrg );
}


bool DAGTriangleTree::setBBox( const Interval<double>& xrg,
			       const Interval<double>& yrg)
{
    triangles_.erase();
    const double xlength = xrg.width();
    const double ylength = yrg.width();
    if ( mIsZero(xlength,mDefEps) || mIsZero(ylength,mDefEps) )
	return false;

    const Coord center( xrg.center(), yrg.center() );
    double radius = sqrt( xlength*xlength+ylength*ylength )/2;
    radius += mDefEps;	// assures no point can be on edge of initial triangle

    initialcoords_[0] = Coord( center.x-radius*sqrt(3.0), center.y-radius );
    initialcoords_[1] = Coord( center.x+radius*sqrt(3.0), center.y-radius );
    initialcoords_[2] = Coord( center.x, center.y+2*radius );

    DAGTriangle initnode;
    initnode.coordindices_[0] = cInitVertex0();
    initnode.coordindices_[1] = cInitVertex1();
    initnode.coordindices_[2] = cInitVertex2();
    triangles_ += initnode;

    epsilon_ = Math::Sqrt( xlength*xlength+ylength*ylength) * 1e-5;
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

    mMultiThread( coordlock_.writeLock() );

    if ( !coordlist_ )
    {
	coordlist_ = new TypeSet<Coord>;
	ownscoordlist_ = true;
    }

    mMultiThread( coordlock_.writeUnLock() );
    return true;
}


int DAGTriangleTree::insertPoint( const Coord& coord, int& dupid )
{
    mMultiThread( coordlock_.writeLock() );
    const int ci = coordlist_->size();
    (*coordlist_) += coord;
    mMultiThread( coordlock_.writeUnLock() );

    if ( !insertPoint( ci, dupid ) )
    {
	mMultiThread( coordlock_.writeLock() );

	if ( coordlist_->size()==ci+1 )
	    coordlist_->remove( ci );

	mMultiThread( coordlock_.writeUnLock() );
	return cNoVertex();
    }

    return ci;
}


bool DAGTriangleTree::getTriangle( const Coord& pt, int& dupid,
				   TypeSet<int>& vertices ) const
{
    dupid = cNoVertex();
    vertices.erase();
    int ti0;
    const char res = searchTriangle( pt, 0, ti0, dupid );
    if ( dupid!=cNoVertex() )
	return true;

    if ( ti0==cNoVertex() )
	return false;

    vertices += triangles_[ti0].coordindices_[0];
    vertices += triangles_[ti0].coordindices_[1];
    vertices += triangles_[ti0].coordindices_[2];

    return true;
}


#define mCrd( idx ) \
	(idx>=0 ? (*coordlist_)[idx] : initialcoords_[-idx-2])


bool DAGTriangleTree::insertPoint( int ci, int& dupid )
{
    dupid = cNoVertex();
    mMultiThread( coordlock_.readLock() );

    if ( mIsUdf((*coordlist_)[ci].x) || mIsUdf((*coordlist_)[ci].y) ) 
    {
	mMultiThread( coordlock_.readUnLock() );
	BufferString msg = "The point ";
	msg += ci;
	msg +=" is not defined!";
	pErrMsg( msg );
	return true; //For undefined point, skip.
    }
    mMultiThread( coordlock_.readUnLock() );

    int ti0;
    const char res = searchTriangle( mCrd(ci), 0, ti0, dupid );
    
    if ( res==cIsInside() )
    {
	mMultiThread( trianglelock_.permissiveWriteLock() );
	int nti0 = ti0;
	const char nres = searchFurther( mCrd(ci), nti0, dupid );

	if ( nres==cIsInside() )
	{
	    splitTriangleInside( ci, nti0 );
	    mMultiThread( trianglelock_.permissiveWriteUnLock() );
	    return true;
	}
	else
	{
	    mMultiThread( trianglelock_.permissiveWriteUnLock() );

	    if ( nres==cIsDuplicate() )
		return true;
	    else
	    {
		BufferString msg = "\nInsert point ";
		msg += ci;
		msg += " failed!";
		pErrMsg( msg );
		return false;
	    }
	}
    }
    else if ( res==cIsDuplicate() )
	return true;
    else  
    {
	BufferString msg = "\nInsert point ";
	msg += ci;
	msg += " failed!";
	pErrMsg( msg );
	return false;
    }

    return true;
}


char DAGTriangleTree::searchTriangle( const Coord& pt, int startti, int& ti0, 
				      int& dupid ) const
{
    if ( startti<0 )
	startti = 0;

    ti0 = startti;

    return searchFurther( pt, ti0, dupid );
}


char DAGTriangleTree::searchFurther( const Coord& pt, int& ti0,
				     int& dupid ) const
{
    while ( true )
    {
	mMultiThread( trianglelock_.readLock() );

	if (  !triangles_[ti0].hasChildren() )
	{
	    mMultiThread( trianglelock_.readUnLock() );
	    return cIsInside();
	}

	const int* cptr = triangles_[ti0].childindices_;
	const int children[] = { cptr[0], cptr[1], cptr[2] };
	mMultiThread( trianglelock_.readUnLock() );
	  
	bool found = false; 
	for ( int childidx = 0; childidx<3; childidx++ )
	{
	    const int curchild = children[childidx];
	    if ( curchild==cNoTriangle() ) continue;

	    const char mode = isInside( pt, curchild, dupid );
	    if ( mode==cIsOutside() ) 
		continue;

	    if ( mode==cIsDuplicate() )
		return mode;

	    if ( mode==cIsInside() )
	    {
		ti0 = curchild;

		mMultiThread( trianglelock_.readLock() );
		const bool haschildren = triangles_[curchild].hasChildren();
		mMultiThread( trianglelock_.readUnLock() );

		found = true;

		if ( !haschildren )
		    return cIsInside();

		break;
	    }
	    else
		pErrMsg("Hmm");
	}

	if ( !found )
	{
	    pErrMsg("No child found");
	    return cError();
	}
    }

    return cError();
}


static bool clockwise( const Coord& c0, const Coord& c1, const Coord& c2 )
{ return (c1.x-c0.x)*(c2.y-c1.y)-(c1.y-c0.y)*(c2.x-c1.x) < 0; }


static bool isPointLeftOfLine( const Coord& p, const Coord& a, const Coord& b )
{
    const double det_ap_ab = (p.x-a.x)*(b.y-a.y) - (p.y-a.y)*(b.x-a.x);
    const double det_ba_bp = (p.y-b.y)*(a.x-b.x) - (p.x-b.x)*(a.y-b.y);

    // |sum| is numerically invariant to swap of a and b, only sign toggles.
    const double sum = det_ap_ab + det_ba_bp;
    if ( sum != 0.0 )
	return sum > 0.0;

    // exactly on the line: make sure that swap of a and b negates result.
    return ( a.x>b.x ) || ( a.x==b.x && a.y>b.y );
}


char DAGTriangleTree::isInside( const Coord& pt, int ti, int& dupid ) const
{
    if ( ti==cNoTriangle() )
	return cIsOutside();

    mMultiThread( coordlock_.readLock() );
    const int* crds = triangles_[ti].coordindices_;

    const Coord* tricoord0 = &mCrd( crds[0] );
    const Coord* tricoord1 = &mCrd( crds[1] );
    const Coord* tricoord2 = &mCrd( crds[2] );

    const double sqepsilon = epsilon_ * epsilon_;

    if ( Coord(pt - *tricoord0).sqAbs() < sqepsilon )
    {
	dupid = crds[0];
	mMultiThread( coordlock_.readUnLock() );
	return cIsDuplicate();
    }
    if ( Coord(pt - *tricoord1).sqAbs() < sqepsilon )
    {
	dupid = crds[1];
	mMultiThread( coordlock_.readUnLock() );
	return cIsDuplicate();
    }
    if ( Coord(pt - *tricoord2).sqAbs() < sqepsilon )
    {
	dupid = crds[2];
	mMultiThread( coordlock_.readUnLock() );
	return cIsDuplicate();
    }

    if ( clockwise(*tricoord0, *tricoord1, *tricoord2) )
    {
	pErrMsg( "Did not expect clockwise triangle!" );
	const Coord* tmp;
	mSWAP( tricoord0, tricoord2, tmp );
    }

    if ( isPointLeftOfLine(pt, *tricoord0, *tricoord1) ||
	 isPointLeftOfLine(pt, *tricoord1, *tricoord2) ||
	 isPointLeftOfLine(pt, *tricoord2, *tricoord0) )
    {
	mMultiThread( coordlock_.readUnLock() );
	return cIsOutside();
    }

    mMultiThread( coordlock_.readUnLock() );
    return cIsInside();
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
    child0.neighbors_[0] = searchChild( crd0, crd1, nbti[0] );
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

    mMultiThread( trianglelock_.convPermissiveToWriteLock() );

    triangles_ += child0;
    triangles_ += child1;
    triangles_ += child2;

    triangles_[ti].childindices_[0] = ti0;
    triangles_[ti].childindices_[1] = ti1;
    triangles_[ti].childindices_[2] = ti2;

    mMultiThread( trianglelock_.convWriteToPermissive() );

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
		shared0 = triangles_[ti].coordindices_[0];
		shared1 = triangles_[ti].coordindices_[1];  
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

	mMultiThread( coordlock_.readLock() );

	if ( !isInsideCircle(mCrd(checkpt), mCrd(crdci), mCrd(shared0),
		    mCrd(shared1)) || checkpt==crdci )
	{
	    mMultiThread( coordlock_.readUnLock() );
	    continue;
	}

	mMultiThread( coordlock_.readUnLock() );
	
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

	mMultiThread( trianglelock_.convPermissiveToWriteLock() );

	triangles_ += child0;
	triangles_ += child1;

	triangles_[ti].childindices_[0] = nti0;
	triangles_[ti].childindices_[1] = nti1;
	triangles_[checkti].childindices_[0] = nti0;
	triangles_[checkti].childindices_[1] = nti1;

	mMultiThread( trianglelock_.convWriteToPermissive() );

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
	const double weight = 1./vertex.distTo(mCrd(conns[knot]));
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


void DAGTriangleTree::dumpTriangulationToIV(std::ostream& stream) const
{
    stream << "#Inventor V2.1 ascii\n\n"
	   << "Coordinate3 {\n"
	   << "point [\n";

    for ( int idx=0; idx<coordlist_->size(); idx++ )
    {
	stream << (*coordlist_)[idx].x << " "
	       << (*coordlist_)[idx].y << " "
	       << (idx<coordlist_->size()-1 ? "0,\n" : "0\n");
    }
    stream << "]\n}\n\n";

    stream << "IndexedTriangleStripSet {\n"
	   << "coordIndex [\n" ;
	
    for ( int idx=0; idx<triangles_.size(); idx++ )
    {
	const DAGTriangle& triangle = triangles_[idx];

	if ( triangle.hasChildren()      || triangle.coordindices_[0]<0 ||
	     triangle.coordindices_[1]<0 || triangle.coordindices_[2]<0 )
	    continue;

       stream << triangle.coordindices_[0] << ", "
	      << triangle.coordindices_[1] << ", "
	      << triangle.coordindices_[2] << ", -1"
	      << (idx<triangles_.size()-1 ? ",\n" : "\n");
    }
    stream << "]\n}" << std::endl;
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

    mMultiThread( trianglelock_.readLock() );
    const int* cptr = triangles_[ti].childindices_;
    const int children[] = { cptr[0], cptr[1], cptr[2] };
    mMultiThread( trianglelock_.readUnLock() );

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
    triangles_.getConnectionAndWeights(mInitCorner0,corner0_,cornerweights0_);
    triangles_.getConnectionAndWeights(mInitCorner1,corner1_,cornerweights1_);
    triangles_.getConnectionAndWeights(mInitCorner2,corner2_,cornerweights2_);
     
    initcenter_ = ( triangles_.getInitCoord(mInitCorner0) + 
	    	    triangles_.getInitCoord(mInitCorner1) +
	    	    triangles_.getInitCoord(mInitCorner2) )/3;
    const Coord startpt = initcenter_ + Coord(1,0);
    for ( int idx=0; idx<perimeter_.size(); idx++ )
	perimeterazimuth_ += initcenter_.angle( startpt,
		triangles_.coordList()[perimeter_[idx]] );

    sort_coupled(perimeterazimuth_.arr(),perimeter_.arr(),perimeter_.size());
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
    const double ptazim = initcenter_.angle( startpt, pt );

    int preidx = -1, aftidx = -1;
    for ( int idx=0; idx<perimeter_.size(); idx++ )
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
